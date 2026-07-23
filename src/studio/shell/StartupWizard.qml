import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Dialog {
    id: root
    title: "Welcome to GSPL Authoring Studio"
    closePolicy: Dialog.NoAutoClose

    property var recentProjects: []
    property bool visible: false

    signal newProjectRequested()
    signal openProjectRequested()
    signal openRecentRequested(string path)

    standardButtons: Dialog.Close

    onVisibleChanged: {
        if (visible) {
            loadRecentProjects()
        }
    }

    function loadRecentProjects() {
        recentProjects = workspace
            ? workspace.projects().map(function(p) { return p.manifest().name + " — " + p.root() })
            : []
    }

    background: Rectangle {
        color: root.palette.window
        border.color: root.palette.mid
        radius: 8
    }

    contentItem: ColumnLayout {
        spacing: 16

        Item { Layout.preferredHeight: 8 }

        Image {
            id: logo
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/studio/icons/gspl-logo.svg"
            sourceSize.width: 64
            sourceSize.height: 64
            visible: false
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "GSPL Authoring Studio"
            font.pixelSize: 22
            font.bold: true
            color: root.palette.windowText
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Version " + Qt.application.version
            font.pixelSize: 12
            color: root.palette.mid
        }

        Item { Layout.preferredHeight: 8 }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.palette.mid
            opacity: 0.3
        }

        ColumnLayout {
            spacing: 8
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            Button {
                text: "New Project"
                icon.name: "document-new"
                Layout.fillWidth: true
                implicitHeight: 40
                onClicked: {
                    root.newProjectRequested()
                    root.close()
                }
            }

            Button {
                text: "Open Project"
                icon.name: "document-open"
                Layout.fillWidth: true
                implicitHeight: 40
                onClicked: {
                    root.openProjectRequested()
                    root.close()
                }
            }

            Button {
                text: "Clone from Git"
                icon.name: "folder-remote"
                Layout.fillWidth: true
                implicitHeight: 40
                enabled: false
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.palette.mid
            opacity: 0.3
        }

        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            Label {
                text: "Recent Projects"
                font.bold: true
                font.pixelSize: 13
                color: root.palette.windowText
            }

            ListView {
                id: recentList
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(contentHeight, 120)
                clip: true

                model: ListModel { id: recentModel }

                delegate: ItemDelegate {
                    implicitWidth: parent.width
                    implicitHeight: 28

                    contentItem: Label {
                        text: model.text
                        elide: Text.ElideRight
                        color: root.palette.windowText
                        font.pixelSize: 12
                    }

                    onClicked: {
                        root.openRecentRequested(model.path || "")
                        root.close()
                    }
                }

                ScrollBar.vertical: ScrollBar { }
            }

            Label {
                visible: recentModel.count === 0
                text: "No recent projects"
                color: root.palette.mid
                font.pixelSize: 12
                font.italic: true
            }
        }

        Item { Layout.preferredHeight: 8 }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: '<a href="https://gspl.dev/getting-started" style="color: ' + root.palette.highlight + ';">Getting Started</a>'
            textFormat: Text.RichText
            font.pixelSize: 12
            color: root.palette.highlight
            onLinkActivated: function(link) {
                Qt.openUrlExternally(link)
            }
        }

        Item { Layout.preferredHeight: 4 }
    }

    Component.onCompleted: {
        if (workspace && workspace.is_open()) {
            loadRecentProjects()
            for (var i = 0; i < recentProjects.length; i++) {
                recentModel.append({ text: recentProjects[i], path: "" })
            }
        }
    }
}
