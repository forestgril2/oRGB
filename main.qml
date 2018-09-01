import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.0

Window {
    id: oRGB

    property string path

    visible: true
    width: 640
    height: 480
    title: qsTr("oRGB transform viewer")

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: "file:///home/space/Projects/oRGB"
        onAccepted: {
            console.log("You chose: " + fileDialog.fileUrls)
            oRGB.path = fileDialog.fileUrls[0]
            showFile(oRGB.path)
        }
        onRejected: {
            console.log("Canceled")
            Qt.quit()
        }
        Component.onCompleted: visible = true
    }


    Image {
        id: image
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: transformButton.top
        }
    }

    Button {
        id: transformButton
        width: parent.widht
        height: 20
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        text: "Transform to oRGB"

        contentItem: Text {
                text: transformButton.text
                font: transformButton.font
                opacity: enabled ? 1.0 : 0.3
                color: "black"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

        onClicked: {
            oRGB.transform(oRGB.path)
        }
    }

    Item {
        id: transformer
        signal fileReady(string filePath)

        function transform(filePath) {

            console.log(" ### file being transformed")
            fileReady(filePath)
        }
    }

    function showFile(filePath) {
        image.source = filePath
    }

    function transform(filePath) {
        transformer.fileReady.connect(oRGB.showFile)
        transformer.transform(filePath)
    }
}
