import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

Window {
    id: root
    title: "Create Package"
    width: 540
    height: 600
    modality: Qt.ApplicationModal

    property var creator: null

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16

        StackLayout {
            id: pages
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Page 1: Metadata
            ColumnLayout {
                Label { text: "Package Metadata"; font.bold: true; font.pixelSize: 16 }
                GridLayout {
                    columns: 2; columnSpacing: 8; rowSpacing: 6
                    Label { text: "Publisher:" }
                    TextField { id: tfPublisher; Layout.fillWidth: true; placeholderText: "e.g. mycompany" }
                    Label { text: "Package Name:" }
                    TextField { id: tfPackageName; Layout.fillWidth: true; placeholderText: "e.g. my-entity-pack" }
                    Label { text: "Version:" }
                    TextField { id: tfVersion; Layout.fillWidth: true; text: "0.1.0" }
                    Label { text: "Display Name:" }
                    TextField { id: tfDisplayName; Layout.fillWidth: true; placeholderText: "e.g. My Entity Pack" }
                    Label { text: "Description:" }
                    TextArea { id: taDescription; Layout.fillWidth: true; Layout.preferredHeight: 60 }
                    Label { text: "License:" }
                    ComboBox { id: cbLicense; model: ["UNLICENSED", "MIT", "Apache-2.0", "GPL-3.0", "BSD-3-Clause"]; editable: true }
                }
                Item { Layout.fillHeight: true }
                Label { id: metadataError; color: "red"; visible: false }
            }

            // Page 2: Authors & Tags
            ColumnLayout {
                Label { text: "Authors & Tags"; font.bold: true; font.pixelSize: 16 }
                Label { text: "Authors (one per line):" }
                TextArea { id: taAuthors; Layout.fillWidth: true; Layout.preferredHeight: 80; placeholderText: "Author Name\nauthor@example.com" }
                Label { text: "Tags (comma separated):" }
                TextField { id: tfTags; Layout.fillWidth: true; placeholderText: "e.g. animation, utility, tutorial" }
                Item { Layout.fillHeight: true }
            }

            // Page 3: Dependencies
            ColumnLayout {
                Label { text: "Dependencies"; font.bold: true; font.pixelSize: 16 }
                ListView {
                    id: depList
                    Layout.fillWidth: true; Layout.fillHeight: true
                    model: ListModel { id: depModel }
                    delegate: RowLayout {
                        width: parent.width
                        Label { text: model.package_id; Layout.fillWidth: true }
                        Label { text: model.version; color: "#666" }
                        Button { text: "X"; flat: true; onClicked: depModel.remove(index) }
                    }
                }
                RowLayout {
                    TextField { id: tfDepId; Layout.fillWidth: true; placeholderText: "publisher/package" }
                    TextField { id: tfDepVer; placeholderText: "version" }
                    Button { text: "Add"; onClicked: { depModel.append({package_id: tfDepId.text, version: tfDepVer.text}); tfDepId.text = ""; tfDepVer.text = "" } }
                }
                Item { Layout.fillHeight: true }
            }

            // Page 4: Options
            ColumnLayout {
                Label { text: "Package Options"; font.bold: true; font.pixelSize: 16 }
                CheckBox { id: cbExampleSource; text: "Include example source file"; checked: true }
                CheckBox { id: cbReadme; text: "Include README"; checked: true }
                CheckBox { id: cbSign; text: "Sign package" }
                Label { text: "Repository URL:" }
                TextField { id: tfRepo; Layout.fillWidth: true; placeholderText: "https://github.com/user/repo" }
                Label { text: "Homepage:" }
                TextField { id: tfHomepage; Layout.fillWidth: true; placeholderText: "https://example.com/package" }
                Item { Layout.fillHeight: true }
            }

            // Page 5: Review
            ColumnLayout {
                Label { text: "Review"; font.bold: true; font.pixelSize: 16 }
                ScrollView { Layout.fillWidth: true; Layout.fillHeight: true
                    TextArea {
                        id: reviewText
                        readOnly: true
                        text: "Publisher: " + tfPublisher.text + "\n" +
                              "Package: " + tfPackageName.text + "\n" +
                              "Version: " + tfVersion.text + "\n" +
                              "License: " + cbLicense.currentText + "\n" +
                              "Dependencies: " + depModel.count + "\n" +
                              "Tags: " + tfTags.text
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }

        // Navigation buttons
        RowLayout {
            Layout.fillWidth: true
            Button { text: "< Back"; enabled: pages.currentIndex > 0; onClicked: pages.currentIndex-- }
            Item { Layout.fillWidth: true }
            Label { text: "Step " + (pages.currentIndex + 1) + " of 5" }
            Item { Layout.fillWidth: true }
            Button {
                text: pages.currentIndex < 4 ? "Next >" : "Create"
                highlighted: pages.currentIndex === 4
                onClicked: {
                    if (pages.currentIndex < 4) {
                        pages.currentIndex++
                    } else {
                        finish()
                    }
                }
            }
        }
    }

    function finish() {
        if (creator) {
            var state = {
                publisher: tfPublisher.text,
                package_name: tfPackageName.text,
                version: tfVersion.text,
                display_name: tfDisplayName.text,
                description: taDescription.text,
                license: cbLicense.currentText,
                authors: taAuthors.text.split("\n").filter(function(s) { return s.trim().length > 0 }),
                tags: tfTags.text.split(",").map(function(s) { return s.trim() }).filter(function(s) { return s.length > 0 }),
                include_example_source: cbExampleSource.checked,
                include_readme: cbReadme.checked,
                sign_package: cbSign.checked,
                repository: tfRepo.text,
                homepage: tfHomepage.text
            }
            var deps = []
            for (var i = 0; i < depModel.count; i++) {
                deps.push({package_id: depModel.get(i).package_id, version: depModel.get(i).version})
            }
            state.dependencies = deps
            creator.create(state)
        }
        root.close()
    }
}
