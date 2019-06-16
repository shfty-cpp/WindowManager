#include "AppCore.h"
#include "Win.h"

#include <QGuiApplication>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QScreen>
#include <QDateTime>

void streamLog(QTextStream& ts, QString time, QString typePrefix, QString className, QString funcName, QString msg)
{
	ts.setFieldAlignment(QTextStream::AlignLeft);
	ts.setFieldWidth(10);
	ts << time;
	ts << typePrefix;
#ifdef QT_DEBUG
	ts.setFieldWidth(24);
	ts << className;
	ts.setFieldWidth(24);
	ts << funcName;
#endif
	ts.setFieldWidth(0);
	ts << msg;
	ts << endl;
}

void myMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
	static QTextStream stdOutStream(stdout);
	static QTextStream stdErrStream(stderr);

	QTextStream* os = nullptr;
	QString typePrefix;
	switch (type) {
		case QtDebugMsg:
			typePrefix = QString("Debug");
			os = &stdOutStream;
			break;
		case QtInfoMsg:
			typePrefix = QString("Info");
			os = &stdOutStream;
			break;
		case QtWarningMsg:
			typePrefix = QString("Warning");
			os = &stdErrStream;
			break;
		case QtCriticalMsg:
			typePrefix = QString("Critical");
			os = &stdErrStream;
			break;
		case QtFatalMsg:
			typePrefix = QString("Fatal");
			os = &stdErrStream;
			break;
	}
/*
	QFile outFile("P:/Personal/C++/WindowManager/wm-log.txt");
	outFile.open(QIODevice::WriteOnly | QIODevice::Append);
*/
	QString time = QTime::currentTime().toString("HH:mm:ss");

	QRegExp funcRegex("([a-zA-Z]*)(?=::)::([a-zA-Z]*)");
	funcRegex.indexIn(ctx.function);
	QString funcString = funcRegex.capturedTexts().first();

	QStringList funcComponents = funcString.split("::");

	streamLog(*os, time, typePrefix, funcComponents.first(), funcComponents.last(), msg);
/*
	QTextStream ts(&outFile);
	streamLog(ts, time, typePrefix, funcComponents.first(), funcComponents.last(), msg);
*/
}

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);
	QGuiApplication::setOrganizationName("Josh Palmer");
	QGuiApplication::setOrganizationDomain("https://josh-palmer.com");
	QGuiApplication::setQuitOnLastWindowClosed(false);
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	QStringList args = QCoreApplication::arguments();
	if(args.length() <= 2)
	{
		qCritical("No ID / save file specified, quitting...");
		return EXIT_FAILURE;
	}

	// Setup QML renderer
	QString productType = QSysInfo::productType();
	QString productVersion = QSysInfo::productVersion();

	if(productType == "windows" && productVersion == "10")
	{
		//QQuickWindow::setSceneGraphBackend(QSGRendererInterface::Direct3D12);
	}

	QQuickStyle::setStyle("Universal");

	qRegisterMetaType<HWND>();
	qRegisterMetaType<QScreen*>();

	qInstallMessageHandler(myMessageHandler);

	AppCore appCore;

	return app.exec();
}