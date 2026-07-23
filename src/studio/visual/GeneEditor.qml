import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property string entityName: ""
    property var geneModel: []
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        
        Label {
            text: "Gene Editor — " + (root.entityName || "(no entity)")
            font.pixelSize: 16
            font.bold: true
        }
        
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.geneModel
            delegate: Rectangle {
                width: parent.width
                height: 40
                color: index % 2 === 0 ? "#f5f5f5" : "#ffffff"
                border.color: "#ddd"
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    text: modelData || "gene"
                }
            }
        }
    }
}
