import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property var artifactTree: []
    
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 4
        
        RowLayout {
            Layout.fillWidth: true
            Button { text: "⟳ Refresh" }
            Button { text: "✔ Validate All" }
            Button { text: "🗑 Prune Cache" }
            Item { Layout.fillWidth: true }
            TextField { placeholderText: "Search artifacts..." }
        }
        
        SplitView {
            Layout.fillWidth: true; Layout.fillHeight: true
            
            TreeView {
                SplitView.minimumWidth: 200
                model: root.artifactTree
                delegate: Text { text: modelData || "artifact"; padding: 4 }
            }
            
            ScrollView {
                Layout.fillWidth: true
                Label { text: "Select an artifact to inspect"; color: "#888"; padding: 16 }
            }
        }
    }
}
