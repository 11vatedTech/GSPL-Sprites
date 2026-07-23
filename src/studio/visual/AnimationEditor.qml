import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8
        Label { text: "Animation Editor"; font.pixelSize: 16; font.bold: true }
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 60
            color: "#333"; border.color: "#555"
            Row {
                anchors.centerIn: parent; spacing: 8
                Button { text: "◀" }; Button { text: "▶" }
                Button { text: "⏹" }; Button { text: "⏺" }
            }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: "#f9f9f9"; border.color: "#ccc"
            Text { anchors.centerIn: parent; text: "Timeline"; color: "#888" }
        }
    }
}
