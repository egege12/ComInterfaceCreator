import QtQuick 2.15

Rectangle {
    id:buttonRectangle
    signal buttonClicked()
    signal buttonReleased()
    signal buttonEntered()
    signal buttonExited()
    property string buttonImageSource: ""
    property bool disableButtonClick
    property real buttonOpacity : 1
    property color disabledColor : "#9e9b9b"
    property color pressedColor : "#575757"
    property color releasedColor : "#707070"
    property color hoveredColor : "#4A4A4A"
    property real imageRatio : 0.6
    height: width
    radius: width/8
    opacity : buttonRectangle.buttonOpacity
    onDisableButtonClickChanged: disableButtonClick ? disabledColor : releasedColor
    color :disableButtonClick ? disabledColor : releasedColor
    Image{
        width: parent.width*imageRatio
        height: parent.height*imageRatio
        anchors.centerIn: parent
        antialiasing: true
        source: parent.buttonImageSource
        opacity: parent.disableButtonClick ? 0.5 : 1.0
        fillMode:Image.PreserveAspectFit
    }
    MouseArea{
        id:mouseArea
        anchors.fill: parent
        onPressed: parent.color = parent.disableButtonClick ? parent.disabledColor : parent.pressedColor
        onReleased: {
            parent.color = parent.disableButtonClick ? parent.disabledColor : parent.releasedColor
            parent.disableButtonClick ? undefined : parent.buttonReleased()
        }
        onClicked: parent.disableButtonClick ? undefined: parent.buttonClicked()
        hoverEnabled: true
        onEntered: {parent.color = parent.hoveredColor; parent.buttonEntered()}
        onExited: {parent.color = parent.releasedColor; parent.buttonExited()}

    }
}
