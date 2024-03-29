#include "IPCClient.h"

#include <QGuiApplication>
#include <QLocalSocket>
#include <QDataStream>
#include <QCoreApplication>
#include <QTimer>

#include <QDebug>
Q_LOGGING_CATEGORY(ipcClient, "app.ipcClient")

#include <WindowInfo.h>
#include <WindowView.h>

IPCClient::IPCClient(QString socketName, QObject *parent)
	: QObject(parent)
	, m_socketName(socketName)
{
	qCInfo(ipcClient) << "Construction";
}

void IPCClient::startup()
{
	qCInfo(ipcClient) << "Startup";

	m_localSocket = new QLocalSocket(this);

	connect(m_localSocket, &QLocalSocket::connected, [=](){
		qCInfo(ipcClient) << "m_localSocket connected";
		connect(m_localSocket, &QLocalSocket::readyRead, [=](){
			QDataStream stream(m_localSocket);
			stream.setVersion(QDataStream::Qt_5_12);

			while(true)
			{
				QVariant message;

				stream.startTransaction();
				stream >> message;

				if(stream.commitTransaction())
				{
					QString mStr = message.toString();
					handleMessage(stream, mStr);
					QGuiApplication::processEvents();
				}
				else
				{
					break;
				}
			}
		});
	});

	connect(m_localSocket, static_cast<void (QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), [=](QLocalSocket::LocalSocketError error){
		switch(error)
		{
			case QLocalSocket::ServerNotFoundError:
				qCWarning(ipcClient) << "Server not found, retrying";

				QTimer::singleShot(100, [=](){
					m_localSocket->connectToServer("WindowManager", QIODevice::ReadWrite);
				});
				break;
			default:
				qCWarning(ipcClient) << "Local Socket Error:" << m_localSocket->errorString();
				break;
		}
	});

	connect(m_localSocket, &QLocalSocket::disconnected, this, &IPCClient::disconnected);

	qCInfo(ipcClient) << "Connecting to server";
	m_localSocket->connectToServer("WindowManager", QIODevice::ReadWrite);
}

void IPCClient::handleMessage(QDataStream& stream, QString message)
{
	if(message == "Identify")
	{
		qCInfo(ipcClient) << "ID request from server";
		sendMessage({"Identify", m_socketName});
		emit ipcReady();
	}
	else if(message == "SyncObjectPropertyChanged")
	{
		QVariant syncObjectNameVar, syncObjectPropertyVar, syncObjectValue;
		stream >> syncObjectNameVar >> syncObjectPropertyVar >> syncObjectValue;

		QString syncObjectName = syncObjectNameVar.toString();
		QString syncObjectProperty = syncObjectPropertyVar.toString();

		emit syncObjectPropertyChanged(syncObjectName, syncObjectProperty, syncObjectValue);
	}
	else if(message == "WindowList")
	{
		qCInfo(ipcClient) << "WindowList from server";

		QVariant countVar;
		stream >> countVar;
		int count = countVar.toInt();

		for(int i = 0; i < count; ++i)
		{
			QVariant wiVar;
			stream >> wiVar;
			WindowInfo wi = wiVar.value<WindowInfo>();

			emit windowCreated(wi);
		}

		emit receivedWindowList();
	}
	else if(message == "WindowAdded")
	{
		QVariant wiVar;
		stream >> wiVar;
		WindowInfo wi = wiVar.value<WindowInfo>();

		qCInfo(ipcClient) << "WindowAdded from server " << wi;
		emit windowCreated(wi);
	}
	else if(message == "WindowTitleChanged")
	{
		QVariant hwndVar, winTitleVar;
		stream >> hwndVar >> winTitleVar;

		HWND hwnd = hwndVar.value<HWND>();
		QString winTitle = winTitleVar.toString();

		qCInfo(ipcClient) << "WindowTitleChanged from server" << hwnd << " " << winTitle;

		emit windowTitleChanged(hwnd, winTitle);
	}
	else if(message == "WindowRemoved")
	{
		QVariant hwndVar;
		stream >> hwndVar;

		HWND hwnd = hwndVar.value<HWND>();

		qCInfo(ipcClient) << "WindowRemoved from server" << hwnd;

		emit windowDestroyed(hwnd);

	}
	else if(message == "WindowSelected")
	{
		QVariant hwndVariant;
		stream >> hwndVariant;

		HWND hwnd = hwndVariant.value<HWND>();

		qCInfo(ipcClient) << "Window selection from server, hwnd:" << hwnd;

		emit windowSelected(hwnd);
	}
	else if(message == "WindowSelectionCanceled")
	{
		qCInfo(ipcClient) << "Server canceled window selection";
		emit windowSelectionCanceled();
	}
	else if(message == "ReloadQML")
	{
		qCInfo(ipcClient) << "Reload QML request from server";
		emit reloadQml();
	}
	else if(message == "Quit")
	{
		qCInfo(ipcClient) << "Quit request from server";
		emit receivedQuitMessage();
	}
	else
	{
		qCInfo(ipcClient) << "Unknown message from server:" << message;
	}
}

void IPCClient::sendMessage(QVariantList message)
{
	qCInfo(ipcClient) << "Sending" << message << "to server";

	QDataStream stream(m_localSocket);
	stream.setVersion(QDataStream::Qt_5_12);

	for(QVariant submsg : message)
	{
		stream << submsg;
	}

	m_localSocket->flush();
}
