import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property bool isRunning: false
    property var breakpoints: []
    property var callStack: []
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        
        RowLayout {
            Layout.fillWidth: true
            Button { text: "▶ Continue"; enabled: root.isRunning }
            Button { text: "↷ Step Over"; enabled: root.isRunning }
            Button { text: "↪ Step Into"; enabled: root.isRunning }
            Button { text: "↩ Step Out"; enabled: root.isRunning }
            Item { Layout.fillWidth: true }
            Button { text: root.isRunning ? "⏹ Stop" : "▶ Start"; highlighted: !root.isRunning }
        }
        
        SplitView {
            Layout.fillWidth: true; Layout.fillHeight: true
            orientation: Qt.Horizontal
            
            Rectangle {
                SplitView.minimumWidth: 100
                color: "#f5f5f5"
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 4
                    Label { text: "Breakpoints"; font.bold: true }
                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        model: root.breakpoints
                        delegate: Text { text: "Line " + modelData; padding: 4 }
                    }
                }
            }
            
            Rectangle {
                SplitView.minimumWidth: 100
                color: "#fafafa"
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 4
                    Label { text: "Call Stack"; font.bold: true }
                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        model: root.callStack
                        delegate: Text { text: modelData; padding: 4 }
                    }
                }
            }
            
            Rectangle {
                SplitView.minimumWidth: 200
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 4
                    Label { text: "Variables"; font.bold: true }
                    Text { text: "State tree view"; color: "#888" }
                    Item { Layout.fillHeight: true }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField { Layout.fillWidth: true; placeholderText: "Watch expression..." }
                        Button { text: "➕" }
                    }
                }
            }
        }
    }
}
