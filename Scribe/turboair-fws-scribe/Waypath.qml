import QtQuick 2.0
import QtLocation 5.8
import QtPositioning 5.8
import QtQml.Models 2.3
MapPolyline {
    id:pathdrawn
    line.color: 'blue'
    line.width:  3
    path:[]
    opacity: 0.9
    z: 1
    property var totalLength
    property var totalTime
    property var pointOfInterest: []
    property var  pathOfInterest: []
    property var poiIndexes: []
    property var mapView
    //initializes us with some arbitrary coordinates, and plots a line for tests
    function initialize(){
        markPointOfInterest(0);
        //if (path.length() > 1){
        markPointOfInterest(pathdrawn.pathLength() - 1)
        //}
        //markPathOfInterest(0,2)
    }
    //marks a path of interest between given gps points
    function markPathOfInterest(startIndex, stopIndex){
        var p = Qt.createQmlObject('PathOfInterest{}', parent)
        for(var i = startIndex; i <stopIndex; i++){
            p.addCoordinate(path[i])
        }
        pathOfInterest.push(p)
        mapview.addMapItem(p)
        //markPointOfInterest(startIndex)
        //markPointOfInterest(stopIndex)
    }
    //slaps a waypoint down on whatever index of the path we want
    function markPointOfInterest(indexOfPathPoint){
        poiIndexes.push(indexOfPathPoint)
        var m = Qt.createQmlObject('Point{}', parent)
        m.coordinate = path[indexOfPathPoint]
        pointOfInterest.push(m)
        mapview.addMapItem(m)
    }
    function removeMapItems(){
        for (var i = 0; i < pointOfInterest.length; i++){
            mapView.removeMapItem(pointOfInterest[i]);
            pointOfInterest[i].destroy();
        }
    }
}
