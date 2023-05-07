import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
Item {
    id:mainItem
    Component.onCompleted: {
        root.width= 240
        root.height = 200
        root.x= Screen.width / 2 - root.width / 2
        root.y= Screen.height / 2 - root.height / 2
    }
    height: parent.height
    width : parent.width
    anchors.fill: parent


    Rectangle {
        id: background
        color: "#fdfbfb"
        border.width: 0

        anchors.fill : parent
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#fdfbfb"
            }

            GradientStop {
                position: 1
                color: "#ebedee"
            }
            orientation: Gradient.Vertical
        }
        z: 0


        Text{

            anchors.left:parent.left
            anchors.leftMargin:5
            anchors.bottom: buttonCodesysCAN.top
            anchors.bottomMargin: 5
            text:"Haberleşme tipini seçiniz"
            antialiasing: true
            font.hintingPreference: Font.PreferNoHinting
            style: Text.Normal
            focus: false
            font.weight: Font.Medium
            font.pixelSize: 16
            font.family: "Verdana"
            elide: Text.ElideRight
        }


            MenuButton {
                id : buttonCodesysCAN
                width: 105
                height: 105
                x : parent.width/2 - parent.width*.2
                buttonImageSource : "qrc:/img/img/typeCAN.png"
                anchors.left:parent.left
                anchors.leftMargin: 10
                anchors.bottom:parent.bottom
                anchors.bottomMargin:20
                radius: 1
                disableButtonClick: false
                imageRatio: 1.0
                hoveredColor:"#0045F5"
                releasedColor:"transparent"

            }

            MenuButton {
                id : buttonCodesysETH
                width: 105
                height: 105
                buttonImageSource : "qrc:/img/img/typeETH.png"
                radius: 1
                anchors.left:buttonCodesysCAN.right
                anchors.leftMargin: 5
                anchors.bottom:buttonCodesysCAN.bottom
                disableButtonClick : false
                imageRatio: 1.0
                hoveredColor:"#F70202"
                releasedColor:"transparent"


            }

        }

        Connections{
        target: buttonCodesysCAN
        onButtonClicked:{
            comObj.setComType("CAN");
            stack.clear();
            stack.push("fileDialogPage.qml");
        }
        }
        Connections{
        target: buttonCodesysETH
        onButtonClicked:{
            comObj.setComType("ETH");
            stack.clear();
            stack.push("fileDialogPage.qml");
        }
        }


}


