import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Pane {
    id: root

    property string accentColor: "#4488ff"
    property string editorFont: "Consolas"
    property int editorFontSize: 12

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16

        Label { text: "Theme Settings"; font.bold: true; font.pixelSize: 16 }

        GroupBox {
            title: "Theme"
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                RowLayout {
                    Label { text: "Color Scheme:" }
                    ComboBox {
                        id: themeSelector
                        model: ["Light", "Dark", "High Contrast", "System Default"]
                    }
                }
                Label { text: "Preview:"; color: "#888" }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    border.color: "#ccc"
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8
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
                anchors.fill: parent
                RowLayout {
                    Label { text: "Accent:" }
                    TextField { id: accentField; text: root.accentColor; placeholderText: "#RRGGBB" }
                    Button { text: "Apply"; onClicked: root.accentColor = accentField.text }
                    Button { text: "Reset"; onClicked: { accentField.text = "#4488ff"; root.accentColor = accentField.text } }
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
                                onClicked: { accentField.text = modelData; root.accentColor = modelData }
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
                anchors.fill: parent
                RowLayout {
                    Label { text: "Editor Font:" }
                    ComboBox {
                        id: fontSelector
                        model: ["JetBrains Mono", "Fira Code", "Cascadia Code", "Consolas", "monospace"]
                        onCurrentTextChanged: root.editorFont = currentText
                    }
                }
                RowLayout {
                    Label { text: "Size:" }
                    SpinBox { id: fontSizeSpinner; from: 8; to: 24; value: root.editorFontSize; onValueChanged: root.editorFontSize = value }
                }
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button { text: "Apply"; highlighted: true }
        }
    }
}
