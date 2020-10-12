import QtQuick 2.14 // 5.8.2020 from 2.0 to 2.5
import QtQuick.Controls 2.5 //2.0 to 2.5
import QtLocation 5.12 //5.8
import QtPositioning 5.12 //5.8
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.12
import QtQuick.Window 2.14

import "helper.js" as Helper

Canvas{
    id : appWindow

    property string mapPlug: "esri"
    property string mapType: "default"

    //loading map plugin
    Plugin{
        id: mapPluginOsm
        name: "osm"

        //appWindow.mapPlug: mapPluginOsm
        /* I am not sure if should we this host?
        PluginParameter { name: "osm.useragent"; value: "NerdyDragonScribe" }
        PluginParameter { name: "osm.mapping.cache.disk.size"; value: "512 MiB" }
        PluginParameter { name: "osm.mapping.custom.host"; value: "https://tile.openstreetmap.org/" }
        PluginParameter { name: "osm.mapping.copyright"; value: "Open Database License" }
        PluginParameter { name: "osm.routing.host"; value: "http://map.project-osrm.org/?srv=0" }
        PluginParameter { name: "osm.places.host"; value: "https://nominatim.openstreetmap.org/search" }
        PluginParameter { name: "osm.geocoding.host"; value: "https://nominatim.openstreetmap.org/" }
        PluginParameter { name: "osm.mapping.highdpi_tiles";   value: true }*/
    }

    Plugin{
        id: mapPluginMapBox
        name: "mapbox"//tested 7.16.2020
        PluginParameter { //scribe token active(not used)
            name: "mapbox.access_token"; value: "pk.eyJ1IjoiamNvZmZpZWxkIiwiYSI6ImNrY3RtYjdoaTBpajcycnFscW1lZWFudHoifQ.TQjmanqldNCYI7P4dOJ4eg"
        }
        PluginParameter{
            name: "mapbox.mapping.additional_map_ids"; value: "mapbox.mapbox-streets-v8"
        }

    }

    Plugin{
        id: mapPluginHere
        name: "here"
        PluginParameter{
            name: "here.app_id";
            value: "" //API_ID
        }
        PluginParameter{
            name: "here.token";
            value: "" //TOKEN_ID
        }
        PluginParameter{
            name: "here.proxy";
            value: "system"
        }
        /*PluginParameter{
            name: "here.mapping.host";
            value: "https://1.base.maps.ls.hereapi.com/"
        }*/

        /*name: "here"
         PluginParameter { name: "here.app_id";
             value: "WEdwl6NZWuCYL0ruUgIw"; }
         PluginParameter { name: "here.token";
            value: "RmL5iNAR5B94WTSD1EqJIXEsoZQeETHPgAhB5ckmayo"; }
             // value: "eyJhbGciOiJSUzUxMiIsImN0eSI6IkpXVCIsImlzcyI6IkhFUkUiLCJhaWQiOiJHOEMwR2YzZmxZajVybkswZFlGNyIsImlhdCI6MTU5MzM5NzUzMywiZXhwIjoxNTkzNDgzOTMzLCJraWQiOiJqMSJ9.ZXlKaGJHY2lPaUprYVhJaUxDSmxibU1pT2lKQk1qVTJRMEpETFVoVE5URXlJbjAuLjdhTTk2WFE4bkY5YlRFRmtZa2RvSmcuY0VxNDdxcDI4ZzNuSk5LZFVEZUIwRzMxQjBsSHo4bVJIRjhSeVZHdW9ZN1VsYjkwLTlsQzFlRmVLaTNRNUZJNTAzamN4emtZY25VaTI5N0h2VV9ueFVuM01JbGtKcmZLcjhaZnRBRllTU0s2eGNaVmxqeVVTbjZ4SUpjYlE3NVguUEFfUjdxR3c3U2hTbmFkRGZuVEFIWXctbVNOYmQzc0dVMm5KaVlrVXNWMA.enNhosCdnxJxEVZVSm4Wuak9xseoEFuYSs4gq4H_8viF74rfEZHcQOmbppn4jsVngSxhf5VDCvfZhmQY7u-mxcdk8mD_IibF53p269D_zzQ_lNQmUYcJoDXKZglk69YbsgGDwbA3VRX5cGpTKxFDKgWNX-KooSp7BDfv6bBeHKtIIPiNWmNSbU-GSwrtQACjZEaItZgmGFAZfxGmCPcvjzqtgU6qAMfhO0e6dHS9wb_KTMdAApC0EPSbqEOAhZ6-Igw4y-MfmU_PuBYSVNz2LNjL3NlnM5acXxjeHQbtptCL7BRW5f_zcgiThhQ21su_vZ1o-DfDLZ522MA3wSR2KA"; }
        */
    }

    Plugin{//scribe token active(not used)
        id: mapPluginMapBoxGl
        name: "mapboxgl"

        PluginParameter {
            name: "mapboxgl.access_token"; value: "pk.eyJ1IjoiamNvZmZpZWxkIiwiYSI6ImNrY3RtYjdoaTBpajcycnFscW1lZWFudHoifQ.TQjmanqldNCYI7P4dOJ4eg"
        }

        PluginParameter{
            name: "mapboxgl.api_base_url"; value: "https://api.mapbox.com"
        }

        PluginParameter {
            name: "mapboxgl.mapping.additional_style_urls";
            value: "mapbox://styles/mapbox/streets-v8"
        }
    }

    Plugin{//used
        id: mapPluginEsri
        name: "esri"

        /*PluginParameter{
            name: "esri.token";
            value: ""
        }*/
    }

    //
    //  https://stackoverflow.com/questions/42402732/qt-qml-qtlocation-map-plugin
    //  actually defining our map component, this is where all the magic happens
    //
    Map{
        id: mapCanvas
        objectName: "mapCanvas"
        anchors.fill: parent
        plugin: if(appWindow.mapPlug === "osm"){
                    mapPluginOsm
                }else if(appWindow.mapPlug === "mapbox"){
                    mapPluginMapBox
                }else if(appWindow.mapPlug === "mapboxgl"){
                    mapPluginMapBoxGl
                }else if(appWindow.mapPlug === "here"){
                    mapPluginHere
                }else if(appWindow.mapPlug === "esri"){
                    mapPluginEsri
                }

        center: QtPositioning.coordinate(43.62, -116.21) // Boise ID
        //zoomLevel: zoomSlider.value / 2  //remarked 5.8.2020
        zoomLevel: 12
        copyrightsVisible: false//6.5.2020

        activeMapType: if(appWindow.mapPlug === "mapbox"){
                           /*for (var i = 0; i<mapCanvas.supportedMapTypes.length; i++) {
                               var mt = mapCanvas.supportedMapTypes[i]
                               console.log(i + ": " + mt.name)
                           }*/

                           if(appWindow.mapType == "default"){
                               mapCanvas.supportedMapTypes[0] //default street view
                           }else if(appWindow.mapType == "satellite"){
                               mapCanvas.supportedMapTypes[3] //added 7.16.2020 satellite image
                           }
                       }else if(appWindow.mapPlug === "osm"){
                           //console.log("D")
                           mapCanvas.supportedMapTypes[0]//default
                       }else if(appWindow.mapPlug === "mapboxgl"){
                           //console.log("E")
                           mapCanvas.supportedMapTypes[0]//default
                       }else if(appWindow.mapPlug === "here"){
                           //console.log("F")
                           for (var ii = 0; i<mapCanvas.supportedMapTypes.length; ii++) {
                               var mt1 = mapCanvas.supportedMapTypes[ii]
                               console.log(ii + ": " + mt1.name)
                           }
                           mapCanvas.supportedMapTypes[0]//default
                       }else if(appWindow.mapPlug === "esri"){
                           if(appWindow.mapType == "default"){
                                mapCanvas.supportedMapTypes[0] //default street view
                           }else if(appWindow.mapType == "satellite"){
                                mapCanvas.supportedMapTypes[1] //added satellite image
                           }
                       }

        //creating our waypoint handler
        property WayHandler handler
        property string stratranseg
        property string agseg
        property variant mouseclickx: 0
        property variant mouseclicky: 0

        //added 5.8.2020
        property bool followme: false
        property variant scaleLengths: [5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]
        property var mouselat: "0.0"
        property var mouselon: "0.0"
        property string textsegmenttitle

        property string textstratumno
        property string texttransectno
        property string textsegmentno
        property string textagname
        property string textsegmenttitle1
        property string textplotlabel
        property string textpolytitle
        property string polyplotlabel

        property string surveytype
        property string goeaval
        property string textbcr
        property string texttrn

        property bool withFlightData: false
        property bool withAirGroundSurveyData: false
        property bool withSurveyData: false
        property bool withWavFile: false

        //added 5.10.2020
        MouseArea{
            anchors.fill: parent

            enabled: true
            hoverEnabled: true

            onMouseXChanged: {
                var coord = mapCanvas.toCoordinate(Qt.point(mouse.x,mouse.y))
                mapCanvas.mouselat = coord.latitude
                mapCanvas.mouselon = coord.longitude

//                console.log(mapCanvas.mouselat )
//                console.log(mapCanvas.mouselon )
            }

            onReleased: {
                mapCanvas.mouseclickx = mouse.x
                mapCanvas.mouseclicky = mouse.y
                var clickcoord = mapCanvas.toCoordinate(Qt.point(mouse.x,mouse.y))

                //mapCanvas.checkSurveyList(clickcoord)
                mapCanvas.getSurveyType(clickcoord);

            }

        }

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

        onMapItemsChanged: {
            validateItemExistance();
        }



        //added 5.8.2020 start
        function calculateScale()//added 5.8.2020
        {
            var coord1, coord2, dist, text, f
            f = 0
            coord1 = mapCanvas.toCoordinate(Qt.point(0,scale.y))
            coord2 = mapCanvas.toCoordinate(Qt.point(0+scaleImage.sourceSize.width,scale.y))
            dist = Math.round(coord1.distanceTo(coord2))

            if (dist === 0) {
                // not visible
            } else {
                for (var i = 0; i < scaleLengths.length-1; i++) {
                    if (dist < (scaleLengths[i] + scaleLengths[i+1]) / 2 ) {
                        f = scaleLengths[i] / dist
                        dist = scaleLengths[i]
                        break;
                    }
                }
                if (f === 0) {
                    f = dist / scaleLengths[i]
                    dist = scaleLengths[i]
                }
            }

            //console.log(dist);
            text = Helper.formatDistance(dist)
            //console.log(text)
            scaleImage.width = (scaleImage.sourceSize.width * f) - 2 * scaleImageLeft.sourceSize.width
            scaleText.text = text
        }

        onCenterChanged:{
            scaleTimer.restart()
            if (mapCanvas.followme)
                if (mapCanvas.center !== positionSource.position.coordinate) mapCanvas.followme = false
        }

        onZoomLevelChanged:{
            scaleTimer.restart()
            if (mapCanvas.followme) mapCanvas.center = positionSource.position.coordinate
        }

        onWidthChanged: {
            scaleTimer.restart()
        }

        onHeightChanged:{
            scaleTimer.restart()
        }

        Keys.onPressed: {
            if (event.key === Qt.Key_Plus) {
                mapCanvas.zoomLevel++
            } else if (event.key === Qt.Key_Minus) {
                mapCanvas.zoomLevel--
            }
        }

        Item {
            id: scale
            visible: scaleText.text != "0 m"
            z: mapCanvas.z + 3
            anchors.bottom: mapCanvas.bottom //parent.bottom
            anchors.right: mapCanvas.right //parent.right
            //anchors.margins: 20
            height: scaleText.height * 2
            width: scaleImage.width
            anchors.bottomMargin: 8
            anchors.rightMargin: 8

            Image {
                id: scaleImageLeft
                source: "qrc:/img/map/scale_end.png"
                anchors.bottom: parent.bottom
                anchors.right: scaleImage.left
            }
            Image {
                id: scaleImage
                source: "qrc:/img/map/scale.png"
                anchors.bottom: parent.bottom
                anchors.right: scaleImageRight.left
            }
            Image {
                id: scaleImageRight
                source: "qrc:/img/map/scale_end.png"
                anchors.bottom: parent.bottom
                anchors.right: parent.right
            }
            Label {
                id: scaleText
                color: "#004EAE"
                anchors.centerIn: parent
                text: "0 m"
            }
            Component.onCompleted: {
                mapCanvas.calculateScale();
            }
        }

        Timer {
            id: scaleTimer
            interval: 100
            running: false
            repeat: false
            onTriggered: {
                mapCanvas.calculateScale()
            }
        }

        PositionSource{
            id: positionSource
            active: mapCanvas.followme

            onPositionChanged: {
                mapCanvas.center = positionSource.position.coordinate
            }
        }

        Rectangle{
            id: buttonmaximizemap
            objectName: "buttonmaximizemap"
            width: 20
            height: 20
            anchors.top: mapCanvas.top
            anchors.topMargin: 8
            anchors.right: mapCanvas.right
            anchors.rightMargin: 8
            z: maplegend.z + 4
            color: "transparent"

            property string tooltiptext: "Maximize Map"

            Image {
                id: maximizeimage
                source: "qrc:/img/map/Icon-feather-maximize.png"
                z: parent.z + 1
                scale: 1
                smooth: true
                width: buttonmaximizemap.width
                height: buttonmaximizemap.height

                onSourceChanged: {
                    if(maximizeimage.source == "qrc:/img/map/Icon-feather-maximize.png")
                        buttonmaximizemap.tooltiptext = "Maximize Map"
                    else
                        buttonmaximizemap.tooltiptext = "Minimize Map"
                }
            }

            MouseArea{
                id: mouseareabuttonmaximizemap
                objectName: "mouseareabuttonmaximizemap"
                anchors.fill: maximizeimage
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: {
                    ToolTip.visible = true
                    ToolTip.text = buttonmaximizemap.tooltiptext
                }

                onExited: {
                    ToolTip.visible = false
                }

                signal clickedButton(string str)

                onClicked: {
                    if(maximizeimage.source == "qrc:/img/map/Icon-feather-maximize.png"){
                        maximizeimage.source = "qrc:/img/map/Icon-feather-minimize.png"
                        ToolTip.text = buttonmaximizemap.tooltiptext

                        //set the map to maximum size
                        mouseareabuttonmaximizemap.clickedButton("1")
                    }
                    else{
                        maximizeimage.source = "qrc:/img/map/Icon-feather-maximize.png"

                        //set the map to minimum size
                        mouseareabuttonmaximizemap.clickedButton("0")
                    }
                }
            }

        }

        //added 5.10.2020
        Rectangle{
            id:rectlatlong
            anchors.top: buttonmaximizemap.top //mapCanvas.top
            //anchors.topMargin: 5
            anchors.right: buttonmaximizemap.left //mapCanvas.right
            anchors.rightMargin: 5
            width: 260//childrenRect.width
            height: childrenRect.height
            z: maplegend.z + 2

            Rectangle{
                id: rectlat
                width: 120
                height: 20
                anchors.top: rectlong.top
                anchors.right: rectlong.left

                Text {
                    id: textlat
                    text: "Latitude: " +
                          "<font size = '12' face ='Segue UI' ><b>" +
                          (mapCanvas.mouselat).toLocaleString(Qt.locale("de_DE"),'f',6) + "</b></font>" //.toFixed(6)
                    font.family: "Segoe UI"
                    font.pixelSize: 12
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.top: parent.top
                    anchors.topMargin: 2
                }
            }

            Rectangle{
                id: rectlong
                width: 140
                height: 20

                anchors.top: rectlatlong.top
                anchors.right: rectlatlong.right

                Text {
                    id: textlong
                    text: "Longitude: " +
                          "<font size = '12' face ='Segue UI' ><b>" +
                          mapCanvas.mouselon.toLocaleString(Qt.locale("de_DE"),'f',6) + "</b></font>";
                    font.family: "Segoe UI"
                    font.pixelSize: 12
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.topMargin: 2
                }
            }


        }

        Rectangle{
            id: plusButton
            smooth: true
            width: 18
            height: 18
            x: zoomSlider.x
            y: zoomSlider.y - height + 2
            radius: 14
            color: "#efefef"
            border.color: "#999999"
            Text {
                id: plus
                anchors.fill: parent
                width: 18
                height: 18
                color: "#1c1c1c"
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

        Slider {
            id: zoomSlider
            z: mapCanvas.z + 3

            //minimumValue: mapCanvas.minimumZoomLevel;
            //maximumValue: mapCanvas.maximumZoomLevel;

            from: mapCanvas.minimumZoomLevel
            to: mapCanvas.maximumZoomLevel

            //anchors.margins: 10
            //anchors.bottom: scale.top
            anchors.top: buttonmaximizemap.bottom //rectlatlong.bottom
            anchors.topMargin: 20
            anchors.right: mapCanvas.right//parent.right
            anchors.rightMargin: -3
            orientation : Qt.Vertical
            value: mapCanvas.zoomLevel
            onValueChanged: {
                mapCanvas.zoomLevel = value
            }

            background: Rectangle {
                x: zoomSlider.leftPadding
                y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                implicitWidth: 6
                implicitHeight: 100 //edited 5.7.2020 from 240 to 130
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

        Rectangle{
            id: rectclickinfo
            //height: 150
            //width: 80
            x: mapCanvas.mouseclickx
            y: mapCanvas.mouseclicky - (childrenRect.height + 8)
            width: childrenRect.width + 12
            height: childrenRect.height + 8
            color: "white"
            border.color: "#B7B6B5"
            border.width: 1
            opacity: 1.0
            z: 500

            visible: rectclickinfotimer.running

            property int counter: 0
            onCounterChanged: if(counter == 0){
                                  rectclickinfo.visible = false;
                                  rectclickinfo1.visible = false;
                                  rectclickinfotimer.stop();
                                  counter = 2 //second timer on closing
                              }

            Column {
                id: columnclickinfo
                spacing: 3

                Row{
                    id: rowclickifotitle
                    leftPadding: 16
                    topPadding: 8
                    Text {
                        id: textinfotitle
                        text: mapCanvas.textsegmenttitle
                        font.family: "Segue UI"
                        font.pixelSize: 12
                        color: "#757575"

                    }
                }
                Row{
                    id: rowstratum
                    leftPadding: 10
                    Text {
                        id: textstratum
                        text: "Stratum: " +
                              "<div style = 'font: Bold 11px Segue UI; color: red;'>" +
                              mapCanvas.textstratumno + "</div>"
                        font.family: "Segue UI"
                        font.pixelSize: 11
                    }
                }
                Row{
                    id: rowtransect
                    leftPadding: 10
                    Text {
                        id: texttransect
                        text: "Transect: " +
                              "<div style = 'font: Bold 11px Segue UI; color: red;'>" +
                              mapCanvas.texttransectno + "</div>"
                        font.family: "Segue UI"
                        font.pixelSize: 11
                    }
                }
                Row{
                    id: rowsegment
                    leftPadding: 10
                    Text {
                        id: textsegment
                        text: "Segment: " +
                              "<div style = 'font: Bold 11px Segue UI; color: red;'>" +
                              mapCanvas.textsegmentno + "</div>"
                        font.family: "Segue UI"
                        font.pixelSize: 11

                    }
                }

            }
        }

        Rectangle{
            id: rectclickinfo1
            width: childrenRect.width + 12
            height: childrenRect.height + 8
            color: "white"
            border.color: "#B7B6B5"
            border.width: 1
            opacity: 1.0
            anchors.top: rectclickinfo.bottom
            anchors.left: rectclickinfo.left
            z: 501

            visible: rectclickinfotimer.running

            Column{
                id: columnairground
                spacing: 3
                visible: true
                //anchors.top: columnclickinfo.bottom
                //anchors.left: columnclickinfo.left

                Row{
                    id: rowclickifotitle1
                    leftPadding: 20
                    topPadding: 8
                    Text {
                        id: textinfotitle1
                        text: mapCanvas.textsegmenttitle1
                        font.family: "Segue UI"
                        font.pixelSize: 12
                        color: "#757575"

                    }
                }
                Row{
                    id: rowagname
                    leftPadding: 10
                    Text {
                        id: textrowagname
                        text: "A_G Name: " +
                              "<div style = 'font: Bold 11px Segue UI; color: red;'>" +
                              mapCanvas.textagname + "</div>"
                        font.family: "Segue UI"
                        font.pixelSize: 11
                    }
                }
            }

        }

        Timer{
            id:rectclickinfotimer
            interval: 1000
            onTriggered: {
                rectclickinfo.counter --
            }
            repeat: true
            running: {
                rectclickinfo.counter > 0
            }
        }

        Rectangle{
            id:polyinfo
            width: childrenRect.width + 12
            height: childrenRect.height + 8
            color: "white"
            border.color: "#B7B6B5"
            border.width: 1
            opacity: 1.0
            x: mapCanvas.mouseclickx
            y: mapCanvas.mouseclicky - (childrenRect.height + 8)
            z: 601

            visible: rectpolyinfotimer.running
            property int counter: 0
            onCounterChanged: if(counter == 0){
                                  polyinfo.visible = false;
                                  rectpolyinfotimer.stop();
                                  counter = 2 //second timer on closing
                              }


            Column{
                id: colpolyinfo
                spacing: 3
                visible: true

                Row{
                    id: rowpolyinfotitle
                    leftPadding: 20
                    topPadding: 8
                    Text {
                        id: textpolyinfotitle
                        text: mapCanvas.textpolytitle
                        font.family: "Segue UI"
                        font.pixelSize: 12
                        color: "#757575"

                    }
                }
                Row{
                    id: rowplotname
                    leftPadding: 10
                    Text {
                        id: textrowplotnm
                        text: "Plot Name: " + mapCanvas.textplotlabel
                        font.family: "Segue UI"
                        font.pixelSize: 11
                    }
                }
            }
        }

        Timer{
            id:rectpolyinfotimer
            interval: 1000
            onTriggered: {
                polyinfo.counter --
            }
            repeat: true
            running: {
                polyinfo.counter > 0
            }
        }

        function checkPolySurveyList(coor){
            mapCanvas.textpolytitle = "Survey JSON";
            mapCanvas.textplotlabel = "";
            if(polyinfo.visible == true)
                polyinfo.visible = false;
            //for survey json
            polyplotlabel = handler.checkPtWithin(coor,"SURVEY")//check if the clickpt is within the line
            if(polyplotlabel.length > 0){
                mapCanvas.textplotlabel = polyplotlabel;
                polyinfo.visible = true;
                polyinfo.counter = 2;
                rectpolyinfotimer.start();
            }

        }

        Rectangle{
            id:goeainfo
            width: childrenRect.width + 12
            height: childrenRect.height + 8
            color: "white"
            border.color: "#B7B6B5"
            border.width: 1
            opacity: 1.0
            anchors.top: rectclickinfo.bottom
            anchors.left: rectclickinfo.left
            z:701

            visible: goeatimer.running
            property int counter: 0
            onCounterChanged: if(counter == 0){
                                  goeainfo.visible = false;
                                  goeatimer.stop();
                                  counter = 2 //second timer on closing
                              }


            Column{
                id: col1
                spacing: 3
                visible: true

                Row{
                    id: row1
                    leftPadding: 20
                    topPadding: 8
                    Text {
                        id: textgoeainfotitle
                        text: mapCanvas.textsegmenttitle
                        font.family: "Segue UI"
                        font.pixelSize: 12
                        color: "#757575"

                    }
                }
                Row{
                    id: rowbcr
                    leftPadding: 10
                    Text {
                        id: textrowbcr
                        text: "BCR: " + mapCanvas.textbcr
                        font.family: "Segue UI"
                        font.pixelSize: 11
                    }
                }
                Row{
                    id: rowtrn
                    leftPadding: 10
                    Text {
                        id: textrowtrn
                        text: "Transect: " + mapCanvas.texttrn
                        font.family: "Segue UI"
                        font.pixelSize: 11
                    }
                }
            }
        }

        function getSurveyType(clickcoord){
            mapCanvas.surveytype = handler.surveytype;
            if(mapCanvas.surveytype == "GOEA"){
                mapCanvas.checkSurveyListGOEA(clickcoord)
            }else if(mapCanvas.surveytype == "BAEA"){
                mapCanvas.checkPolySurveyList(clickcoord)
            }else{
                mapCanvas.checkSurveyList(clickcoord)
            }
        }

        function checkSurveyListGOEA(coor){
            mapCanvas.textsegmenttitle = "Survey JSON"
            mapCanvas.textbcr = ""
            mapCanvas.texttrn = ""
            if(goeainfo.visible == true)
                goeainfo.visible = false

            //for survey json
            goeaval = handler.checkPtExist(coor,"GOEA")//check if the clickpt is within the line
            //console.log(goeaval)

            if(goeaval.length > 0){
                var splt = goeaval.split(",")
                mapCanvas.textbcr = splt[0]
                mapCanvas.texttrn = splt[1]

                goeainfo.visible = true
                goeainfo.counter = 2
                goeatimer.start()
            }
        }

        Timer{
            id:goeatimer
            interval: 1000
            onTriggered: {
                goeainfo.counter --
            }
            repeat: true
            running: {
                goeainfo.counter > 0
            }
        }

        function checkSurveyList(coor){
            mapCanvas.textsegmenttitle = "Survey JSON"
            mapCanvas.textstratumno = ""
            mapCanvas.texttransectno = ""
            mapCanvas.textsegmentno = ""
            mapCanvas.textagname = ""
            if(rectclickinfo.visible == true)
                rectclickinfo.visible = false

            //for survey json
            stratranseg = handler.checkPtExist(coor,"SURVEY")//check if the clickpt is within the line
            if(stratranseg.length > 0){
                var splt = stratranseg.split(",")
                mapCanvas.textstratumno = splt[0]
                mapCanvas.texttransectno = splt[1]
                mapCanvas.textsegmentno = splt[2]
                //console.log(stratranseg)
                rectclickinfo.visible = true
                rectclickinfo.counter = 2
                rectclickinfotimer.start()
            }

            if(rectclickinfo1.visible == true)
                rectclickinfo1.visible = false
            //for airground json
            agseg = handler.checkPtExist(coor,"AIRGROUND")//check if the clickpt is within the line
            //console.log(agseg)
            if(agseg.length > 0){
                mapCanvas.textagname = agseg
                rectclickinfo1.visible = true
            }
        }

        Rectangle{
            id: buttonlegend
            width: 20
            height: 20
            anchors.top: mapCanvas.top
            anchors.topMargin: 8
            anchors.left: mapCanvas.left
            anchors.leftMargin: 8
            z: maplegend.z + 3
            color: "transparent"


            Image {
                id: legendimage
                source: "qrc:/img/map/Icon-feather-map.png"
                z: parent.z + 1
                scale: 1
                smooth: true
                width: buttonlegend.width
                height: buttonlegend.height
            }

            MouseArea{
                anchors.fill: legendimage
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: {
                    ToolTip.visible = true
                    ToolTip.text = "Toggle Map Legend"
                }

                onExited: {
                    ToolTip.visible = false
                }

                onClicked: {
                    if(maplegend.visible == true)
                        maplegend.visible = false
                    else
                        maplegend.visible = true
                }
            }


        }

        Rectangle{
            id:maplegend
            color: "white"
            anchors.top: buttonlegend.top
            anchors.left: buttonlegend.left
            anchors.topMargin: -8
            anchors.leftMargin: -8
            width: 165
            height: 135
            z:10000
            visible: false

            Column{
                id: maplegendcolumn
                Row{
                    id:maplegendrow1
                    topPadding: 10
                    leftPadding: 48
                    bottomPadding: 3
                    Text {
                        id: legend
                        text: qsTr("Map Legend")
                        font.family: "Segue UI"
                        font.pointSize: 9
                        font.bold: true
                    }
                }

                Row{
                    id:maplegendrow2
                    topPadding: -5
                    bottomPadding: -8

                    CheckBox{
                        checked: mapCanvas.withFlightData
                        enabled: mapCanvas.withFlightData
                        id:flightdataline

                        indicator.width: 20
                        indicator.height: 20

                        onClicked: {
                            if(flightdataline.checked)
                                mapCanvas.setVisibility("Flight Data",1)
                            else
                                mapCanvas.setVisibility("Flight Data",0)
                        }
                    }

                    Image {
                        width: 20
                        height: 20
                        id: blueline
                        source: "qrc:/img/map/blueline.png"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text{
                        text: qsTr("Flight Data")
                        font.family: "Segoe UI"
                        opacity: flightdataline.enabled ? 1.0 : 0.3
                        color: flightdataline.down ? "#000000" : "#212121"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        topPadding: 10
                        leftPadding: 10
                    }
                }

                Row{
                    id:maplegendrow3
                    topPadding: -8
                    bottomPadding: -8

                    CheckBox{
                        checked: mapCanvas.withSurveyData
                        enabled: mapCanvas.withSurveyData
                        id:surveyline

                        indicator.width: 20
                        indicator.height: 20

                        onClicked: {
                            if(surveyline.checked)
                                mapCanvas.setVisibility("Survey JSON",1)
                            else
                                mapCanvas.setVisibility("Survey JSON",0)
                        }
                    }

                    Image {
                        width: 20
                        height: 20
                        id: redline
                        source: "qrc:/img/map/redline.png"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text{
                        text: qsTr("Survey JSON")
                        font.family: "Segoe UI"
                        opacity: surveyline.enabled ? 1.0 : 0.3
                        color: surveyline.down ? "#000000" : "#212121"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        topPadding: 10
                        leftPadding: 10
                    }
                }

                Row{
                    id:maplegendrow4
                    topPadding: -8
                    bottomPadding: -8

                    CheckBox{
                        checked: mapCanvas.withAirGroundSurveyData
                        enabled: mapCanvas.withAirGroundSurveyData
                        id:airgroundline

                        indicator.width: 20
                        indicator.height: 20

                        onClicked: {
                            if(airgroundline.checked)
                                mapCanvas.setVisibility("Air Ground JSON",1)
                            else
                                mapCanvas.setVisibility("Air Ground JSON",0)
                        }
                    }

                    Image {
                        width: 20
                        height: 20
                        id: yellowline
                        source: "qrc:/img/map/yellowline.png"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text{
                        text: qsTr("Air Ground JSON")
                        font.family: "Segoe UI"
                        opacity: airgroundline.enabled ? 1.0 : 0.3
                        color: airgroundline.down ? "#000000" : "#212121"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        topPadding: 10
                        leftPadding: 10
                    }
                }

                Row{
                    id:maplegendrow5
                    topPadding: -8
                    bottomPadding: -8

                    CheckBox{
                        checked: mapCanvas.withWavFile
                        enabled: mapCanvas.withWavFile
                        id:wavfile

                        indicator.width: 20
                        indicator.height: 20

                        onClicked: {
                            if(wavfile.checked)
                                mapCanvas.setVisibility("Wav File",1)
                            else
                                mapCanvas.setVisibility("Wav File",0)
                        }
                    }

                    Image {
                        width: 20
                        height: 20
                        id: wavpt
                        source: "qrc:/img/map/pin_unselected.png"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text{
                        text: qsTr("Wav File")
                        font.family: "Segoe UI"
                        opacity: wavfile.enabled ? 1.0 : 0.3
                        color: wavfile.down ? "#000000" : "#212121"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        topPadding: 10
                        leftPadding: 10
                    }
                }

            }
        }

        //added 5.2.2020 map legend for testing
        /*Rectangle{
            id:maplegend
            color: "white"
            //x: 2
            //y: 2
            anchors.top: buttonlegend.top
            anchors.left: buttonlegend.left
            anchors.topMargin: 5
            anchors.leftMargin: 5
            width: 160
            height: 120
            z:10000
            visible: false

            Column{
                id: maplegendcolumn
                Row{
                    id:maplegendrow1
                    topPadding: 5
                    leftPadding: 40
                    bottomPadding: 5
                    Text {
                        id: legend
                        text: qsTr("Map Legend")
                        font.family: "Segue UI"
                        font.pointSize: 9
                        font.bold: true
                    }
                }

                Row{
                    id:maplegendrow2
                    leftPadding: 10
                    topPadding: -2
                    bottomPadding: -5

                    Image {
                        width: 20
                        height: 20
                        id: blueline
                        source: "qrc:/img/map/blueline.png"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    CheckBox{
                        checked: mapCanvas.withFlightData
                        enabled: mapCanvas.withFlightData
                        id:flightdataline
                        text: qsTr("Flight Data")
                        indicator: Rectangle {
                            implicitWidth: 20
                            implicitHeight: 20
                            x: flightdataline.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 1
                            border.color: flightdataline.down ? "#000000" : "#BDBDBD"

                            Rectangle {
                                width: parent.width - 10
                                height: parent.height - 10
                                anchors.centerIn: parent
                                radius: 1
                                color: flightdataline.down ? "#4542F2" : "#8381F6"
                                visible: flightdataline.checked
                            }
                        }

                        contentItem: Text {
                            text: flightdataline.text
                            font.family: "Segoe UI"
                            opacity: enabled ? 1.0 : 0.3
                            color: flightdataline.down ? "#000000" : "#212121"
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: flightdataline.indicator.width + flightdataline.spacing
                        }

                        onClicked: {
                            if(flightdataline.checked)
                                mapCanvas.setVisibility(flightdataline.text,1)
                            else
                                mapCanvas.setVisibility(flightdataline.text,0)
                        }
                    }
                }
                Row{
                    id:maplegendrow3
                    leftPadding: 10
                    topPadding: -5
                    bottomPadding: -5
                    Image {
                        width: 20
                        height: 20
                        id: redline
                        source: "qrc:/img/map/redline.png"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    CheckBox{
                        checked: mapCanvas.withSurveyData
                        enabled: mapCanvas.withSurveyData
                        id:surveyline
                        text: qsTr("Survey JSON")
                        indicator: Rectangle {
                            implicitWidth: 20
                            implicitHeight: 20
                            x: surveyline.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 1
                            border.color: surveyline.down ? "#000000" : "#BDBDBD"

                            Rectangle {
                                width: parent.width - 10
                                height: parent.height - 10
                                anchors.centerIn: parent
                                radius: 1
                                color: surveyline.down ? "#4542F2" : "#8381F6"
                                visible: surveyline.checked
                            }
                        }

                        contentItem: Text {
                            text: surveyline.text
                            font.family: "Segoe UI"
                            opacity: enabled ? 1.0 : 0.3
                            color: surveyline.down ? "#000000" : "#212121"
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: surveyline.indicator.width + surveyline.spacing
                        }

                        onClicked: {
                            if(surveyline.checked)
                                mapCanvas.setVisibility(surveyline.text,1)
                            else
                                mapCanvas.setVisibility(surveyline.text,0)
                        }
                    }
                }
                Row{
                    id:maplegendrow4
                    leftPadding: 10
                    topPadding: -5
                    bottomPadding: -5
                    Image {
                        width: 20
                        height: 20
                        id: yellowline
                        source: "qrc:/img/map/yellowline.png"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    CheckBox{
                        id: airgroundline
                        checked: mapCanvas.withAirGroundSurveyData
                        enabled: mapCanvas.withAirGroundSurveyData
                        text: qsTr("Air Ground JSON")
                        indicator: Rectangle {
                            implicitWidth: 20
                            implicitHeight: 20
                            x: airgroundline.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 1
                            border.color: airgroundline.down ? "#000000" : "#BDBDBD"

                            Rectangle {
                                width: parent.width - 10
                                height: parent.height - 10
                                anchors.centerIn: parent
                                radius: 1
                                color: airgroundline.down ? "#4542F2" : "#8381F6"
                                visible: airgroundline.checked
                            }
                        }

                        contentItem: Text {
                            text: airgroundline.text
                            font.family: "Segoe UI"
                            opacity: enabled ? 1.0 : 0.3
                            color: airgroundline.down ? "#000000" : "#212121"
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: airgroundline.indicator.width + airgroundline.spacing
                        }

                        onClicked: {
                            if(airgroundline.checked)
                                mapCanvas.setVisibility(airgroundline.text,1)
                            else
                                mapCanvas.setVisibility(airgroundline.text,0)
                        }
                    }
                }
                Row{
                    id:maplegendrow5
                    leftPadding: 10
                    topPadding: -5
                    bottomPadding: -5
                    Image {
                        id: wavpt
                        source: "qrc:/img/map/pin_unselected.png"
                        width: 20
                        height: 20
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    CheckBox{
                        id:wavfile
                        checked: mapCanvas.withWavFile
                        enabled: mapCanvas.withWavFile
                        text: qsTr("Wav File")
                        indicator: Rectangle {
                            implicitWidth: 20
                            implicitHeight: 20
                            x: wavfile.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 1
                            border.color: wavfile.down ? "#000000" : "#BDBDBD"

                            Rectangle {
                                width: parent.width - 10
                                height: parent.height - 10
                                anchors.centerIn: parent
                                radius: 1
                                color: wavfile.down ? "#4542F2" : "#8381F6"
                                visible: wavfile.checked
                            }
                        }

                        contentItem: Text {
                            text: wavfile.text
                            font.family: "Segoe UI"
                            opacity: enabled ? 1.0 : 0.3
                            color: wavfile.down ? "#000000" : "#212121"
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: wavfile.indicator.width + wavfile.spacing
                        }

                        onClicked: {
                            if(wavfile.checked)
                                mapCanvas.setVisibility(wavfile.text,1)
                            else
                                mapCanvas.setVisibility(wavfile.text,0)
                        }
                    }
                }
            }
        }*/

        /*Rectangle{
            id:legendnamerect
            //anchors.fill: maplegendrow1
            //anchors.bottom: maplegendrow1.bottom
            //anchors.left: maplegendrow1.left
            width: maplegend.width + 2
            height: maplegendrow1.height
            color: "#B8B8B8"
            z:maplegend.z + 1
            border.color: "#B8B8B8"
            border.width: 3

            visible: false

            Text {
                id: textlegendnamerect
                text: qsTr("Map Legend")
                font.family: "Segue UI"
                font.pointSize: 9
                font.bold: true
                color: "white"
                anchors.centerIn: legendnamerect
            }

            MouseArea{
                id:legendnamerectmousearea
                anchors.fill: legendnamerect
                hoverEnabled: true
                onClicked: {
                    if(maplegend.height == 120){
                        maplegend.height = legendnamerect.height
                        maplegendrow1.visible = false
                        maplegendrow2.visible = false
                        maplegendrow3.visible = false
                        maplegendrow4.visible = false
                    }else{
                        maplegend.height = 120
                        maplegendrow1.visible = true
                        maplegendrow2.visible = true
                        maplegendrow3.visible = true
                        maplegendrow4.visible = true
                    }
                }
                cursorShape: Qt.PointingHandCursor
            }
        }*/

        function setVisibility(layer,visibility){
            if(typeof(handler) != "undefined")
                var val1 = handler.layerSetVisible(layer,visibility)
        }

        function validateItemExistance(){
            if(typeof(handler) != "undefined"){
                if(handler.withFlightData == 1)
                    withFlightData = true
                if(handler.withAirGroundSurvey == 1)
                    withAirGroundSurveyData = true
                if(handler.withSurveyData == 1)
                    withSurveyData = true
                if(handler.withWavFile == 1)
                    withWavFile = true
            }

        }

        Rectangle{
            id: buttonmapoverlay
            width: 20
            height: 20
            anchors.bottom: mapCanvas.bottom
            anchors.bottomMargin: 5
            anchors.left: mapCanvas.left
            anchors.leftMargin: 5
            z: maplegend.z + 4
            color: "transparent"

            property string exporttooltiptext: "Map Overlay"

            Image {
                id: overlayimage
                source: "qrc:/img/map/Icon-exportmap.png"
                z: parent.z + 1
                scale: 1
                smooth: true
                width: buttonmapoverlay.width
                height: buttonmapoverlay.height

            }

            MouseArea{
                anchors.fill: overlayimage
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: {
                    ToolTip.visible = true
                    ToolTip.text = buttonmapoverlay.exporttooltiptext
                }

                onExited: {
                    ToolTip.visible = false
                }

                onClicked: {
                    //show map overlay choices
                    if(mapoverlay.visible == true)
                        mapoverlay.visible = false
                    else
                        mapoverlay.visible = true
                }
            }
        }

        Rectangle{
            id:mapoverlay
            color: "white"
            anchors.bottom: buttonmapoverlay.top
            anchors.left: buttonmapoverlay.right
            anchors.topMargin: -8
            anchors.leftMargin: -8
            width: 140
            height: 61
            z:10000
            visible: false

            ColumnLayout{
                id: maplayoutcolumn
                spacing: 1
                Rectangle {
                    id: maplayoutrow1
                    Layout.alignment: Qt.AlignCenter
                    color: "lightgray"
                    Layout.preferredWidth: 140
                    Layout.preferredHeight: 30
                    Text {
                        id: maplayouttext1
                        anchors.centerIn: maplayoutrow1
                        text: qsTr("Street Map")
                        font.family: "Segue UI"
                        font.pointSize: 9
                        font.bold: false
                    }
                    MouseArea{
                        anchors.fill: maplayoutrow1
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onClicked: {
                            maplayoutrow1.color = "lightgray"
                            maplayoutrow2.color = "white"
                            mapCanvas.changePlugin("default")
                        }
                    }
                }

                Rectangle {
                    id: maplayoutrow2
                    Layout.alignment: Qt.AlignCenter
                    color: "white"
                    Layout.preferredWidth: 140
                    Layout.preferredHeight: 30
                    Text {
                        id: maplayouttext2
                        anchors.centerIn: maplayoutrow2
                        text: qsTr("Satellite Map")
                        font.family: "Segue UI"
                        font.pointSize: 9
                        font.bold: false
                    }
                    MouseArea{
                        anchors.fill: maplayoutrow2
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onClicked: {
                            maplayoutrow1.color = "white"
                            maplayoutrow2.color = "lightgray"
                            mapCanvas.changePlugin("satellite")
                        }
                    }
                }
            }
        }

        function changePlugin(plugval){
            //console.log(plugval)
            appWindow.mapType = plugval
            appWindow.requestPaint()
            appWindow.update()
            mapCanvas.update()
        }

    }
}
