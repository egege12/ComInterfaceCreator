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
    Item{
        id:topLayout
        anchors.top: parent.top
        anchors.left:parent.left
        height : parent.height*0.8
        anchors.right: parent.right
        property string selectedMessage : "";

        Rectangle {
            id:messageRectangle
            height: parent.height
            width: parent.width*0.495
            anchors.left:parent.left
            anchors.leftMargin: 20
            color:"transparent"
            onWidthChanged: {
                tableViewMessages.columnWidths[0] = 20
                tableViewMessages.columnWidths[1] = Math.max(210,(messageRectangle.width - tableViewMessages.columnWidths[0])*.35)
                tableViewMessages.columnWidths[2] = Math.max(60,(messageRectangle.width - tableViewMessages.columnWidths[0])*.15)
                tableViewMessages.columnWidths[3] = Math.max(40,(messageRectangle.width - tableViewMessages.columnWidths[0])*.10)
                tableViewMessages.columnWidths[4] = Math.max(120,(messageRectangle.width - tableViewMessages.columnWidths[0])*.20)
                tableViewMessages.columnWidths[5] = Math.max(110,(messageRectangle.width - tableViewMessages.columnWidths[0])*.20)
                tableViewMessages.forceLayout();
            }
            Rectangle{
                id:filterArea
                color:"transparent"

                width: parent.width
                anchors.top: parent.top
                anchors.left:parent.left
                height:50
                Rectangle{
                    id:separatorTopMessages
                    anchors.top:parent.top
                    anchors.left:parent.left
                    width:200
                    height:2
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
                        orientation: Gradient.Vertical
                    }
                }
                Rectangle{
                    id:separatorSideMessages
                    anchors.top:parent.top
                    anchors.left:parent.left
                    width:2
                    anchors.bottom: textHeaderMessages.bottom
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
                    id : textHeaderMessages
                    text: qsTr("Mesajlar")
                    width :80
                    height:20
                    anchors.top: separatorTopMessages.bottom
                    anchors.left: separatorSideMessages.right
                    anchors.leftMargin: 1
                    font.pixelSize: 18
                    antialiasing: true
                    font.hintingPreference: Font.PreferNoHinting
                    style: Text.Normal
                    focus: false
                    font.weight: Font.Medium
                    font.family: "Verdana"
                }


                TextField{

                    id:textFieldMsgTableSearch
                    width :150
                    height:20
                    anchors.top: textHeaderMessages.bottom
                    anchors.topMargin: 5
                    anchors.left:textHeaderMessages.left
                    anchors.leftMargin:5
                    font.pixelSize: 14
                    placeholderText: qsTr("Mesajlarda ara...")
                    font.family: "Verdana"


                }
                Text{
                    id : textMessageHeaderInfoText
                    text: qsTr("...")
                    height:20
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.rightMargin: 0
                    font.pixelSize: 10
                    antialiasing: true
                    font.hintingPreference: Font.PreferNoHinting
                    style: Text.Normal
                    elide: Text.ElideRight
                    focus: false
                    font.weight: Font.Medium
                    font.family: "Verdana"
                    color:"gray"
                }

            }

            TableView {
                id:tableViewMessages
                anchors.top: filterArea.bottom
                anchors.left:parent.left
                width: parent.width
                height: parent.height-filterArea.height - scrollbarHMessages.height
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
                    id:delegatedMessageCell
                    implicitHeight: text.implicitHeight + 2
                    implicitWidth: text.implicitWidth +2
                    color: (heading===true)?"#303030": (selected === true)? ((topLayout.selectedMessage === messageid )? "#7ff27c" :"#1fe81a") : (topLayout.selectedMessage === messageid )? "#decc73" :"#ebedee"

                    Text {
                        id:text
                        text: tabledata
                        width:tableMessages.width
                        padding: 1
                        font.pointSize: 10
                        elide: Text.ElideRight
                        font.preferShaping: false
                        color: (heading===true)?"#FFFFFF": (selected === true)? ((topLayout.selectedMessage === messageid )? "#000000" :"#838383") : (topLayout.selectedMessage === messageid )? "#000000" :"#838383"
                    }
                    MouseArea{
                        anchors.fill: parent
                        onClicked: {
                            if(heading===true){
                                tableMessages.sortColumn();
                            }else{
                                if (topLayout.selectedMessage !== messageid){
                                    comObj.setDisplayReqSignal(messageid)
                                    topLayout.selectedMessage = messageid
                                }else{
                                    topLayout.selectedMessage = "FFFFFFFF"
                                }
                                if(topLayout.selectedMessage !=="FFFFFFFF"){
                                    listModelWarnings.setList(comObj.getMsgWarningList())
                                }else{
                                    listModelWarnings.setList(comObj.getWarningList())
                                }
                            }
                        }
                        hoverEnabled: true
                        onEntered: {if (delegatedMessageCell.width < text.implicitWidth){ToolTip.show(tabledata)}}
                        onExited: ToolTip.hide()
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

                        visible:(heading===true && selectioncolumn ===true )? (searchactive ===false):(selectioncolumn ===true)
                        checked: (heading===true)? tableViewMessages.isAllSelected:selected
                        //indicator: {width:8; height:8;}
                        onClicked: (heading===true)? comObj.setAllSelected():comObj.setSelected(messageid)
                        anchors.fill: parent
                    }


                }

            }
        }
        Rectangle{
            id:tabArea
            height: parent.height
            anchors.right:parent.right
            anchors.rightMargin: 0
            anchors.left:messageRectangle.right
            anchors.leftMargin: 10
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
                //Signals tab
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
                        Rectangle{
                            id:separatorTopSignals
                            anchors.top:parent.top
                            anchors.topMargin: 2
                            anchors.left:parent.left
                            width:200
                            height:2
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
                                orientation: Gradient.Vertical
                            }
                        }
                        Rectangle{
                            id:separatorSideSignals
                            anchors.top:separatorTopSignals.bottom
                            anchors.left:parent.left
                            anchors.leftMargin: 0
                            anchors.bottom:parent.bottom
                            width:2
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

                            anchors.left:separatorSideSignals.right
                            anchors.leftMargin:1
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
                        Text{
                            id : textSignalsHeaderInfoText
                            text: qsTr("...")
                            height:20
                            anchors.bottom: parent.bottom
                            anchors.right: parent.right
                            anchors.rightMargin: 0
                            font.pixelSize: 10
                            antialiasing: true
                            font.hintingPreference: Font.PreferNoHinting
                            style: Text.Normal
                            elide: Text.ElideRight
                            focus: false
                            font.weight: Font.Medium
                            font.family: "Verdana"
                            color:"gray"
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
                            tableViewSignals.columnWidths[0] = Math.max(200,signalRectangle.width*.2)
                            tableViewSignals.columnWidths[1] = Math.max(110,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                            tableViewSignals.columnWidths[2] = Math.max(110,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                            tableViewSignals.columnWidths[3] = Math.max(110,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                            tableViewSignals.columnWidths[4] = Math.max(110,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                            tableViewSignals.columnWidths[5] = Math.max(110,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                            tableViewSignals.columnWidths[6] = Math.max(110,(signalRectangle.width-tableViewSignals.columnWidths[0])*.1)
                            tableViewSignals.columnWidths[7] = Math.max(120,(signalRectangle.width-tableViewSignals.columnWidths[0])*.133)
                            tableViewSignals.columnWidths[8] = Math.max(70,(signalRectangle.width-tableViewSignals.columnWidths[0])*.066)
                            tableViewSignals.columnWidths[9] = Math.max(110,(signalRectangle.width-tableViewSignals.columnWidths[0])*.133)
                            tableViewSignals.columnWidths[10] = Math.max(110,(signalRectangle.width-tableViewSignals.columnWidths[0])*.066)
                            tableViewSignals.columnWidths[11] = Math.max(90,(signalRectangle.width-tableViewSignals.columnWidths[0])*.066)
                            tableViewSignals.columnWidths[12] = Math.max(200,(signalRectangle.width-tableViewSignals.columnWidths[0])*.133)
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
                            property var columnWidths: [220, 100, 80, 80,80,80,80,80,80,100,100,100,120]
                            columnWidthProvider: function (column) {
                                return  columnWidths[column]
                            }


                            delegate: Rectangle {
                                id:delegatedSignalCell
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
                                    hoverEnabled: true
                                    onEntered: {if (delegatedSignalCell.width < textSignal.implicitWidth){ToolTip.show(tabledata)}}
                                    onExited: ToolTip.hide()
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
                        Rectangle{
                            id:separatorTopWarnings
                            anchors.top:parent.top
                            anchors.topMargin: 2
                            anchors.left:parent.left
                            width:200
                            height:2
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
                                orientation: Gradient.Vertical
                            }
                        }
                        Rectangle{
                            id:separatorSideWarnings
                            anchors.top:separatorTopWarnings.bottom
                            anchors.left:parent.left
                            anchors.leftMargin: 0
                            anchors.bottom:parent.bottom
                            width:2
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

                            anchors.left:parent.left
                            anchors.leftMargin:1
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
                        anchors.margins: 2
                        clip:true
                        Item{
                            anchors.fill:parent
                            anchors.bottomMargin:hscrollBarWarning.height

                            ListView{
                                id: listViewWarnings
                                anchors.fill: parent

                                model: ListModelWarnings {
                                    id: listModelWarnings
                                }
                                contentWidth: 1000
                                property bool enableVScrollbar: true
                                ScrollBar.vertical: ScrollBar{
                                    policy: ((listViewWarnings.height - listViewWarnings.contentHeight) < -3) ?
                                                ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                                    visible: ((listViewWarnings.height - listViewWarnings.contentHeight) < -3) ?
                                                 true : false
                                }
                                property bool enableHScrollbar: true
                                ScrollBar.horizontal: ScrollBar{
                                    id:hscrollBarWarning
                                    anchors.top:listViewWarnings.bottom
                                    policy: (listViewWarnings.width - listViewWarnings.contentWidth < -3) ?
                                                ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                                    visible: ((listViewWarnings.width - listViewWarnings.contentWidth) < -3) ?
                                                 true : false
                                }
                                flickableDirection : Flickable.AutoFlickDirection
                                delegate:Rectangle{
                                    id:delegateRectangle
                                    implicitHeight: textWarnings.implicitHeight+2
                                    implicitWidth: impwidth*textWarnings.font.pointSize*0.6 + 5
                                    color:"transparent"
                                    Component.onCompleted: {
                                        if(listViewWarnings.contentWidth < delegateRectangle.implicitWidth){
                                            listViewWarnings.contentWidth = delegateRectangle.implicitWidth;
                                        }
                                    }

                                    Text {
                                        id: textWarnings
                                        text: listdata
                                        font.pointSize: 10
                                        font.preferShaping: false
                                        color:"#838383"

                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true;
                                        onEntered: {delegateRectangle.color= "#decc73"; textWarnings.color = "#000000"
                                            if (listViewWarnings.width < (impwidth*(textWarnings.font.pointSize*0.6) +5)){ToolTip.show(listdata)}}
                                        onExited:{ delegateRectangle.color= "transparent" ; textWarnings.color = "#838383"
                                            ToolTip.hide()}

                                    }


                                }




                            }
                        }





                    }

                }
                //Info console tab
                Item{
                    id: consoleInfo
                    anchors.fill: parent

                    Rectangle{
                        id:headerInfoList
                        anchors.top:parent.top
                        anchors.margins:1
                        height:30
                        width:parent.width
                        color:"transparent"
                        Rectangle{
                            id:separatorTopInfos
                            anchors.top:parent.top
                            anchors.topMargin: 2
                            anchors.left:parent.left
                            width:200
                            height:2
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
                                orientation: Gradient.Vertical
                            }
                        }
                        Rectangle{
                            id:separatorSideInfos
                            anchors.top:separatorTopInfos.bottom
                            anchors.left:parent.left
                            anchors.leftMargin: 0
                            anchors.bottom:parent.bottom
                            width:2
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

                            anchors.left:separatorSideInfos.right
                            anchors.leftMargin:1
                            anchors.verticalCenter: parent.verticalCenter
                            text:"Bilgi konsolu"
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
                    Item{
                        anchors.bottom:parent.bottom
                        anchors.left:parent.left
                        anchors.right:parent.right
                        anchors.top:headerInfoList.bottom
                        width : parent.width
                        clip:true
                        ListView{
                            id: listViewInfos
                            anchors.fill: parent
                            anchors.bottomMargin: hScrollBarInfos.height
                            model: ListModelInfos {
                                id: listModelInfos
                            }
                            contentWidth:1000
                            property bool enableVScrollbar: true
                            ScrollBar.vertical: ScrollBar{
                                policy: ((listViewInfos.height - listViewInfos.contentHeight) < -3) ?
                                            ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                                visible: ((listViewInfos.height - listViewInfos.contentHeight) < -3) ?
                                             true : false
                            }
                            property bool enableHScrollbar: true
                            ScrollBar.horizontal: ScrollBar{
                                id:hScrollBarInfos
                                anchors.top:listViewInfos.bottom
                                policy: (listViewInfos.width - listViewInfos.contentWidth < -3) ?
                                            ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                                visible: (listViewInfos.width - listViewInfos.contentWidth < -3) ?
                                             true : false
                            }

                            flickableDirection : Flickable.AutoFlickDirection
                            delegate:Rectangle{
                                id:delegateRectangleInfo
                                implicitHeight: textInfos.implicitHeight+2
                                implicitWidth: impwidth*textInfos.font.pointSize*0.6 +5
                                color:"transparent"
                                Component.onCompleted: {
                                    if(listViewInfos.contentWidth < (delegateRectangleInfo.implicitWidth)){
                                        listViewInfos.contentWidth = delegateRectangleInfo.implicitWidth;
                                    }
                                }

                                Text {
                                    id: textInfos
                                    width:parent.width
                                    text: listdata
                                    font.pointSize: 10
                                    font.preferShaping: false
                                    color:generationinfo? "#000000":"#838383"
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true;
                                    onEntered: {delegateRectangleInfo.color= "#decc73"; textInfos.color = "#000000";
                                        if (listViewInfos.width < (impwidth*(textInfos.font.pointSize*0.6) +5)){ToolTip.show(listdata)}}
                                    onExited:{ delegateRectangleInfo.color= "transparent" ; textInfos.color = generationinfo? "#000000":"#838383";
                                        ToolTip.hide()}
                                    onDoubleClicked: {
                                        if(clickable === true){
                                            comObj.setPastCreatedMessages(listdata);
                                        }
                                    }
                                }


                            }



                        }
                    }
                }
            }


        }
    }
    Item{
        id:botLayout
        anchors.bottom:parent.bottom
        anchors.left:parent.left
        anchors.top:topLayout.bottom
        height:parent.height*0.3
        width:parent.width
        Item {
            id:areaConfig
            height: parent.height
            width: parent.width*0.7

            property string dutUnitHeader : (comObj.ComType === "CAN")? "UNIT" : "ETH"
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
                id : textHeaderCanCombo
                text: qsTr("CAN Hattı")
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
                visible: (comObj.ComType === "CAN")

            }
            ComboBox{
                id:canComboBox
                width: 200
                height: 46
                displayText : "Seçim Yapınız"
                editable:false
                anchors.top: textHeaderCanCombo.bottom
                anchors.topMargin: 5
                anchors.left:textHeaderCanCombo.left
                anchors.leftMargin: 5
                model:
                    [
                    "CAN0"           ,
                    "CAN1"           ,
                    "CAN2"           ,
                    "CAN3"           ,
                    "CAN4"           ,
                    "CAN5"           ,
                    "CAN6"           ,
                ]
                onActivated: {
                    displayText=currentText;
                    comObj.setCANLine(currentText);
                }
                visible: (comObj.ComType === "CAN")
            }
            Rectangle{
                id: seperator0
                width :5
                height:100
                radius: 100
                anchors.top: textHeaderCanCombo.top
                anchors.left:textHeaderCanCombo.left
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
                id : textHeaderDutCombo
                text: qsTr("DUT Ünite İsimlendirmesi")
                width :80
                height:20
                anchors.top: textHeaderCanCombo.top
                anchors.topMargin: 0
                anchors.left:textHeaderCanCombo.left
                anchors.leftMargin:220
                font.pixelSize: 14
                antialiasing: true
                font.hintingPreference: Font.PreferNoHinting
                style: Text.Normal
                focus: false
                font.weight: Font.Medium
                font.family: "Verdana"
                visible: (comObj.ComType === "CAN")

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
                visible: (comObj.ComType === "CAN")
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
                    "VD - Hata ayıklama Hattı"           ,
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
                anchors.top:(comObj.ComType === "CAN")? textHeaderDutCombo.top : textConfig.bottom
                anchors.left: (comObj.ComType === "CAN")? textHeaderDutCombo.left : textConfig.left
                anchors.leftMargin:(comObj.ComType === "CAN")? 220 : 5
                anchors.topMargin:(comObj.ComType === "CAN")? 0 : 10
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
            Switch {
                id: switchTest
                text: qsTr("Test modu")
                anchors.left:textFieldPreview.right
                anchors.leftMargin:15
                anchors.top: textHeaderPreview.top
                anchors.topMargin : 5
                visible: true
                indicator: Rectangle {
                    implicitWidth: 48
                    implicitHeight: 26
                    x: switchTest.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 13
                    color: switchTest.checked ? "#17a81a" : "#ffffff"
                    border.color: switchTest.checked ? "#17a81a" : "#cccccc"

                    Rectangle {
                        x: switchTest.checked ? parent.width - width : 0
                        width: 26
                        height: 26
                        radius: 13
                        color: switchTest.down ? "#cccccc" : "#ffffff"
                        border.color: switchTest.checked ? (switchTest.down ? "#17a81a" : "#21be2b") : "#999999"
                    }
                }

                contentItem: Text {
                    text: switchTest.text
                    font: switchTest.font
                    opacity: enabled ? 1.0 : 0.3
                    color: switchTest.down ? "#17a81a" : "#21be2b"
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: switchTest.indicator.width + switchTest.spacing
                }
                onCheckedChanged: {
                    comObj.setTestMode(switchTest.checked);
                }

            }
            Switch {
                id: switchFrc
                text: qsTr("Force Değişkenleri")
                anchors.left:textFieldPreview.right
                anchors.leftMargin:15
                anchors.top: switchTest.bottom
                anchors.topMargin: 5
                visible: (comObj.ComType === "CAN")
                indicator: Rectangle {
                    implicitWidth: 48
                    implicitHeight: 26
                    x: switchFrc.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 13
                    color: switchFrc.checked ? "#17a81a" : "#ffffff"
                    border.color: switchFrc.checked ? "#17a81a" : "#cccccc"

                    Rectangle {
                        x: switchFrc.checked ? parent.width - width : 0
                        width: 26
                        height: 26
                        radius: 13
                        color: switchFrc.down ? "#cccccc" : "#ffffff"
                        border.color: switchFrc.checked ? (switchFrc.down ? "#17a81a" : "#21be2b") : "#999999"
                    }
                }

                contentItem: Text {
                    text: switchFrc.text
                    font: switchFrc.font
                    opacity: enabled ? 1.0 : 0.3
                    color: switchFrc.down ? "#17a81a" : "#21be2b"
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: switchFrc.indicator.width + switchFrc.spacing
                }
                onCheckedChanged: {
                    comObj.setFrcVar(switchFrc.checked);
                }

            }
            Switch {
                id: switchMultiEnable
                text: qsTr("Çoklu CAN aktif")
                anchors.left:switchFrc.right
                anchors.leftMargin:15
                anchors.top: switchTest.top
                visible: (comObj.ComType === "CAN")
                indicator: Rectangle {
                    implicitWidth: 48
                    implicitHeight: 26
                    x: switchMultiEnable.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 13
                    color: switchMultiEnable.checked ? "#17a81a" : "#ffffff"
                    border.color: switchMultiEnable.checked ? "#17a81a" : "#cccccc"

                    Rectangle {
                        x: switchMultiEnable.checked ? parent.width - width : 0
                        width: 26
                        height: 26
                        radius: 13
                        color: switchMultiEnable.down ? "#cccccc" : "#ffffff"
                        border.color: switchMultiEnable.checked ? (switchMultiEnable.down ? "#17a81a" : "#21be2b") : "#999999"
                    }
                }

                contentItem: Text {
                    text: switchMultiEnable.text
                    font: switchMultiEnable.font
                    opacity: enabled ? 1.0 : 0.3
                    color: switchMultiEnable.down ? "#17a81a" : "#21be2b"
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: switchMultiEnable.indicator.width + switchMultiEnable.spacing
                }
                onCheckedChanged: {
                    comObj.setMultiEnableMode(switchMultiEnable.checked);
                }

            }
            Switch {
                id: switchBitTrue
                text: qsTr("Boş bitler \"TRUE\"")
                anchors.left:switchFrc.right
                anchors.leftMargin:15
                anchors.top: switchFrc.top
                visible: (comObj.ComType === "CAN")
                indicator: Rectangle {
                    implicitWidth: 48
                    implicitHeight: 26
                    x: switchBitTrue.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 13
                    color: switchBitTrue.checked ? "#17a81a" : "#ffffff"
                    border.color: switchBitTrue.checked ? "#17a81a" : "#cccccc"

                    Rectangle {
                        x: switchBitTrue.checked ? parent.width - width : 0
                        width: 26
                        height: 26
                        radius: 13
                        color: switchBitTrue.down ? "#cccccc" : "#ffffff"
                        border.color: switchBitTrue.checked ? (switchBitTrue.down ? "#17a81a" : "#21be2b") : "#999999"
                    }
                }

                contentItem: Text {
                    text: switchBitTrue.text
                    font: switchBitTrue.font
                    opacity: enabled ? 1.0 : 0.3
                    color: switchBitTrue.down ? "#17a81a" : "#21be2b"
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: switchBitTrue.indicator.width + switchBitTrue.spacing
                }
                onCheckedChanged: {
                    comObj.setBitTrue(switchBitTrue.checked);
                }

            }

        }
        Item{
            id:areaGenerate
            height: parent.height
            width:parent.width*0.3
            anchors.left:areaConfig.right
            anchors.right:parent.right
            anchors.bottom: parent.bottom

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
                anchors.rightMargin: 30
                disableButtonClick: false
                anchors.verticalCenter: parent.verticalCenter
            }
            MenuButton{
                id:buttonTurnBack
                width: 84
                height: 46
                onVisibleChanged: console.log("turnback button visible changed")
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
                anchors.right: buttonGenerate.right
                anchors.bottom :buttonGenerate.top
                anchors.bottomMargin: 10
                width:50
                fillMode:Image.PreserveAspectFit
                source:"qrc:/img/img/progressSucces.png"
            }

            Timer {
                id:progressDoneTimer
                interval: 3000;
                running: true;
                repeat:true
                onTriggered: { progressDoneImage.visible=false
                    buttonGenerate.visible=true
                    buttonTurnBack.visible=true
                    generationProgress.visible=false}
            }

            ProgressBar{
                id:generationProgress
                width:190
                height: 46
                anchors.left: buttonTurnBack.left
                anchors.top: buttonTurnBack.top
                visible:false
                indeterminate: true

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
    /*Connections{
        target: comObj
        onProgressStarted : {buttonGenerate.visible=false
                             buttonTurnBack.visible=false
                             generationProgress.visible=true}
    }*/
    Connections{
        target: comObj
        onProgressCompleted : {
            progressDoneImage.visible=true
            progressDoneTimer.restart()}
    }
    /*Connections{
        target: comObj
        onProgressChanged : {generationProgress.value=comObj.progress
                            console.log("progress changed"+comObj.progress)}
    }*/
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
            tableViewMessages.isAllSelected = false;
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
    Connections{
        target:tableMessages
        onTableChanged: textMessageHeaderInfoText.text= tableMessages.getInfoText();
    }
    Connections{
        target:tableSignals
        onTableChanged: textSignalsHeaderInfoText.text= tableSignals.getInfoText();
    }

}


