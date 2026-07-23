import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property color baseColor: "#ffffff"
    property real opacity: 1.0
    
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8
        Label { text: "Form Editor"; font.pixelSize: 16; font.bold: true }
        GridLayout { columns: 2; Layout.fillWidth: true
            Label { text: "Base Color" }
            Rectangle { width: 80; height: 30; color: root.baseColor; border.color: "#ccc" }
            Label { text: "Opacity" }
            Slider { from: 0; to: 1; value: root.opacity }
        }
    }
}
