/*
** Copyright (C) 2021 Victron Energy B.V.
*/

import QtQuick
import QtQuick.Controls as C
import Victron.VenusOS

C.Dialog {
	id: root

	property string titleText
	property bool active: false

	enum DialogDoneOptions {
		OkOnly = 0,
		OkAndCancel = 1,
		SetAndClose = 2
	}
	property int dialogDoneOptions: ModalDialog.DialogDoneOptions.SetAndClose

	modal: true

	verticalPadding: 0
	horizontalPadding: 0

	implicitWidth: background.implicitWidth
	implicitHeight: background.implicitHeight

	anchors.centerIn: parent

	enter: Transition {
		NumberAnimation { properties: "opacity"; from: 0.0; to: 1.0; duration: 300 }
	}
	exit: Transition {
		NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 300 }
	}

	background: Rectangle {
		implicitWidth: 624
		implicitHeight: 368
		radius: 8
		color: Theme.controlCardBackgroundColor
		border.color: Theme.separatorBarColor

		Rectangle {
			// TODO: do this with shader, or with border image taking noise sample.
			id: dropshadowRect
			anchors.fill: parent
			anchors.margins: -dropshadowRect.border.width
			color: "transparent"
			border.color: Qt.rgba(0.0, 0.0, 0.0, 0.7)
			border.width: Math.max(root.parent.width - parent.width, root.parent.height - parent.height)
			radius: parent.radius + dropshadowRect.border.width
		}
	}

	header: Item {
		width: parent.width
		height: root.titleText.length ? 64 : 0

		Label {
			anchors {
				top: parent.top
				topMargin: 21
				horizontalCenter: parent.horizontalCenter
			}
			horizontalAlignment: Text.AlignHCenter
			color: Theme.primaryFontColor
			font.pixelSize: Theme.fontSizeLarge
			text: root.titleText
		}
	}

	footer: Item {
		height: 64
		SeparatorBar {
			id: footerTopSeparator
			anchors {
				top: parent.top
				left: parent.left
				right: parent.right
			}
		}
		Button {
			anchors {
				left: parent.left
				right: footerMidSeparator.left
				top: footerTopSeparator.bottom
				bottom: parent.bottom
			}

			font.pixelSize: Theme.fontSizeControlValue
			color: Theme.primaryFontColor
			spacing: 0
			enabled: root.dialogDoneOptions !== ModalDialog.DialogDoneOptions.OkOnly
			text: root.dialogDoneOptions === ModalDialog.DialogDoneOptions.OkOnly ?
					""
				: root.dialogDoneOptions === ModalDialog.DialogDoneOptions.OkAndCancel ?
					//% "Cancel"
					qsTrId("controlcard_cancel")
				: /* SetAndClose */
					//% "Close"
					qsTrId("controlcard_close")
			onClicked: root.reject()
		}
		SeparatorBar {
			id: footerMidSeparator
			anchors {
				horizontalCenter: parent.horizontalCenter
				bottom: parent.bottom
				bottomMargin: 8
				top: parent.top
				topMargin: 8
			}
			width: 1
		}
		Button {
			anchors {
				left: footerMidSeparator.right
				right: parent.right
				top: parent.top
				bottom: parent.bottom
			}

			font.pixelSize: Theme.fontSizeControlValue
			color: Theme.primaryFontColor
			spacing: 0
			text: root.dialogDoneOptions === ModalDialog.DialogDoneOptions.SetAndClose ?
					//% "Set"
					qsTrId("controlcard_set")
				:   //% "Ok"
					qsTrId("controlcard_ok")
			onClicked: root.accept()
		}
	}
}

