import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property var abilities: []
    
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8
        Label { text: "Combat Editor"; font.pixelSize: 16; font.bold: true }
        TreeView {
            Layout.fillWidth: true; Layout.fillHeight: true
            model: root.abilities
        }
    }
}
