import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property var segments: []
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        Label { text: "Morphology Editor"; font.pixelSize: 16; font.bold: true }
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: "#f0f0f0"; border.color: "#ccc"
            Text { anchors.centerIn: parent; text: "Morphology Canvas"; color: "#888" }
        }
    }
}
