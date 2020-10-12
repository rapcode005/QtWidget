import QtQuick 2.0
import QtLocation 5.9
import QtPositioning 5.8
import QtGraphicalEffects 1.0

MapCircle {
    property real agradius
    property var agcenter
    property string agbordercolor
    property real agopacity

    id: agcircle
    border.width: 3
    color: "transparent"

    function setColor(bordercolor){
        agcircle.agbordercolor = bordercolor;
    }

    function setValues(){
        agcircle.radius = agradius
        agcircle.center = agcenter
        agcircle.border.color = agbordercolor
        agcircle.opacity = agopacity
    }
}
