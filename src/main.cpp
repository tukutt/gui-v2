/*
** Copyright (C) 2021 Victron Energy B.V.
*/

#include "src/language.h"
#include "src/logging.h"
#include "src/theme.h"
#include "src/enums.h"
#include "src/notificationsmodel.h"
#include "src/clocktime.h"
#include "src/uidhelper.h"
#include "src/backendconnection.h"

#include "velib/qt/ve_qitem.hpp"
#include "velib/qt/ve_quick_item.hpp"
#include "velib/qt/ve_qitems_mqtt.hpp"
#include "velib/qt/ve_qitem_table_model.hpp"
#include "velib/qt/ve_qitem_sort_table_model.hpp"
#include "velib/qt/ve_qitem_child_model.hpp"

#if !defined(VENUS_WEBASSEMBLY_BUILD)
#include "src/connman/cmtechnology.h"
#include "src/connman/cmservice.h"
#include "src/connman/cmagent.h"
#include "src/connman/clockmodel.h"
#include "src/connman/cmmanager.h"
#endif

#if defined(VENUS_WEBASSEMBLY_BUILD)
#include <emscripten/val.h>
#include <emscripten.h>
#include <QUrl>
#include <QUrlQuery>
#endif

#include <QGuiApplication>
#include <QQuickView>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QScreen>

#include <QtDebug>

Q_LOGGING_CATEGORY(venusGui, "venus.gui")

