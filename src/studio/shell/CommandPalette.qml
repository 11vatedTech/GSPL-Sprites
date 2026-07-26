import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Popup {
    id: root
    property string title: "Command Palette"
    signal commandTriggered(string commandId)
    property var commandModel: null
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: root.palette.window
        border.color: root.palette.mid
        radius: 6
    }

    onOpened: {
        searchField.forceActiveFocus()
        searchField.selectAll()
    }

    function updateFilter(text) {
        if (root.commandModel) {
            root.commandModel.filter = text
            commandListView.currentIndex = root.commandModel.count() > 0 ? 0 : -1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "Type a command..."
            font.pixelSize: 16
            leftPadding: 12
            rightPadding: 12
            topPadding: 10
            bottomPadding: 10
            background: Rectangle {
                color: root.palette.base
                radius: 4
                border.color: root.palette.highlight
            }
            onTextChanged: root.updateFilter(text)
        }

        ListView {
            id: commandListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            clip: true

            model: root.commandModel

            delegate: ItemDelegate {
                id: delegate
                implicitWidth: parent.width
                implicitHeight: 36
                highlighted: ListView.isCurrentItem

                contentItem: RowLayout {
                    spacing: 8
                    Label {
                        text: model.name
                        color: delegate.highlighted ? root.palette.highlightedText : root.palette.windowText
                        font.pixelSize: 14
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Label {
                        text: model.shortcut
                        color: root.palette.mid
                        font.pixelSize: 11
                        font.family: "Consolas"
                    }
                }

                background: Rectangle {
                    color: delegate.highlighted ? root.palette.highlight : "transparent"
                }

                onClicked: {
                    executeCommand(model.commandId)
                    root.close()
                }
            }

            highlightMoveDuration: 100

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (root.commandModel && currentIndex >= 0 && currentIndex < root.commandModel.count()) {
                        root.executeCommand(root.commandModel.commandIdAt(currentIndex))
                        root.close()
                    }
                    event.accepted = true
                } else if (event.key === Qt.Key_Down) {
                    incrementCurrentIndex()
                    event.accepted = true
                } else if (event.key === Qt.Key_Up) {
                    decrementCurrentIndex()
                    event.accepted = true
                }
            }

            ScrollBar.vertical: ScrollBar { }
        }
    }

    function executeCommand(commandId) {
        commandTriggered(commandId)
    }

    Component.onCompleted: {
        updateFilter("")
    }
}
