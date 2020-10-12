import QtQuick 2.0
import QtLocation 5.9
import QtPositioning 5.8
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.14

MapPolygon {
    id:surveypolygon
    border.width: 3
    border.color: "red"
    color: "red"
    opacity: 0.3


    property WayHandler wayHandle
    property var mapView
    property string plotlabel
    property string txtlabel
    property string labelcolor
    property int fontsize

    function populateCoor(coor){
        addCoordinate(coor);
    }
}
