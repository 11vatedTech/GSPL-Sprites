import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Dialog {
    id: root
    title: "About GSPL Authoring Studio"
    closePolicy: Popup.CloseOnEscape

    property bool modal: false

    standardButtons: Dialog.Ok

    background: Rectangle {
        color: root.palette.window
        border.color: root.palette.mid
        radius: 6
    }

    contentItem: ColumnLayout {
        spacing: 8

        Item { Layout.preferredHeight: 8 }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "GSPL Authoring Studio"
            font.pixelSize: 20
            font.bold: true
            color: root.palette.windowText
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Version " + Qt.application.version
            font.pixelSize: 13
            color: root.palette.windowText
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Build: 2026-07-22 (commit d37b88e)"
            font.pixelSize: 11
            color: root.palette.mid
        }

        Item { Layout.preferredHeight: 8 }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.palette.mid
            opacity: 0.3
        }

        Item { Layout.preferredHeight: 4 }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Copyright 2026 GSPL Contributors"
            font.pixelSize: 11
            color: root.palette.windowText
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Licensed under the Apache License, Version 2.0"
            font.pixelSize: 11
            color: root.palette.mid
        }

        Item { Layout.preferredHeight: 8 }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Built with:"
            font.pixelSize: 11
            font.bold: true
            color: root.palette.windowText
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Qt 6, SQLite, zlib, libspng"
            font.pixelSize: 11
            color: root.palette.mid
        }

        Item { Layout.preferredHeight: 8 }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.palette.mid
            opacity: 0.3
        }

        Item { Layout.preferredHeight: 4 }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Made with GSPL"
            font.pixelSize: 14
            font.italic: true
            color: root.palette.highlight
        }

        Item { Layout.preferredHeight: 8 }
    }
}
