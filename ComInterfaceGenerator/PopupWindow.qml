import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.0
Item {
    id: popupWarning
    property var textMessage;
    signal escapedClicked;
    signal buttonClicked;
    Frame {
        id: frameWarning
        Rectangle{
            id:rectWarning
            width: 400
            height: 200
            border.color: "#636363"
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
            anchors.centerIn: parent
            RowLayout {
                id: rowLayoutWarning
                anchors.fill: parent
                anchors.rightMargin: 10
                anchors.leftMargin: 10
                anchors.bottomMargin: 10
                anchors.topMargin: 10
            }

            Text {
                id: textHeaderWarning
                x: 116
                y: 25
                width: 59
                height: 23
                text: qsTr("Uyarı!")
                anchors.right: parent.right
                font.pixelSize: 18
                font.styleName: "Kalın"
                font.family: "Verdana"
                anchors.rightMargin: 213
            }

            Image {
                id: imgWarning
                x: 21
                y: 56
                width: 84
                height: 87
                source: "qrc:/img/img/600600.png"
                autoTransform: true
                mipmap: false
                mirror: false
                smooth: true
                fillMode: Image.PreserveAspectFit
            }


                Text {
                    id: textDetailWarning
                    text: popupWarning.textMessage
                    x: 116
                    width: 250
                    height: 77
                    elide: Text.ElideLeft
                    anchors.right: parent.right
                    anchors.top: textHeaderWarning.bottom
                    font.pixelSize: 18
                    font.family: "Verdana"
                    anchors.topMargin: 6
                    anchors.rightMargin: 21
                    wrapMode: "WordWrap"
                }


            DelayButton {
                id: buttonWarning
                x: 258
                y: 132
                width: 109
                height: 39
                text: qsTr("Tamam")
                font.pixelSize: 16
                onActivated: popupWarning.buttonClicked();
            }
            MenuButton {
                id : escapePopup
                width: 35
                height: 35
                buttonImageSource : "qrc:/img/img/iconEscapeButton.png"
                clip: false
                opacity: 1
                visible: true
                radius: 1
                anchors.top: parent.top
                anchors.right: parent.right
                hoveredColor:"#f00505"
                pressedColor: "#969595"
                releasedColor: "#a80303"
                onButtonClicked: popupWarning.escapedClicked()
            }
        }
    }

}
