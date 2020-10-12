import QtQuick 2.0
import QtQuick.Controls 2.0
import QtLocation 5.8
import QtPositioning 5.8

Canvas{
    id : appWindow
    //loading map plugin
    Plugin{
        id: mapPlugin
        name: "osm"
        PluginParameter { name: "osm.useragent";               value: "NerdyDragonScribe" }
        PluginParameter { name: "osm.mapping.host";            value: "https://tile.openstreetmap.org/" }
        PluginParameter { name: "osm.mapping.copyright";       value: "Open Database License" }
        PluginParameter { name: "osm.mapping.cache.disk.size"; value: "512 MiB" }
        PluginParameter { name: "osm.geocoding.host";          value: "https://nominatim.openstreetmap.org" }
        PluginParameter { name: "osm.routing.host";            value: "https://router.project-osrm.org/viaroute" }
        PluginParameter { name: "osm.places.host";             value: "https://nominatim.openstreetmap.org/search" }
        PluginParameter { name: "osm.mapping.copyright";       value: "" }
        PluginParameter { name: "osm.mapping.highdpi_tiles";   value: true }
    }
    //
    //  https://stackoverflow.com/questions/42402732/qt-qml-qtlocation-map-plugin
    //  actually defining our map component, this is where all the magic happens
    //
    Map{
        id:             mapCanvas
        //property real 3: value
        anchors.fill:   parent
        plugin:         mapPlugin
        center:  QtPositioning.coordinate(43.62, -116.21) // Boise ID
        //center: QtPositioning.coordinate(59.9485, 10.7686)
        /* center {
                // The Qt Company in Oslo
                latitude: 59.9485
                longitude: 10.7686
            }*/

        zoomLevel:      zoomSlider.value
        //creating our waypoint handler
        property WayHandler handler

        //pretty self explanatory
        Component.onCompleted : {

            //  for custom URLs
            for( var i_type in supportedMapTypes ) {
                if( supportedMapTypes[i_type].name.localeCompare( "Custom URL Map" ) === 0 ) {
                    activeMapType = supportedMapTypes[i_type]
                }
            }
            //creating the waypoint handler
            handler = Qt.createQmlObject('WayHandler{}', appWindow)
            handler.mapview = mapCanvas
            handler.initialize()
        }
        onCopyrightLinkActivated: Qt.openUrlExternally(link)
    }

    Slider{
        id:zoomSlider
        orientation: Qt.Vertical
        from: 3.0
        to: 15.0
        stepSize: 15.0/40   // going through 10 steps
        value: 8                              // initial zoom level
        anchors{
             right: parent.right
             rightMargin: -4
             verticalCenter:parent.verticalCenter
        }
        background: Rectangle {
            x: zoomSlider.leftPadding
            y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
            implicitWidth: 6
            implicitHeight: 240
            width: implicitWidth
            height: zoomSlider.availableHeight
            radius: 4
            opacity: 0.3
            color: "#21be2b"

            Rectangle {
                width: parent.width
                height: zoomSlider.visualPosition * parent.height
                        //opacity: 0.5
                color: "#ff0000"
                radius: 4
            }
        }

        handle: Rectangle {
            x: zoomSlider.leftPadding + 3 - width / 2
            y: zoomSlider.topPadding + zoomSlider.visualPosition * (zoomSlider.availableHeight - height)
            implicitWidth: 18
            implicitHeight: 10
            radius: 4
            color: zoomSlider.pressed ? "#d3d3d3" : "#efefef"
            border.color: "#999999"
        }
    }

    Rectangle{
       //y: zoomSlider.topPadding + zoomSlider.availableHeight/2 +
        //source: "qrc:/img/map/zoom.png"
        id: plusButton
        smooth: true
        width: 18
        height: 18
        x: zoomSlider.x
        y: zoomSlider.y - height
        radius: 14
        color: "#efefef"
        border.color: "#999999"
        Text {
            id: plus
            anchors.fill: parent
            width: 18
            height: 18
            color: "#1c1c1c"
            //font.bold: true
            font.pointSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment:  Text.AlignVCenter
            text: qsTr("+")
        }
        MouseArea {
            id: plusArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            hoverEnabled: true
            onEntered: {
                plusButton.color =  "#dfdfdf"
            }
            onExited:  {
                plusButton.color = "#efefef"
            }
            onPressed: {
                plusButton.color =  "#dadada"
                zoomSlider.value += 1;
                plusButton.color =  "#dfdfdf"
            }
        }
    }

    Rectangle{
       //y: zoomSlider.topPadding + zoomSlider.availableHeight/2 +
        //source: "qrc:/img/map/zoom.png"
        id: minusButton
        smooth: true
        width: 18
        height: 18
        x: zoomSlider.x
        y: zoomSlider.y + zoomSlider.height
        radius: 14
        color: "#efefef"
        border.color: "#999999"
        Text {
            id: minus
            anchors.fill: parent
            width: 18
            height: 18
            color: "#1c1c1c"
            y: -8
            //font.bold: true
            font.pointSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment:  Text.AlignVCenter
            text: qsTr("-")
        }
        MouseArea {
            id: minusArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            hoverEnabled: true
            onEntered: {
                minusButton.color =  "#dfdfdf"
            }
            onExited:  {
                minusButton.color = "#efefef"
            }
            onPressed: {
                minusButton.color =  "#dadada"
                zoomSlider.value -= 1;
                minusButton.color =  "#dfdfdf"
            }
        }
    }
}
