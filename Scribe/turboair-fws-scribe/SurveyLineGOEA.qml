import QtQuick 2.14
import QtLocation 5.9
import QtPositioning 5.8
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.14

MapPolyline{
    property WayHandler wayHandle
    property var mapView
    property string id
    property string fid
    property string bcr
    property string trn
    property string bcrtrn
    property string km
    property string beglng
    property string beglat
    property string endlng
    property string endlat

    id: surveyline
    //line.color: "red"
    line.width: 3

    function populateCoor(coor){
        addCoordinate(coor);
    }

    function setLineColor(strcolor){//added 7.21.2020
        line.color = strcolor;
    }

}
