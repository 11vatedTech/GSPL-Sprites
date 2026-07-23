import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQuick.Window 6.0
import Qt.labs.platform 1.1

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 800
    title: "GSPL Authoring Studio"

    property bool toolAreasVisible: true
    property int activeToolArea: 0

    palette {
        window: SystemPalette.window
        windowText: SystemPalette.windowText
        base: SystemPalette.base
        text: SystemPalette.text
        button: SystemPalette.button
        buttonText: SystemPalette.buttonText
        highlight: SystemPalette.highlight
        highlightedText: SystemPalette.highlightedText
    }

    SystemPalette { id: SystemPalette }

    Action {
        id: actionNewProject
        text: "New Project"
        shortcut: "Ctrl+N"
        icon.name: "document-new"
    }

    Action {
        id: actionOpenProject
        text: "Open Project"
        shortcut: "Ctrl+O"
        icon.name: "document-open"
    }

    Action {
        id: actionCloseProject
        text: "Close Project"
        shortcut: "Ctrl+W"
        enabled: false
    }

    Action {
        id: actionSave
        text: "Save"
        shortcut: "Ctrl+S"
        icon.name: "document-save"
    }

    Action {
        id: actionSaveAll
        text: "Save All"
        shortcut: "Ctrl+Shift+S"
    }

    Action {
        id: actionExit
        text: "Exit"
        shortcut: "Ctrl+Q"
        onTriggered: Qt.quit()
    }

    Action {
        id: actionUndo
        text: "Undo"
        shortcut: "Ctrl+Z"
        icon.name: "edit-undo"
    }

    Action {
        id: actionRedo
        text: "Redo"
        shortcut: "Ctrl+Shift+Z"
        icon.name: "edit-redo"
    }

    Action {
        id: actionCut
        text: "Cut"
        shortcut: "Ctrl+X"
        icon.name: "edit-cut"
    }

    Action {
        id: actionCopy
        text: "Copy"
        shortcut: "Ctrl+C"
        icon.name: "edit-copy"
    }

    Action {
        id: actionPaste
        text: "Paste"
        shortcut: "Ctrl+V"
        icon.name: "edit-paste"
    }

    Action {
        id: actionToggleToolWindows
        text: "Toggle Tool Windows"
        shortcut: "Ctrl+`"
        checkable: true
        checked: true
        onCheckedChanged: toolAreasVisible = checked
    }

    Action {
        id: actionCommandPalette
        text: "Command Palette"
        shortcut: "Ctrl+Shift+P"
        onTriggered: commandPalette.open()
    }

    Action {
        id: actionPreferences
        text: "Preferences"
        shortcut: "Ctrl+,"
        onTriggered: preferencesDialog.open()
    }

    Action {
        id: actionBuild
        text: "Build"
        shortcut: "Ctrl+B"
    }

    Action {
        id: actionRebuild
        text: "Rebuild"
        shortcut: "Ctrl+Shift+B"
    }

    Action {
        id: actionClean
        text: "Clean"
    }

    Action {
        id: actionAbout
        text: "About GSPL Authoring Studio"
        onTriggered: aboutDialog.open()
    }

    Action {
        id: actionAboutQt
        text: "About Qt"
        onTriggered: Qt.callLater(function() { Qt.openUrlExternally("https://www.qt.io"); })
    }

    menuBar: MenuBar {
        Menu {
            title: "File"
            Action { action: actionNewProject }
            Action { action: actionOpenProject }
            Action { action: actionCloseProject }
            MenuSeparator { }
            Action { action: actionSave }
            Action { action: actionSaveAll }
            MenuSeparator { }
            Action { action: actionExit }
        }

        Menu {
            title: "Edit"
            Action { action: actionUndo }
            Action { action: actionRedo }
            MenuSeparator { }
            Action { action: actionCut }
            Action { action: actionCopy }
            Action { action: actionPaste }
        }

        Menu {
            title: "View"
            Action { action: actionToggleToolWindows }
            MenuSeparator { }
            Menu {
                title: "Themes"
                MenuItem { text: "System Theme"; checkable: true; checked: true }
                MenuItem { text: "Light Theme"; checkable: true }
                MenuItem { text: "Dark Theme"; checkable: true }
            }
        }

        Menu {
            title: "Project"
            Action { action: actionBuild }
            Action { action: actionRebuild }
            Action { action: actionClean }
        }

        Menu {
            title: "Tools"
            Action { action: actionCommandPalette }
            MenuSeparator { }
            Action { action: actionPreferences }
        }

        Menu {
            title: "Help"
            Action { action: actionAbout }
            Action { action: actionAboutQt }
        }
    }

    header: ToolBar {
        visible: toolAreasVisible
        RowLayout {
            anchors.fill: parent
            ToolButton { action: actionNewProject }
            ToolButton { action: actionOpenProject }
            ToolButton { action: actionSave }
            ToolSeparator { }
            ToolButton { action: actionUndo }
            ToolButton { action: actionRedo }
            ToolSeparator { }
            ToolButton { action: actionBuild }
            Item { Layout.fillWidth: true }
            Label {
                text: "GSPL Authoring Studio v0.1.0"
                color: window.palette.mid
                font.pixelSize: 10
            }
        }
    }

    SplitView {
        anchors.fill: parent
        visible: toolAreasVisible
        orientation: Qt.Horizontal

        Pane {
            id: projectTree
            SplitView.minimumWidth: 180
            SplitView.preferredWidth: 250
            SplitView.maximumWidth: 400
            clip: true

            ColumnLayout {
                anchors.fill: parent
                Label {
                    text: "Project Explorer"
                    font.bold: true
                    padding: 8
                    background: Rectangle {
                        color: window.palette.window
                        border.color: window.palette.mid
                    }
                }
                TreeView {
                    id: projectTreeView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: ListModel {
                        ListElement { name: "No project open"; type: "info" }
                    }
                    delegate: Item {
                        implicitHeight: 24
                        implicitWidth: parent.width
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 4
                            text: model.name
                            color: window.palette.windowText
                        }
                    }
                }
            }
        }

        SplitView {
            orientation: Qt.Vertical
            SplitView.fillWidth: true

            Pane {
                id: documentArea
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                clip: true

                Label {
                    anchors.centerIn: parent
                    text: "Open a project to begin editing"
                    color: window.palette.mid
                    font.pixelSize: 16
                }
            }

            Pane {
                id: outputPane
                SplitView.minimumHeight: 60
                SplitView.preferredHeight: 150
                SplitView.maximumHeight: 300
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: "Output"
                            font.bold: true
                            padding: 4
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Clear"
                            flat: true
                            onClicked: outputLog.clear()
                        }
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        TextArea {
                            id: outputLog
                            readOnly: true
                            placeholderText: "Build output and diagnostics appear here"
                            font.family: "Consolas"
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }

        Pane {
            id: propertiesPane
            SplitView.minimumWidth: 150
            SplitView.preferredWidth: 250
            SplitView.maximumWidth: 400
            clip: true

            ColumnLayout {
                anchors.fill: parent
                Label {
                    text: "Properties"
                    font.bold: true
                    padding: 8
                    background: Rectangle {
                        color: window.palette.window
                        border.color: window.palette.mid
                    }
                }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Column {
                        width: parent.width
                        spacing: 4
                        padding: 8
                        Label {
                            text: "No selection"
                            color: window.palette.mid
                        }
                    }
                }
            }
        }
    }

    Pane {
        anchors.fill: parent
        visible: !toolAreasVisible
        Label {
            anchors.centerIn: parent
            text: "Tool windows hidden — press Ctrl+` to restore"
            color: window.palette.mid
            font.pixelSize: 14
        }
    }

    statusBar: StatusBar {
        id: statusBar
        RowLayout {
            anchors.fill: parent
            ProgressBar {
                id: progressIndicator
                indeterminate: false
                visible: false
                Layout.preferredWidth: 120
            }

            Label {
                id: statusLabel
                text: "Ready"
                color: window.palette.windowText
            }

            Item { Layout.fillWidth: true }

            Row {
                spacing: 12
                Label {
                    text: "L:1  C:1"
                    color: window.palette.mid
                    font.family: "Consolas"
                    font.pixelSize: 11
                }
                BusyIndicator {
                    id: workerIndicator
                    visible: false
                    implicitWidth: 14
                    implicitHeight: 14
                }
                Label {
                    id: workerCountLabel
                    text: "0 workers"
                    color: window.palette.mid
                    font.pixelSize: 11
                }
            }
        }
    }

    CommandPalette {
        id: commandPalette
        anchors.centerIn: Overlay.overlay
        width: 500
        height: 400
        modal: true
    }

    PreferencesDialog {
        id: preferencesDialog
        anchors.centerIn: Overlay.overlay
        width: 600
        height: 450
        modal: true
    }

    AboutDialog {
        id: aboutDialog
        anchors.centerIn: Overlay.overlay
        width: 420
        height: 320
        modal: true
    }

    Component.onCompleted: {
        var isDark = colorScheme === Window.Dark
        if (isDark) {
            palette.window = "#2d2d30"
            palette.windowText = "#f0f0f0"
            palette.base = "#1e1e1e"
            palette.text = "#f0f0f0"
            palette.button = "#3c3c3c"
            palette.buttonText = "#f0f0f0"
            palette.highlight = "#264f78"
            palette.highlightedText = "#ffffff"
        }
    }
}
