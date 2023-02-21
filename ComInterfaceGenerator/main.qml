import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.0

Window {
    id: root
    width: 1080
    height: 615
    visible: true
    property bool fullscreen
    property bool iconSize
    flags: Qt.FramelessWindowHint | Qt.Window | Qt.MaximizeUsingFullscreenGeometryHint  // to hide windows window and still show minimized icon
    title: qsTr("Interface Creator")

    Frame{
        width: parent.width
        height: parent.height
        anchors.bottom:parent.bottom
        anchors.bottomMargin: 2
        anchors.top:parent.top
        anchors.topMargin:2
        anchors.left:parent.left
        anchors.leftMargin:2
        anchors.right:parent.right
        anchors.rightMargin:2
        z:3
        StackView {
            id: stack
            anchors.fill:parent
            initialItem: "fileDialogPage.qml"
            Item{
                anchors.top:parent.top
                width:parent.width
                height:35
                z:2
                ButtonBar{
                    id:windowButtonBar
                    anchors.top: parent.top
                    anchors.topMargin:1
                    anchors.right:parent.right
                    anchors.rightMargin:-6
                    onFullScreenClicked:{
                        if(fullscreen === true){
                            root.showNormal()
                        }else{
                            root.showFullScreen()
                        }
                        fullscreen= !fullscreen}
                    onIconSizeClicked: {
                        if(iconSize === true){
                            root.showNormal()
                        }else{
                            root.showMinimized()
                        }
                        iconSize= !iconSize}
                    onEscapeClicked: Qt.quit()

                }
                Image{
                    source:"qrc:/img/img/headerIcon.png"
                    fillMode: Image.PreserveAspectFit
                    height:parent.height-10
                    antialiasing: true
                    anchors.top: parent.top
                    anchors.topMargin:6
                    anchors.left:parent.left
                    anchors.leftMargin:5
                }

            }
        }

        Popup{
            id:popupWindow
            spacing :5
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
            focus: true
            modal:true
            anchors.centerIn: parent

            PopupWindow{
                id:windowElement
                onButtonClicked:popupWindow.close();
                onEscapedClicked: popupWindow.close();
            }

        }
        Connections{
            target:comObj
            onErrCodeChanged: {
                popupWindow.open();
                windowElement.textMessage = comObj.errCode
            }

        }
    }
    DragHandler {
        onActiveChanged: if(active) startSystemMove();
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        z:1
        property int edges: 2;
        property int edgeOffest: 5;

        function setEdges(x, y) {
            edges = 0;
            if(x < edgeOffest) edges |= Qt.LeftEdge;
            if(x > (width - edgeOffest))  edges |= Qt.RightEdge;
            if(y < edgeOffest) edges |= Qt.TopEdge;
            if(y > (height - edgeOffest)) edges |= Qt.BottomEdge;
        }

        cursorShape: {
            return !containsMouse ? Qt.ArrowCursor:
                                    edges == 3 || edges == 12 ? Qt.SizeFDiagCursor :
                                                                edges == 5 || edges == 10 ? Qt.SizeBDiagCursor :
                                                                                            edges & 9 ? Qt.SizeVerCursor :
                                                                                                        edges & 6 ? Qt.SizeHorCursor : Qt.ArrowCursor;
        }

        onPositionChanged: setEdges(mouseX, mouseY);
        onPressed: {
            setEdges(mouseX, mouseY);
            if(edges && containsMouse) {
                startSystemResize(edges);
            }
        }
    }
}
