import QtQuick 2.14
import QtLocation 5.9
import QtPositioning 5.8
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.14

MapPolyline{
    id: airgroundline
    line.color: "yellow"
    line.width: 4
    //smooth: true

    property WayHandler wayHandle
    property var mapView
    property string agcode

    /*property string txtlabel
    property string labelcolor
    property int fontsize

    MouseArea {
        id: mouseArea
        containmentMask: surveyline
        acceptedButtons: Qt.RightButton | Qt.LeftButton
        hoverEnabled: true
        property bool isOpen: false
        onEntered: {
            label.visible = true
            labelrect.visible = true
        }
        onExited:  {
            label.visible = false
            labelrect.visible = false
        }

        onPressed: {
            if(mouseArea.pressedButtons & Qt.RightButton) {

            }
        }
    }

    Rectangle{
        id: labelrect
        objectName: "labelrect"
        width: label.width + 16
        height: label.height + 20
        color: "white"
        border.color: "#B7B6B5"
        border.width: 1
        opacity: 1.0
        anchors.centerIn: label
        visible: false
    }

    Label{
        id: label
        anchors.bottom: parent.top
        anchors.horizontalCenterOffset: 5
        //anchors.centerIn: parent
        //anchors.horizontalCenterOffset: 80
        //anchors.verticalCenterOffset: -55

        font: textMetrics.font
        text: textMetrics.text
        color: labelcolor

        visible: false
    }

    TextMetrics {
        id: textMetrics
        font.family: "Segui UI"
        font.pixelSize: fontsize

        text: txtlabel
    }*/

    function populateCoor(coor){
        addCoordinate(coor);
    }

}





