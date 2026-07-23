import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property var logEntries: []
    property string filterText: ""
    property string levelFilter: "ALL"
    
    function appendEntry(level, component, message) {
        logEntries.push({level: level, component: component, message: message, time: new Date()});
        if (logEntries.length > 1000) logEntries.shift();
    }
    
    function clearLog() {
        logEntries = [];
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        
        RowLayout {
            Layout.fillWidth: true
            ComboBox {
                id: levelCombo
                model: ["ALL", "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"]
                onCurrentTextChanged: root.levelFilter = currentText
            }
            TextField {
                Layout.fillWidth: true
                placeholderText: "Filter logs..."
                onTextChanged: root.filterText = text
            }
            Button { text: "🗑 Clear" }
        }
        
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.logEntries
            clip: true
            
            delegate: Rectangle {
                width: parent.width
                height: 20
                color: {
                    switch (modelData.level) {
                        case "ERROR": return "#3d1a1a"
                        case "WARN": return "#3d3d1a"
                        case "FATAL": return "#5d1a1a"
                        default: return "#1e1e1e"
                    }
                }
                
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    color: {
                        switch (modelData.level) {
                            case "ERROR": return "#f44"
                            case "WARN": return "#fa0"
                            case "FATAL": return "#f22"
                            case "TRACE": return "#888"
                            default: return "#ccc"
                        }
                    }
                    font.family: "Consolas"
                    font.pixelSize: 11
                    text: "[" + modelData.level + "] (" + modelData.component + ") " + modelData.message
                    elide: Text.ElideRight
                }
            }
        }
    }
}
