/*
** Copyright (C) 2023 Victron Energy B.V.
** See LICENSE.txt for license information.
*/

import QtQuick
import Victron.VenusOS

OverviewWidget {
	id: root

	//% "AC Loads"
	title: qsTrId("overview_widget_acloads_title")
	icon.source: "qrc:/images/acloads.svg"
	type: VenusOS.OverviewWidget_Type_AcLoads
	enabled: false

	quantityLabel.dataObject: Global.system.ac.consumption

	extraContentChildren: [
		ThreePhaseDisplay {
			anchors {
				left: parent ? parent.left : undefined
				leftMargin: Theme.geometry_overviewPage_widget_content_horizontalMargin
				right: parent ? parent.right : undefined
				rightMargin: Theme.geometry_overviewPage_widget_content_horizontalMargin
				bottom: parent ? parent.bottom : undefined
				bottomMargin: root.verticalMargin
			}

			visible: model != null && root.size >= VenusOS.OverviewWidget_Size_L
			model: Global.system.ac.consumption.phases.count > 1 ? Global.system.ac.consumption.phases : null
			widgetSize: root.size
			valueType: VenusOS.Gauges_ValueType_RisingPercentage
			phaseModelProperty: "current"
			maximumValue: Global.system.ac.consumption.maximumCurrent
		}
	]
}
