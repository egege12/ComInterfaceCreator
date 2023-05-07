import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
Item {
    id:mainItem
    Component.onCompleted: {
        root.width= 1336;
        root.height= 860;
        root.x= Screen.width / 2 - root.width / 2
        root.y= Screen.height / 2 - root.height / 2
    }

    height: parent.height
    width : parent.width

    property bool isFileSelected : false;
    property bool isFolderselected : false;

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

        Item {
            id: item1
            y: 240
            width: 640
            height: 270
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 0

            FolderDialog {
                id: folderDialogSaveLoc
                title: "DBC dosyası seçin "
                onAccepted: {
                    var path = folderDialogSaveLoc.currentFolder.toString();
                    // remove prefixed "file:///"
                    path= path.replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,"");
                    // unescape html codes like '%23' for '#'
                    var cleanPath = decodeURIComponent(path);
                    locNameTextField.text = cleanPath
                    mainItem.isFolderselected = true;
                }
                onRejected:{
                    locNameTextField.text = "Dosya konumu seçilmedi"
                    mainItem.isFolderselected = false;
                }
                Component.onCompleted: visible=false;
            }

            MenuButton {
                id : buttonSearchSaveLoc
                x: 512
                y: -30
                width: 45
                height: 45
                buttonImageSource : "qrc:/img/img/iconSearchFile.png"
                radius: 1
                disableButtonClick: false
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 80
                onButtonClicked:{
                    folderDialogSaveLoc.open();
                }
                TextField {
                    id: locNameTextField
                    height: parent.height
                    width: item1.width*0.7
                    anchors{
                        right: parent.left
                        rightMargin: 5
                    }
                    placeholderText: qsTr("Seçim için sağdaki kutucuğa tıklayın")
                    font.family: "Verdana"
                }
            }
            FileDialog {
                id: fileDialog
                title: "Please choose a DBC file"
                nameFilters: ["DBC files (*.dbc)"]
                onAccepted: {
                    var path = fileDialog.currentFile.toString();
                    // remove prefixed "file:///"
                    path= path.replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,"");
                    // unescape html codes like '%23' for '#'
                    var cleanPath = decodeURIComponent(path);
                    fileNameTextField.text = cleanPath
                    mainItem.isFileSelected = true;
                }
                onRejected:{
                    fileNameTextField.text = "Dosya seçilmedi"
                    mainItem.isFileSelected = false;
                }
                Component.onCompleted: visible=false;
            }
            MenuButton {
                id : buttonSearch
                x: 512
                y: 30
                width: 45
                height: 45
                buttonImageSource : "qrc:/img/img/iconSearchFile.png"
                radius: 1
                anchors.right: parent.right
                anchors.rightMargin: 80
                disableButtonClick : false
                onButtonClicked:{
                    fileDialog.open();
                }
                TextField {
                    id: fileNameTextField
                    height: parent.height
                    width: item1.width*0.7
                    anchors{
                        right: parent.left
                        rightMargin: 5

                    }
                    placeholderText: qsTr("Seçim için sağdaki kutucuğa tıklayın")
                    font.family: "Verdana"

                }
            }
//Next page button
            MenuButton {
                id : buttonToPage1
                x: 517
                y: 188
                width: 84
                height: 46
                buttonImageSource : "qrc:/img/img/arrowRight.png"
                clip: false
                opacity: 1
                visible: true
                radius: 1
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 79
                anchors.bottomMargin: 35
                disableButtonClick: (mainItem.isFileSelected==false) | (mainItem.isFolderselected==false)
            }//buttonToPage1
            MenuButton{
                id:buttonTurnToComType
                width: 84
                height: 46
                Text{
                    anchors.centerIn: parent
                    text: qsTr("Geri")
                    color:"white"
                    font.hintingPreference: Font.PreferNoHinting
                    style: Text.Normal
                    focus: false
                    font.weight: Font.Medium
                    font.family: "Verdana"
                }

                clip: false
                opacity: 1
                visible: true
                radius: 1
                anchors.right: buttonToPage1.left
                anchors.rightMargin : (fileNameTextField.width + buttonSearch.width + 5) - (buttonToPage1.width + buttonTurnToComType.width)
                anchors.top :buttonToPage1.top
                disableButtonClick: false
            }

            Text {
                id: text1
                y: 8
                width: 224
                height: 18
                text: qsTr("DBC Dosyası Seçin")
                elide: Text.ElideRight
                anchors.left: parent.left
                anchors.bottom: buttonSearch.top
                font.pixelSize: 16
                anchors.bottomMargin: 4
                anchors.leftMargin: 61
                font.family: "Verdana"
                font.hintingPreference: Font.PreferNoHinting
                style: Text.Normal
                focus: false
                antialiasing: true
                font.weight: Font.Medium
            }

            Text {
                id: text2
                y: 90
                width: 224
                height: 18
                text: qsTr("Kayıt Konumu Seçin")
                elide: Text.ElideRight
                anchors.left: parent.left
                anchors.bottom: buttonSearchSaveLoc.top
                font.pixelSize: 16
                anchors.leftMargin: 61
                anchors.bottomMargin: 5
                antialiasing: true
                font.hintingPreference: Font.PreferNoHinting
                style: Text.Normal
                focus: false
                font.weight: Font.Medium
                font.family: "Verdana"
            }
        }

        Item {
            id: item2
            width: 640
            height: 270
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.horizontalCenter: parent.horizontalCenter

            Image {
                id: logo
                x: 16
                y: 276
                source: "qrc:/img/img/imgLogo.png"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 70
                width: parent.width*.55
                height: 228
                fillMode:Image.PreserveAspectFit
                mipmap:true
            }
        }
    }
    Connections{
        target: buttonToPage1
        onButtonClicked:  {
            comObj.readFile(fileNameTextField.text)
            comObj.setFolderLoc(locNameTextField.text)
        }
    }
    Connections{
        target: comObj
        onFileandLockOk:stack.push("MessageSelectPage.qml")
    }
    Connections{
        target: buttonTurnToComType
        onButtonClicked:{stack.clear();
                        stack.push("typeMenu.qml");
                        comObj.setComType("");}
    }
}


