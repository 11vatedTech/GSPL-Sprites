import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property string filePath: ""
    property string content: ""
    property string documentName: "untitled.gspl"
    property bool isDirty: false
    property var diagnostics: []
    
    signal contentChanged()
    signal cursorPositionChanged(int line, int column)
    
    function loadFile(path) {
        root.filePath = path;
        root.documentName = path.split('/').pop().split('\\').pop();
        root.content = editor.text;
    }
    
    function saveFile() {
        root.isDirty = false;
    }
    
    function insertSnippet(snippet) {
        editor.insert(editor.cursorPosition, snippet);
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0
        
        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            color: "#f0f0f0"
            border.bottom: "1px solid #ccc"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                
                Label {
                    text: root.documentName + (root.isDirty ? " *" : "")
                    font.bold: root.isDirty
                    elide: Text.ElideLeft
                }
                
                Item { Layout.fillWidth: true }
                
                Label {
                    text: "Ln " + (editor.cursorLine + 1) + ", Col " + (editor.cursorColumn + 1)
                    color: "#888"
                    font.pixelSize: 11
                }
            }
        }
        
        // Editor area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1e1e1e"
            
            Flickable {
                id: flick
                anchors.fill: parent
                contentWidth: editor.contentWidth
                contentHeight: editor.contentHeight
                clip: true
                
                function ensureVisible(cursorRect) {
                    if (contentX >= cursorRect.x) contentX = cursorRect.x;
                    else if (contentX + width <= cursorRect.x + cursorRect.width) contentX = cursorRect.x + cursorRect.width - width;
                    if (contentY >= cursorRect.y) contentY = cursorRect.y;
                    else if (contentY + height <= cursorRect.y + cursorRect.height) contentY = cursorRect.y + cursorRect.height - height;
                }
                
                TextArea {
                    id: editor
                    width: Math.max(flick.width, contentWidth)
                    height: Math.max(flick.height, contentHeight)
                    
                    font.family: "Consolas"
                    font.pixelSize: 14
                    color: "#d4d4d4"
                    selectionColor: "#264f78"
                    selectedTextColor: "#ffffff"
                    wrapMode: TextEdit.NoWrap
                    
                    placeholderText: "Start typing GSPL code..."
                    placeholderTextColor: "#555"
                    
                    Keys.onPressed: function(event) {
                        if (event.key === Qt.Key_Tab) {
                            editor.insert(editor.cursorPosition, "    ");
                            event.accepted = true;
                        }
                    }
                    
                    onCursorPositionChanged: {
                        var line = editor.text.substring(0, editor.cursorPosition).split('\n').length - 1;
                        var col = editor.cursorPosition - editor.text.lastIndexOf('\n', editor.cursorPosition - 1) - 1;
                        if (col < 0) col = 0;
                        root.cursorPositionChanged(line, col);
                    }
                    
                    onTextChanged: {
                        root.isDirty = true;
                        root.content = editor.text;
                        root.contentChanged();
                    }
                }
            }
        }
        
        // Error squiggles / diagnostics bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.diagnostics.length > 0 ? 24 : 0
            color: "#2d2d2d"
            visible: root.diagnostics.length > 0
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                
                Rectangle {
                    width: 12; height: 12; radius: 6
                    color: "#f44"
                    visible: root.diagnostics.length > 0
                }
                
                Label {
                    text: root.diagnostics.length + " diagnostic(s)"
                    color: "#f44"
                    font.pixelSize: 11
                }
                
                Item { Layout.fillWidth: true }
                
                Label {
                    text: root.diagnostics[0] ? root.diagnostics[0].message : ""
                    color: "#aaa"
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }
            }
        }
    }
}
