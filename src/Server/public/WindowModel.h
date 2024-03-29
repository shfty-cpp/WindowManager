#ifndef WINDOWEVENTMODEL_H
#define WINDOWEVENTMODEL_H

#include <QObject>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(windowModel);

#include <Win.h>
#include <WindowInfo.h>

class WindowModel : public QObject
{
	Q_OBJECT

public:
	explicit WindowModel(QObject* parent);
	~WindowModel();

	static WindowModel* instance;

	QString getWinTitle(HWND hwnd);
	QString getWinClass(HWND hwnd);
	QString getWinProcess(HWND hwnd);
	qint32 getWinStyle(HWND hwnd);

signals:
	void startupComplete();

	void windowCreated(WindowInfo wi);
	void windowRenamed(HWND hwnd, QString winTitle);
	void windowDestroyed(HWND hwnd);

	void activeWindowChanged();

public slots:
	void startup();

	void handleWindowCreated(HWND hwnd);
	void handleWindowRenamed(HWND hwnd);
	void handleWindowDestroyed(HWND hwnd);

	void handleActiveWindowChanged();

protected:
	HWINEVENTHOOK hookEvent(DWORD event, WINEVENTPROC callback);

private:
	HWINEVENTHOOK m_createHook;
	HWINEVENTHOOK m_renameHook;
	HWINEVENTHOOK m_destroyHook;

	QList<HWND> m_windowList;
};

#endif // WINDOWEVENTMODEL_H
