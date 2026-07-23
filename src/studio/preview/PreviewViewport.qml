import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property int fps: 0
    property string viewMode: "canonical"
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        
        RowLayout {
            Layout.fillWidth: true
            ComboBox {
                id: viewSelector
                model: ["Canonical Model", "Sprite Sheet", "Runtime Simulation"]
                onCurrentIndexChanged: root.viewMode = ["canonical", "spritesheet", "runtime"][currentIndex]
            }
            Label { text: "FPS: " + root.fps }
            Item { Layout.fillWidth: true }
            Button { text: "⏺ Record" }
            Button { text: "📷 Screenshot" }
        }
        
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1a1a2e"
            border.color: "#333"
            
            Text {
                anchors.centerIn: parent
                color: "#666"
                text: {
                    if (viewSelector.currentIndex === 0) return "Canonical Model Viewport"
                    if (viewSelector.currentIndex === 1) return "Sprite Sheet Viewport"
                    return "Runtime Simulation Viewport"
                }
                font.pixelSize: 18
            }
            
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: 8
                width: 120; height: 24
                color: "#00000080"
                radius: 4
                Text {
                    anchors.centerIn: parent
                    color: "#0f0"
                    font.pixelSize: 11
                    text: root.fps + " FPS"
                }
            }
        }
    }
}
