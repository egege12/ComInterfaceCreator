import QtQuick 2.15
import TableModel 0.1
import TableModel2 0.1
import ListModel1 0.1
import ListModel2 0.1
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.0
import QtQuick.Layouts

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
    Component.onCompleted: {listModelInfos.setList(comObj.getInfoList())
                            comObj.setDutName(textFieldPreview.text)}
    ColumnLayout{
        RowLayout{
            id:rowLayout
            Layout.alignment: {Qt.AlignTop}
            spacing : 2
            property string selectedMessage : "";

            Rectangle {
                id:messageRectangle
                Layout.preferredHeight: messageSelPage.height*0.7
                Layout.preferredWidth: messageSelPage.width/2
                anchors.left:parent.left
                anchors.leftMargin: 20
                color:"transparent"
                onWidthChanged: {
                    tableViewMessages.columnWidths[0] = 20
                    tableViewMessages.columnWidths[1] = Math.max(210,(messageRectangle.width - tableViewMessages.columnWidths[0])*.35)
                    tableViewMessages.columnWidths[2] = Math.max(70,(messageRectangle.width - tableViewMessages.columnWidths[0])*.15)
                    tableViewMessages.columnWidths[3] = Math.max(50,(messageRectangle.width - tableViewMessages.columnWidths[0])*.10)
                    tableViewMessages.columnWidths[4] = Math.max(100,(messageRectangle.width - tableViewMessages.columnWidths[0])*.20)
                    tableViewMessages.columnWidths[5] = Math.max(90,(messageRectangle.width - tableViewMessages.columnWidths[0])*.20)
                    tableViewMessages.forceLayout();
                }
                Rectangle{
                    id:filterArea
                    color:"transparent"

                    width: parent.width
                    anchors.top: parent.top
                    anchors.left:parent.left
                    height:50
                    Text{
                        id:headerMsgTableSearch
                        text:"Mesajlarda Ara"
                        anchors.top: parent.top
                        anchors.topMargin: 1
                        anchors.left:parent.left
                        anchors.leftMargin:15
                        font.family: "Verdana"
                    }

                    TextField{

                        id:textFieldMsgTableSearch
                        width :130
                        height:20
                        anchors.top: parent.top
                        anchors.topMargin: 20
                        anchors.left:headerMsgTableSearch.left
                        anchors.leftMargin:5
                        font.pixelSize: 14
                        placeholderText: qsTr("Arama...")
                        font.family: "Verdana"


                    }

                }

                TableView {
                    id:tableViewMessages
                    anchors.top: filterArea.bottom
                    anchors.left:parent.left
                    width: parent.width
                    height: parent.height-filterArea.height
                    columnSpacing: 1
                    rowSpacing: 1
                    clip: true

                    property bool isAllSelected :false
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
                        id:scrollbarHMessages
                        parent:tableViewMessages.parent
                        anchors.top: tableViewMessages.bottom
                        anchors.left: tableViewMessages.left
                        anchors.right: tableViewMessages.right
                        policy: (tableViewMessages.width - tableViewMessages.contentWidth < -3) ?
                                    ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                        visible: (tableViewMessages.width - tableViewMessages.contentWidth < -3) ?
                                     true : false

                    }
                    property var columnWidths: [10,150, 100, 80, 80, 100]
                    columnWidthProvider: function (column) { return columnWidths[column] }


                    delegate: Rectangle {

                        implicitHeight: text.implicitHeight + 2
                        implicitWidth: text.implicitWidth +2
                        color: (heading===true)?"#303030": (selected === true)? ((rowLayout.selectedMessage === messageid )? "#7ff27c" :"#1fe81a") : (rowLayout.selectedMessage === messageid )? "#decc73" :"#ebedee"

                        Text {
                            id:text
                            text: tabledata
                            width:tableMessages.width
                            padding: 1
                            font.pointSize: 10
                            Layout.alignment: Qt.AlignLeft
                            elide: Text.ElideRight
                            font.preferShaping: false
                            color: (heading===true)?"#FFFFFF": (selected === true)? ((rowLayout.selectedMessage === messageid )? "#000000" :"#838383") : (rowLayout.selectedMessage === messageid )? "#000000" :"#838383"
                        }
                        MouseArea{
                            anchors.fill: parent
                            onClicked: {
                                if(heading===true){
                                    tableMessages.sortColumn();
                                }else{
                                    if (rowLayout.selectedMessage !== messageid){
                                        comObj.setDisplayReqSignal(messageid)
                                        rowLayout.selectedMessage = messageid
                                    }else{
                                        rowLayout.selectedMessage = "FFFFFFFF"
                                    }
                                    if(rowLayout.selectedMessage !=="FFFFFFFF"){
                                        listModelWarnings.setList(comObj.getMsgWarningList())
                                    }else{
                                        listModelWarnings.setList(comObj.getWarningList())
                                    }

                                }
                            }
                        }
                        Image{
                            visible: (selectioncolumn===true)? false :((heading===true)? true : false)
                            source: (activesortheader===true)? ((sortheader===true)? "qrc:/img/img/sortDnEnabled.png" :"qrc:/img/img/sortUpEnabled.png"):"qrc:/img/img/sortNotEnable.png"
                            height:parent.height*0.8
                            anchors.right:parent.right
                            anchors.rightMargin: width*0.05
                            anchors.verticalCenter: parent.verticalCenter
                            fillMode:Image.PreserveAspectFit
                            antialiasing: true
                            mipmap:true
                        }
                        CheckBox{

                            visible:(selectioncolumn ===true)
                            checked: (heading===true)? tableViewMessages.isAllSelected:selected
                            //indicator: {width:8; height:8;}
                            onClicked: (heading===true)? comObj.setAllSelected():comObj.setSelected(messageid)
                            anchors.fill: parent
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

            Rectangle{
                Layout.preferredHeight: messageSelPage.height*0.7
                Layout.preferredWidth: messageSelPage.width/2
                anchors.right:parent.right
                anchors.rightMargin: 20
                anchors.left:messageRectangle.right
                anchors.leftMargin: 5
                color:"transparent"

                TabBar{
                    id:bar
                    width:parent.width
                    TabButton {
                        text:qsTr("Mesaj İçeriği")
                    }

                    TabButton{
                        text:qsTr("Bilgi Konsolu")
                    }
                }
                StackLayout{
                    anchors.fill: parent
                    anchors.topMargin:20
                    currentIndex:bar.currentIndex

                    Rectangle{
                        id: rectangleSignalWarning
                        anchors.fill: parent
                        color:"transparent"

                        Rectangle{
                            id:headerSignalList
                            anchors.top:parent.top
                            anchors.margins:1
                            height:30
                            width:parent.width
                            color:"transparent"
                            Text{
                                anchors.top: parent.top
                                anchors.left:parent.left
                                anchors.leftMargin:15
                                anchors.verticalCenter: parent.verticalCenter
                                text:"Sinyaller"
                                antialiasing: true
                                font.hintingPreference: Font.PreferNoHinting
                                style: Text.Normal
                                focus: false
                                font.weight: Font.Medium
                                font.pixelSize: 16
                                font.family: "Verdana"
                                elide: Text.ElideRight
                            }
                        }
                        Rectangle {
                            anchors.top:headerSignalList.bottom
                            anchors.margins:1
                            height:(parent.height-(headerSignalList.height+headerWarningList.height))*0.7
                            width:parent.width
                            id:signalRectangle
                            color:"transparent"
                            onWidthChanged: {
                                tableViewSignals.columnWidths[0] = Math.max(100,signalRectangle.width*.2)
                                tableViewSignals.columnWidths[1] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                                tableViewSignals.columnWidths[2] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                                tableViewSignals.columnWidths[3] = Math.max(90,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                                tableViewSignals.columnWidths[4] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                                tableViewSignals.columnWidths[5] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                                tableViewSignals.columnWidths[6] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                                tableViewSignals.columnWidths[7] = Math.max(110,(signalRectangle.width-tableViewSignals.columnWidths[0])*.133)
                                tableViewSignals.columnWidths[8] = Math.max(110,(signalRectangle.width-tableViewSignals.columnWidths[0])*.133)
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
                                    parent:tableViewSignals.parent
                                    anchors.top: tableViewSignals.bottom
                                    anchors.left: tableViewSignals.left
                                    anchors.right: tableViewSignals.right
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
                                    color: (heading===true)?"#303030": (signalRectangle.selectedSignalName === messagename )? "#decc73":"#ebedee"
                                    radius: 1
                                    Text {
                                        id:textSignal
                                        text: tabledata
                                        padding: 1
                                        font.pointSize: 10
                                        elide: Text.ElideRight
                                        font.preferShaping: false
                                        color: (heading===true)?"#FFFFFF": (signalRectangle.selectedSignalName === messagename )? "#000000":"#838383"
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
                                        anchors.verticalCenter: parent.verticalCenter
                                        mipmap:true
                                    }

                                }

                            }
                        }
                        Rectangle{
                            id:headerWarningList
                            anchors.top:signalRectangle.bottom
                            anchors.topMargin:20
                            height:30
                            width:parent.width
                            color:"transparent"
                            Text{
                                anchors.top: parent.top
                                anchors.left:parent.left
                                anchors.leftMargin:15
                                anchors.verticalCenter: parent.verticalCenter
                                text:"Uyarılar"
                                antialiasing: true
                                font.hintingPreference: Font.PreferNoHinting
                                style: Text.Normal
                                focus: false
                                font.weight: Font.Medium
                                font.pixelSize: 16
                                font.family: "Verdana"
                                elide: Text.ElideRight
                            }
                        }
                        Rectangle{
                            id: warningList
                            anchors.bottom:parent.bottom
                            anchors.bottomMargin:1
                            anchors.top:headerWarningList.bottom
                            anchors.topMargin:1
                            height:(parent.height-(headerSignalList.height+headerWarningList.height))*0.3
                            width:parent.width
                            clip:true
                            anchors.margins: 2
                            Item{
                                anchors.fill: parent
                                ListView{
                                    id: listViewWarnings
                                    anchors.fill: parent
                                    model: ListModelWarnings {
                                        id: listModelWarnings
                                    }
                                    property bool enableVScrollbar: true
                                    ScrollBar.vertical: ScrollBar{
                                        policy: ((listViewWarnings.height - listViewWarnings.contentHeight) < -3) ?
                                                    ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                                        visible: ((listViewWarnings.height - listViewWarnings.contentHeight) < -3) ?
                                                     true : false
                                    }
                                    property bool enableHScrollbar: true
                                    ScrollBar.horizontal: ScrollBar{
                                        policy: (listViewWarnings.width - listViewWarnings.contentWidth < -3) ?
                                                    ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                                        visible: (listViewWarnings.width - listViewWarnings.contentWidth < -3) ?
                                                     true : false
                                    }
                                    delegate:Rectangle{
                                        id:delegateRectangle
                                        implicitHeight: textWarnings.implicitHeight+2
                                        implicitWidth: textWarnings.implicitWidth+2
                                        color:"transparent"
                                        Text {
                                            id: textWarnings
                                            width:parent.width
                                            text: listdata
                                            font.pointSize: 10
                                            elide: Text.ElideRight
                                            font.preferShaping: false
                                            Layout.alignment: Qt.AlignLeft
                                            color:"#838383"
                                        }
                                        MouseArea {
                                            anchors.fill: parent

                                            hoverEnabled: true;
                                            onEntered: {delegateRectangle.color= "#decc73"; textWarnings.color = "#000000"}
                                            onExited:{ delegateRectangle.color= "transparent" ; textWarnings.color = "#838383"}
                                        }
                                    }


                                }
                            }





                        }

                    }
                    Item{
                        id: consoleInfo
                        anchors.fill: parent
                        clip:true
                        Item{
                            anchors.fill: parent
                            ListView{
                                id: listViewInfos
                                anchors.fill: parent
                                model: ListModelInfos {
                                    id: listModelInfos
                                }
                                property bool enableVScrollbar: true
                                ScrollBar.vertical: ScrollBar{
                                    policy: ((listViewInfos.height - listViewInfos.contentHeight) < -3) ?
                                                ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                                    visible: ((listViewInfos.height - listViewInfos.contentHeight) < -3) ?
                                                 true : false
                                }
                                property bool enableHScrollbar: true
                                ScrollBar.horizontal: ScrollBar{
                                    policy: (listViewInfos.width - listViewInfos.contentWidth < -3) ?
                                                ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                                    visible: (listViewInfos.width - listViewInfos.contentWidth < -3) ?
                                                 true : false
                                }
                                delegate:Rectangle{
                                    id:delegateRectangleInfo
                                    implicitHeight: textInfos.implicitHeight+2
                                    implicitWidth: textInfos.implicitWidth+2
                                    color:"transparent"
                                    Text {
                                        id: textInfos
                                        width:parent.width
                                        text: listdata
                                        font.pointSize: 10
                                        elide: Text.ElideRight
                                        font.preferShaping: false
                                        Layout.alignment: Qt.AlignLeft
                                        color:generationinfo? "#000000":"#838383"
                                    }
                                    MouseArea {
                                        anchors.fill: parent

                                        hoverEnabled: true;
                                        onEntered: {delegateRectangleInfo.color= "#decc73"; textInfos.color = "#000000"}
                                        onExited:{ delegateRectangleInfo.color= "transparent" ; textInfos.color = generationinfo? "#000000":"#838383"}
                                    }
                                }


                            }
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
                    displayText: "Seçim Yapınız"
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
                        displayText=currentText;
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
                    displayText:"Seçim Yapınız"
                    model:
                        ["II - Gelen Mesajlar"          ,
                        "IO - Giden Mesajlar"]


                    onActivated: {
                        var contt = currentText;
                        contt = contt.slice(0,2);
                        parent.dutIOHeader = contt;
                        displayText=currentText;
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
                Image{
                    id:progressDoneImage
                    visible:false
                    anchors.right: parent.right
                    anchors.rightMargin : 10
                    anchors.bottom :parent.bottom
                    anchors.bottomMargin: 10
                    width:50
                    fillMode:Image.PreserveAspectFit
                    source:"qrc:/img/img/progressSucces.png"
                }
                Rectangle{
                    Timer {
                        id:progressDoneTimer
                        interval: 3000;
                        running: true;
                        repeat:true
                        onTriggered: { progressDoneImage.visible=false}
                    }
                }
                ProgressBar{
                    id:generationProgress
                    value:comObj.progress
                    width:190
                    height: 46
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 50
                    anchors.bottomMargin: 60
                    visible:false
                }
            }


        }
    }
    Connections{
        target: comObj
        onInterfaceReady : {tableMessages.setTable(comObj.messagesList())
            listModelWarnings.setList(comObj.getWarningList())
        }
    }
    Connections{
        target: comObj
        onSelectedStatChanged : tableMessages.updateTable(comObj.messagesList())
    }
    Connections{
        target: comObj
        onSelectedViewChanged :{tableSignals.setTable(comObj.signalsList())

        }
    }
    Connections{
        target: comObj
        onProgressStarted : {buttonGenerate.visible=false
            buttonTurnBack.visible=false
            generationProgress.visible=true}
    }
    Connections{
        target: comObj
        onProgressCompleted : {buttonGenerate.visible=true
            buttonTurnBack.visible=true
            generationProgress.visible=false
            progressDoneTimer.restart()
            progressDoneImage.visible=true}
    }
    Connections{
        target: comObj
        onProgressChanged : generationProgress.value=comObj.progress
    }

    Connections{
        target:textFieldPreview
        onTextChanged: comObj.setDutName(textFieldPreview.text)
    }
    Connections{
        target: buttonGenerate
        onButtonClicked: {comObj.startToGenerate()}
    }
    Connections{
        target: buttonTurnBack
        onButtonClicked: {
            comObj.clearData();
            stack.push("fileDialogPage.qml")
        }
    }
    Connections{
        target:comObj
        onAllSelectedChanged: tableViewMessages.isAllSelected=comObj.getAllSelected();
    }

    Connections{
        target:ioComboBox
        onActivated: comObj.setIOType(areaConfig.dutIOHeader);
    }
    Connections{
        target:textFieldMsgTableSearch
        onTextChanged: tableMessages.search(textFieldMsgTableSearch.text);
    }
    Connections{
        target:comObj
        onInfoListChanged: listModelInfos.setList(comObj.getInfoList())
    }

}