namespace {

#if !defined(VENUS_WEBASSEMBLY_BUILD)
static QObject* connmanInstance(QQmlEngine *, QJSEngine *)
{
	return CmManager::instance();
}
#endif

void initBackend()
{
	Victron::VenusOS::BackendConnection *backend = Victron::VenusOS::BackendConnection::instance();

#if defined(VENUS_WEBASSEMBLY_BUILD)
	emscripten::val webLocation = emscripten::val::global("location");
	const QUrl webLocationUrl = QUrl(QString::fromStdString(webLocation["href"].as<std::string>()));
	const QUrlQuery query(webLocationUrl);
	const QString mqttUrl(query.queryItemValue("mqtt")); // e.g.: "ws://192.168.5.96:9001/"
	backend->setType(Victron::VenusOS::BackendConnection::MqttSource, mqttUrl);
#else
	QCommandLineParser parser;
	parser.setApplicationDescription("Venus GUI");
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption dbusAddress({ "d",  "dbus" },
		QGuiApplication::tr("main", "Use D-Bus data source: connect to the specified D-Bus address"),
		QGuiApplication::tr("main", "dbus"));
	parser.addOption(dbusAddress);

	QCommandLineOption dbusDefault(QStringLiteral("dbus-default"),
		QGuiApplication::tr("main", "Use D-Bus data source: connect to the default D-Bus address"));
	parser.addOption(dbusDefault);

	QCommandLineOption mqttAddress({ "m",  "mqtt" },
		QGuiApplication::tr("main", "Use MQTT data source: connect to the specified MQTT broker address"),
		QGuiApplication::tr("main", "mqtt"));
	parser.addOption(mqttAddress);

	QCommandLineOption mockMode(QStringLiteral("mock"),
		QGuiApplication::tr("main", "Use mock data source for testing"));
	parser.addOption(mockMode);

	parser.process(*QCoreApplication::instance());

	if (parser.isSet(mqttAddress)) {
		backend->setType(Victron::VenusOS::BackendConnection::MqttSource, parser.value(mqttAddress));
	} else if (parser.isSet(mockMode)) {
		backend->setType(Victron::VenusOS::BackendConnection::MockSource);
	} else {
		const QString address = parser.isSet(dbusDefault) ? QStringLiteral("tcp:host=localhost,port=3000") : parser.value(dbusAddress);
		backend->setType(Victron::VenusOS::BackendConnection::DBusSource, address);
	}
#endif
}

void registerQmlTypes()
{
	/* QML type registrations.  As we (currently) don't create an installed module,
	   we need to register them into the appropriate type namespace manually. */
	qmlRegisterSingletonType<Victron::VenusOS::Theme>(
		"Victron.VenusOS", 2, 0, "Theme",
		&Victron::VenusOS::Theme::instance);
	qmlRegisterSingletonType<Victron::VenusOS::BackendConnection>(
		"Victron.VenusOS", 2, 0, "BackendConnection",
		&Victron::VenusOS::BackendConnection::instance);
	qmlRegisterSingletonType<Victron::VenusOS::Language>(
		"Victron.VenusOS", 2, 0, "Language",
		[](QQmlEngine *engine, QJSEngine *) -> QObject* {
			return new Victron::VenusOS::Language(engine);
		});
	qmlRegisterSingletonType<Victron::VenusOS::Enums>(
		"Victron.VenusOS", 2, 0, "VenusOS",
		&Victron::VenusOS::Enums::instance);
	qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/components/VenusFont.qml")),
		"Victron.VenusOS", 2, 0, "VenusFont");
	qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/components/CommonWords.qml")),
		"Victron.VenusOS", 2, 0, "CommonWords");
	qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/Global.qml")),
		"Victron.VenusOS", 2, 0, "Global");
	qmlRegisterSingletonType<Victron::VenusOS::ActiveNotificationsModel>(
		"Victron.VenusOS", 2, 0, "ActiveNotificationsModel",
		[](QQmlEngine *, QJSEngine *) -> QObject * {
		return Victron::VenusOS::ActiveNotificationsModel::instance();
	});
	qmlRegisterSingletonType<Victron::VenusOS::HistoricalNotificationsModel>(
		"Victron.VenusOS", 2, 0, "HistoricalNotificationsModel",
		[](QQmlEngine *, QJSEngine *) -> QObject * {
		return Victron::VenusOS::HistoricalNotificationsModel::instance();
	});
	qmlRegisterSingletonType<Victron::VenusOS::ClockTime>(
		"Victron.VenusOS", 2, 0, "ClockTime",
		[](QQmlEngine *, QJSEngine *) -> QObject * {
		return Victron::VenusOS::ClockTime::instance();
	});
	qmlRegisterSingletonType<Victron::VenusOS::UidHelper>(
		"Victron.VenusOS", 2, 0, "UidHelper",
		&Victron::VenusOS::UidHelper::instance);

	/* main content */
	qmlRegisterType(QUrl(QStringLiteral("qrc:/ApplicationContent.qml")),
		"Victron.VenusOS", 2, 0, "ApplicationContent");

	/* data sources */
	qmlRegisterType(QUrl(QStringLiteral("qrc:/data/DataManager.qml")),
		"Victron.VenusOS", 2, 0, "DataManager");

	/* controls */
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/controls/Button.qml")),
		"Victron.VenusOS", 2, 0, "Button");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/controls/ListItemButton.qml")),
		"Victron.VenusOS", 2, 0, "ListItemButton");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/controls/Label.qml")),
		"Victron.VenusOS", 2, 0, "Label");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/controls/ProgressBar.qml")),
		"Victron.VenusOS", 2, 0, "ProgressBar");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/controls/RadioButton.qml")),
		"Victron.VenusOS", 2, 0, "RadioButton");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/controls/ScrollBar.qml")),
		"Victron.VenusOS", 2, 0, "ScrollBar");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/controls/Slider.qml")),
		"Victron.VenusOS", 2, 0, "Slider");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/controls/SpinBox.qml")),
		"Victron.VenusOS", 2, 0, "SpinBox");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/controls/Switch.qml")),
		"Victron.VenusOS", 2, 0, "Switch");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/controls/TextField.qml")),
		"Victron.VenusOS", 2, 0, "TextField");

	/* components */
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ActionButton.qml")),
		"Victron.VenusOS", 2, 0, "ActionButton");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/Arc.qml")),
		"Victron.VenusOS", 2, 0, "Arc");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ArcGauge.qml")),
		"Victron.VenusOS", 2, 0, "ArcGauge");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ArcGaugeQuantityLabel.qml")),
		"Victron.VenusOS", 2, 0, "ArcGaugeQuantityLabel");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/AsymmetricRoundedRectangle.qml")),
		"Victron.VenusOS", 2, 0, "AsymmetricRoundedRectangle");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ButtonControlValue.qml")),
		"Victron.VenusOS", 2, 0, "ButtonControlValue");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/CircularMultiGauge.qml")),
		"Victron.VenusOS", 2, 0, "CircularMultiGauge");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/CircularSingleGauge.qml")),
		"Victron.VenusOS", 2, 0, "CircularSingleGauge");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ControlCard.qml")),
		"Victron.VenusOS", 2, 0, "ControlCard");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ControlValue.qml")),
		"Victron.VenusOS", 2, 0, "ControlValue");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/EnvironmentGauge.qml")),
		"Victron.VenusOS", 2, 0, "EnvironmentGauge");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/EnvironmentGaugePanel.qml")),
		"Victron.VenusOS", 2, 0, "EnvironmentGaugePanel");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ExpandedTanksView.qml")),
		"Victron.VenusOS", 2, 0, "ExpandedTanksView");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/InputPanel.qml")),
		"Victron.VenusOS", 2, 0, "InputPanel");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/GeneratorIconLabel.qml")),
		"Victron.VenusOS", 2, 0, "GeneratorIconLabel");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/LevelsPageGaugeDelegate.qml")),
		"Victron.VenusOS", 2, 0, "LevelsPageGaugeDelegate");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/LoadGraph.qml")),
		"Victron.VenusOS", 2, 0, "LoadGraph");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/LoadGraphShapePath.qml")),
		"Victron.VenusOS", 2, 0, "LoadGraphShapePath");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/MainView.qml")),
		"Victron.VenusOS", 2, 0, "MainView");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ModalDialog.qml")),
		"Victron.VenusOS", 2, 0, "ModalDialog");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/NavBar.qml")),
		"Victron.VenusOS", 2, 0, "NavBar");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/NavButton.qml")),
		"Victron.VenusOS", 2, 0, "NavButton");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/NotificationDelegate.qml")),
		"Victron.VenusOS", 2, 0, "NotificationDelegate");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/NotificationsView.qml")),
		"Victron.VenusOS", 2, 0, "NotificationsView");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/Page.qml")),
		"Victron.VenusOS", 2, 0, "Page");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/PageStack.qml")),
		"Victron.VenusOS", 2, 0, "PageStack");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ProgressArc.qml")),
		"Victron.VenusOS", 2, 0, "ProgressArc");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/QuantityLabel.qml")),
		"Victron.VenusOS", 2, 0, "QuantityLabel");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/EnergyQuantityLabel.qml")),
		"Victron.VenusOS", 2, 0, "EnergyQuantityLabel");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/RadioButtonControlValue.qml")),
		"Victron.VenusOS", 2, 0, "RadioButtonControlValue");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ScaledArc.qml")),
		"Victron.VenusOS", 2, 0, "ScaledArc");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ScaledArcGauge.qml")),
		"Victron.VenusOS", 2, 0, "ScaledArcGauge");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/SegmentedButtonRow.qml")),
		"Victron.VenusOS", 2, 0, "SegmentedButtonRow");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/SeparatorBar.qml")),
		"Victron.VenusOS", 2, 0, "SeparatorBar");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ShinyProgressArc.qml")),
		"Victron.VenusOS", 2, 0, "ShinyProgressArc");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/SideGauge.qml")),
		"Victron.VenusOS", 2, 0, "SideGauge");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/SolarYieldGauge.qml")),
		"Victron.VenusOS", 2, 0, "SolarYieldGauge");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/SolarYieldGraph.qml")),
		"Victron.VenusOS", 2, 0, "SolarYieldGraph");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/Spacer.qml")),
		"Victron.VenusOS", 2, 0, "Spacer");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/SplashView.qml")),
		"Victron.VenusOS", 2, 0, "SplashView");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/StatusBar.qml")),
		"Victron.VenusOS", 2, 0, "StatusBar");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/SwitchControlValue.qml")),
		"Victron.VenusOS", 2, 0, "SwitchControlValue");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/TabBar.qml")),
		"Victron.VenusOS", 2, 0, "TabBar");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/TankGauge.qml")),
		"Victron.VenusOS", 2, 0, "TankGauge");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ThreePhaseDisplay.qml")),
		"Victron.VenusOS", 2, 0, "ThreePhaseDisplay");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/TimeSelector.qml")),
		"Victron.VenusOS", 2, 0, "TimeSelector");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ToastNotification.qml")),
		"Victron.VenusOS", 2, 0, "ToastNotification");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/ViewGradient.qml")),
		"Victron.VenusOS", 2, 0, "ViewGradient");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/WeatherDetails.qml")),
		"Victron.VenusOS", 2, 0, "WeatherDetails");

	/* data points */
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/datapoints/DataPoint.qml")),
		"Victron.VenusOS", 2, 0, "DataPoint");

	/* settings list items */
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsLabel.qml")),
		"Victron.VenusOS", 2, 0, "SettingsLabel");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListButton.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListButton");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListCGwacsBatterySchedule.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListCGwacsBatterySchedule");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListItem.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListItem");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListTextItem.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListTextItem");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListView.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListView");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListNavigationItem.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListNavigationItem");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListRadioButton.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListRadioButton");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListRadioButtonGroup.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListRadioButtonGroup");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListSlider.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListSlider");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListSpinBox.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListSpinBox");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListSwitch.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListSwitch");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListDvccSwitch.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListDvccSwitch");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListTextField.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListTextField");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListIpAddressField.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListIpAddressField");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListTextGroup.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListTextGroup");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsListTimeSelector.qml")),
		"Victron.VenusOS", 2, 0, "SettingsListTimeSelector");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/SettingsSlider.qml")),
		"Victron.VenusOS", 2, 0, "SettingsSlider");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/TemperatureRelayNavigationItem.qml")),
		"Victron.VenusOS", 2, 0, "TemperatureRelayNavigationItem");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/settings/TemperatureRelaySettings.qml")),
		"Victron.VenusOS", 2, 0, "TemperatureRelaySettings");

	/* widgets */
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/OverviewWidget.qml")),
		"Victron.VenusOS", 2, 0, "OverviewWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/SegmentedWidgetBackground.qml")),
		"Victron.VenusOS", 2, 0, "SegmentedWidgetBackground");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/AlternatorWidget.qml")),
		"Victron.VenusOS", 2, 0, "AlternatorWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/AcGeneratorWidget.qml")),
		"Victron.VenusOS", 2, 0, "AcGeneratorWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/DcGeneratorWidget.qml")),
		"Victron.VenusOS", 2, 0, "DcGeneratorWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/GridWidget.qml")),
		"Victron.VenusOS", 2, 0, "GridWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/ShoreWidget.qml")),
		"Victron.VenusOS", 2, 0, "ShoreWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/SolarYieldWidget.qml")),
		"Victron.VenusOS", 2, 0, "SolarYieldWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/WindWidget.qml")),
		"Victron.VenusOS", 2, 0, "WindWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/InverterWidget.qml")),
		"Victron.VenusOS", 2, 0, "InverterWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/BatteryWidget.qml")),
		"Victron.VenusOS", 2, 0, "BatteryWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/AcLoadsWidget.qml")),
		"Victron.VenusOS", 2, 0, "AcLoadsWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/DcLoadsWidget.qml")),
		"Victron.VenusOS", 2, 0, "DcLoadsWidget");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/VerticalGauge.qml")),
		"Victron.VenusOS", 2, 0, "VerticalGauge");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/WidgetConnector.qml")),
		"Victron.VenusOS", 2, 0, "WidgetConnector");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/WidgetConnectorAnchor.qml")),
		"Victron.VenusOS", 2, 0, "WidgetConnectorAnchor");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/WidgetConnectorPath.qml")),
		"Victron.VenusOS", 2, 0, "WidgetConnectorPath");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/components/widgets/WidgetHeader.qml")),
		"Victron.VenusOS", 2, 0, "WidgetHeader");

	/* control cards */
	qmlRegisterType(QUrl(QStringLiteral("qrc:/controlcards/ESSCard.qml")),
		"Victron.VenusOS", 2, 0, "ESSCard");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/controlcards/GeneratorCard.qml")),
		"Victron.VenusOS", 2, 0, "GeneratorCard");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/controlcards/InverterCard.qml")),
		"Victron.VenusOS", 2, 0, "InverterCard");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/controlcards/SwitchesCard.qml")),
		"Victron.VenusOS", 2, 0, "SwitchesCard");

	/* dialogs */
	qmlRegisterType(QUrl(QStringLiteral("qrc:/dialogs/DialogManager.qml")),
		"Victron.VenusOS", 2, 0, "DialogManager");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/dialogs/ModalDialog.qml")),
		"Victron.VenusOS", 2, 0, "ModalDialog");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/dialogs/ModalWarningDialog.qml")),
		"Victron.VenusOS", 2, 0, "ModalWarningDialog");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/dialogs/InputCurrentLimitDialog.qml")),
		"Victron.VenusOS", 2, 0, "InputCurrentLimitDialog");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/dialogs/InverterChargerModeDialog.qml")),
		"Victron.VenusOS", 2, 0, "InverterChargerModeDialog");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/dialogs/GeneratorDisableAutostartDialog.qml")),
		"Victron.VenusOS", 2, 0, "GeneratorDisableAutostartDialog");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/dialogs/GeneratorDurationSelectorDialog.qml")),
		"Victron.VenusOS", 2, 0, "GeneratorDurationSelectorDialog");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/dialogs/NumberSelectorDialog.qml")),
		"Victron.VenusOS", 2, 0, "NumberSelectorDialog");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/dialogs/TimeSelectorDialog.qml")),
		"Victron.VenusOS", 2, 0, "TimeSelectorDialog");

	/* pages */
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/PageManager.qml")),
		"Victron.VenusOS", 2, 0, "PageManager");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/BriefMonitorPanel.qml")),
		"Victron.VenusOS", 2, 0, "BriefMonitorPanel");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/ControlCardsPage.qml")),
		"Victron.VenusOS", 2, 0, "ControlCardsPage");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/EnvironmentTab.qml")),
		"Victron.VenusOS", 2, 0, "EnvironmentTab");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/LevelsPage.qml")),
		"Victron.VenusOS", 2, 0, "LevelsPage");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/MainPage.qml")),
		"Victron.VenusOS", 2, 0, "MainPage");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/OverviewPage.qml")),
		"Victron.VenusOS", 2, 0, "OverviewPage");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/SettingsPage.qml")),
		"Victron.VenusOS", 2, 0, "SettingsPage");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/TanksTab.qml")),
		"Victron.VenusOS", 2, 0, "TanksTab");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/BriefPage.qml")),
		"Victron.VenusOS", 2, 0, "BriefPage");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/NotificationsPage.qml")),
		"Victron.VenusOS", 2, 0, "NotificationsPage");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzAfricaData.qml")),
		"Victron.VenusOS", 2, 0, "TzAfricaData");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzAmericaData.qml")),
		"Victron.VenusOS", 2, 0, "TzAmericaData");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzAntarcticaData.qml")),
		"Victron.VenusOS", 2, 0, "TzAntarcticaData");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzArcticData.qml")),
		"Victron.VenusOS", 2, 0, "TzArcticData");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzAsiaData.qml")),
		"Victron.VenusOS", 2, 0, "TzAsiaData");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzAtlanticData.qml")),
		"Victron.VenusOS", 2, 0, "TzAtlanticData");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzAustraliaData.qml")),
		"Victron.VenusOS", 2, 0, "TzAustraliaData");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzEtcData.qml")),
		"Victron.VenusOS", 2, 0, "TzEtcData");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzEuropeData.qml")),
		"Victron.VenusOS", 2, 0, "TzEuropeData");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzIndianData.qml")),
		"Victron.VenusOS", 2, 0, "TzIndianData");
	qmlRegisterType(QUrl(QStringLiteral("qrc:/pages/settings/tz/TzPacificData.qml")),
		"Victron.VenusOS", 2, 0, "TzPacificData");

	// These types do not use dbus, so are safe to import even in the Qt Wasm build.
	qmlRegisterType<VeQuickItem>("Victron.Velib", 1, 0, "VeQuickItem");
	qmlRegisterType<VeQItem>("Victron.Velib", 1, 0, "VeQItem");
	qmlRegisterType<VeQItemChildModel>("Victron.Velib", 1, 0, "VeQItemChildModel");
	qmlRegisterType<VeQItemSortDelegate>("Victron.Velib", 1, 0, "VeQItemSortDelegate");
	qmlRegisterType<VeQItemSortTableModel>("Victron.Velib", 1, 0, "VeQItemSortTableModel");
	qmlRegisterType<VeQItemTableModel>("Victron.Velib", 1, 0, "VeQItemTableModel");

	qmlRegisterType<Victron::VenusOS::SingleUidHelper>("Victron.VenusOS", 2, 0, "SingleUidHelper");
	qmlRegisterType<Victron::VenusOS::LanguageModel>("Victron.VenusOS", 2, 0, "LanguageModel");

