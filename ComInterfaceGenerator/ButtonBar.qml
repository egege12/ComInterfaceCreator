import QtQuick 2.15
import QtQuick.Controls 6.3
import QtQuick.Layouts 1.0
Item {
    id: item
    signal iconSizeClicked()
    signal escapeClicked()
    signal fullScreenClicked()
    property int buttonRadius : height
    property real buttonOpacity : 0.4

        width: 105
        height: 35
    MouseArea{
            width:parent.width
            height:parent.height
            anchors.top: parent.top
            anchors.topMargin:0
            hoverEnabled: true
            onEntered: item.buttonOpacity = 1
            onExited: item.buttonOpacity = 0.4

            MenuButton {
                id : buttonIconSize
                width: parent.width/3
                height: parent.height
                buttonImageSource : "qrc:/img/img/iconIconSizeButton.png"
                clip: false
                buttonOpacity: item.buttonOpacity
                visible: true
                radius: item.buttonRadius
                anchors.right: buttonFullscreen.left
                anchors.left: parent.left
                hoveredColor:"#d6d4d4"
                pressedColor: "#4f4e4e"
                releasedColor: "#878686"
                onButtonClicked: item.iconSizeClicked()

            }

            MenuButton {
                id : buttonFullscreen
                width: parent.width/3
                height: parent.height
                buttonImageSource : "qrc:/img/img/iconFullscreenButton.png"
                clip: false
                buttonOpacity: item.buttonOpacity
                visible: true
                radius: item.buttonRadius
                anchors.horizontalCenter: parent.horizontalCenter
                hoveredColor:"#d6d4d4"
                pressedColor: "#4f4e4e"
                releasedColor: "#878686"
                onButtonClicked: item.fullScreenClicked()

            }
                MenuButton {
                    id : escapeButton
                    width: parent.width/3
                    height: parent.height
                    buttonImageSource : "qrc:/img/img/iconEscapeButton.png"
                    clip: false
                    buttonOpacity: item.buttonOpacity
                    visible: true
                    radius: item.buttonRadius
                    anchors.left: buttonFullscreen.right
                    anchors.right: parent.right
                    hoveredColor:"#f00505"
                    pressedColor: "#969595"
                    releasedColor: "#a80303"
                    onButtonClicked: item.escapeClicked()

                }

        }


}
