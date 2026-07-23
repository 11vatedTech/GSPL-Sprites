import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property int workerCount: 0
    property int activeWorkers: 0
    property real cpuUsage: 0
    property real memoryUsage: 0
    property real diskUsage: 0
    
    GridLayout {
        anchors.fill: parent
        anchors.margins: 16
        columns: 2
        rowSpacing: 16
        columnSpacing: 16
        
        // Worker Status
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: "#f5f5f5"
            radius: 8
            border.color: "#ddd"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                Label { text: "Worker Processes"; font.bold: true; font.pixelSize: 14 }
                Label { text: "Active: " + root.activeWorkers + " / " + root.workerCount }
                Rectangle {
                    width: parent.width; height: 8; radius: 4
                    color: "#ddd"
                    Rectangle {
                        height: 8; radius: 4
                        color: root.activeWorkers === root.workerCount ? "#4caf50" : "#ff9800"
                        width: parent.width * (root.activeWorkers / Math.max(1, root.workerCount))
                    }
                }
            }
        }
        
        // Resource Usage
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: "#f5f5f5"
            radius: 8
            border.color: "#ddd"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                Label { text: "Resource Usage"; font.bold: true; font.pixelSize: 14 }
                Label { text: "CPU: " + root.cpuUsage.toFixed(1) + "%" }
                Label { text: "Memory: " + root.memoryUsage.toFixed(1) + "%" }
            }
        }
        
        // Recent Activity
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.columnSpan: 2
            color: "#fafafa"
            radius: 8
            border.color: "#ddd"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                Label { text: "Recent Activity"; font.bold: true; font.pixelSize: 14 }
                Label { text: "No recent activity"; color: "#888" }
            }
        }
    }
}
