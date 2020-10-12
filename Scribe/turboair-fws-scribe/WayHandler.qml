import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

//handles all of the waypoint and path side of things
Item {
    id: wayHandler
    objectName: "waypointHandler"
    property Map mapview
    property var sets : []
    property int setCount
    property var bookMarks : []
    property var pilotMarker
    property var transects : []
    property var wavFileSets: [] //added 4.25.2020
    property bool initScale: true //added 4.27.2020
    property var wavFileSelected //added 4.28.2020
    property int intLocGPS  // added 5.2.2020
    property var surveySegSet: []
    property var surveySegSet2: []
    property var airGroundSegSet: []
    property var surveyPolygonSet: []
    property var surveySetGOEA: []

    property string surveytype
    property string strBAEAsurveyPlotNm: ""
    property string strWBPHSsurveyAGNm: ""
    property variant maxDistance //kilometer
    property string strGOEAsurveyBcrTrn
    property string strWBPHSsurveyVal: ""
    property variant airgroundMaxDistance

    property var wavCircleSelected
    property var agCircleSelected
    property int withFlightData: 0
    property int withSurveyData: 0
    property int withAirGroundSurvey: 0
    property int withWavFile: 0

    signal removeLog(var index)
    //currently a work in progress, will have it so we can
    //import coordinates and make a waypoint set out of it
    function initialize(){
        pilotMarker = Qt.createQmlObject('PilotMarker{}',mapview)
        mapview.addMapItem(pilotMarker)
        pilotMarker.coordinate = mapview.center


    }

    function updatePilotCoordinates(coords){
            var splt = coords.split(',')
            if (splt.length > 1){
                pilotMarker.xCoord = parseFloat(splt[0])
                pilotMarker.yCoord = parseFloat(splt[1])
            }
            pilotMarker.opacity = 1
            pilotMarker.smoothing();
    }

    function clearOldPaths(){
        for(var s = 0; s<sets.length; s++){
            sets[s].removeMapItems()
            mapview.removeMapItem(sets[s])
            sets[s].destroy()
        }
        sets = []
    }

    function plotCoordinates(data){
        pilotMarker.opacity = 0

        //TO DO: Add something that either hides or removes other gps streams
        var datas = data
        //creating a new "waypath" object, and adding it to our sets list
        var m = Qt.createQmlObject('Waypath{}',mapview)
        m.mapView = mapview
        //for every coordinate in our string of data, split it up into a readable coordinate
        for(var i = 0; i<datas.length; i++){
            var splt = datas[i].split(',')
            if (splt.length > 1){
                m.addCoordinate(QtPositioning.coordinate(parseFloat(splt[0]),parseFloat(splt[1])))
            }
        }
        m.initialize()
        mapview.addMapItem(m)
        sets.push(m)
        mapview.fitViewportToVisibleMapItems();
        withFlightData = 1
    }
    function zoomToValue(value){
        mapview.zoomLevel = value;
    }

    function clearAllBookmarks(){
        for (var i = 0; i < bookMarks.length; i ++){
            mapview.removeMapItem(bookMarks[i])
            wayHandler.removeBookMark(i)
        }
        bookMarks = []
        console.log(bookMarks.length)
    }

    function markPathOfInterest(data){

    }

    function plotTransects(data){
        for (var s = 0; s < transects.length; s ++){
            mapview.removeMapItem(transects[s])
            //if (transects[s]){
                transects[s].destroy();
            //}
        }
        transects = [];
        var datas = data
        for(var i = 0; i < datas.length; i++){
            createTransectBox(datas[i]);
        }
    }
    function showTransects(data){
        if (data === "false"){
            for (var i = 0; i < transects.length; i++){
                transects[i].opacity = 0;
            }
        }else{
            for (var f = 0; f < transects.length; f++){
                transects[f].opacity = 0.3;
            }
        }
    }

    function createTransectBox(data){
        var datas = data
        var tranbox = Qt.createQmlObject('TransectBox{}',mapview)
        var coords = datas.split(';')
        for (var i = 0; i < coords.length; i ++){
            var point  = coords[i].split(',')
            tranbox.addCoordinate(QtPositioning.coordinate(parseFloat(point[0]),parseFloat(point[1])))
        }
        transects.push(tranbox)
        mapview.addMapItem(tranbox)
    }
    function toggleTransects(isActive){
        for (var i = 0; i < transects.length; i ++){
            if (isActive === "true"){
                transects[i].visible = true
            }else{
                transects[i].visible = false
            }
        }
    }

    function addBookMarkAtCurrent(desc){
            //adding a waypoint when this is called
        var m = Qt.createQmlObject('Waypoint{}',mapview)
        m.wayHandle = wayHandler
            //placeholder coordinates
        m.coordinate = pilotMarker.coordinate
            //setting that 4 character descriptor above the waypoint
        m.descriptor = desc
            //adding the map item and pushing it to our bookmarks array
        mapview.addMapItem(m)
        bookMarks.push(m)
    }
    function addBookMarkWithCoord(desc, lattitude, longitude){
            //adding a waypoint when this is called
        var m = Qt.createQmlObject('Waypoint{}',mapview)
        m.wayHandle = wayHandler
            //placeholder coordinates
        m.coordinate = QtPositioning.coordinate(parseFloat(lattitude),parseFloat(longitude))
            //setting that 4 character descriptor above the waypoint
        m.descriptor = desc
            //adding the map item and pushing it to our bookmarks array
        mapview.addMapItem(m)
        bookMarks.push(m)
    }
    function removeBookMark(item){
        //we have an index and have to create a new array for our bookmark list
        //this is because qml doesn't do dynamic removal in any of its list types
        var index
        var newBookList = []
        mapview.removeMapItem(item)
        for (var i = 0; i < bookMarks.length; i ++){
            if (bookMarks[i] === item){
                //saving that index so we can kill it in our log list on the c++ side
                index = i
            }else{
                //if it's not the one we're removing, fill up our new array with it
                newBookList.push(bookMarks[i])
            }
        }
        //this should assign our new array, thus letting us line up our waypoints with our logs
        bookMarks = newBookList
        console.log(bookMarks.length)
        //sending over a signal to the c++ side
        wayHandler.removeLog(index)
    }

    //added 4.25.2020 to show th wav file location
    function plotWavFile(data, intchk, survytype){
        var datas = data;
        initScale = true;
        intLocGPS = intchk;
        surveytype = survytype;
        //for every coordinate in our string of data, split it up into a readable coordinate
        for(var i = 0; i<datas.length; i++){
            var splt = datas[i].split(',');
            if (splt.length > 1){
                //creating a new "Waypoint" object, and adding it to our sets list
                var m = Qt.createQmlObject('Wavpoint{}',mapview);//Waypoint
                //m.mapView = mapview

                var coor = QtPositioning.coordinate(parseFloat(splt[0]),parseFloat(splt[1]));
                m.coordinate = coor; //QtPositioning.coordinate(parseFloat(splt[1]),parseFloat(splt[2]))
                m.descriptor = splt[splt.length - 1];
                m.latitude = coor.latitude;
                m.longitude = coor.longitude;
                m.wavdate = parseFloat(splt[3]) + "-" + parseFloat(splt[4]) + "-" + parseFloat(splt[2]);// mm-dd-yyyy
                m.wavtime = splt[5]; //hh:mm:ss
                m.imgdefault = "qrc:/img/map/pin_unselected.png";
                m.imghover = "qrc:/img/map/pin_selected.png";
                m.imgselected = "qrc:/img/map/pin_locked.png";
                m.labelglowcolor = "yellow";
                m.labelcolor = "black";
                m.fontsize = 10;
                m.labelrectbgcolor = "white";
                m.zorder = datas.length + 1;
                mapview.addMapItem(m);
                wavFileSets.push(m);

            }
        }
        withWavFile = 1;
    }

    function populateWavList(data){//version 2 test
        var datas = data
        initScale = true;
        var m = Qt.createQmlObject('WavItemView{}',mapview);//Waypoint
        m.mapView = mapview;
        m.imgdefault = "qrc:/img/map/pin_unselected.png";
        m.imghover = "qrc:/img/map/pin_selected.png";
        m.imgselected = "qrc:/img/map/pin_locked.png";
        m.labelglowcolor = "yellow";
        m.labelcolor = "black";
        m.fontsize = 10;

        m.populateListModel(datas);

        mapview.addMapItemView(m);
        wavFileSets.push(m)
    }



    //added 4.27.2020 zoom to selected wav file
    function selectWavFile(wavfilenm, airgd, maxd){
        //version 1 for testing wavpoint.qml
        var returnval = "";
        initScale = false;
        if(wavFileSelected){
            wavFileSelected.setProperties(false, "qrc:/img/map/pin_unselected.png", "yellow", "black", 10, "white");//to default
            mapview.update();
        }

        for(var i = 0; i < wavFileSets.length; i++){
            if(wavFileSets[i].descriptor === wavfilenm){
                //console.log(wavFileSets[i].descriptor);
                //mapview.scale = 15;

                //console.log("lat:"  + wavFileSets[i].latitude + " longitude:"  + wavFileSets[i].longitude);
                var wavcoor = QtPositioning.coordinate(wavFileSets[i].latitude,wavFileSets[i].longitude);
                var newcentercoor = wavcoor.atDistanceAndAzimuth(10,0);
                mapview.center = newcentercoor; //QtPositioning.coordinate(wavFileSets[i].latitude,wavFileSets[i].longitude);

                if(i === 0)
                    mapview.zoomLevel = 11;

                wavFileSelected = wavFileSets[i];
                wavFileSets[i].setProperties(true, "qrc:/img/map/pin_selected.png", "red", "black", 10, "white");
                wavFileSets[i].update();
                //mapview.removeMapItem(wavFileSets[i]);

                //added 5.14.2020
                //calculate the distance for each survey polygon
                //if (autoUnit === "Yes") {
                    if(surveytype == "BAEA"){
                        strBAEAsurveyPlotNm = validateDistBAEA(wavcoor);
                        returnval = strBAEAsurveyPlotNm;
                    }
                    else if(surveytype == "WBPHS"){
                        strWBPHSsurveyAGNm = validateDistWBPHS3(wavcoor);
                        strWBPHSsurveyVal = validateDistWBPHS2(wavcoor);
                        //console.log("strWBPHSsurveyAGNm: " + strWBPHSsurveyAGNm)
                        //console.log("strWBPHSsurveyVal: " + strWBPHSsurveyVal)

                        //do further validation for airground survey
                        var splt1 = String(strWBPHSsurveyAGNm).split(',')
                        var val1 = splt1[0]
                        var valag1 = splt1[1]
                        const dist1 = Math.round(splt1[2] * 1000) / 1000

                        var splt2 = String(strWBPHSsurveyVal).split(',')
                        var val2 = splt2[3]
                        const dist2 = Math.round(splt2[4] * 1000) / 1000

                        var warn = "no"
                        if(dist2 < dist1)
                            val1 = "0" //change the AGS to offag
                        else if(dist2 == dist1){
                            if(val1 == "0")
                                warn = "yes" //show warning that the ag survey threshold did'nt reach the observation pt
                        }
                        var newval = val1 + "," + valag1
                        //console.log("newval: " + newval)

                        returnval = newval + "," + strWBPHSsurveyVal + "," + warn;
                        //console.log("returnval: " + returnval);

                        createAgCircle(wavcoor,airgd)
                    }else if(surveytype == "GOEA"){
                        strGOEAsurveyBcrTrn = validateDistGOEA2(wavcoor);
                        returnval = strGOEAsurveyBcrTrn;
                    }
                    createWavCircle(wavcoor,maxd)//added 5.28.2020
                //}
                break;
            }
        }
        return returnval;
    }

    function createWavCircle(wavcoor,maxd){//added 5.28.2020
        //create circle around the wavFileSelected
        if(wavCircleSelected){
            mapview.removeMapItem(wavCircleSelected);
            mapview.update();
        }

        var m = Qt.createQmlObject('WavCircle{}',mapview);
        m.wavradius = maxd * 1000;
        m.wavcenter = wavcoor;
        m.wavbordercolor = "lightgreen"; //#E2E2E2
        m.wavopacity = 1;
        m.setValues();

        mapview.addMapItem(m);
        wavCircleSelected = m;
        mapview.update();
        //console.log(wavCircleSelected);

    }

    function createAgCircle(wavcoor,agdist){//added 5.28.2020
        //create circle around the wavFileSelected
        if(agCircleSelected){
            mapview.removeMapItem(agCircleSelected);
            mapview.update();
        }

        var m = Qt.createQmlObject('AgCircle{}',mapview);
        m.agradius = agdist * 1000;
        m.agcenter = wavcoor;
        m.agbordercolor = "orange";
        m.agopacity = 1;
        m.setValues();

        mapview.addMapItem(m);
        agCircleSelected = m;
        mapview.update();
    }

    function validateDistWBPHS2(wavcoor){
        var returnval = "0,0,0";
        var wavpt = []
        wavpt.push(wavcoor.latitude); //for latitude
        wavpt.push(wavcoor.longitude); //for longitude

        //1st validation get the closest survey line from wavpt
        var poly = [];
        poly = surveySegSet
        var distlist = []
        outerloop:
        for(var ii = 0; ii < poly.length; ii++){
            if(poly[ii].path.length > 0){
                for(var jj=0; jj < poly[ii].path.length; jj++){
                    var polydistlist = []
                    var polycoor1 = poly[ii].path[jj]
                    var dist = (wavcoor.distanceTo(polycoor1)) / 1000 //this returns meters need to convert to km

                    polydistlist.push(ii);
                    polydistlist.push(dist)
                    polydistlist.push(poly[ii].stratum)
                    polydistlist.push(poly[ii].transect)
                    polydistlist.push(poly[ii].segment)

                    distlist.push(polydistlist)
                }
            }
        }

        /*for(var i = 0; i < surveySegSet.length; i++){
            console.log(surveySegSet[i].id + ": " + surveySegSet[i])
        }*/


        //look for the line with closest distance
        var tempdist = 0;
        var closestidx = 0  //is the closest polygon to the wav pt
        var tempstratum = ""
        var temptransect = ""
        var tempsegment = ""
        var closestsegment = [];
        var sortedsegment = sortByCol(distlist,1);

        for(var i = 0; i < sortedsegment.length; i++){
            //console.log(sortedsegment[i])
            if(i == 0){
                closestsegment.push(sortedsegment[i]);
                closestidx = sortedsegment[i][0]
                tempdist = sortedsegment[i][1]
                tempstratum = sortedsegment[i][2]
                temptransect = sortedsegment[i][3]
                tempsegment = sortedsegment[i][4]
            }else{
                if(Number(tempdist) == Number(sortedsegment[i][1])){
                    closestsegment.push(sortedsegment[i]);
                }
            }
        }

        /*console.log(closestidx)
        console.log(tempdist)
        console.log(tempstratum)
        console.log(temptransect)
        console.log(tempsegment)
        for(var i = 0; i < closestsegment.length; i++){
            console.log(closestsegment[i])
        }*/

        var distexist = "0"
        if(Number(tempdist) <= Number(maxDistance)){
            returnval = tempstratum +
                "," + temptransect +
                "," + tempsegment;
            distexist = "1";
        }else{
            returnval = tempstratum +
                "," + temptransect +
                "," + tempsegment;
            distexist = "0";
        }
        //console.log("returnval: " + returnval)

        //look for the closest distance
        var ptx = wavpt[0];
        var pty = wavpt[1];
        var tdist = 0;
        var tdist2 = 0;
        var distexist2 = false;
        var tdist3 = 0;
        var actualdist = -1;
        outerloop:
        for(var ii = 0; ii < closestsegment.length; ii++){
            var idx = closestsegment[ii][0];
            if(poly[idx].path.length > 0){
                for(var jjjj=0; jjjj < poly[idx].path.length-1; jjjj++){
                    var ax = poly[idx].path[jjjj].latitude;
                    var ay = poly[idx].path[jjjj].longitude;
                    var bx = poly[idx].path[(jjjj+1)].latitude;
                    var by = poly[idx].path[(jjjj+1)].longitude;

                    //var disttest0 = sqrDistToSegment(ptx,pty,ax,ay,bx,by);//test 6.19.2020
                    //console.log("disttest0: " + disttest0);

                    /*var dist2 = Math.abs(calcPtLineSegDist3(,"K"));
                    console.log("dist2: 0" + dist2);
                    if(tdist == 0){
                        tdist = dist2;
                        returnval = poly[idx].stratum +
                            "," + poly[idx].transect +
                            "," + poly[idx].segment;
                    }else{
                        if(Number(dist2) < Number(tdist)){
                            returnval = poly[idx].stratum +
                                "," + poly[idx].transect +
                                "," + poly[idx].segment;
                            tdist = dist2;
                        }
                    }

                    if(Number(tdist) <= Number(maxDistance)){//added 6.4.2020
                        distexist = "1";
                        //break outerloop;
                    }
                    console.log("returnval1: " + returnval + " distexist: " + distexist);*/

                    //azimuth of wavpt to first pt of line
                    var wavazimpt1 = (QtPositioning.coordinate(ptx,pty)).azimuthTo(QtPositioning.coordinate(ax,ay));
                    //console.log("wavazimpt1: " + wavazimpt1)

                    //azimuth of wavpt to the second pt of baseline
                    var wavazimpt2 = (QtPositioning.coordinate(ptx,pty)).azimuthTo(QtPositioning.coordinate(bx,by));
                    //console.log("wavazimpt2: " + wavazimpt2)

                    var dist3 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(ax,ay))
                    if(Number(dist3/1000) <= Number(maxDistance)){
                        distexist = "1";
                        actualdist = dist3;
                    }else{
                        if(actualdist == -1)
                            actualdist = dist3;
                        else{
                            if(Number(dist3) < Number(actualdist))
                                actualdist = dist3;
                        }
                    }

                    var dist4 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(bx,by))
                    //console.log("dist4: 0" + dist4/1000);
                    if(Number(dist4) < Number(dist3)){
                        if(Number(dist4/1000) <= Number(maxDistance)){
                            distexist = "1";
                            if(Number(dist4) < Number(actualdist))
                                actualdist = dist4;
                        }
                    }

                    //validate if they come from the same quadrant
                    var pt1quad, pt2quad;
                    if(wavazimpt1 > 0 && wavazimpt1 < 90)
                        pt1quad = 1;
                    else if(wavazimpt1 >= 90 && wavazimpt1 < 180)
                         pt1quad = 2;
                    else if(wavazimpt1 >= 180 && wavazimpt1 < 270)
                         pt1quad = 3;
                    else if(wavazimpt1 >= 270 && wavazimpt1 < 360)
                         pt1quad = 4;

                    if(wavazimpt2 > 0 && wavazimpt2 < 90)
                        pt2quad = 1;
                    else if(wavazimpt2 >= 90 && wavazimpt2 < 180)
                         pt2quad = 2;
                    else if(wavazimpt2 >= 180 && wavazimpt2 < 270)
                         pt2quad = 3;
                    else if(wavazimpt2 >= 270 && wavazimpt2 <= 360)
                         pt2quad = 4;

                    if(pt1quad != pt2quad){//they belong to different quadrant
                        //this means that the wavpt is between the line segment

                        //get the linse segment angle
                        var linesegazimuth = poly[idx].path[jjjj].azimuthTo(poly[idx].path[(jjjj+1)]);
                        //deduct 90 degrees to get its mid angle
                        var newazimuth = linesegazimuth - 90;
                        var newcoor = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(maxDistance*1000,newazimuth)

                        //make a line with coordiantes of wavpt and newcoor
                        //and test if the newline crosses with line segment
                        var returnpt = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor.latitude,newcoor.longitude);
                        if(returnpt === false){
                        }else{
                            distexist = "1";

                            var disttest = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt.x,returnpt.y));
                            //console.log("disttest; " + disttest)

                            returnval = poly[idx].stratum +
                                "," + poly[idx].transect +
                                "," + poly[idx].segment;

                            actualdist = disttest;
                        }

                        //added 6.4.2020
                        if(returnpt === false){
                            var newazimuth0 = linesegazimuth + 90;
                            var newazimuth1 = newazimuth0;
                            if(newazimuth0 > 360)
                                newazimuth1 = Math.abs(newazimuth0 - 360);
                            var newcoor1 = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(maxDistance*1000,newazimuth1)
                            var returnpt1 = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor1.latitude,newcoor1.longitude);

                            //conclusion
                            /*
                              if distexist == 0 then the maxdistance is less than
                                the nearest survey line segment
                            */

                            if(returnpt1 === false){
                                //to test for the nearest segment_intersection
                                //and get the stratum transect and segment
                                //the test distance is 100km / 100,000m

                                var newazimuth2 = linesegazimuth + 90;
                                var newazimuth3 = newazimuth2;
                                if(newazimuth2 > 360)
                                    newazimuth3 = Math.abs(newazimuth2 - 360);
                                var newcoor2 = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(100000,newazimuth3)
                                var returnpt2 = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor2.latitude,newcoor2.longitude);
                                if(returnpt2 === false){
                                    var newazimuth4 = linesegazimuth - 90;
                                    var newazimuth5 = newazimuth4;
                                    if(newazimuth4 > 360)
                                        newazimuth5 = Math.abs(newazimuth4 - 360);
                                    var newcoor4 = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(100000,newazimuth5)
                                    var returnpt4 = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor4.latitude,newcoor4.longitude);
                                    if(returnpt4 === false){
                                    }else{
                                        var disttest4 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt4.x,returnpt4.y));
                                        //console.log("disttest4; " + disttest4);
                                        actualdist = disttest4;
                                        returnval = poly[idx].stratum +
                                            "," + poly[idx].transect +
                                            "," + poly[idx].segment;

                                        break outerloop;
                                    }
                                }else{
                                    var disttest2 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt2.x,returnpt2.y));
                                    //console.log("disttest2; " + disttest2);
                                    actualdist = disttest2;

                                    returnval = poly[idx].stratum +
                                        "," + poly[idx].transect +
                                        "," + poly[idx].segment;

                                    break outerloop;
                                }

                            }else{
                                distexist = "1";
                                var disttest1 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt1.x,returnpt1.y));
                                //console.log("disttest1; " + disttest1);
                                actualdist = disttest1;

                                returnval = poly[idx].stratum +
                                    "," + poly[idx].transect +
                                    "," + poly[idx].segment;

                                break outerloop;
                            }
                        }



                        //break outerloop;
                    }
                }
            }
        }


        return returnval + "," + distexist + "," + actualdist;
    }

    //not used
    function lineSegmentsIntersect(x1, y1, x2, y2, x3, y3, x4, y4){
        var a_dx = x2 - x1;
        var a_dy = y2 - y1;
        var b_dx = x4 - x3;
        var b_dy = y4 - y3;
        var s = (-a_dy * (x1 - x3) + a_dx * (y1 - y3)) / (-b_dx * a_dy + a_dx * b_dy);
        var t = (+b_dx * (y1 - y3) - b_dy * (x1 - x3)) / (-b_dx * a_dy + a_dx * b_dy);
        return (s >= 0 && s <= 1 && t >= 0 && t <= 1);
    }

    //inconclusive
    function intersects(a,b,c,d,p,q,r,s) {
      var det, gamma, lambda;
      det = (c - a) * (s - q) - (r - p) * (d - b);
      if (det === 0) {
        return false;
      } else {
        lambda = ((s - q) * (r - a) + (p - r) * (s - b)) / det;
        gamma = ((b - d) * (r - a) + (c - a) * (s - b)) / det;
        return (0 < lambda && lambda < 1) && (0 < gamma && gamma < 1);
      }
    }

    //var eps = 0.0000001;
    function between(a, b, c) {
        return a-0.0000001 <= b && b <= c+0.0000001;
    }

    //used
    function segment_intersection(x1,y1,x2,y2, x3,y3,x4,y4) {
        var x=((x1*y2-y1*x2)*(x3-x4)-(x1-x2)*(x3*y4-y3*x4)) /
                ((x1-x2)*(y3-y4)-(y1-y2)*(x3-x4));
        var y=((x1*y2-y1*x2)*(y3-y4)-(y1-y2)*(x3*y4-y3*x4)) /
                ((x1-x2)*(y3-y4)-(y1-y2)*(x3-x4));
        if (isNaN(x)||isNaN(y)) {
            return false;
        } else {
            if (x1>=x2) {
                if (!between(x2, x, x1)) {return false;}
            } else {
                if (!between(x1, x, x2)) {return false;}
            }
            if (y1>=y2) {
                if (!between(y2, y, y1)) {return false;}
            } else {
                if (!between(y1, y, y2)) {return false;}
            }
            if (x3>=x4) {
                if (!between(x4, x, x3)) {return false;}
            } else {
                if (!between(x3, x, x4)) {return false;}
            }
            if (y3>=y4) {
                if (!between(y4, y, y3)) {return false;}
            } else {
                if (!between(y3, y, y4)) {return false;}
            }
        }
        return {x: x, y: y};
    }

    //inconclusive
    function pDistance(x, y, x1, y1, x2, y2) {

      var A = x - x1;
      var B = y - y1;
      var C = x2 - x1;
      var D = y2 - y1;

      var dot = A * C + B * D;
      var len_sq = C * C + D * D;
      var param = -1;
      if (len_sq != 0) //in case of 0 length line
          param = dot / len_sq;

      var xx, yy;

      if (param < 0) {
        xx = x1;
        yy = y1;
      }
      else if (param > 1) {
        xx = x2;
        yy = y2;
      }
      else {
        xx = x1 + param * C;
        yy = y1 + param * D;
      }

      var dx = x - xx;
      var dy = y - yy;
      return Math.sqrt(dx * dx + dy * dy);
    }

    //inconclusive
    function findPerpendicularDistance(point, line) {
        var pointX = point[0],
            pointY = point[1],
            lineStart = {
                x: line[0][0],
                y: line[0][1]
            },
            lineEnd = {
                x: line[1][0],
                y: line[1][1]
            },
            slope = (lineEnd.y - lineStart.y) / (lineEnd.x - lineStart.x),
            intercept = lineStart.y - (slope * lineStart.x),
            result;

        /*console.log(pointX)
        console.log(pointY)
        console.log(lineStart.x)
        console.log(lineStart.y)
        console.log(lineEnd.x)
        console.log(lineEnd.y)
        console.log(slope)
        console.log(intercept)*/

        result = Math.abs(slope * pointX - pointY + intercept) / Math.sqrt(Math.pow(slope, 2) + 1);
        //console.log(result)
        return result;
    }

    //used
    function sortByCol(arr, colIndex){
        function sortFunction(a, b) {
            a = a[colIndex];
            b = b[colIndex];
           return isNaN(a-b) ? (a === b) ? 0 : (a < b) ? -1 : 1 : a-b  ;  // test if text string - ie cannot be coerced to numbers.
           // Note that sorting a column of mixed types will always give an entertaining result as the strict equality test will always return false
           // see https://developer.mozilla.org/en-US/docs/Web/JavaScript/Equality_comparisons_and_sameness

        }
        arr.sort(sortFunction);
        return arr;
    }

    //inconclusive
    function validateDistGOEA(wavcoor){
        var returnval = "";
        var wavpt = []
        wavpt.push(wavcoor.latitude); //for latitude
        wavpt.push(wavcoor.longitude); //for longitude

        //1st validation get the closest survey line from wavpt
        var poly = [];
        poly =  surveySetGOEA
        var distlist = []
        outerloop:
        for(var ii = 0; ii < poly.length; ii++){
            if(poly[ii].path.length > 0){
                for(var jj=0; jj < poly[ii].path.length; jj++){
                    var polydistlist = []
                    var polycoor1 = poly[ii].path[jj]
                    var dist = (wavcoor.distanceTo(polycoor1)) / 1000 //this returns meters need to convert to km

                    polydistlist.push(ii);
                    polydistlist.push(dist)
                    polydistlist.push(poly[ii].bcr)
                    polydistlist.push(poly[ii].trn)

                    distlist.push(polydistlist)
                }
            }
        }


        //look for the line with closest distance
        var tempdist = 0;
        var closestidx = 0  //is the closest polygon to the wav pt
        var tempbcr = ""
        var temptrn = ""

        loopmain:
        for(var iii = 0; iii < distlist.length; iii++){
            //console.log(distlist.length)
            if(iii === 0){
                closestidx = distlist[iii][0]
                tempdist = distlist[iii][1]
                tempbcr = distlist[iii][2]
                temptrn = distlist[iii][3]
            }
            if(Number(distlist[iii][1]) < Number(tempdist)){
                closestidx = distlist[iii][0]
                tempdist = distlist[iii][1]
                tempbcr = distlist[iii][2]
                temptrn = distlist[iii][3]
            }
        }

        //look for the distance
        var ptx = wavpt[0];
        var pty = wavpt[1];
        outerloop:
        if(poly[closestidx].path.length > 0){
            var tdist = 0;
            for(var jjjj=0; jjjj < poly[closestidx].path.length - 1; jjjj++){
                var ax = poly[closestidx].path[jjjj].latitude;
                var ay = poly[closestidx].path[jjjj].longitude;
                var bx = poly[closestidx].path[(jjjj+1)].latitude;
                var by = poly[closestidx].path[(jjjj+1)].longitude;

                //console.log(ax + "," + ay + ": " + bx + "," + by + ": " + ptx + "," + pty)
                var dist3 = Math.abs(calcPtLineSegDist2(ax,ay,bx,by,ptx,pty,"K"));
                //console.log(dist3)

                if(Number(dist3) <= Number(maxDistance)){
                    returnval = poly[closestidx].bcr + "," + poly[closestidx].trn + "," + "1"
                    break outerloop;
                }
            }
        }

        if(returnval === ""){
            returnval = tempbcr + "," + temptrn + "," + "0"
        }

        /*//for testing
        var ax = 52.205
        var ay = 0.119
        var ptx = 48.857
        var pty = 2.351
        var rad = 6371000

        var dist3 = ptDistToPt(ax, ay, ptx, pty, rad)
        console.log("dist3: " + dist3);

        var initbear = initialBearingTo(ax,ay,ptx,pty)
        console.log("initbear: " + initbear);

        ax = 53.3206
        ay = -1.7297
        var bx = 53.1887
        var by = 0.1334
        ptx = 53.2611
        pty = -0.7972
        rad = 6371000

        var dist4 = calcPtLineSegDist2(ax,ay,bx,by,ptx,pty,"K");
        console.log("dist4: " + dist4);*/

        return returnval
    }

    //used
    function validateDistGOEA2(wavcoor){
        var returnval = "0,0,0";
        var wavpt = []
        wavpt.push(wavcoor.latitude); //for latitude
        wavpt.push(wavcoor.longitude); //for longitude

        //1st validation get the closest survey line from wavpt
        var poly = [];
        poly = surveySetGOEA
        var distlist = []
        outerloop:
        for(var ii = 0; ii < poly.length; ii++){
            if(poly[ii].path.length > 0){
                for(var jj=0; jj < poly[ii].path.length; jj++){
                    var polydistlist = []
                    var polycoor1 = poly[ii].path[jj]
                    var dist = (wavcoor.distanceTo(polycoor1)) / 1000 //this returns meters need to convert to km

                    polydistlist.push(ii);
                    polydistlist.push(dist)
                    polydistlist.push(poly[ii].bcr)
                    polydistlist.push(poly[ii].trn)

                    distlist.push(polydistlist)
                }
            }
        }

        //look for the line with closest distance
        var tempdist = 0;
        var closestidx = 0  //is the closest polygon to the wav pt
        var tempbcr = ""
        var temptrn = ""
        var closestsegment = [];
        var sortedsegment = sortByCol(distlist,1);

        closestsegment.push(sortedsegment[0]);
        closestsegment.push(sortedsegment[1]);
        closestsegment.push(sortedsegment[2]);
        closestsegment.push(sortedsegment[3]);
        closestsegment.push(sortedsegment[4]);
        closestsegment.push(sortedsegment[5]);
        closestsegment.push(sortedsegment[6]);
        closestsegment.push(sortedsegment[7]);

        closestidx = sortedsegment[0][0]
        tempdist = sortedsegment[0][1]
        tempbcr = sortedsegment[0][2]
        temptrn = sortedsegment[0][3]

        var distexist = "0"
        if(Number(tempdist) <= Number(maxDistance)){
            returnval = tempbcr + "," + temptrn;
            distexist = "1";
        }else{
            returnval = tempbcr + "," + temptrn;
            distexist = "0";
        }

        //look for the closest distance
        var ptx = wavpt[0];
        var pty = wavpt[1];
        var tdist = -1;//maxDistance * 1000;
        var tdist2 = 0;
        var distexist2 = false;
        var tdist3 = 0;
        var actualdist = -1;
        outerloop:
        for(var ii = 0; ii < closestsegment.length; ii++){
            var idx = closestsegment[ii][0];
            if(poly[idx].path.length > 0){
                for(var jjjj=0; jjjj < poly[idx].path.length-1; jjjj++){
                    var ax = poly[idx].path[jjjj].latitude;
                    var ay = poly[idx].path[jjjj].longitude;
                    var bx = poly[idx].path[(jjjj+1)].latitude;
                    var by = poly[idx].path[(jjjj+1)].longitude;

                    var dist3 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(ax,ay))
                    if(actualdist === -1){
                        actualdist = dist3;
                        tdist = dist3/1000;
                        returnval = poly[idx].bcr +
                            "," + poly[idx].trn;
                    }

                    if(Number(dist3/1000) <= Number(maxDistance)){
                        tdist = dist3/1000;
                        distexist = "1";
                        returnval = poly[idx].bcr +
                            "," + poly[idx].trn;

                        if(Number(dist3) < Number(actualdist))
                            actualdist = dist3;
                    }else{
                        if(Number(dist3) < Number(actualdist))
                            actualdist = dist3;
                    }

                    var dist4 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(bx,by))
                    if(Number(dist4) < Number(dist3)){
                        if(Number(dist4/1000) <= Number(maxDistance)){
                            tdist = dist4/1000;
                            distexist = "1";
                            returnval = poly[idx].bcr +
                                "," + poly[idx].trn;

                            if(Number(dist4) < Number(actualdist))
                                actualdist = dist4;

                        }else{
                            if(Number(dist4) < Number(actualdist)){
                                tdist = dist4/1000;
                                actualdist = dist4;
                                returnval = poly[idx].bcr +
                                    "," + poly[idx].trn;
                            }
                        }
                    }else{
                        if(Number(dist4) < Number(actualdist))
                            actualdist = dist4;
                    }

                    //azimuth of wavpt to first pt of line
                    var wavazimpt1 = (QtPositioning.coordinate(ptx,pty)).azimuthTo(QtPositioning.coordinate(ax,ay));

                    //azimuth of wavpt to the second pt of baseline
                    var wavazimpt2 = (QtPositioning.coordinate(ptx,pty)).azimuthTo(QtPositioning.coordinate(bx,by));


                    //validate if they come from the same quadrant
                    var pt1quad, pt2quad;
                    if(wavazimpt1 > 0 && wavazimpt1 < 90)
                        pt1quad = 1;
                    else if(wavazimpt1 >= 90 && wavazimpt1 < 180)
                         pt1quad = 2;
                    else if(wavazimpt1 >= 180 && wavazimpt1 < 270)
                         pt1quad = 3;
                    else if(wavazimpt1 >= 270 && wavazimpt1 < 360)
                         pt1quad = 4;

                    if(wavazimpt2 > 0 && wavazimpt2 < 90)
                        pt2quad = 1;
                    else if(wavazimpt2 >= 90 && wavazimpt2 < 180)
                         pt2quad = 2;
                    else if(wavazimpt2 >= 180 && wavazimpt2 < 270)
                         pt2quad = 3;
                    else if(wavazimpt2 >= 270 && wavazimpt2 < 360)
                         pt2quad = 4;

                    if(pt1quad != pt2quad){//they belong to different quadrant
                        //this means that the wavpt is between the line segment

                        //get the linse segment angle
                        var linesegazimuth = poly[idx].path[jjjj].azimuthTo(poly[idx].path[(jjjj+1)]);
                        //deduct 90 degrees to get its mid angle
                        var newazimuth = linesegazimuth - 90;
                        var newcoor = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(maxDistance*1000,newazimuth)

                        //make a line with coordiantes of wavpt and newcoor
                        //and test if the newline crosses with line segment
                        var returnpt = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor.latitude,newcoor.longitude);
                        //distance of wavpt to returnpt

                        if(returnpt === false){
                        }else{
                            var newdist0 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt.x,returnpt.y));
                            if(Number(newdist0/1000) < Number(tdist)){
                                tdist = newdist0/1000;
                                distexist = "1";//less than maximum distance
                                returnval = poly[idx].bcr +
                                    "," + poly[idx].trn;

                                actualdist = newdist0;
                           }
                        }

                        //added 6.5.2020
                        if(returnpt === false){
                            var newazimuth0 = linesegazimuth + 90;
                            var newazimuth1 = newazimuth0;
                            if(newazimuth0 > 360)
                                newazimuth1 = Math.abs(newazimuth0 - 360);
                            var newcoor1 = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(maxDistance*1000,newazimuth1)
                            var returnpt1 = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor1.latitude,newcoor1.longitude);

                            if(returnpt1 === false){
                                //to test for the nearest segment_intersection
                                //and get the stratum transect and segment
                                //the test distance is 100km / 100,000m

                                var newazimuth2 = linesegazimuth + 90;
                                var newazimuth3 = newazimuth2;
                                if(newazimuth2 > 360)
                                    newazimuth3 = Math.abs(newazimuth2 - 360);
                                var newcoor2 = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(100000,newazimuth3)
                                var returnpt2 = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor2.latitude,newcoor2.longitude);
                                if(returnpt2 === false){
                                    var newazimuth4 = linesegazimuth - 90;
                                    var newazimuth5 = newazimuth4;
                                    if(newazimuth4 > 360)
                                        newazimuth5 = Math.abs(newazimuth4 - 360);
                                    var newcoor4 = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(100000,newazimuth5)
                                    var returnpt4 = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor4.latitude,newcoor4.longitude);
                                    if(returnpt4 === false){
                                    }else{
                                        var newdist4 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt4.x,returnpt4.y));
                                        if(Number(newdist4/1000) < Number(tdist)){
                                            tdist = newdist4/1000;
                                            returnval = poly[idx].bcr +
                                                "," + poly[idx].trn;

                                            actualdist = newdist4;
                                        }
                                    }
                                }else{
                                    var newdist2 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt2.x,returnpt2.y));
                                    if(Number(newdist2/1000) < Number(tdist)){
                                        tdist = newdist2/1000;
                                        returnval = poly[idx].bcr +
                                            "," + poly[idx].trn;

                                        actualdist = newdist2;
                                    }
                                }

                            }else{
                                //distance of wavpt to returnpt
                                var newdist1 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt1.x,returnpt1.y));
                                if(Number(newdist1/1000) < Number(tdist)){
                                    tdist = newdist1/1000;
                                    distexist = "1";
                                    returnval = poly[idx].bcr +
                                        "," + poly[idx].trn;

                                    actualdist = newdist1;
                                }
                            }
                        }
                    }
                }
            }
        }

        return returnval + "," + distexist + "," + actualdist;
    }

    function validateDistBAEA(wavcoor){
        var plotnm = "";
        var wavpt = []
        wavpt.push(wavcoor.latitude); //for latitude
        wavpt.push(wavcoor.longitude); //for longitude

        //1st validation wavpt within the polygon
        var poly = [];
        poly =  surveyPolygonSet

        //console.log(surveyPolygonSet[0].path);
        //console.log("A");
        mainloop0:
        for(var i = 0; i < poly.length; i++){
            var surveypolycoor = [];
            var isExisting = false;
            if(poly[i].path.length > 0){
                for(var j=0; j < poly[i].path.length; j++){
                    var polycoor = [];
                    var polyx = poly[i].path[j].latitude
                    var polyy = poly[i].path[j].longitude
                    polycoor.push(polyx,polyy);
                    surveypolycoor.push(polycoor);
                }
                isExisting = isPtInside(wavpt,surveypolycoor);
                if(isExisting){
                    plotnm = poly[i].plotlabel
                    break mainloop0;
                }
            }
        }

        var closestidx = 0  //is the closest polygon to the wav pt
        var tempplotnm = ""
        var distlist = []
        //2nd validation distance of wavpt and polygon vertex
        if(plotnm === ""){
            //console.log("B");
            outerloop:
            for(var ii = 0; ii < poly.length; ii++){
                if(poly[ii].path.length > 0){
                    for(var jj=0; jj < poly[ii].path.length; jj++){
                        var polydistlist = []
                        //var tempdist = 0
                        var polycoor1 = poly[ii].path[jj]
                        var dist = (wavcoor.distanceTo(polycoor1)) / 1000 //this returns meters need to convert to km

                        polydistlist.push(ii);
                        polydistlist.push(dist)
                        polydistlist.push(poly[ii].plotlabel)

                        distlist.push(polydistlist)

                        if(Number(dist) <= Number(maxDistance)){
                            plotnm = poly[ii].plotlabel
                            break outerloop;
                        }
                    }
                }
            }
        }

        //look for the polygon with closest distance
        var tempdist = 0;
        loopmain:
        for(var iii = 0; iii < distlist.length; iii++){
            //console.log(distlist.length)
            if(iii === 0){
                closestidx = distlist[iii][0]
                tempdist = distlist[iii][1]
                tempplotnm = distlist[iii][2]
                //break loopmain;
            }
            if(Number(distlist[iii][1]) < Number(tempdist)){
                closestidx = distlist[iii][0]
                tempplotnm = distlist[iii][2]
                tempdist = distlist[iii][1]
            }
        }

//        console.log("closestidx: " + closestidx)
//        console.log("tempplotnm: " + tempplotnm)
//        console.log("tempdist: " + tempdist)

        //3rd validation get new pt coordinate based on the angle of survey centroid
        // and distance of the selected wavfile
        if(plotnm === ""){
            //console.log("C");
            //centercoor = get polygon centroid(center of mass)
            var centroid = polyCentroid2(poly,closestidx);
            //console.log("centroidlat: " + centroid.latitude + " centroidlon: " + centroid.longitude);

            //angle = wavfile azimuteto (centercoor)centroid
            var centroidazimuth = wavcoor.azimuthTo(centroid);
            //console.log("centroidazimuth: " + centroidazimuth);

            //coor = wavfile pt atdistanceandazimute((maxdistance)distance,(angle)azimute)
            var wavdistazimcoor = wavcoor.atDistanceAndAzimuth(Number(maxDistance) * 1000,centroidazimuth)
            //console.log("wavdistazimcoorlatitude: " + wavdistazimcoor.latitude + " wavdistazimcoorlongitude: " + wavdistazimcoor.longitude);

            //check if coor is inside the polygon
            if(poly[closestidx].path.length > 0){
                var surveypolycoor2 = [];
                for(var f=0; f < poly[closestidx].path.length; f++){
                    var polycoor2 = [];
                    var polyx2 = poly[closestidx].path[f].latitude
                    var polyy2 = poly[closestidx].path[f].longitude
                    polycoor2.push(polyx2,polyy2);
                    surveypolycoor2.push(polycoor2);
                }
                var wavdistazimcoorarray = [];//need to convert coordinate to array list
                wavdistazimcoorarray.push(wavdistazimcoor.latitude);
                wavdistazimcoorarray.push(wavdistazimcoor.longitude);

                var isExisting2 = isPtInside(wavdistazimcoorarray,surveypolycoor2);
                //console.log(isExisting2)
                if(isExisting2){
                    plotnm = poly[closestidx].plotlabel
                }
            }
        }

        var returnval;
        //console.log("Qml plotnm; "+ plotnm)
        if(plotnm === "")
            returnval = tempplotnm + ",0";//0 not within the maxdistance
        else
            returnval = plotnm + ",1";//1 within the maxdistance

        return returnval;
    }

    function validateDistWBPHS(wavcoor) {
        var agnm = "";
        var returnval = "";
        var wavpt = []
        wavpt.push(wavcoor.latitude); //for latitude
        wavpt.push(wavcoor.longitude); //for longitude

        //Within line segment
        //console.log("Within " + checkPtExist(wavcoor, "AIRGROUND"));
        if(checkPtExist(wavcoor, "AIRGROUND").length > 0){
            returnval =  checkPtExist(wavcoor, "AIRGROUND");
            return returnval;
        }

        var d = [];
        //Without line segment
        var air = airGroundSegSet;
        for(var x=0; x < air.length; x++) {
            var ax, ay, bx, by;

            if(air[x].path.length > 0){
                ax = air[x].path[0].latitude
                ay = air[x].path[0].longitude
                bx = air[x].path[air[x].path.length - 1].latitude
                by = air[x].path[air[x].path.length - 1].longitude

                if (x === 0) {
                    d.push(getDistance(ax, ay, wavcoor));
                    d.push(x);
                }
                else {
                    //Get Lowest Distance for first line
                    if (d[0] > getDistance(ax, ay, wavcoor)) {
                        d = [];
                        d.push(getDistance(ax, ay, wavcoor));
                        d.push(x);
                    }
                }

                //Get Lowest Distance for Last line
                if (d[0] > getDistance(bx, by, wavcoor)) {
                    d = [];
                    d.push(getDistance(bx, by, wavcoor));
                    d.push(x);
                }

            }
        }

        //console.log("Without :" + air[d[1]].agcode);
        //console.log("Current Coor: " + d);
        //Check maxDistance
        if (d[0] >= airgroundMaxDistance) {
            returnval = "1";
        }
        else {

            returnval = "0";
        }

        returnval += "," + air[d[1]].agcode;
        //console.log(returnval);
        return returnval;
    }

    function getDistance(ax, ay, wavcoor) {
        var a = Math.abs(ax - wavcoor.latitude);
        var b = Math.abs(ay - wavcoor.longitude);
        //var a = ax - wavcoor.latitude;
        //var b = ay - wavcoor.longitude;

        var c = Math.sqrt( a*a + b*b );

        return c;
    }

    function polyArea(poly,ii) {
        //var surveyPolyArea; // = [];
        //for(var ii = 0; ii < poly.length; ii++){
            if(poly[ii].path.length > 0){
                var polyarealist = []
                var area = 0,
                    i,
                    j,
                    point1,point2;
                for (i = 0, j = poly[ii].path.length - 1; i < poly[ii].path.length; j = i, i += 1) {
                    point1 = poly[ii].path[i];
                    point2 = poly[ii].path[j];
                    area += point1.latitude * point2.longitude;
                    area -= point1.longitude * point2.latitude;
                }
                area /= 2;



                //polyarealist.push(poly[ii].plotlabel);
                //polyarealist.push(area);
                //surveyPolyArea.push(polyarealist);
            }
        //}
        return area;
    }

    function polyCentroid2(poly,ii) {
        var coor;
        if(poly[ii].path.length > 0){
            var lats = [],lons = [];
            for (var i = 0; i < poly[ii].path.length; i++) {
                var point1 = poly[ii].path[i];
                lats.push(point1.latitude);
                lons.push(point1.longitude);
            }
            lats.sort();
            lons.sort();

            const lowX = lats[0];
            const highX = lats[lats.length - 1];
            const lowy = lons[0];
            const highy = lons[lons.length - 1];

            const centerX = lowX + ((highX - lowX) / 2);
            const centerY = lowy + ((highy - lowy) / 2);

            coor = QtPositioning.coordinate(centerX, centerY);
        }
        return coor;
    }


    function polyCentroid(poly,ii,area) {
        var coor;
        if(poly[ii].path.length > 0){
            var x = 0,
                y = 0,
                i,
                j,
                f,
                point1,
                point2;

            for (i = 0, j = poly[ii].path.length - 1; i < poly[ii].path.length; j = i, i += 1) {
                point1 = poly[ii].path[i];
                point2 = poly[ii].path[j];

                f = point1.latitude * point2.longitude - point2.latitude * point1.longitude;
                x += (point1.latitude + point2.latitude) * f;
                y += (point1.longitude + point2.longitude) * f;
            }

            f = area * 6;

            coor = QtPositioning.coordinate(parseFloat(x / f),parseFloat(y / f))
        }

        return coor;
    }

    //cross track distance
    function calcPtLineSegDist2(ax,ay,bx,by,ptx,pty,strunit){
        //var dist = 0
        var rad = 6371; //radius
        if(strunit === "M"){//for meter
            rad = 3959;
        }else if(strunit === "K"){ //for kilometer
            rad = 6371;
        }

        const adist = (ptDistToPt(ax,ay,ptx,pty,rad)) / rad;
        const abearing = (initialBearingTo(ax,ay,ptx,pty)) * (Math.PI / 180);
        const bbearing = (initialBearingTo(ax,ay,bx,by)) * (Math.PI / 180);

        const xxt = Math.asin(Math.sin(adist) * Math.sin(abearing - bbearing));
        return xxt * rad;
    }

    //cross track distance3
    function calcPtLineSegDist3(ax,ay,bx,by,ptx,pty,strunit){
        var dist = 0;
        var rad = 3959; //radius
        if(strunit === "M"){//for meter
            rad = 3959;
        }else if(strunit === "K"){ //for kilometer
            rad = 6371;
        }

        var a = QtPositioning.coordinate(ax,ay);
        var b = QtPositioning.coordinate(bx,by);
        var pt = QtPositioning.coordinate(ptx,pty);

        var adistpt = a.distanceTo(pt) / rad;
        var aazimpt = a.azimuthTo(pt) * (Math.PI / 180);
        var aazimb = a.azimuthTo(b) * (Math.PI / 180);

        dist = Math.asin(Math.sin(adistpt) * Math.sin(aazimpt - aazimb)) * rad;
        return dist
    }

    function initialBearingTo(ax,ay,x,y){
        const φ1 = ax * (Math.PI / 180); //this.lat.toRadians();
        const φ2 = x * (Math.PI / 180); //point.lat.toRadians();
        const Δλ = (y - ay) * (Math.PI / 180); //(point.lon - this.lon).toRadians();

        const x1 = Math.cos(φ1) * Math.sin(φ2) - Math.sin(φ1) * Math.cos(φ2) * Math.cos(Δλ);
        const y1 = Math.sin(Δλ) * Math.cos(φ2);
        const θ = Math.atan2(y1, x1);

        const bearing = θ * (180 / Math.PI);
        return wrap360(bearing);
        //return bearing;
    }

    function wrap360(degrees) {
        if (0<=degrees && degrees<360)
            return degrees; // avoid rounding due to arithmetic ops if within range
        return (degrees%360+360) % 360; // sawtooth wave p:360, a:360
    }


    //cross track distance
    function calcPtLineSegDist(ax,ay,bx,by,ptx,pty,strunit){
        var dist = 0;
        var rad = 3959; //radius
        if(strunit === "M"){//for meter
            rad = 3959;
        }else if(strunit === "K"){ //for kilometer
            rad = 6371;
        }

        var a = QtPositioning.coordinate(ax,ay);
        var b = QtPositioning.coordinate(bx,by);
        var pt = QtPositioning.coordinate(ptx,pty);

        var adistpt = a.distanceTo(pt);
        var aazimpt = a.azimuthTo(pt);
        var aazimb = a.azimuthTo(b);

        dist = Math.asin(Math.sin(adistpt) * Math.sin(aazimpt - aazimb)) * rad;
        return dist;
    }

    function checkPtDistancePoly(pt,dist){
        if(surveytype == "BAEA"){
            var poly = [];
            poly =  surveyPolygonSet
            var plotnm = "";
            var kmdist = 0;
            var ptx = pt[0];
            var pty = pt[1];

            for(var i = 0; i < poly.length; i++){
                if(poly[i].path.length > 0){
                    var polyx = poly[i].path[0].latitude;
                    var polyy = poly[i].path[0].longitude;

                    /*console.log(ptx)
                    console.log(pty)
                    console.log(polyx)
                    console.log(polyy)*/
                    kmdist = distance(ptx, pty, polyx, polyy, "K");//in kilometer 'N' for nautical miles 'M' for miles

                    /*console.log(kmdist)
                    console.log(dist)*/
                    if( parseFloat(kmdist) <= parseFloat(dist)){//if maxDistance(dist) is within distance{
                        plotnm = poly[i].plotlabel;
                        break;
                    }

                }
            }

            return plotnm;
        }
    }

    function setCenterPosition(curWavfilenm) {
        var wavcoor = QtPositioning.coordinate(wavFileSets[curWavfilenm].latitude,
                                               wavFileSets[curWavfilenm].longitude);
        mapview.center = wavcoor;
        if(curWavfilenm === 0)
            mapview.zoomLevel = 11;
        mapview.update();
    }

    function changeMarkerToLockGPS(curWavfilenm, locked, selectWavfilem) {
        //Select New Locked
        for(var x = 0; x < wavFileSets.length; x++){
            //Lock selected wavfile
            if (locked) {
                //Set to red locked wavfile
                if(wavFileSets[x].descriptor === curWavfilenm){
                    wavFileSets[x].setProperties(false, "qrc:/img/map/pin_locked.png", "red", "black", 10, "white");
                    wavFileSets[x].imghover = "qrc:/img/map/pin_locked.png";
                    wavFileSets[x].update();
                    //mapview.update();
                }
                else if(wavFileSets[x].descriptor === selectWavfilem) {
                    //Center the Screen
                    var wavcoor = QtPositioning.coordinate(wavFileSets[x].latitude,wavFileSets[x].longitude);
                    var newcentercoor = wavcoor.atDistanceAndAzimuth(10,0);
                    mapview.center = newcentercoor;
                    mapview.update();

                    wavFileSets[x].setProperties(true, "qrc:/img/map/pin_selected.png",
                                                 "", "black", 10, "white");
                    wavFileSets[x].imghover = "qrc:/img/map/pin_selected.png";
                    wavFileSets[x].update();

                }
                else {
                    //Set to gray all unselected/locked wavfile
                    wavFileSets[x].setProperties(false, "qrc:/img/map/pin_unselected.png",
                                                 "", "black", 10, "white");
                    wavFileSets[x].imghover = "qrc:/img/map/pin_selected.png";
                    wavFileSets[x].update();
                }

            }
            else {
                //Unselected all wavfile
                wavFileSets[x].setProperties(false, "qrc:/img/map/pin_unselected.png",
                                             "", "black", 10, "white");
                wavFileSets[x].imghover = "qrc:/img/map/pin_selected.png";
                wavFileSets[x].update();
            }
         }
    }

    function removeLockGPS(wavfilenm) {
        //console.log(wavfilenm);
        for(var i = 0; i < wavFileSets.length; i++){
            if(wavFileSets[i].descriptor === wavfilenm){
                //mapview.center = QtPositioning.coordinate(wavFileSets[i].latitude,wavFileSets[i].longitude);

                //if(i === 0)
                    //mapview.zoomLevel = 11;

                //wavFileSelected = wavFileSets[i];

                wavFileSets[i].setProperties(false, "qrc:/img/map/pin_unselected.png", "yellow", "black", 10, "white");

                //wavFileSets[i].setProperties(false);

                wavFileSets[i].update();
                break;
            }
        }
    }

    function plotSurveyJson(surveylist){
        //for every coordinate in our string of data, split it up into a readable coordinate
        for(var i = 0; i < surveylist.length; i++){
            //creating a new "SurveyLine" object, and adding it to our sets list
            var m = Qt.createQmlObject('SurveyLine{}',mapview)
            //m.mapView = mapview

            var splt = String(surveylist[i]).split(',')
            if (splt.length > 4){
                var ii = 4;
                do{
                    var coor = QtPositioning.coordinate(parseFloat(splt[ii + 2]),parseFloat(splt[ii + 1]))
                    m.addCoordinate(coor);
                    ii += 2;
                }while(ii <= splt.length);

                m.id = splt[0]
                m.stratum = splt[2]
                m.transect= splt[3]
                m.segment = splt[1]
                m.version = splt[4]

                var lbl = "id: " + splt[0] +
                          "\nstratum: " + splt[2] +
                          "\ntransect: " + splt[3] +
                          "\nsegment: " + splt[1] +
                          "\nversion: " + splt[4];

                m.txtlabel = lbl
                m.labelcolor = "black"
                m.fontsize = 10

                mapview.addMapItem(m)
                surveySegSet.push(m)

            }
            //console.log(i);
        }
        withSurveyData = 1
    }

    function plotSurveyJson2(surveylist){
        for(var i = 0; i < surveylist.length; i++){
            var m = Qt.createQmlObject('SurveyLineSegment{}',mapview)
            var splt = String(surveylist[i]).split(',')
            if (splt.length > 4){
                var ii = 4;
                do{
                    var coor = QtPositioning.coordinate(parseFloat(splt[ii + 2]),parseFloat(splt[ii + 1]))
                    m.populateCoor(coor);

                    if(ii === 4){
                        m.plotPt(coor);
                    }

                    if((ii+2) === splt.length){
                        m.plotPt(coor);
                    }

                    ii += 2;
                }while(ii <= splt.length);

                m.stratum = splt[2]
                m.transect= splt[3]
                m.segment = splt[1]

                m.populaterectangle();

                mapview.addMapItemGroup(m)
                surveySegSet2.push(m)

            }
        }
        withSurveyData = 1
    }

    function plotSurveyJsonGOEA(surveylist){
        //for every coordinate in our string of data, split it up into a readable coordinate
        for(var i = 0; i < surveylist.length; i++){
            //creating a new "SurveyLine" object, and adding it to our sets list
            var m = Qt.createQmlObject('SurveyLineGOEA{}',mapview)
            //m.mapView = mapview
            //BCR, BCRTrn, BegLat, BegLng, EndLat, EndLng, FID, ID, KM, TRN
            var splt = String(surveylist[i]).split(',')
            if (splt.length > 8){ //update 7.21.2020 (> 2)
                var coorbeg = QtPositioning.coordinate(parseFloat(splt[2]),parseFloat(splt[3]))
                m.addCoordinate(coorbeg);
                var coorend = QtPositioning.coordinate(parseFloat(splt[4]),parseFloat(splt[5]))
                m.addCoordinate(coorend);

                m.bcr = splt[0]
                m.bcrtrn = splt[1]
                m.beglat= splt[2]
                m.beglng = splt[3]
                m.endlat = splt[4]
                m.endlng = splt[5]
                m.fid = splt[6]
                m.id = splt[7]
                m.km = splt[8]
                m.trn = splt[9]

                var lbl = "\BCR: " + splt[0] +
                          "\nTransect: " + splt[9] +
                          "\nKM: " + splt[8] +
                          "\nBeg Lat: " + splt[2] +
                          "\nBeg Lng: " + splt[3] +
                          "\nEnd Lat: " + splt[4] +
                          "\nEnd Lng: " + splt[5];

                if(splt[9].indexOf("R") != -1){
                    m.setLineColor("purple")
                }else{
                    m.setLineColor("red")
                }

                mapview.addMapItem(m)
                surveySetGOEA.push(m)

            }else if (splt.length == 6){ //added 7.21.2020
                var coorbeg1 = QtPositioning.coordinate(parseFloat(splt[3]),parseFloat(splt[2]))
                m.addCoordinate(coorbeg1);
                var coorend1 = QtPositioning.coordinate(parseFloat(splt[5]),parseFloat(splt[4]))
                m.addCoordinate(coorend1);

                if(splt[0].indexOf(".") !== -1){
                    var idx = splt[0].indexOf(".")
                    m.bcr = splt[0].substr(0,idx)
                    m.bcrtrn = splt[0]
                    m.beglat= splt[3]
                    m.beglng = splt[2]
                    m.endlat = splt[5]
                    m.endlng = splt[4]
                    m.fid = ""
                    m.id = ""
                    m.km = ""
                    m.trn = splt[0].substr(idx + 1, splt[0].length - idx)

                    var lbl1 = "\BCR: " + splt[0].substr(0,idx) +
                              "\nTransect: " + splt[0].substr(idx + 1, splt[0].length - idx) +
                              "\nKM: " + "" +
                              "\nBeg Lat: " + splt[3] +
                              "\nBeg Lng: " + splt[2] +
                              "\nEnd Lat: " + splt[5] +
                              "\nEnd Lng: " + splt[4];

                    if(splt[0].indexOf("R") != -1){
                        m.setLineColor("purple")
                    }else{
                        m.setLineColor("red")
                    }
                }

                mapview.addMapItem(m)
                surveySetGOEA.push(m)

            }
            //console.log(i);
        }
        withSurveyData = 1
    }

    function plotAirGroundJson(segmentlist){
        //for every coordinate in our string of data, split it up into a readable coordinate
        for(var i = 0; i < segmentlist.length; i++){
            //creating a new "AirGroundLine" object, and adding it to our sets list
            var m = Qt.createQmlObject('AirGroundLine{}',mapview)
            //m.mapView = mapview

            var splt = String(segmentlist[i]).split(',')
            if (splt.length > 0){
                var ii = 0;
                do{
                    var coor = QtPositioning.coordinate(parseFloat(splt[ii + 2]),parseFloat(splt[ii + 1]))
                    m.addCoordinate(coor);
                    ii += 2;
                }while(ii <= splt.length);

                var lbl = "agcode: " + splt[0]
                //m.txtlabel = lbl
                //m.labelcolor = "black"
                //m.fontsize = 10
                m.agcode = splt[0]

                mapview.addMapItem(m)
                airGroundSegSet.push(m)

            }
            //console.log(i);
        }
        withAirGroundSurvey = 1
    }

    function checkPtWithin(pt,strpoly){
        var isexist = false
        var strval = ""
        var poly = [];
        poly =  surveyPolygonSet

        var ptclick = [];
        ptclick.push(pt.latitude);
        ptclick.push(pt.longitude);

        var survpoly = [];
        /*for(var i = 0; i < poly.length; i++){
            var polypt = [];
            if(poly[i].path.length > 0){
                var polyx = poly[i].path[0].latitude
                var polyy = poly[i].path[0].longitude
                polypt.push(polyx);
                polypt.push(polyy);
            }
            survpoly.push(polypt);

            isexist = isPtInside(ptclick,survpoly);

            console.log(isexist);

        }*/
        var plotnm = "";
        for(var i = 0; i < poly.length; i++){
            var surveypolycoor = [];
            var isExisting = false;
            if(poly[i].path.length > 0){
                for(var j=0; j < poly[i].path.length; j++){
                    var polycoor = [];
                    var polyx = poly[i].path[j].latitude;
                    var polyy = poly[i].path[j].longitude;
                    polycoor.push(polyx,polyy);
                    surveypolycoor.push(polycoor);
                }
                isExisting = isPtInside(ptclick,surveypolycoor);
                if(isExisting){
                    plotnm = poly[i].plotlabel;
                    break;
                }
            }
        }
        return plotnm;
    }

    function checkPtExist(pt,strseg){
        var isexisting = false
        var strval = "";

        var seg = [];
        if(strseg.toUpperCase() === "SURVEY"){
            seg = surveySegSet
        }else if(strseg.toUpperCase() === "AIRGROUND"){
            seg = airGroundSegSet
        }else if(strseg.toUpperCase() === "GOEA"){
            seg = surveySetGOEA
        }

        for(var i = 0; i < seg.length; i++){
            //console.log(seg[i].path)
            //console.log(seg[i].path.length)
            var ax, ay, bx, by

            if(seg[i].path.length > 0){
                ax = seg[i].path[0].latitude
                ay = seg[i].path[0].longitude
                bx = seg[i].path[seg[i].path.length - 1].latitude
                by = seg[i].path[seg[i].path.length - 1].longitude
                /*console.log(ax)
                console.log(ay)
                console.log(bx)
                console.log(by)*/
                isexisting = containsPt(ax, ay, bx, by,pt.latitude,pt.longitude,0.009)//.009 tolerance
                //console.log(isexisting);
                if(isexisting){
                    //console.log(seg[i].stratum + ": " + seg[i].transect + ": " + seg[i].segment)

                    if(strseg.toUpperCase() === "SURVEY"){
                        strval = seg[i].stratum + "," + seg[i].transect + "," + seg[i].segment;
                    }else if(strseg.toUpperCase() === "AIRGROUND"){
                        //strval.push(seg[i].agcode);

                        //Check if Max distance
                        var dist1 = (pt.distanceTo(seg[i].path[0])) / 1000;
                        var dist2 = (pt.distanceTo(seg[i].path[seg[i].path.length - 1])) / 1000;

                        if (dist1 <= dist2) {
                            if (dist1 >= maxDistance) {
                                strval = "1"
                            }
                            else {
                                strval = "0"
                            }
                        }
                        else {
                            if (dist2 >= maxDistance) {
                                strval = "1"
                            }
                            else {
                                strval = "0";
                            }
                        }

                        //strval += "," + seg[i].agcode;//updated 7.21.2020
                        //the label of line segment when click on map
                        strval = seg[i].agcode;

                    }else if(strseg.toUpperCase() === "GOEA"){
                        strval = seg[i].bcr + "," + seg[i].trn
					}

                    break
                }
            }
        }
        return strval
    }

    /**
     ax - latitude startpt
     ay - longitude startpt
     bx - latitude endpt
     by - longitude endpt
     cx - latitude clickpt
     cy - longitude clickpt
     tolerance - distance (0.009)
    */
    function containsPt(ax, ay, bx, by, cx, cy, tolerance) {//pt in line segment
        //test if the point c is inside a pre-defined distance (tolerance) from the line
        var distance = Math.abs((cy - by) * ax - (cx - bx) * ay + cx * by - cy * bx) /
                Math.sqrt(Math.pow((cy-by),2) + Math.pow((cx-bx),2));

        //console.log(0.05160686365277151 > 0);
        if (distance > tolerance) { return false; }

        //test if the point c is between a and b
        var dotproduct = (cx - ax) * (bx - ax) + (cy - ay) * (by - ay);
        if (dotproduct < 0) { return false; }

        //console.log("dotproduct :" + dotproduct);
        var squaredlengthba = (bx - ax) * (bx - ax) + (by - ay) * (by - ay);
        //console.log(squaredlengthba);
        if (dotproduct > squaredlengthba) { return false; }

         //console.log("squaredlengthba: " + squaredlengthba);
        return true;
    }

    function plotSurveyPolygonJson(polygonlist){//added 5.14.2020
        for(var i = 0; i < polygonlist.length; i++){
            //creating a new "polygon" object, and adding it to our sets list
            var m = Qt.createQmlObject('SurveyPolygon{}',mapview)
            m.mapView = mapview

            var splt = String(polygonlist[i]).split(',')
            if (splt.length > 0){
                var ii = 0;
                do{
                    var coor = QtPositioning.coordinate(parseFloat(splt[ii + 2]),parseFloat(splt[ii + 1]))
                    m.addCoordinate(coor);
                    ii += 2;
                }while(ii <= splt.length);

                var lbl = "plotlabel: " + splt[0]
                //m.txtlabel = lbl
                //m.labelcolor = "black"
                //m.fontsize = 10
                m.plotlabel = splt[0]

                mapview.addMapItem(m)
                surveyPolygonSet.push(m)

            }
        }
        withSurveyData = 1
    }

    function isPtInside(point, vs) {
        // ray-casting algorithm based on
        // http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html

        var x = point[0], y = point[1];

        var inside = false;
        for (var i = 0, j = vs.length - 1; i < vs.length; j = i++) {
            var xi = vs[i][0], yi = vs[i][1];
            var xj = vs[j][0], yj = vs[j][1];

            var intersect = ((yi > y) != (yj > y))
                && (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
            if (intersect) inside = !inside;
        }

        return inside;
    }



    //lat1, lon1 = Latitude and Longitude of point 1 (in decimal degrees)
    //lat2, lon2 = Latitude and Longitude of point 2 (in decimal degrees)
    //unit = the unit you desire for results
    //       where: 'M' is statute miles (default)
    //              'K' is kilometers
    //              'N' is nautical miles
    function distance(lat1, lon1, lat2, lon2, unit) {
        if ((lat1 === lat2) && (lon1 === lon2)) {
            return 0;
        }
        else {
            var radlat1 = Math.PI * lat1/180;
            var radlat2 = Math.PI * lat2/180;
            var theta = lon1-lon2;
            var radtheta = Math.PI * theta/180;
            var dist = Math.sin(radlat1) * Math.sin(radlat2) + Math.cos(radlat1) * Math.cos(radlat2) * Math.cos(radtheta);
            if (dist > 1) {
                dist = 1;
            }
            dist = Math.acos(dist);
            dist = dist * 180/Math.PI;
            dist = dist * 60 * 1.1515;
            if (unit === "K") { dist = dist * 1.609344 }
            if (unit === "N") { dist = dist * 0.8684 }
            return dist;
        }
    }

    function setMaxDist(maxdist, status, airgd){
        var returnval = "";
        maxDistance = maxdist
        airgroundMaxDistance = airgd
        if(status === "update"){
            //refresh the wavfie selection
            //console.log(wavFileSelected.descriptor)
            returnval = selectWavFile(wavFileSelected.descriptor,airgd,maxdist);
        }else if(status === "new"){

        }
        return returnval;
    }

    function changeWavFileProperties(bgcolor){
        if(wavFileSelected){
            //wavFileSelected.setProperties(false, "qrc:/img/map/pin_unselected.png", "yellow", "black", 10, "white");
            wavFileSelected.setProperties(true, "qrc:/img/map/pin_selected.png", "yellow", "black", 10, bgcolor);//to default
            //mapview.update();
        }
    }

    function layerSetVisible(layer, visibility){
        var layerset = [];
        var vis = false;
        if(visibility === 1)
            vis = true;

        if(layer.toUpperCase() === "FLIGHT DATA"){
            layerset = sets;
        }else if(layer.toUpperCase() === "SURVEY JSON"){
            if(surveytype == "WBPHS")
                layerset = surveySegSet;
            else if(surveytype == "GOEA")
                layerset = surveySetGOEA;
            else if(surveytype == "BAEA")
                layerset = surveyPolygonSet;
        }else if(layer.toUpperCase() === "AIR GROUND JSON"){
            layerset = airGroundSegSet
        }else if(layer.toUpperCase() === "WAV FILE"){
            layerset  = wavFileSets
            wavCircleSelected.visible = vis;
        }

        for(var ls = 0; ls < layerset.length; ls++){
            var m = layerset[ls];
            m.visible = vis;
            m.update();
        }
        return "";
    }

    //for testing distance 6.19.2020
    function sqrDistToSegment(mX,mY, x1, y1, x2, y2){
        var nx;
        var ny;
        var minx;
        var miny;

        nx = y2 - y1;
        ny = -( x2 - x1 );

        var t = ( mX * ny - mY * nx - x1 * ny + y1 * nx ) / ( ( x2 - x1 ) * ny - ( y2 - y1 ) * nx );

        if ( t < 0.0 ){
            minx = x1 ;
            miny = y1 ;
        }else if ( t > 1.0 ){
            minx = x2;
            miny = y2;
        }else{
            minx = x1 + t * ( x2 - x1 );
            miny = y1 + t * ( y2 - y1 );
        }

        var ptcoor = QtPositioning.coordinate(parseFloat(mX),parseFloat(mY));
        var minDistPoint = QtPositioning.coordinate(parseFloat(minx),parseFloat(miny));

        var dist = ptcoor.distanceTo( minDistPoint );

        //prevent rounding errors if the point is directly on the segment
        if ( qgsDoubleNear( dist, 0.0)){
            minDistPoint.latitude = mX;
            minDistPoint.longitude = mY;
            return 0.0;
        }
        return dist;
    }

    function qgsDoubleNear( a, b)
    {
        var epsilon = 4 * parseFloat('1e-8').toFixed(8);
        if ( isNaN( a ) || isNaN( b ) )
            return isNaN( a ) && isNaN( b ) ;

        var diff = a - b;
        return diff > -epsilon && diff <= epsilon;
    }

    function findCoor(data){
        var pt1 = QtPositioning.coordinate(parseFloat(data[0]),parseFloat(data[1]))
        var pt2 = QtPositioning.coordinate(parseFloat(data[2]),parseFloat(data[3]))
        var bearing = pt1.azimuthTo(pt2)
        var dist = pt1.distanceTo(pt2)
        var share = dist/data[4] * data[5]
        var ptcoor = pt1.atDistanceAndAzimuth(share,bearing)


        return (ptcoor.latitude + "," + ptcoor.longitude)
    }

    function validateDistWBPHS3(wavcoor){
        var returnval = "0,0,0";
        var wavpt = []
        wavpt.push(wavcoor.latitude); //for latitude
        wavpt.push(wavcoor.longitude); //for longitude

        //1st validation get the closest airground survey from wavpt
        var poly = [];
        poly = airGroundSegSet
        var distlist = []
        outerloop:
        for(var ii = 0; ii < poly.length; ii++){
            if(poly[ii].path.length > 0){
                for(var jj=0; jj < poly[ii].path.length; jj++){
                    var polydistlist = []
                    var polycoor1 = poly[ii].path[jj]
                    var dist = (wavcoor.distanceTo(polycoor1)) / 1000 //this returns meters need to convert to km

                    polydistlist.push(ii);
                    polydistlist.push(dist)
                    polydistlist.push(poly[ii].agcode)

                    distlist.push(polydistlist)
                }
            }
        }

        //look for the line with closest distance
        var tempdist = 0;
        var closestidx = 0  //is the closest polygon to the wav pt
        var tempagcode = ""
        var closestsegment = [];
        var sortedsegment = sortByCol(distlist,1);

        for(var i = 0; i < sortedsegment.length; i++){
            if(i == 0){
                closestsegment.push(sortedsegment[i]);
                closestidx = sortedsegment[i][0]
                tempdist = sortedsegment[i][1]
                tempagcode = sortedsegment[i][2]
            }else{
                if(Number(tempdist) == Number(sortedsegment[i][1])){
                    closestsegment.push(sortedsegment[i]);
                }
            }
        }

        var distexist = "0"
        if(Number(tempdist) <= Number(airgroundMaxDistance)){
            returnval = tempagcode;
            distexist = "1";
        }else{
            returnval = tempagcode;
            distexist = "0";
        }

        //look for the closest distance
        var ptx = wavpt[0];
        var pty = wavpt[1];
        var tdist = 0;
        var tdist2 = 0;
        //var distexist2 = false;
        var tdist3 = 0;
        var actualdist = -1;
        outerloop:
        for(var ii = 0; ii < closestsegment.length; ii++){
            var idx = closestsegment[ii][0];
            if(poly[idx].path.length > 0){
                for(var jjjj=0; jjjj < poly[idx].path.length-1; jjjj++){
                    var ax = poly[idx].path[jjjj].latitude;
                    var ay = poly[idx].path[jjjj].longitude;
                    var bx = poly[idx].path[(jjjj+1)].latitude;
                    var by = poly[idx].path[(jjjj+1)].longitude;

                    //azimuth of wavpt to first pt of line
                    var wavazimpt1 = (QtPositioning.coordinate(ptx,pty)).azimuthTo(QtPositioning.coordinate(ax,ay));
                    //console.log("wavazimpt1: " + wavazimpt1)

                    //azimuth of wavpt to the second pt of baseline
                    var wavazimpt2 = (QtPositioning.coordinate(ptx,pty)).azimuthTo(QtPositioning.coordinate(bx,by));
                    //console.log("wavazimpt2: " + wavazimpt2)

                    var dist3 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(ax,ay))
                    if(Number(dist3/1000) <= Number(airgroundMaxDistance)){
                        distexist = "1";
                        actualdist = dist3;
                    }else{
                        if(actualdist == -1)
                            actualdist = dist3;
                        else{
                            if(Number(dist3) < Number(actualdist))
                                actualdist = dist3;
                        }
                    }

                    var dist4 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(bx,by))
                    //console.log("dist4: 0" + dist4/1000);
                    if(Number(dist4) < Number(dist3)){
                        if(Number(dist4/1000) <= Number(airgroundMaxDistance)){
                            distexist = "1";
                            if(Number(dist4) < Number(actualdist))
                                actualdist = dist4;
                        }
                    }

                    //validate if they come from the same quadrant
                    var pt1quad, pt2quad;
                    if(wavazimpt1 > 0 && wavazimpt1 < 90)
                        pt1quad = 1;
                    else if(wavazimpt1 >= 90 && wavazimpt1 < 180)
                         pt1quad = 2;
                    else if(wavazimpt1 >= 180 && wavazimpt1 < 270)
                         pt1quad = 3;
                    else if(wavazimpt1 >= 270 && wavazimpt1 < 360)
                         pt1quad = 4;

                    if(wavazimpt2 > 0 && wavazimpt2 < 90)
                        pt2quad = 1;
                    else if(wavazimpt2 >= 90 && wavazimpt2 < 180)
                         pt2quad = 2;
                    else if(wavazimpt2 >= 180 && wavazimpt2 < 270)
                         pt2quad = 3;
                    else if(wavazimpt2 >= 270 && wavazimpt2 <= 360)
                         pt2quad = 4;

                    if(pt1quad != pt2quad){//they belong to different quadrant
                        //this means that the wavpt is between the line segment

                        //get the linse segment angle
                        var linesegazimuth = poly[idx].path[jjjj].azimuthTo(poly[idx].path[(jjjj+1)]);
                        //deduct 90 degrees to get its mid angle
                        var newazimuth = linesegazimuth - 90;
                        var newcoor = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(airgroundMaxDistance*1000,newazimuth)

                        //make a line with coordiantes of wavpt and newcoor
                        //and test if the newline crosses with line segment
                        var returnpt = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor.latitude,newcoor.longitude);
                        if(returnpt === false){
                        }else{
                            distexist = "1";
                            var disttest = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt.x,returnpt.y));
                            returnval = poly[idx].agcode;

                            //console.log("disttest")

                            actualdist = disttest;
                        }

                        //added 6.4.2020
                        if(returnpt === false){
                            var newazimuth0 = linesegazimuth + 90;
                            var newazimuth1 = newazimuth0;
                            if(newazimuth0 > 360)
                                newazimuth1 = Math.abs(newazimuth0 - 360);
                            var newcoor1 = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(airgroundMaxDistance*1000,newazimuth1)
                            var returnpt1 = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor1.latitude,newcoor1.longitude);

                            //conclusion
                            /*
                              if distexist == 0 then the airgroundMaxDistance is less than
                                the nearest survey line segment
                            */

                            if(returnpt1 === false){
                                //to test for the nearest segment_intersection
                                //and get the stratum transect and segment
                                //the test distance is 100km / 100,000m

                                var newazimuth2 = linesegazimuth + 90;
                                var newazimuth3 = newazimuth2;
                                if(newazimuth2 > 360)
                                    newazimuth3 = Math.abs(newazimuth2 - 360);
                                var newcoor2 = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(100000,newazimuth3)
                                var returnpt2 = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor2.latitude,newcoor2.longitude);
                                if(returnpt2 === false){
                                    var newazimuth4 = linesegazimuth - 90;
                                    var newazimuth5 = newazimuth4;
                                    if(newazimuth4 > 360)
                                        newazimuth5 = Math.abs(newazimuth4 - 360);
                                    var newcoor4 = (QtPositioning.coordinate(ptx,pty)).atDistanceAndAzimuth(100000,newazimuth5)
                                    var returnpt4 = segment_intersection(ax,ay,bx,by,ptx,pty,newcoor4.latitude,newcoor4.longitude);
                                    if(returnpt4 === false){
                                    }else{
                                        var disttest4 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt4.x,returnpt4.y));
                                        returnval = poly[idx].agcode;

                                        //console.log("disttest4")

                                        actualdist = disttest4;

                                        break outerloop;
                                    }
                                }else{
                                    var disttest2 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt2.x,returnpt2.y));
                                    returnval = poly[idx].agcode;

                                    //console.log("disttest2")

                                    actualdist = disttest2;
                                    break outerloop;
                                }

                            }else{
                                distexist = "1";
                                var disttest1 = (QtPositioning.coordinate(ptx,pty)).distanceTo(QtPositioning.coordinate(returnpt1.x,returnpt1.y));
                                returnval = poly[idx].agcode;

                                //console.log("disttest1")

                                actualdist = disttest1;
                                break outerloop;
                            }
                        }
                    }
                }
            }
        }
        return distexist + "," + returnval + "," + actualdist;
    }

    function setMapRecentView(intzoom, mapcenterlat, mapcenterlon, maptype){
        mapview.zoomLevel = intzoom;
        mapview.center = QtPositioning.coordinate(mapcenterlat, mapcenterlon);
        var mapparent = mapview.parent;
        mapparent.mapType = maptype;
        /*if(maptype === "satellite"){
            mapview.activeMapType = mapview.supportedMapTypes[1];
        }else if(maptype === "default"){
            mapview.activeMapType = mapview.supportedMapTypes[0];
        }*/
    }

    function getMapRecentSetting(){
        var mapcoor = mapview.center;
        var maptype = "default"
        if(mapview.activeMapType === mapview.supportedMapTypes[1])
            maptype = "satellite"
        else if(mapview.activeMapType === mapview.supportedMapTypes[0])
            maptype = "default"
        return mapview.zoomLevel + "," + mapcoor.latitude + "," + mapcoor.longitude + "," + maptype;
    }
}
