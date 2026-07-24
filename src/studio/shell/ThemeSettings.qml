import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Pane {
    id: root
    title: "Theme Settings"

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16

        Label { text: "Appearance"; font.bold: true; font.pixelSize: 16 }

        GroupBox {
            title: "Theme"
            Layout.fillWidth: true
            ColumnLayout {
                RowLayout {
                    Label { text: "Color Scheme:" }
                    ComboBox {
                        id: themeSelector
                        model: ["Light", "Dark", "High Contrast", "System Default"]
                        onCurrentIndexChanged: themeManager.selectTheme(currentIndex)
                    }
                }
                Label { text: "Preview:"; color: "#888" }
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 40
                    border.color: "#ccc"
                    RowLayout {
                        anchors.centerIn: parent; spacing: 8
                        Rectangle { width: 24; height: 24; color: "#ffffff"; border.color: "#ccc"; radius: 4 }
                        Rectangle { width: 24; height: 24; color: "#333333"; border.color: "#ccc"; radius: 4 }
                        Rectangle { width: 24; height: 24; color: "#000000"; border.color: "#ff0"; radius: 4 }
                    }
                }
            }
        }

        GroupBox {
            title: "Accent Color"
            Layout.fillWidth: true
            ColumnLayout {
                RowLayout {
                    Label { text: "Accent:" }
                    TextField { id: accentField; text: themeManager.accentColor || "#4488ff"; placeholderText: "#RRGGBB" }
                    Button { text: "Apply"; onClicked: themeManager.setAccentColor(accentField.text) }
                    Button { text: "Reset"; onClicked: { accentField.text = "#4488ff"; themeManager.setAccentColor("") } }
                }
                RowLayout {
                    Repeater {
                        model: ["#4488ff", "#44cc88", "#ff6644", "#aa44ff", "#ffcc00"]
                        Rectangle {
                            width: 24; height: 24; radius: 12; color: modelData
                            border.color: accentField.text === modelData ? "#333" : "transparent"
                            border.width: 2
                            MouseArea {
                                anchors.fill: parent
                                onClicked: { accentField.text = modelData; themeManager.setAccentColor(modelData) }
                            }
                        }
                    }
                }
            }
        }

        GroupBox {
            title: "Font"
            Layout.fillWidth: true
            ColumnLayout {
                RowLayout {
                    Label { text: "Editor Font:" }
                    ComboBox {
                        id: fontSelector
                        model: ["JetBrains Mono", "Fira Code", "Cascadia Code", "Consolas", "monospace"]
                        onCurrentIndexChanged: themeManager.setEditorFont(currentText)
                    }
                }
                RowLayout {
                    Label { text: "Size:" }
                    SpinBox { id: fontSizeSpinner; from: 8; to: 24; value: 12; onValueChanged: themeManager.setFontSize(value) }
                }
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button { text: "Apply"; highlighted: true; onClicked: themeManager.apply() }
        }
    }
}
