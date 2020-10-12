import QtQuick 2.0
import QtLocation 5.8
import QtPositioning 5.8
//just draws basic map lines with a specific color
MapPolyline {
    id: poiLine
    path:[]
    line.width:5
    line.color: 'yellow'
}
