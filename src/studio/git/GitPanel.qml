import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property string branch: "main"
    property var changes: []
    
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 4
        
        RowLayout {
            Layout.fillWidth: true
            Label { text: "Branch:"; font.bold: true }
            Label { text: root.branch }
            Item { Layout.fillWidth: true }
            Button { text: "⟳ Refresh" }
        }
        
        RowLayout {
            Layout.fillWidth: true
            Button { text: "◻ Stage All" }
            Button { text: "◻ Unstage All" }
            Button { text: "💬 Commit" }
            TextField { Layout.fillWidth: true; placeholderText: "Commit message..." }
        }
        
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true
            model: root.changes
            delegate: Rectangle {
                width: parent.width; height: 36
                color: "#fafafa"; border.color: "#eee"
                RowLayout {
                    anchors.fill: parent; anchors.margins: 4
                    CheckBox { checked: false }
                    Label { text: "M"; font.bold: true; color: "#fa0" }
                    Label { text: modelData || "file.gspl" }
                }
            }
        }
    }
}
