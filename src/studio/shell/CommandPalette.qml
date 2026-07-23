import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Popup {
    id: root
    title: "Command Palette"
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property bool modal: false

    background: Rectangle {
        color: root.palette.window
        border.color: root.palette.mid
        radius: 6
    }

    onOpened: {
        searchField.forceActiveFocus()
        searchField.selectAll()
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
            onTextChanged: commandListView.model.updateFilter(text)
        }

        ListView {
            id: commandListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            clip: true

            model: CommandFilterModel {
                id: filterModel
            }

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
                    if (currentItem) {
                        currentItem.clicked()
                    }
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
        switch (commandId) {
        case "newProject":     actionNewProject.trigger(); break;
        case "openProject":    actionOpenProject.trigger(); break;
        case "closeProject":   actionCloseProject.trigger(); break;
        case "save":           actionSave.trigger(); break;
        case "saveAll":        actionSaveAll.trigger(); break;
        case "undo":           actionUndo.trigger(); break;
        case "redo":           actionRedo.trigger(); break;
        case "cut":            actionCut.trigger(); break;
        case "copy":           actionCopy.trigger(); break;
        case "paste":          actionPaste.trigger(); break;
        case "build":          actionBuild.trigger(); break;
        case "rebuild":        actionRebuild.trigger(); break;
        case "clean":          actionClean.trigger(); break;
        case "preferences":    actionPreferences.trigger(); break;
        case "about":          actionAbout.trigger(); break;
        default:               break;
        }
    }

    Component.onCompleted: {
        filterModel.populate()
    }
}

ListModel {
    id: masterCommandList

    ListElement { name: "New Project";            shortcut: "Ctrl+N";        commandId: "newProject" }
    ListElement { name: "Open Project";           shortcut: "Ctrl+O";        commandId: "openProject" }
    ListElement { name: "Close Project";          shortcut: "Ctrl+W";        commandId: "closeProject" }
    ListElement { name: "Save";                   shortcut: "Ctrl+S";        commandId: "save" }
    ListElement { name: "Save All";               shortcut: "Ctrl+Shift+S";  commandId: "saveAll" }
    ListElement { name: "Undo";                   shortcut: "Ctrl+Z";        commandId: "undo" }
    ListElement { name: "Redo";                   shortcut: "Ctrl+Shift+Z";  commandId: "redo" }
    ListElement { name: "Cut";                    shortcut: "Ctrl+X";        commandId: "cut" }
    ListElement { name: "Copy";                   shortcut: "Ctrl+C";        commandId: "copy" }
    ListElement { name: "Paste";                  shortcut: "Ctrl+V";        commandId: "paste" }
    ListElement { name: "Build";                  shortcut: "Ctrl+B";        commandId: "build" }
    ListElement { name: "Rebuild";                shortcut: "Ctrl+Shift+B";  commandId: "rebuild" }
    ListElement { name: "Clean";                  shortcut: "";              commandId: "clean" }
    ListElement { name: "Preferences";            shortcut: "Ctrl+,";        commandId: "preferences" }
    ListElement { name: "About GSPL Studio";      shortcut: "";              commandId: "about" }
}

QtObject {
    id: CommandFilterModel

    property var sourceModel: masterCommandList
    property var filteredItems: []

    function populate() {
        var items = []
        for (var i = 0; i < sourceModel.count; i++) {
            items.push(sourceModel.get(i))
        }
        filteredItems = items
        return filteredItems
    }

    function updateFilter(text) {
        var lower = text.toLowerCase()
        var items = []
        for (var i = 0; i < sourceModel.count; i++) {
            var item = sourceModel.get(i)
            if (lower === "" || item.name.toLowerCase().indexOf(lower) !== -1) {
                items.push(item)
            }
        }
        filteredItems = items
        commandListView.model = filteredItems
        commandListView.currentIndex = 0
    }

    Component.onCompleted: {
        commandListView.model = populate()
    }
}