#if !defined(VENUS_WEBASSEMBLY_BUILD)
	qmlRegisterType<CmTechnology>("net.connman", 0, 1, "CmTechnology");
	qmlRegisterType<CmService>("net.connman", 0, 1, "CmService");
	qmlRegisterType<CmAgent>("net.connman", 0, 1, "CmAgent");
	qmlRegisterType<ClockModel>("net.connman", 0, 1, "ClockModel");
	qmlRegisterSingletonType<CmManager>("net.connman", 0, 1, "Connman", &connmanInstance);
#endif
}

} // namespace


int main(int argc, char *argv[])
{
	qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

	registerQmlTypes();

	QGuiApplication app(argc, argv);
	QGuiApplication::setApplicationName("Venus");
	QGuiApplication::setApplicationVersion("2.0");

	initBackend();

	QQmlEngine engine;
	engine.setProperty("colorScheme", Victron::VenusOS::Theme::Dark);

	/* Force construction of translator */
	int languageSingletonId = qmlTypeId("Victron.VenusOS", 2, 0, "Language");
	Q_ASSERT(languageSingletonId);
	(void)engine.singletonInstance<Victron::VenusOS::Language*>(languageSingletonId);

#if !defined(VENUS_WEBASSEMBLY_BUILD)
	const QSizeF physicalScreenSize = QGuiApplication::primaryScreen()->physicalSize();
	const int screenDiagonalMm = static_cast<int>(sqrt((physicalScreenSize.width() * physicalScreenSize.width())
			+ (physicalScreenSize.height() * physicalScreenSize.height())));
	engine.setProperty("screenSize", (round(screenDiagonalMm / 10 / 2.5) == 7)
			? Victron::VenusOS::Theme::SevenInch
			: Victron::VenusOS::Theme::FiveInch);
#else
	engine.setProperty("screenSize", Victron::VenusOS::Theme::SevenInch);
#endif

	QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/main.qml")));
	if (component.isError()) {
		qWarning() << component.errorString();
		return EXIT_FAILURE;
	}

	QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
	const auto window = qobject_cast<QQuickWindow *>(object.data());
	if (!window) {
		component.completeCreate();
		qWarning() << "The scene root item is not a window." << object.data();
		return EXIT_FAILURE;
	}

#if defined(VENUS_DESKTOP_BUILD)
	QSurfaceFormat format = window->format();
	format.setSamples(4); // enable MSAA
	window->setFormat(format);
#endif
	engine.setIncubationController(window->incubationController());

	/* Write to window properties here to perform any additional initialization
	   before initial binding evaluation. */
	component.completeCreate();

#if defined(VENUS_DESKTOP_BUILD)
	const bool desktop(true);
#else
	const bool desktop(QGuiApplication::primaryScreen()->availableSize().height() > 600);
#endif
	if (desktop) {
		window->show();
	} else {
		window->showFullScreen();
	}

	return app.exec();
}
