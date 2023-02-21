import QtQuick 2.15
import TableModel 0.1
import TableModel2 0.1
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.0

Rectangle {
    id: messageSelPage
    anchors.fill: parent
    anchors.topMargin: 37
    color: "#d3d3d3"
    border.width: 0
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
    ColumnLayout{
        RowLayout{
            id:rowLayout
            Layout.alignment: {Qt.AlignTop}

            property string selectedMessage : "";

            spacing : 2
            Rectangle {
                id:messageRectangle
                Layout.preferredHeight: messageSelPage.height*0.7
                Layout.preferredWidth: messageSelPage.width/2
                anchors.left:parent.left
                anchors.leftMargin: 20
                onWidthChanged: {
                    tableViewMessages.columnWidths[0] = Math.max(150,messageRectangle.width*.3)
                    tableViewMessages.columnWidths[1] = (messageRectangle.width - tableViewMessages.columnWidths[0])*.20
                    tableViewMessages.columnWidths[2] = (messageRectangle.width - tableViewMessages.columnWidths[0])*.10
                    tableViewMessages.columnWidths[3] = (messageRectangle.width - tableViewMessages.columnWidths[0])*.25
                    tableViewMessages.columnWidths[4] = (messageRectangle.width - tableViewMessages.columnWidths[0])*.25
                    tableViewMessages.columnWidths[5] = (messageRectangle.width - tableViewMessages.columnWidths[0])*.20
                    tableViewMessages.forceLayout();
                }

                TableView {
                    id:tableViewMessages
                    anchors.fill: parent
                    columnSpacing: 1
                    rowSpacing: 1
                    clip: true

                    model: TableModel{
                        id: tableMessages}

                    property bool enableVScrollbar: true
                    ScrollBar.vertical: ScrollBar{
                        policy: ((tableViewMessages.height - tableViewMessages.contentHeight) < -3) ?
                                 ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                        visible: ((tableViewMessages.height - tableViewMessages.contentHeight) < -3) ?
                                     true : false
                    }
                    property bool enableHScrollbar: true
                    ScrollBar.horizontal: ScrollBar{
                        policy: (tableViewMessages.width - tableViewMessages.contentWidth < -3) ?
                                 ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                        visible: (tableViewMessages.width - tableViewMessages.contentWidth < -3) ?
                                     true : false
                    }
                    property var columnWidths: [220, 100, 80, 80, 100]
                    columnWidthProvider: function (column) { return columnWidths[column] }


                    delegate: Rectangle {

                        implicitHeight: text.implicitHeight + 2
                        implicitWidth: text.implicitWidth +2
                        color: (heading===true)?"#2d2d2d": (rowLayout.selectedMessage === messageid )? "#EDEDE0":(selected === true)? "#FEBBAA" :"#EDEDF0"

                        Text {
                            id:text
                            text: tabledata
                            width:tableMessages.width
                            padding: 1
                            font.pointSize: 10
                            Layout.alignment: Qt.AlignLeft
                            elide: Text.ElideRight
                            font.preferShaping: false
                            color: "#838383"
                        }
                        MouseArea{
                            anchors.fill: parent
                            onClicked: {
                                if(heading===true){
                                    tableMessages.sortColumn();
                                }else{
                                    comObj.setDisplayReqSignal(messageid)
                                    rowLayout.selectedMessage = messageid
                                }
                            }
                            onDoubleClicked: {
                                if(heading ===false){
                                    comObj.setSelected(messageid)
                                }
                            }
                        }
                        Image{
                            visible: (heading===true)? true : false
                            source: (activesortheader===true)? ((sortheader===true)? "qrc:/img/img/sortDnEnabled.png" :"qrc:/img/img/sortUpEnabled.png"):"qrc:/img/img/sortNotEnable.png"
                            height:parent.height*0.8
                            anchors.right:parent.right
                            anchors.rightMargin: width*0.05
                            fillMode:Image.PreserveAspectFit
                            antialiasing: true
                        }


                    }

                }
            }
            /*Rectangle {
            id:midArea
            Layout.preferredHeight: 400
            Layout.preferredWidth: 70
            Layout.alignment: Qt.AlignCenter

        }*/
            Rectangle {
                id:signalRectangle
                Layout.preferredHeight: messageSelPage.height*0.7
                Layout.preferredWidth: messageSelPage.width/2
                anchors.right:parent.right
                anchors.rightMargin: 20
                anchors.left:messageRectangle.right
                anchors.leftMargin: 5


                onWidthChanged: {
                    tableViewSignals.columnWidths[0] = Math.max(100,signalRectangle.width*.2)
                    tableViewSignals.columnWidths[1] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                    tableViewSignals.columnWidths[2] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                    tableViewSignals.columnWidths[3] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                    tableViewSignals.columnWidths[4] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                    tableViewSignals.columnWidths[5] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                    tableViewSignals.columnWidths[6] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                    tableViewSignals.columnWidths[7] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.133)
                    tableViewSignals.columnWidths[8] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.133)
                    tableViewSignals.columnWidths[9] = Math.max(200,(signalRectangle.width-tableViewSignals.columnWidths[0])*.133)
                    tableViewSignals.forceLayout();
                }

                property string selectedSignalName : ""

                TableView {
                    id:tableViewSignals
                    anchors.fill: parent
                    columnSpacing: 1
                    rowSpacing: 1
                    clip: true



                    model: TableModelSignal{
                        id: tableSignals
                    }



                    property bool enableVScrollbar: true
                    ScrollBar.vertical: ScrollBar{
                        policy: ((tableViewSignals.height - tableViewSignals.contentHeight) < -3) ?
                                 ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                        visible: ((tableViewSignals.height - tableViewSignals.contentHeight) < -3) ?
                                     true : false
                    }
                    property bool enableHScrollbar: true
                    ScrollBar.horizontal: ScrollBar{
                        policy: (tableViewSignals.width - tableViewSignals.contentWidth < -3) ?
                                 ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                        visible: (tableViewSignals.width - tableViewSignals.contentWidth < -3) ?
                                     true : false
                    }
                    property var columnWidths: [220, 100, 80, 80,80,80,80,100,100,100]
                    columnWidthProvider: function (column) {
                        return  columnWidths[column]
                    }


                        delegate: Rectangle {

                            implicitHeight: textSignal.implicitHeight+2
                            implicitWidth: textSignal.implicitWidth+2
                            color: (heading===true)?"#2d2d2d": (signalRectangle.selectedSignalName === messagename )? "#EDEDE0":"#EDEDF0"

                            Text {
                                id:textSignal
                                text: tabledata

                                padding: 1
                                font.pointSize: 10
                                elide: Text.ElideRight
                                font.preferShaping: false
                                color: "#838383"
                                Layout.alignment: Qt.AlignLeft

                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if(heading===true){
                                        tableSignals.sortColumn();
                                    }else{
                                        signalRectangle.selectedSignalName = messagename
                                    }
                                }
                            }
                            Image{
                                visible: (heading===true)? true : false
                                source: (activesortheader===true)? ((sortheader===true)? "qrc:/img/img/sortDnEnabled.png" :"qrc:/img/img/sortUpEnabled.png"):"qrc:/img/img/sortNotEnable.png"
                                height:parent.height*0.8
                                anchors.right:parent.right
                                anchors.rightMargin: width*0.05
                                anchors.top:parent.top
                                anchors.topMargin: 2
                                fillMode:Image.PreserveAspectFit
                                antialiasing: true
                            }

                        }



                }
            }
        }
        RowLayout{
            id:rowLayoutBottom
            Layout.alignment: {Qt.AlignBottom}
            Item {
                id:areaConfig
                Layout.preferredHeight: messageSelPage.height*0.3
                Layout.preferredWidth: messageSelPage.width*0.7
                Layout.alignment: Qt.AlignLeft

                property string dutUnitHeader : "UNIT"
                property string dutIOHeader :"IO"
                property string dutName

                Text{
                    id : textConfig
                    text: qsTr("Seçenekler")
                    width :80
                    height:20
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    font.pixelSize: 18
                    antialiasing: true
                    font.hintingPreference: Font.PreferNoHinting
                    style: Text.Normal
                    focus: false
                    font.weight: Font.Medium
                    font.family: "Verdana"
                }
                Text{
                    id : textHeaderDutCombo
                    text: qsTr("DUT Ünite İsimlendirmesi")
                    width :80
                    height:20
                    anchors.top: textConfig.bottom
                    anchors.topMargin: 10
                    anchors.left:textConfig.left
                    anchors.leftMargin:5
                    font.pixelSize: 14
                    antialiasing: true
                    font.hintingPreference: Font.PreferNoHinting
                    style: Text.Normal
                    focus: false
                    font.weight: Font.Medium
                    font.family: "Verdana"

                }
                ComboBox{
                    id:dutComboBox
                    width: 200
                    height: 46
                    editable:false
                    anchors.top: textHeaderDutCombo.bottom
                    anchors.topMargin: 5
                    anchors.left:textHeaderDutCombo.left
                    anchors.leftMargin:5
                    model:
                        ["BA - Fren Kontrol Ünitesi"          ,
                        "BS - Süspansiyon Kontrol Ünitesi"   ,
                        "BT - Körük Kontrol Ünitesi"         ,
                        "CH - Klima Kontrol Ünitesi"         ,
                        "DE - Kapı Kontrol Ünitesi"          ,
                        "EG - Hata Ayıklama Bilgisayarı"     ,
                        "ME - Batarya Kontrol Ünitesi"       ,
                        "MP - Pantograf Kontrol Ünitesi"     ,
                        "LS - Ses Jeneratörü Kontrolcüsü"    ,
                        "PA - Gaz Pedalı"                    ,
                        "PT - Tahrik Kontrol Ünitesi"        ,
                        "R  - Dijital Sinyal Modülü"         ,
                        "RS - Direksiyon Açı Sensörü"        ,
                        "RV - Araç Kontrol Ünitesi"          ,
                        "VC - Sürü Gösterge Paneli"          ,
                        "XB - Batarya Kontrol Ünitesi"       ,
                        "XB - 24V Batarya Şarj Ünitesi"      ,
                        "XC - Hava Kompresörü Kontrolcüsü"   ,
                        "XP - Yardımcı Güç Kontrol Ünitesi"  ,
                        "XS - Direksiyon Pompası Kontrolcüsü"]
                    onActivated: {
                        var contt = currentText;
                        if(currentIndex !== 11)
                            contt = contt.slice(0,2);
                        else
                            contt = contt.slice(0,1);
                        parent.dutUnitHeader = contt;
                    }
                }//Combobox
                Rectangle{
                    id: seperator1
                    width :5
                    height:100
                    radius: 100
                    anchors.top: textHeaderDutCombo.top
                    anchors.left:textHeaderDutCombo.left
                    anchors.leftMargin: 210
                    color :"#ebedee"
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: "#c1c1c1"
                        }

                        GradientStop {
                            position: 0.6
                            color: "#ebedee"
                        }
                        orientation: Gradient.Horizontal
                    }
                }
                ComboBox{
                    id:ioComboBox
                    width: 200
                    height: 46
                    editable:false
                    anchors.top: textHeaderIOSel.bottom
                    anchors.topMargin: 5
                    anchors.left:textHeaderIOSel.left
                    anchors.leftMargin: 5
                    model:
                        ["II - Gelen Mesajlar"          ,
                        "IO - Giden Mesajlar"]


                    onActivated: {
                        var contt = currentText;
                        contt = contt.slice(0,2);
                        parent.dutIOHeader = contt;
                    }


                }
                Text{
                    id : textHeaderIOSel
                    text: qsTr("Giriş / Çıkış Seçimi")
                    width :80
                    height:20
                    anchors.top: textHeaderDutCombo.top
                    anchors.left:textHeaderDutCombo.left
                    anchors.leftMargin:220
                    font.pixelSize: 14
                    antialiasing: true
                    font.hintingPreference: Font.PreferNoHinting
                    style: Text.Normal
                    focus: false
                    font.weight: Font.Medium
                    font.family: "Verdana"

                }
                Rectangle{
                    id: seperator2
                    width :5
                    height:100
                    radius: 100
                    anchors.top: textHeaderIOSel.top
                    anchors.left:textHeaderIOSel.left
                    anchors.leftMargin: 210
                    color :"#ebedee"
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: "#c1c1c1"
                        }

                        GradientStop {
                            position: 0.6
                            color: "#ebedee"
                        }
                        orientation: Gradient.Horizontal
                    }
                }
                Text{
                    id : textHeaderPreview
                    text: qsTr("Önizleme")
                    width :80
                    height:20
                    anchors.top: textHeaderIOSel.top
                    anchors.left:textHeaderIOSel.left
                    anchors.leftMargin:220
                    font.pixelSize: 14
                    antialiasing: true
                    font.hintingPreference: Font.PreferNoHinting
                    style: Text.Normal
                    focus: false
                    font.weight: Font.Medium
                    font.family: "Verdana"

                }
                TextField{
                    id:textFieldPreview
                    width :130
                    height:46
                    anchors.top: textHeaderPreview.bottom
                    anchors.topMargin: 5
                    anchors.left:textHeaderPreview.left
                    anchors.leftMargin: 5
                    font.pixelSize: 28

                    text: parent.dutIOHeader+parent.dutUnitHeader+"_T"
                }

            }
            Item{
                id:areaGenerate
                Layout.preferredHeight: messageSelPage.height*0.3
                Layout.preferredWidth: messageSelPage.width*0.3
                Layout.alignment: Qt.AlignRight

                MenuButton{
                    id:buttonGenerate
                    width: 84
                    height: 46
                    Text{
                        anchors.centerIn: parent
                        text: qsTr("Oluştur")
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
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 50
                    anchors.bottomMargin: 60
                    disableButtonClick: false
                }
                MenuButton{
                    id:buttonTurnBack
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
                    anchors.right: buttonGenerate.left
                    anchors.rightMargin : 30
                    anchors.top :buttonGenerate.top
                    disableButtonClick: false
                }
            }


        }
    }
    Connections{
        target: comObj
        onInterfaceReady : tableMessages.setTable(comObj.messagesList())
    }
    Connections{
        target: comObj
        onSelectedStatChanged : tableMessages.setTable(comObj.messagesList())
    }
    Connections{
        target: comObj
        onSelectedViewChanged :tableSignals.setTable(comObj.signalsList())
    }
    Connections{
        target:textFieldPreview
        onTextChanged: comObj.setDutName(textFieldPreview.text)
    }
    Connections{
        target: buttonGenerate
        onButtonClicked: comObj.startToGenerate();
    }
    Connections{
        target: buttonTurnBack
        onButtonClicked: {
            comObj.clearData();
            stack.push("fileDialogPage.qml")
        }
    }
    Connections{
        target:ioComboBox
        onActivated: comObj.setIOType(areaConfig.dutIOHeader);
    }
}


