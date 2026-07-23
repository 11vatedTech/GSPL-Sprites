import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property var targets: []
    
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 4
        
        Label { text: "Target Adapters"; font.pixelSize: 16; font.bold: true }
        
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true
            model: root.targets
            delegate: Rectangle {
                width: parent.width; height: 70
                color: "#fafafa"; border.color: "#eee"; radius: 4
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 8
                    Label { text: modelData || "Target"; font.bold: true }
                    RowLayout {
                        Label { text: "SDK: detected"; color: "#080" }
                        Item { Layout.fillWidth: true }
                        ComboBox { model: ["Debug", "Release", "MinSize"] }
                    }
                }
            }
        }
    }
}
