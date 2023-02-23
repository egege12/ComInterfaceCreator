import QtQuick 2.15
import QtQuick.Controls 6.3
import QtQuick.Dialogs
Item {

    Switch {
        id: switch1
        x: 253
        y: 248
        width: 162
        height: 68
        text: qsTr("Switch")
    }

    ComboBox {
        id: comboBox
        x: 233
        y: 156
        width: 189
        height: 40
    }

    TabButton {
        id: tabButton
        x: 313
        y: 106
        text: qsTr("Tab Button")
    }

}
