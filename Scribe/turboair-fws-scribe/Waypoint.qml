import QtQuick 2.0
import QtLocation 5.8
import QtPositioning 5.8
import QtQuick.Controls 2.2

//marks a waypoint on the map
//TO DO: consider having a way to change the icon for the point
MapQuickItem{
    id: way
    anchorPoint.x: image.width / 2
    anchorPoint.y: image.height
    z: 5
    property string descriptor
    property WayHandler wayHandle
    property var mapView //added 4.25.2020
    sourceItem: Image{
        id: image
        source: "qrc:/img/map/bookmark.png"
        smooth: true
        width: 24 //changed 4.25.2020 46
        height: 24 //changed 4.25.2020 46
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            acceptedButtons: Qt.RightButton | Qt.LeftButton
            hoverEnabled: true
            property bool isOpen: false
            onEntered: {
                way.scale = 1.1
                //color.color =  '#696969FF'
                descriptionbox.visible = true
                way.z = 10
            }
            onExited:  {
                way.scale = 1.0
                //color.color =  '#FFFFFFFF'
                descriptionbox.visible = false
                way.z = 5
            }
            onPressed: {
                if(mouseArea.pressedButtons & Qt.RightButton) {
                    wayHandle.removeBookMark(way)
                    /*if (!isOpen){
                        deletebox.visible = true
                        isOpen = true
                    }else{
                        deletebox.visible = false
                        isOpen = false
                    }*/
                }
            }
        }
        Image{
            id: deletebox
            source:  "qrc:/img/map/label.png"
            visible: false
            width: 40
            height: 40
            x: 55
            y: -10
            scale : 1

            MouseArea{
                id: deleteboxMouseArea
                anchors.fill:parent
                acceptedButtons: Qt.RightButton | Qt.LeftButton
                hoverEnabled: true
                onPressed: {
                    if (deleteboxMouseArea.pressedButtons & Qt.LeftButton){
                        wayHandle.removeBookMark(way)
                    }
                }
                onEntered: {
                    deletebox.scale = 1.01
                }
                onExited: {
                    deletebox.scale = 1
                    deletebox.visible = false
                    mouseArea.isOpen = false
                }
            }
        }

        Image{
        id: descriptionbox
        width: 200
        height: 50
        y : -45
        x : 32
        z : 1+ way.z
        visible: false
        source: "qrc:/img/map/label.png"
        opacity: 1
        Text{
            z : 7
            id: tex
            elide: Text.ElideRight

            color: 'black'
            text: way.descriptor
            renderType: Text.NativeRendering
            smooth: true
            font.pointSize: 8//4.25.2020 20
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment:  Text.AlignVCenter
            anchors.fill: parent
            //color: 'black'
            }
        }
    }
}
