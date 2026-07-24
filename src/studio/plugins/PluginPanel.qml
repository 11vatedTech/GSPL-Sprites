import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Pane {
    id: root
    title: "Plugins"

    ColumnLayout {
        anchors.fill: parent

        // Toolbar
        RowLayout {
            Layout.fillWidth: true
            Button { text: "Refresh"; onClicked: pluginModel.refresh() }
            Button { text: "Install..."; onClicked: installDialog.open() }
            Item { Layout.fillWidth: true }
            Label { text: pluginModel.count + " plugins" }
        }

        // Plugin list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: pluginModel
            clip: true
            delegate: Rectangle {
                width: parent.width
                height: 48
                color: index % 2 === 0 ? "#f0f0f0" : "#ffffff"
                border.color: "#ddd"
                RowLayout {
                    anchors.fill: parent; anchors.margins: 8
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: model.name; font.bold: true }
                        Label { text: model.version + " — " + model.author; color: "#666"; font.pixelSize: 11 }
                    }
                    Switch {
                        checked: model.active
                        onCheckedChanged: pluginModel.toggle(index, checked)
                    }
                    Button {
                        text: "Remove"
                        flat: true
                        onClicked: pluginModel.remove(index)
                    }
                }
            }
        }

        // Detail panel
        Frame {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            visible: pluginModel.selectedIndex >= 0
            ColumnLayout {
                Label { text: "Details"; font.bold: true }
                Label { text: pluginModel.selectedDescription; wrapMode: Text.WordWrap }
            }
        }
    }

    Dialog {
        id: installDialog
        title: "Install Plugin"
        standardButtons: Dialog.Ok | Dialog.Cancel
        ColumnLayout {
            Label { text: "Plugin path or registry ID:" }
            TextField { id: pluginPathField; Layout.fillWidth: true; placeholderText: "e.g. publisher/plugin-name" }
        }
        onAccepted: pluginModel.install(pluginPathField.text)
    }
}
