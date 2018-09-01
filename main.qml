import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: shortcuts.home
        onAccepted: {
            console.log("You chose: " + fileDialog.fileUrls)
            showFile(fileDialog.fileUrls[0])
        }
        onRejected: {
            console.log("Canceled")
            Qt.quit()
        }
        Component.onCompleted: visible = true
    }

    Image {
        id: image
        anchors.fill: parent
    }

    function showFile(filePath) {
        image.source = filePath
    }
}
