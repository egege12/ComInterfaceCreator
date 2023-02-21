import QtQuick 2.15

Rectangle {

    property alias id: menuButton
    property string buttonImageSource: ""
    height: width
    radius: width/8
    color :"#707070"
    Image{
        width: parent.width*.8
        height: parent.height*.8
        anchors.centerIn: parent
        source: parent.buttonImageSource
        fillMode:Image.PreserveAspectFit
    }
    MouseArea{
        anchors.fill:parent
        onPressed: parent.color = "#575757"
        onReleased: parent.color = "#707070"
        onClicked: parent.buttonClicked()
    }
}
