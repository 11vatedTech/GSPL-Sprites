import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Pane {
    id: root

    property int selectedIndex: -1

    ListModel {
        id: pluginModel
        ListElement { name: "Sample Plugin"; version: "0.1.0"; author: "GSPL"; active: false; description: "Example plugin manifest discovered from the bundled SDK sample." }
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Button { text: "Refresh" }
            Button { text: "Install..."; onClicked: installDialog.open() }
            Item { Layout.fillWidth: true }
            Label { text: pluginModel.count + " plugins" }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: pluginModel
            clip: true
            delegate: Rectangle {
                width: ListView.view.width
                height: 48
                color: index % 2 === 0 ? "#f0f0f0" : "#ffffff"
                border.color: "#ddd"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: model.name; font.bold: true }
                        Label { text: model.version + " — " + model.author; color: "#666"; font.pixelSize: 11 }
                    }
                    Switch {
                        checked: model.active
                        onToggled: pluginModel.setProperty(index, "active", checked)
                    }
                    Button {
                        text: "Remove"
                        flat: true
                        onClicked: pluginModel.remove(index)
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: root.selectedIndex = index
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            visible: root.selectedIndex >= 0 && root.selectedIndex < pluginModel.count
            ColumnLayout {
                anchors.fill: parent
                Label { text: "Details"; font.bold: true }
                Label {
                    text: root.selectedIndex >= 0 && root.selectedIndex < pluginModel.count ? pluginModel.get(root.selectedIndex).description : ""
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    Dialog {
        id: installDialog
        title: "Install Plugin"
        standardButtons: Dialog.Ok | Dialog.Cancel
        ColumnLayout {
            anchors.fill: parent
            Label { text: "Plugin path or registry ID:" }
            TextField { id: pluginPathField; Layout.fillWidth: true; placeholderText: "e.g. publisher/plugin-name" }
        }
        onAccepted: pluginModel.append({ name: pluginPathField.text, version: "unknown", author: "local", active: false, description: "Plugin queued for validation." })
    }
}
