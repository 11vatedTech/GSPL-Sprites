import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property var nodes: []
    property var connections: []
    
    Rectangle {
        anchors.fill: parent
        color: "#fcfcfc"
        
        Canvas {
            id: canvas
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.strokeStyle = "#999";
                ctx.lineWidth = 2;
                for (var i = 0; i < root.connections.length; i++) {
                    var c = root.connections[i];
                    ctx.beginPath();
                    ctx.moveTo(c.x1, c.y1);
                    ctx.lineTo(c.x2, c.y2);
                    ctx.stroke();
                }
            }
        }
        
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 36
            color: "#eee"
            Row {
                anchors.centerIn: parent; spacing: 4
                Button { text: "➕ Add Node"; flat: true }
                Button { text: "🔗 Connect"; flat: true }
                Button { text: "⌗ Auto Layout"; flat: true }
                Button { text: "⊡ Zoom To Fit"; flat: true }
            }
        }
    }
}
