/*
** Copyright (C) 2021 Victron Energy B.V.
*/

import QtQuick
import QtQuick.Controls as C
import Victron.VenusOS

C.Dialog {
	id: root

	property int dialogDoneOptions: VenusOS.ModalDialog_DoneOptions_SetAndClose
	property alias canAccept: doneButton.enabled
	property var tryAccept  // optional function: called when accept is attempted, return true if can accept.

	property string acceptText: dialogDoneOptions === VenusOS.ModalDialog_DoneOptions_SetAndClose
			  //% "Set"
			? qsTrId("controlcard_set")
			: CommonWords.ok

	property string rejectText: dialogDoneOptions === VenusOS.ModalDialog_DoneOptions_OkOnly
			? ""
			: dialogDoneOptions === VenusOS.ModalDialog_DoneOptions_OkAndCancel
				//% "Cancel"
				? qsTrId("controlcard_cancel")
				//% "Close"
				: qsTrId("controlcard_close")

	anchors.centerIn: parent
	implicitWidth: background.implicitWidth
	implicitHeight: background.implicitHeight
	verticalPadding: 0
	horizontalPadding: 0
	modal: true

	enter: Transition {
		NumberAnimation { properties: "opacity"; from: 0.0; to: 1.0; duration: Theme.animation.page.fade.duration }
	}
	exit: Transition {
		NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: Theme.animation.page.fade.duration }
	}

	background: Rectangle {
		implicitWidth: Theme.geometry.modalDialog.width
		implicitHeight: Theme.geometry.modalDialog.height
		radius: Theme.geometry.modalDialog.radius
		color: Theme.color.background.secondary
		border.color: Theme.color.modalDialog.border

		DialogShadow {
			backgroundRect: parent
			dialog: root
		}
	}

	header: Item {
		width: parent ? parent.width : 0
		height: headerLabel.y + (root.title.length ? headerLabel.implicitHeight : 0)

		Label {
			id: headerLabel

			anchors {
				top: parent.top
				topMargin: Theme.geometry.modalDialog.header.title.topMargin
			}
			width: parent.width
			horizontalAlignment: Text.AlignHCenter
			color: Theme.color.font.primary
			font.pixelSize: Theme.font.size.body3
			text: root.title
			wrapMode: Text.Wrap
		}
	}

	footer: Item {
		visible: root.dialogDoneOptions !== VenusOS.ModalDialog_DoneOptions_NoOptions
		height: visible ? Theme.geometry.modalDialog.footer.height : 0
		SeparatorBar {
			id: footerTopSeparator
			anchors {
				top: parent.top
				left: parent.left
				right: parent.right
			}
		}
		Button {
			visible: root.dialogDoneOptions !== VenusOS.ModalDialog_DoneOptions_OkOnly
			anchors {
				left: parent.left
				right: footerMidSeparator.left
				top: footerTopSeparator.bottom
				bottom: parent.bottom
			}

			font.pixelSize: Theme.font.size.body2
			color: Theme.color.font.primary
			spacing: 0
			enabled: root.dialogDoneOptions !== VenusOS.ModalDialog_DoneOptions_OkOnly
			text: root.dialogDoneOptions === VenusOS.ModalDialog_DoneOptions_OkOnly ?
					""
				: root.dialogDoneOptions === VenusOS.ModalDialog_DoneOptions_OkAndCancel ?
					//% "Cancel"
					qsTrId("controlcard_cancel")
				: /* SetAndClose */
					//% "Close"
					qsTrId("controlcard_close")
			onClicked: root.reject()
		}
		SeparatorBar {
			id: footerMidSeparator
			visible: root.dialogDoneOptions !== VenusOS.ModalDialog_DoneOptions_OkOnly
			anchors {
				horizontalCenter: parent.horizontalCenter
				bottom: parent.bottom
				bottomMargin: Theme.geometry.modalDialog.footer.midSeparator.margins
				top: parent.top
				topMargin: Theme.geometry.modalDialog.footer.midSeparator.margins
			}
			width: Theme.geometry.modalDialog.footer.midSeparator.width
		}
		Button {
			id: doneButton
			anchors {
				left: root.dialogDoneOptions === VenusOS.ModalDialog_DoneOptions_OkOnly ? parent.left : footerMidSeparator.right
				right: parent.right
				top: parent.top
				bottom: parent.bottom
			}

			font.pixelSize: Theme.font.size.body2
			color: Theme.color.font.primary
			spacing: 0
			text: root.acceptText
			onClicked: {
				if (!!root.tryAccept && !root.tryAccept()) {
					return
				}
				root.accept()
			}
		}
	}
}

