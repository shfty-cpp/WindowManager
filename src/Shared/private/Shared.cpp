#include "Shared.h"

#include <QMetaType>

#include "Win.h"
#include "WindowInfo.h"
#include "WindowView.h"
#include "SettingsContainer.h"

void Shared::registerMetatypes()
{
	qInfo() << "Registering shared metatypes";

	qRegisterMetaType<HWND>();
	qRegisterMetaTypeStreamOperators<HWND>();
	QMetaType::registerDebugStreamOperator<HWND>();

	qRegisterMetaType<WindowInfo>();
	qRegisterMetaTypeStreamOperators<WindowInfo>();
	QMetaType::registerDebugStreamOperator<WindowInfo>();

	qRegisterMetaType<WindowObject*>();
	qRegisterMetaType<WindowView*>();
	qRegisterMetaType<SettingsContainer*>();
}
