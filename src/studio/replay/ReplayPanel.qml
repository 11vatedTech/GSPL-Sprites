import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property string traceFile: ""
    property int currentFrame: 0
    property int totalFrames: 0
    
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8
        
        RowLayout {
            Layout.fillWidth: true
            Button { text: "📂 Open Trace" }
            Label { text: root.traceFile || "(no trace loaded)"; color: "#888" }
        }
        
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: "#fafafa"; border.color: "#ddd"
            Text { anchors.centerIn: parent; text: "Replay Viewport"; color: "#aaa"; font.pixelSize: 24 }
        }
        
        RowLayout {
            Layout.fillWidth: true
            Button { text: "⏮"; enabled: root.currentFrame > 0 }
            Button { text: "◀"; enabled: root.currentFrame > 0 }
            Label { text: root.currentFrame + " / " + root.totalFrames }
            Button { text: "▶"; enabled: root.currentFrame < root.totalFrames }
            Button { text: "⏭"; enabled: root.currentFrame < root.totalFrames }
            Slider { Layout.fillWidth: true; from: 0; to: root.totalFrames; value: root.currentFrame }
        }
    }
}
