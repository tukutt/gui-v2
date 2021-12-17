/*
** Copyright (C) 2021 Victron Energy B.V.
*/

import QtQuick
import Victron.VenusOS

Row {
	id: root

	property alias model: buttonRepeater.model

	signal buttonClicked(buttonIndex: int)

	anchors.horizontalCenter: parent.horizontalCenter
	height: Theme.geometry.navigationBar.height
	spacing: Theme.geometry.navigationBar.spacing

	Repeater {
		id: buttonRepeater

		height: parent.height
		property int currentIndex: 0

		delegate: NavButton {
			height: parent.height
			width: Theme.geometry.navigationBar.button.width
			text: model.text
			icon.source: model.icon
			icon.width: model.iconWidth
			icon.height: model.iconHeight
			checked: buttonRepeater.currentIndex === model.index
			color: checked ? Theme.color.ok : Theme.color.font.tertiary

			onClicked: {
				buttonRepeater.currentIndex = model.index
				root.buttonClicked(model.index)
			}
		}
	}
}
