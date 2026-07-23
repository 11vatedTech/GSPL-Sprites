import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Item {
    id: root
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8
        Label { text: "Behavior Editor"; font.pixelSize: 16; font.bold: true }
        ListModel { id: ruleModel }
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true
            model: ruleModel
            delegate: Rectangle {
                width: parent.width; height: 50
                color: "#fafafa"; border.color: "#ddd"
                Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 8; text: "Rule " + (index + 1) }
            }
        }
    }
}
