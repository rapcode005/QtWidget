import QtQuick 2.0
import QtLocation 5.9
import QtPositioning 5.8
import QtGraphicalEffects 1.0

MapCircle {
    property real wavradius
    property var wavcenter
    property string wavbordercolor
    property real wavopacity

    id: wavcircle
    border.width: 3
    color: "transparent"

    function setColor(bordercolor){
        wavcircle.wavbordercolor = bordercolor;
    }

    function setValues(){
        wavcircle.radius = wavradius
        wavcircle.center = wavcenter
        wavcircle.border.color = wavbordercolor
        wavcircle.opacity = wavopacity
    }
}
