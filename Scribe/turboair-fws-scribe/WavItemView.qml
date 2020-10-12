import QtQuick 2.14
import QtLocation 5.9
import QtPositioning 5.8
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.14

MapItemView {
    property string labelglowcolor
    property string imghover
    property string imgselected
    property string imgdefault
    property WayHandler wayHandle
    property var mapView
    property string labelcolor
    property int fontsize
    property var wavList

    id: wavitemview
    ListModel {
        id: wavListModel
    }

    model: wavListModel
    delegate:
        MapQuickItem {
            id: wavpoint
            objectName: "wavpoint"
            anchorPoint.x: image.width / 2
            anchorPoint.y: image.height
            z: 5
            coordinate: QtPositioning.coordinate(latitude, longitude)

            sourceItem: Image{
                id: image
                objectName: "image"
                source: imgdefault
                smooth: true
                width: 30
                height: 30

                MouseArea {
                    id: mouseArea
                    objectName: "mouseArea"
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton | Qt.LeftButton
                    hoverEnabled: true
                    property bool isOpen: false
                    onEntered: {
                        image.state = "entered"
                        label.visible = true
                        labelrect.visible = true
                    }
                    onExited:  {
                        image.state = "exited"
                        label.visible = false
                        labelrect.visible = false
                    }

                    onPressed: {
                        if(mouseArea.pressedButtons & Qt.RightButton) {

                        }
                    }

                    onClicked: {
                        image.state = "entered"
                        label.visible = true
                        labelrect.visible = true
                    }

                }

                states: [
                    State {
                        name: "entered"
                        PropertyChanges {
                            target: image;
                            source: imghover;
                        }
                    },
                    State {
                        name: "exited"
                        PropertyChanges { target: image; source: imgdefault }
                    }
                ]

                Rectangle{
                    id: labelrect
                    objectName: "labelrect"
                    width: label.width + 16
                    height: label.height + 20
                    color: "white"
                    border.color: "#B7B6B5"
                    border.width: 1
                    opacity: 1.0
                    anchors.centerIn: label
                    visible: false
                }
                Label{
                    id: label
                    objectName: "label"
                    anchors.left: image.right
                    anchors.bottom: image.top

                    font: textMetrics.font
                    text: textMetrics.text
                    color: labelcolor

                    visible: false
                }

                TextMetrics {
                    id: textMetrics
                    objectName: "textMetrics"
                    font.family: "Segui UI"
                    font.pixelSize: fontsize

                    text: "File: " + wavfilename +
                          "\nDate: " + wavdate +
                          "\nTime: " + wavtime +
                          "\nLatitude: " + latitude +
                          "\nLongitude: " + longitude
                }
            }
    }


    function populateListModel(data){
        wavListModel.clear();
        for(var i =0; i < data.length; i++){
            var splt = data[i].split(',')
            if (splt.length > 1){
                var coor = QtPositioning.coordinate(parseFloat(splt[0]),parseFloat(splt[1]));
                /*console.log("latitude: " + coor.latitude);
                console.log("longitude" + coor.longitude);*/

                wavListModel.append({
                    wavfilename: splt[splt.length - 1],
                    latitude: coor.latitude,
                    longitude: coor.longitude,
                    wavdate: parseFloat(splt[3]) + "-" + parseFloat(splt[4]) + "-" + parseFloat(splt[2]), // mm-dd-yyyy
                    wavtime: splt[5] //hh:mm:ss
                });
            }
        }

    }

    function toggleSelection(wavnm){
        for(var i = 0; i < wavListModel.count; i++){
            var wav = wavListModel.get(i).wavfilename;
            //console.log(wav);
            if (wav === wavnm){
                mapView.center = QtPositioning.coordinate(wavListModel.get(i).latitude,
                                                          wavListModel.get(i).longitude);
                mapView.zoomLevel = 11;
                break;
            }
        }
    }

}


