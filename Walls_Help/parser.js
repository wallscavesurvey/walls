function HlpPathName() {
	var X, Y;
	if(location.href.search(/:/) == 2) X = 14; else X = 7;
	Y = location.href.lastIndexOf("\\") + 1;
	return location.href.substring(X, Y) + "Walls32p.hlp";
}

function WinPopup(ObjectID,TopicID) {

  // Create an instance of the control if one doesn't already exist.
  if(!document[ObjectID]) {

	  var popupObject = "<object id=" + ObjectID +
		" type='application/x-oleobject' " +
		"classid='clsid:adb880a6-d8ff-11cf-9377-00aa003b7a11'>" +
		"<param name='Command' value='Winhelp, Popup'>" +
		"<param name='Item1' value='" + HlpPathName() + "'>" +
		"<param name='Item2' value='" + TopicID + "'>" +
		"</object>";

	  // Add control to the document.
	  document.body.insertAdjacentHTML("BeforeBegin",popupObject);
  }
  document[ObjectID].hhclick();
}
