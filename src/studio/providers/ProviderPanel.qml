import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property var providers: []
    
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 4
        
        RowLayout {
            Layout.fillWidth: true
            Button { text: "🔍 Discover Providers" }
            Button { text: "📦 Install Provider" }
            Item { Layout.fillWidth: true }
        }
        
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true
            model: root.providers
            delegate: Rectangle {
                width: parent.width; height: 60
                color: "#fafafa"; border.color: "#eee"; radius: 4
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 8
                    Label { text: modelData || "Provider"; font.bold: true }
                    RowLayout {
                        Label { text: "Status: Enabled"; color: "#080" }
                        Item { Layout.fillWidth: true }
                        Button { text: "⚙ Configure" }
                        Switch { checked: true }
                    }
                }
            }
        }
        
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 100
            color: "#f5f5f5"; border.color: "#ddd"; radius: 4
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 8
                Label { text: "Provider Monitor"; font.bold: true }
                Label { text: "Latency: --  |  Throughput: --  |  Errors: 0" }
            }
        }
    }
}
