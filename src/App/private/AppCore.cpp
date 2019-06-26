#include "AppCore.h"

#include <QScreen>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QLocalSocket>
#include <QMetaObject>

#include <QDebug>
Q_LOGGING_CATEGORY(appCore, "app.core")

#include <WindowView.h>
#include <SettingsContainer.h>
#include <Win.h>

#include "QMLController.h"
#include "TreeModel.h"
#include "TreeItem.h"
#include "DWMThumbnail.h"
#include "IPCClient.h"

AppCore::AppCore(QObject* parent)
	: QObject(parent)
	, m_qmlController(nullptr)
	, m_windowView(nullptr)
	, m_settingsContainer(nullptr)
	, m_treeModel(nullptr)
	, m_exitExpected(false)
	, m_ipcClient(nullptr)
	, m_pendingWindowRecipient(nullptr)
{
	setObjectName("App Core");

	qCInfo(appCore) << "Construction";

	registerMetatypes();

	m_qmlController = new QMLController(this);

	m_windowView = new WindowView(this);

	m_ipcClientThread.setObjectName("IPC Client Thread");
	m_ipcClient = new IPCClient(this);
	m_ipcClient->moveToThread(&m_ipcClientThread);
	connect(&m_ipcClientThread, &QThread::started, m_ipcClient, &IPCClient::connectToServer);
	connect(&m_ipcClientThread, &QThread::finished, m_ipcClient, &QObject::deleteLater);

	m_settingsContainer = new SettingsContainer(this);
	m_treeModel = new TreeModel(this);

	ipcClientChanged();
	windowViewChanged();
	settingsContainerChanged();
	treeModelChanged();

	qCInfo(appCore) << "Construction Complete";

	makeConnections();

	emit startup();
}

void AppCore::syncObjectPropertyChanged(QString object, QString property, QVariant value)
{
	QObject* syncObject = findChild<QObject*>(object);
	Q_ASSERT(syncObject != nullptr);
	syncObject->setProperty(property.toStdString().c_str(), value);
}

void AppCore::exitRequested()
{
	m_exitExpected = true;

	m_treeModel->save();

	m_settingsContainer->setProperty("headerSize", 0);
	m_treeModel->getRootItem()->playShutdownAnimation();

	connect(m_treeModel->getRootItem(), &TreeItem::animationFinished, [=](){
		m_qmlController->closeWindow();
	});

}

void AppCore::logReceived(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
	QVariantList message = {"Log", int(type), ctx.category, msg};
	QMetaObject::invokeMethod(m_ipcClient, "sendMessage", Q_ARG(QVariantList, message));
}

void AppCore::registerMetatypes()
{
	// Register Metatypes
	qCInfo(appCore) << "Registering Metatypes";
	qRegisterMetaType<HWND>();
	qRegisterMetaTypeStreamOperators<HWND>();
	QMetaType::registerDebugStreamOperator<HWND>();
	qRegisterMetaType<QScreen*>();
	qRegisterMetaType<TreeModel*>();
	qRegisterMetaType<TreeItem*>();
	qRegisterMetaType<IPCClient*>();
	qRegisterMetaType<WindowInfo*>();
	qRegisterMetaType<WindowView*>();
	qRegisterMetaType<SettingsContainer*>();

	// Register QML types
	qCInfo(appCore) << "Registering QML Types";
	qmlRegisterType<DWMThumbnail>("DWMThumbnail", 1, 0, "DWMThumbnail");
	qmlRegisterInterface<HWND>("HWND");
}

void AppCore::makeConnections()
{
	// IPC Client -> Window View
	connect(m_ipcClient, &IPCClient::windowAdded, m_windowView, &WindowView::windowAdded);
	connect(m_ipcClient, &IPCClient::windowTitleChanged, m_windowView, &WindowView::windowTitleChanged);
	connect(m_ipcClient, &IPCClient::windowRemoved, m_windowView, &WindowView::windowRemoved);

	// IPC Client -> Tree Model
	connect(m_ipcClient, &IPCClient::receivedWindowList, m_treeModel, &TreeModel::startup);

	// Tree Model -> IPC Client
	connect(m_treeModel, &TreeModel::moveWindows, [=](QVariantList message) {
		QMetaObject::invokeMethod(m_ipcClient, "sendMessage", Q_ARG(QVariantList, message));
	});

	// IPC Client -> App Core
	connect(m_ipcClient, &IPCClient::receivedQuitMessage, this, &AppCore::exitRequested);

	// IPC Client -> QML Window
	connect(m_ipcClient, &IPCClient::disconnected, m_qmlController, &QMLController::closeWindow);

	// Tree Model -> IPC Client
	connect(m_treeModel, &TreeModel::setWindowStyle,	m_ipcClient, &IPCClient::setWindowStyle);

	// IPC Client -> App Core
	connect(m_ipcClient, &IPCClient::syncObjectPropertyChanged, this, &AppCore::syncObjectPropertyChanged);

	connect(m_ipcClient, &IPCClient::windowSelected, [=](HWND hwnd) {
		WindowInfo* wi = m_windowView->getWindowInfo(hwnd);

		m_pendingWindowRecipient->setWindowInfo(wi);
		m_pendingWindowRecipient->updateWindowPosition();
		m_pendingWindowRecipient = nullptr;
	});

	connect(m_ipcClient, &IPCClient::windowSelectionCanceled, [=]() {
		m_pendingWindowRecipient = nullptr;
	});

	// Startup
	connect(this, SIGNAL(startup()), m_ipcClient, SLOT(connectToServer()));
	connect(this, SIGNAL(startup()), &m_ipcClientThread, SLOT(start()));

	// Cleanup on exit
	QGuiApplication* app = static_cast<QGuiApplication*>(QGuiApplication::instance());
	connect(app, &QGuiApplication::lastWindowClosed, [=](){
		qCInfo(appCore) << "All windows closed, shutting down";

		// Save if this is an unexpected force-quit
		if(m_exitExpected == false)
		{
			qCInfo(appCore) << "Unexpected exit";
			m_treeModel->save();
			m_treeModel->getRootItem()->cleanup();
		}
		else {
			qCInfo(appCore) << "Expected exit";
		}

		qCInfo(appCore) << "Quitting";
		QGuiApplication::quit();
	});

	connect(app, &QGuiApplication::aboutToQuit, [=]() {
		m_ipcClientThread.quit();
		m_ipcClientThread.wait();
	});
}
