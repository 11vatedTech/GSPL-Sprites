import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQuick.Controls.Universal 6.0

Dialog {
    id: root
    title: "Preferences"
    closePolicy: Popup.CloseOnEscape

    property bool modal: false

    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Apply

    onApplied: saveSettings()
    onAccepted: saveSettings()

    background: Rectangle {
        color: root.palette.window
        border.color: root.palette.mid
        radius: 6
    }

    function loadSettings() {
        generalAutosave.value = 300
        generalDefaultDir.text = ""
        editorFontFamily.currentIndex = 0
        editorFontSize.value = 12
        editorTabWidth.value = 4
        editorWordWrap.checked = true
        themeSelector.currentIndex = 0
        accentColorPicker.text = "#0078d4"
        buildProfile.currentIndex = 0
        buildParallel.value = 4
        debuggerGranularity.currentIndex = 0
    }

    function saveSettings() {
    }

    Component.onCompleted: loadSettings()

    contentItem: ColumnLayout {
        spacing: 8

        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton { text: "General" }
            TabButton { text: "Editor" }
            TabButton { text: "Themes" }
            TabButton { text: "Build" }
            TabButton { text: "Debugger" }
        }

        StackLayout {
            id: settingsStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 8
            currentIndex: tabBar.currentIndex

            // General
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 12
                    padding: 8

                    GroupBox {
                        title: "General"
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            RowLayout {
                                spacing: 8
                                Label {
                                    text: "Autosave interval (seconds):"
                                    Layout.fillWidth: true
                                }
                                SpinBox {
                                    id: generalAutosave
                                    from: 0
                                    to: 3600
                                    stepSize: 30
                                    Layout.preferredWidth: 100
                                }
                            }

                            RowLayout {
                                spacing: 8
                                Label {
                                    text: "Default project directory:"
                                    Layout.fillWidth: true
                                }
                                TextField {
                                    id: generalDefaultDir
                                    Layout.preferredWidth: 200
                                    placeholderText: "/home/user/gspl-projects"
                                }
                                Button {
                                    text: "..."
                                    implicitWidth: 30
                                }
                            }
                        }
                    }
                }
            }

            // Editor
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 12
                    padding: 8

                    GroupBox {
                        title: "Editor"
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            RowLayout {
                                spacing: 8
                                Label { text: "Font family:"; Layout.fillWidth: true }
                                ComboBox {
                                    id: editorFontFamily
                                    Layout.preferredWidth: 200
                                    model: ["Consolas", "Courier New", "Fira Code", "JetBrains Mono", "Source Code Pro"]
                                }
                            }

                            RowLayout {
                                spacing: 8
                                Label { text: "Font size:"; Layout.fillWidth: true }
                                SpinBox {
                                    id: editorFontSize
                                    from: 8; to: 36
                                    Layout.preferredWidth: 80
                                }
                            }

                            RowLayout {
                                spacing: 8
                                Label { text: "Tab width:"; Layout.fillWidth: true }
                                SpinBox {
                                    id: editorTabWidth
                                    from: 1; to: 8
                                    Layout.preferredWidth: 80
                                }
                            }

                            RowLayout {
                                spacing: 8
                                Label { text: "Word wrap:"; Layout.fillWidth: true }
                                Switch {
                                    id: editorWordWrap
                                }
                            }
                        }
                    }
                }
            }

            // Themes
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 12
                    padding: 8

                    GroupBox {
                        title: "Themes"
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            RowLayout {
                                spacing: 8
                                Label { text: "Theme:"; Layout.fillWidth: true }
                                ComboBox {
                                    id: themeSelector
                                    Layout.preferredWidth: 200
                                    model: ["System Default", "Light", "Dark", "High Contrast"]
                                }
                            }

                            RowLayout {
                                spacing: 8
                                Label { text: "Accent color:"; Layout.fillWidth: true }
                                TextField {
                                    id: accentColorPicker
                                    Layout.preferredWidth: 120
                                    placeholderText: "#0078d4"
                                    maximumLength: 7
                                }
                                Rectangle {
                                    implicitWidth: 24
                                    implicitHeight: 24
                                    radius: 4
                                    color: accentColorPicker.text
                                    border.color: root.palette.mid
                                }
                            }
                        }
                    }
                }
            }

            // Build
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 12
                    padding: 8

                    GroupBox {
                        title: "Build"
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            RowLayout {
                                spacing: 8
                                Label { text: "Default build profile:"; Layout.fillWidth: true }
                                ComboBox {
                                    id: buildProfile
                                    Layout.preferredWidth: 200
                                    model: ["debug", "release", "release-with-debug", "min-size"]
                                }
                            }

                            RowLayout {
                                spacing: 8
                                Label { text: "Parallel jobs:"; Layout.fillWidth: true }
                                SpinBox {
                                    id: buildParallel
                                    from: 1; to: 64
                                    Layout.preferredWidth: 80
                                }
                            }
                        }
                    }
                }
            }

            // Debugger
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 12
                    padding: 8

                    GroupBox {
                        title: "Debugger"
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            RowLayout {
                                spacing: 8
                                Label { text: "Default capture granularity:"; Layout.fillWidth: true }
                                ComboBox {
                                    id: debuggerGranularity
                                    Layout.preferredWidth: 200
                                    model: ["None", "Frame", "Sprite", "Channel", "Sample"]
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
