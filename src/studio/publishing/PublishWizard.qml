import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    property string packagePath: ""
    
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16
        
        Label { text: "Publish Package"; font.pixelSize: 20; font.bold: true }
        
        GroupBox { title: "Version"; Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                RowLayout {
                    Label { text: "Current:" }
                    Label { text: "0.1.0"; font.bold: true }
                    Label { text: "New:" }
                    TextField { text: "0.2.0" }
                }
            }
        }
        
        GroupBox { title: "Channel"; Layout.fillWidth: true
            RowLayout {
                RadioButton { text: "Stable"; checked: true }
                RadioButton { text: "Beta" }
                RadioButton { text: "Nightly" }
            }
        }
        
        GroupBox { title: "Changelog"; Layout.fillWidth: true; Layout.fillHeight: true
            TextArea { anchors.fill: parent; placeholderText: "Describe changes..." }
        }
        
        GroupBox { title: "Target"; Layout.fillWidth: true
            ComboBox { Layout.fillWidth: true; model: ["Local Registry", "GitHub Releases"] }
        }
        
        RowLayout {
            Item { Layout.fillWidth: true }
            Button { text: "Cancel" }
            Button { text: "Publish"; highlighted: true }
        }
    }
}
