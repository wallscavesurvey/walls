/***********************************************************
w2dGroupsToLayers.jsx --
Script to convert specific "w2d" groups to layers after
opening an SVG exported from Walls. Tested with AI CS6.
***********************************************************/
#target illustrator
function convert(){
    var doc = app.activeDocument;
    if(doc.layers.length==0) return;
    var oldLayer1 = doc.layers[0];
    if(oldLayer1.pageItems.length==0 || oldLayer1.layers.length>0) {
			alert("In an SVG exported from Walls, the top layer is non-empty and contains no sublayers. No changes were made.");
			return;
    }
    var w2dLayer, w2dGroup, w2dSubGroup;
    var newLayer1 = doc.layers.add();
    newLayer1.name = oldLayer1.name;
    for(var i=oldLayer1.pageItems.length - 1; i > -1; i--) {
        w2dGroup = oldLayer1.pageItems[i];
        w2dName = w2dGroup.name;
				isArtLayer = (w2dName=="w2d Legend" || w2dName=="w2d Walls" || w2dName=="w2d Detail" || w2dName=="w2d Mask");
        if(isArtLayer && w2dGroup.typename=="GroupItem") {
						w2dLayer = doc.layers.add(); 
            w2dLayer.name = w2dName;
            w2dLayer.move(newLayer1,ElementPlacement.PLACEATBEGINNING);
            w2dGroupLen=w2dGroup.pageItems.length;
            //alert(w2dName+" has "+w2dGroupLen+" elements.");
            if(w2dGroupLen>0) {
                for(var j=w2dGroupLen - 1; j > -1; j--) {  
                    w2dSubGroup = w2dGroup.pageItems[j];
                    if(w2dSubGroup.name.indexOf("w2d ")==0) {
												w2dSubLayer=doc.layers.add();
												w2dSubLayer.name=w2dSubGroup.name;
												w2dSubLayer.move(w2dLayer, ElementPlacement.PLACEATBEGINNING);
												w2dSubGroupLen=w2dSubGroup.pageItems.length;
												//alert(w2dSubLayer.name+" has "+w2dSubGroupLen+" elements.");
												if(w2dSubGroupLen>0) {
														for(var k=w2dSubGroupLen - 1; k> -1; k--) {  
																		 w2dSubGroupItem = w2dSubGroup.pageItems[k];
																		 w2dSubGroupItem.move(w2dSubLayer, ElementPlacement.PLACEATBEGINNING); 
														}
												}
												else w2dSubGroup.remove(); //corresponding empty sublayer already created
                    }
                    else w2dSubGroup.move(w2dLayer, ElementPlacement.PLACEATBEGINNING);  
                }
            }
            else w2dGroup.remove(); //corresponding empty layer already created
        }
        else {
						//keep as an unconverted w2d group or as a preexisting art object --
            w2dGroup.move(newLayer1, ElementPlacement.PLACEATBEGINNING);
            if(!isArtLayer && w2dName.indexOf("w2d ")==0) w2dGroup.locked=true;
        }
    }  
    app.redraw();
    if(oldLayer1.pageItems.length == 0)  oldLayer1.remove();
    else alert("Note: "+oldLayer1.name+" at bottom has "+oldLayer1.pageItems.length+" items remaining!");
	doc.activeLayer=doc.layers[0];
 }  
 if(app.documents.length>0 && confirm(
			"This script assumes that the active document is a Walls-generated "+
			"SVG and that you would like to convert to layers the specific groups "+
			"designated to contain artwork you've added or will add.\n\n"+
			"Those groups are w2d Legend, w2d Walls sym, w2d Walls shp, w2d Detail sym, w2d Detail shp, "+
			"and w2d Mask.\n\nProceed with the conversion?",false)) {
    convert();
 }
//end of w2dGroupsToLayers.jsx