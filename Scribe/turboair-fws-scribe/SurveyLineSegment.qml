import QtQuick 2.12
import QtLocation 5.9
import QtPositioning 5.8
import QtGraphicalEffects 1.0
import QtQuick.Controls 2.5

MapItemGroup {
    id:linesegment

    property string stratum
    property string transect
    property string segment
    property var bright
    property var tleft
    property real mousexval
    property real mouseyval

    MapPolyline{
        id: linesegmentline
        line.color: "red"
        line.width: 2
    }

    Rectangle{
        id: tag
        width: 100
        height: 40
        visible: false

        anchors.bottom: maprectangle.top
        anchors.left: maprectangle.left
        //y:linesegment.mouseyval
    }

    function populateCoor(coor){
        linesegmentline.addCoordinate(coor);
    }

    MapQuickItem {
        id: linesegmentpt
        anchorPoint.x: imagept.width
        anchorPoint.y: imagept.height
        z: 700

        sourceItem: Image{
            id: imagept
            source: "qrc:/img/map/bluedot.png"
            smooth: true
            width: 5
            height: 5
        }
    }

    function plotPt(coor){
        linesegmentpt.coordinate = coor;
    }

    function populaterectangle(){
        var path = linesegmentline.path;
        var pt1;
        var pt2;
        var i = 0;
        do{
            pt1 = path[i];
            pt2 = path[i+1];

            var ptazim = pt1.azimuthTo(pt2);
            //console.log(ptazim);

            var newpt;
            var quad;
            var newazim;
            if(ptazim > 0 && ptazim <= 90){
                quad = 1;//NE
                //1st pt bottomright //2nd pt is topleft
            }else if(ptazim > 90 && ptazim <= 180){
                quad = 2;//SE
                //1st pt topleft //2nd pt is bottomright
            }else if(ptazim > 180 && ptazim <= 270){
                quad = 3;//SW
                //1st pt topleft //2nd pt is bottomright
            }else if(ptazim > 270 && ptazim <= 360){
                quad = 4;//NW
                //1st pt bottomright //2nd pt is topleft

            }

            var coor
            var coor1
            if(quad == 4 || quad == 1){
                if((ptazim + 90 ) > 360){
                    newazim = Math.abs((ptazim + 90 ) - 360);
                }else{
                    newazim = (ptazim + 90);
                }

                coor = pt2.atDistanceAndAzimuth(5,(newazim));
                tleft = coor;

                newazim = ptazim - 90;
                coor1 = pt1.atDistanceAndAzimuth(5,(newazim));
                bright = coor1;
            }else{
                if((ptazim + 90 ) > 360){
                    newazim = Math.abs((ptazim + 90 ) - 360);
                }else{
                    newazim = (ptazim + 90);
                }

                coor = pt1.atDistanceAndAzimuth(5,(newazim));
                tleft = coor;

                newazim = ptazim - 90;
                coor1 = pt2.atDistanceAndAzimuth(5,(newazim));
                bright = coor1;
            }

            maprectangle.topLeft = tleft;
            maprectangle.bottomRight = bright;

            i += 1;
        }while(i < (path.length - 1))
    }

    MapRectangle{
        id:maprectangle
        objectName: "maprectangle"
        border.color: "transparent"
        border.width: 1
        color: "transparent"

        MouseArea{
            id:rectanglearea
            anchors.fill: parent
            hoverEnabled: true
            onMouseXChanged: {
                /**/
                linesegment.mousexval = mouseX
                linesegment.mouseyval = mouseY
                console.log(linesegment.mousexval,", ",linesegment.mouseyval)
                //label.visible = true
                tag.visible = true
            }
            onContainsMouseChanged: {
                //label.visible = false
                tag.visible = false
            }

            /*Label{
                id: label
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: 10
                anchors.verticalCenterOffset: -5
                z: 600

                font: textMetrics.font
                text: textMetrics.text
                color: "royalblue"

                visible: false
            }*/

            /*TextMetrics {
                id: textMetrics
                font.family: "Segui UI"
                font.pixelSize: 10
                font.bold: true

                text: linesegment.stratum +
                      ": " + linesegment.transect +
                      ": " + linesegment.segment
            }*/
        }
    }
}
