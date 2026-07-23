import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property var installedPackages: []
    
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 4
        
        RowLayout {
            Layout.fillWidth: true
            Button { text: "📦 New Package" }
            Button { text: "📥 Install Package" }
            Button { text: "🔄 Update All" }
            Item { Layout.fillWidth: true }
            TextField { placeholderText: "Search packages..." }
        }
        
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true
            model: root.installedPackages
            delegate: Rectangle {
                width: parent.width; height: 50
                color: "#fafafa"; border.color: "#eee"
                RowLayout {
                    anchors.fill: parent; anchors.margins: 8
                    ColumnLayout {
                        Label { text: modelData || "package"; font.bold: true }
                        Label { text: "v1.0.0"; color: "#888" }
                    }
                    Item { Layout.fillWidth: true }
                    Button { text: "🔧" }
                    Button { text: "🗑" }
                }
            }
        }
    }
}
