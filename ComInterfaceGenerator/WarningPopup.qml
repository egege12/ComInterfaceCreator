import QtQuick 2.15
import QtQuick.Controls 2.15

    Popup {
            property string  textPopup: " "
            anchors.centerIn: parent
            width:400
            height:200
            modal: true
            focus: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
            contentItem: Text {text: textPopup ; font.pixelSize: 12 }


        }


