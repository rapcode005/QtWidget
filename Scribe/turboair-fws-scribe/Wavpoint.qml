import QtQuick 2.14
import QtLocation 5.9
import QtPositioning 5.8
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.14

MapQuickItem {
    id: wavpoint
    anchorPoint.x: image.width / 2
    anchorPoint.y: image.height
    z: 5
    property string descriptor
    property string latitude
    property string longitude
    property string wavdate
    property string wavtime
    property string labelglowcolor
    property string imghover
    property string imgselected
    property string imgdefault
    property WayHandler wayHandle
    property var mapView
    property string labelcolor
    property int fontsize
    property int zorder
    property string labelrectbgcolor

    sourceItem: Image{
        id: image
        source: imgdefault //"qrc:/img/map/wavblue.png"
        smooth: true
        width: 30
        height: 30

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            acceptedButtons: Qt.RightButton | Qt.LeftButton
            hoverEnabled: true
            property bool isOpen: false
            onEntered: {
                /*image.source = imghover*/ //"qrc:/img/map/wavyellow.png"
                image.state = "entered"
                //wavpoint.scale = 1.2
                label.visible = true
                labelrect.visible = true
                //wavpoint.z = 10
            }
            onExited:  {
                image.state = "exited"
                /*image.source = imgdefault*/ //"qrc:/img/map/wavblue.png"
                //wavpoint.scale = 1.0
                label.visible = false
                labelrect.visible = false
                //wavpoint.z = 5
            }

            onPressed: {
                if(mouseArea.pressedButtons & Qt.RightButton) {

                }
            }
        }



        states: [
            State {
                name: "entered"
                PropertyChanges {
                    target: image;
                    source: imghover;

                }
            },
            State {
                name: "exited"
                PropertyChanges { target: image; source: imgdefault }
            }
        ]

        //added 5.2.2020
        Rectangle{
            id: labelrect
            objectName: "labelrect"
            width: label.width + 16
            height: label.height + 16
            color: labelrectbgcolor
            border.color: "#B7B6B5"
            border.width: 1
            opacity: 1.0
            anchors.centerIn: label
            visible: rectinfotimer.running //false
            z: zorder

            //property int rectinfo
            property int counter: 0
            onCounterChanged: if(counter == 0){
                                  labelrect.visible = false;
                                  label.visible = false;
                                  rectinfotimer.stop();
                                  counter = 2 //second timer on closing
                              }
        }

        Label{
            id: label

            /*anchors.bottom: image.top
            anchors.left: image.right
            anchors.topMargin: 15*/
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: 80
            anchors.verticalCenterOffset: -55
            z: zorder + 1

            font: textMetrics.font
            text: textMetrics.text
            color: labelcolor

            /*layer.enabled: true
            layer.effect:   Glow {
                anchors.fill: label
                radius: 10
                samples: 17
                color: labelglowcolor
                spread: 0.6
                opacity: 0.1
            }*/
            //visible: false
            visible: rectinfotimer.running
        }

        TextMetrics {
            id: textMetrics
            font.family: "Segui UI"
            font.pixelSize: fontsize
            text: "File: <b>" + wavpoint.descriptor +
                  "</b><br>Date: <b>" + wavdate +
                  "</b><br>Time: <b>" + wavtime +
                  "</b><br>Latitude: <b>" + latitude +
                  "</b><br>Longitude: <b>" + longitude + "</b>"
        }

    }

    function setLabelVisible(boolvisible){
        label.visible = boolvisible;
        labelrect.visible = boolvisible;
        rectclickinfo.counter = 10
        rectclickinfotimer.start()
        imghover = "qrc:/img/map/wavred.png";
        image.source = imgdefault;
    }

    function setProperties(boolvisible, imgfile, lblglowclr, lblcolor, fontsz, lblbgcolor){
        //image.source = "";
        //update();
        labelcolor = lblcolor
        fontsize = fontsz
        labelglowcolor = lblglowclr;
        imgdefault = imgfile;
        labelrectbgcolor = lblbgcolor;
        label.visible = boolvisible;
        labelrect.visible = boolvisible;

        rectclickinfo.counter = 2
        rectclickinfotimer.start()//start the timer to utomate closure of info bubble

        //imgdefault = "qrc:/img/map/bookmark.png";
        //image.source = imgdefault;
        //update();
    }

    Timer{
        id:rectinfotimer
        interval: 1000
        onTriggered: {
            labelrect.counter --
        }
        repeat: true
        running: {
            labelrect.counter > 0
        }
    }

}
