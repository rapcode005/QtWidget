import QtQuick 2.0
import QtLocation 5.8
import QtPositioning 5.8
MapQuickItem{
    id: marker
    anchorPoint.x: image.width/2
    anchorPoint.y: image.height/2
    opacity: 0
    property real xCoord : 0
    property real yCoord : 0

    SmoothedAnimation on xCoord {velocity : 200}
    SmoothedAnimation on yCoord {velocity : 200}

    coordinate: QtPositioning.coordinate(xCoord,yCoord)
    z: 4
    sourceItem: Image{
        id: image
        width: 24
        height: 24
        smooth: true
        source: "qrc:/img/map/pilot.png"
    }
    function smoothing(){
        marker.coordinate = QtPositioning.coordinate(xCoord,yCoord)
    }
}
