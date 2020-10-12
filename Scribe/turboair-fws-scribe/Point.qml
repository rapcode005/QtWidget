import QtQuick 2.0
import QtPositioning 5.8
import QtLocation 5.8
MapQuickItem{
    id: way
    anchorPoint.x: image.width/2
    anchorPoint.y: image.height/2
    z: 3
    sourceItem: Image{
        id: image
        width: 24
        height: 24
        source: "qrc:/img/map/waypoint.png"
        smooth: true
    }

}
