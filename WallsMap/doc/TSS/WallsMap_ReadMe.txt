WallsMap (TSS edition) v0.3 Build 2014-07-11 Notes
==================================================

This TSS edition of the WallsMap setup is the same as the publicly available
version except that it doesn't install the Texas Public Caves project. Also, the
link in the program's About box directs you to a summary web page which links to a
password- protected site. From there you can obtain updates for both the program
and the complete TSS karst database.

==============
Recent Changes

* Build 07/11/2014

1) When creating an update archive, the compare function could on rare occasions
fail to include image files linked to new records. This bug was fixed and minor
improvements made to the program and help file.

* Build 06/20/2014

1) Automatic field intialization for new or relocated point features can now occur
when the point is covered by any one of several polygon shapefiles -- for example,
a set of geologic map layers when the field contains a geologic unit name or
abbreviation. Previously only a single polygon layer could be used to initialize a
given field. In the Help file see Export Operations | Template File Format |
Controlling a Field's Behavior | Location-dependent Initialization.

2) Location-dependent intitialization now works as expected for unlocated features.

* Build 05/05/2014

1) Links to six more DRG/DEM layers covering 483 7.5' Texas quadrangles can be
found in the Available Background Imagery section of the help file. This extends
the seamless coverage to all of Far West Texas (1177 quadrangles total).

2) In maintaining the TSS database we've often needed to reconcile changes made by
different contibutors to reports in memo fields. To make this easier the text
editor has new options that allow quick side-by-side comparisons of memo field
text. For details, see the new section, "Editing Memo Field Content", under
"Searching and Editing Shapefiles | Editing Records" in the Help file.

3) A spatial indexing checkbox was added to the Symbols dialog for polygon
shapefiles. The program will use the index for determining which polygon contains a
specific point, such as when initializing field values that depend on location. The
speed-up is dramatic when a large shapefile is used to initialize thousands of
records during an export or database update. I suggest leaving the index "depth"
option set at 5.

4) Miscellaneous improvements and minor bug fixes. Attempting to launch the program
with a project that's already open in another program instance will activate that
instance intead of opening an empty application window and displaying an annoying
message.

* Build 03/29/2014

1) The compare function was enhanced to support more types of keys and tests for
consistency. For example it can check whether or not a point falls within an
appropriate boundary, say of a county, and report the distance to that polygon
if it fails the test. The help file was updated, particularly the Template
File Format section.

2) Miscellaneous minor bugs were fixed. For example, sorting table columns for
character fields containing leading numbers could sometimes produce an unexpected
result.

* Build 03/09/2014

1) More features were added to help with database maintenance in a multi-user
setting. The shapefile compare function, for example, can create an update archive
for submission to the database maintainer and also update the master copy on the
receiving end. This can involve the generation of keys for new records.(See new
Help file topics under Keeping the Database Current.)

2) Three more DRG/DEM layers covering areas surrounding Upton, Irion, Reagan, and
Tom Green counties are linked to in the Available Background Imagery section of the
help file. The total coverage is now 694 7.5' Texas quadrangles.

* Build 12/18/2013

1) This build uses a newer graphics technology (Direct2D) that's available only in
versions of Windows later than XP (preferably 7, 8, or 8.1). Depending on your
video hardware, you should notice much faster display of very large shapefiles.
An XP-compatible alternative will also be available for awhile.

2) The NTI export feature was enhanced with more options and documented in a new
Help file topic.

* Build 09/28/2013

1) Miscellaneous improvements include the option to add a generated NTI file to the
project as opposed to just replacing the source, simplifying before-and-after
comparisons. A rare bug that could corrupt project tree structure was also fixed.

2) Google's new WebP image format is now supported, both as an image type
(extension .webp) and as a tile compression option for creating NTI files. My tests
so far indicate that compared to JPEG2000 the WebP option can produce higher-
quality NTI files of similar or smaller size.

* Build 09/05/2013

1) GPS tracking is enabled in this build. Establish a connection via a real or
virtual COMM port by seleting "GPS" -> GPS Port Connection from the program's main
menu. A small dialog window then remains open (possibly minimized) to allow a GPS
placemark and track line to appear on georeferenced map views. There are options to
customize the marker and line styles and to save the logged track as a shapefile.
My thanks to Peter Sprouse for lending his Bluetooth GPS receiver and tablet
computer for the development and testing of this build.

2) The program was modified to improve compatability with tablet computers like
Microsoft's Surface Pro. Gestures commonly used on touch screens are supported. For
example, a "touch and hold" gesture will open a context menu.

* Build 05/05/2013

Compatibility with Windows XP is restored after having been broken in the previous
build. The symptom was a failure to start. There are no other changes.

* Build 04/30/2013

1) This build introduces a shapefile comparison function, its main purpose being to
help maintain a database when multiple users are independently making contributions.
The function logs the differences between an unedited reference shapefile, say a
snapshot of the master database, and a selected set of shapefiles having the same
structure. The selected set will presumably have records for new features as well as
updated versions of records existing in the reference. Besides the log file, the
function optionally produces an "update archive", a ZIP file containing a shapefile
of revised records along with any new local files referenced in memo fields.

To initiate a comparison, right click on a suitable reference shapefile in the
Layers window and select "Compare with other layers." For you to see this menu item
the reference must have a .tmpshp component that specifies which field to use for
identifying new and existing records. The TSS uses field TSSID with a reference
shapefile named "TSS All Types.shp" that's shipped with each database release.

2) The dialog "URL for Web Map Navigation" was overhauled to allow replacement or
modification of all (now six) of the built-in mapping site addresses. Details are in
a Help file topic under "Preference Settings." New to the built-in list is TxDOT's
Statewide Planning Map, which can display eight kinds of environmental overlays,
including major aquifers, watersheds, natural regions and sub regions. It also can
provide the coordinates of Texas mile post markers.

* Build 03/24/2013

1) The KML files generated for Google Earth for displaying multiple placemarks were
revised to prevent an annoying tilting if the view, both initially and when
zooming to features clicked on in the sidebar. Oddly, this is the default behavior
beginning with GE version 7. With this fix you can expect to be looking straight
down, with all placemarks in view and with the maximum scale determined by your
Range setting in WallsMap's Google Earth (KML) Launch Options dialog (accessed via
Preferences). Also, a label size setting was added to this dialog.

2) The Help topic, Available Background Imagery, was updated with a link to a 0.15-
meter aerial image encompassing several karst preserves near Cedar Park in southern
Williamson County.

* Build 02/11/2013

1) Links to three new 0.15-meter resolution aerial images (NTI files) can be found
in the Help file under Available Background Imagery. The USGS orthoimagery was
acquired in 2010 and covers all or most of Honey Creek SNA, Government Canyon SNA,
and the Spring Branch karst area of Texas.

2) Repairs made: Under Windows XP, grayed checkboxes in the Layers window would not
display, resulting in confusing behavior. Support for TIFF files larger than 4 GB
had been inadvertently removed in recent builds. In the View menu, the options to
outline all or selected layers should have been disabled (grayed) for non- 
georeferenced image files.

3) Miscellaneous minor improvements. Examples: The distance measure tool now
displays the direction (degrees azimuth with respect to the map's up direction) on
the program's status bar. Polylines are drawn with round linecaps to eliminate the
"broken" appearance of traverses that connect end-to-end, as in exported Walls
survey data.

* Build 11/25/2012

1) More coordinate system types, such as state plane, are recognized in this build,
even though support for mixing or converting between different types remains
limited. Projection details are displayed in the layer's properties window. When an
image file doesn't embed this information, a projection (.prj) file is read if one
is found.

2) Fixed issues related to the handling of deleted records in 2D polyline and
polygon shapefiles. Undeletion and deletion is now allowed in table views, with
the map and layer exent (if displayed) updated dynamically.

* Build 11/15/2012

1) In table views, the Find dialog accessed by right-clicking the header will
allow searches for deleted records when the column clicked on is that of the
record number. As before, an opened table view will not show the deleted records.
To include them click the Reload button and choose option "All records."

2) The deleted status of selected records is now retained during "Copy to layer"
operations from either a table view or the Selected Points window. In a shapefile
export, however, including the deleted records is still a dialog option that's
unchecked by default.

3) "Move to Top" and "Move to Bottom" were added as context menu choices in the
Column Visibility dialog for table views.

4) The Find Broken Image Links dialog, accessed by right-clicking a memo field's
name in a table view header, now has a "Create Log" button that's enabled when the
memo field contains links to non-existent files. This function will create or
append to a text file, <shapefile_name>_log.txt, listing the links alphabetically.

* Build 11/04/2012

1) A recently-introduced bug was fixed: The program would abort upon completion of
a successful NTI file export.

* Build 10/15/2012

1) Bugs fixed: (a) Reordering visible layers by dragging a folder wouldn't in
itself cause the map view to be updated. (b) Setting line width to zero with
antialiasing turned off now removes the outlines of rendered polygons as one would
expect.

2) Fixed several issues relating to the handling of 3D shapefiles, with three M
types (pointM, polygonM, and polylineM) now recognized along with the corresponding
Z and 2D types. Also, a new checkbox option, "Make 2D shapefile", was added to the
shapefile export dialog. For details, see "Exporting and Merging" in Help.

3) The Texas project was updated with the 3rd Qtr 2012 roads shapefile from TxDOT,
with its format converted from polylineM to polyline to save space.

* Build 10/10/2012

1) The program was overhauled to support a tree-like hierarchy of map layers
instead of a simple list. In the Layers window you'll find options for creating
named groups (folders) in the context menu for a highlighted item. Like layers, the
folders have visibility check boxes and can be rearranged by drag-and-drop. Scale
range settings can also be applied to folders. The Help file was updated with
images and additional text, mostly in topic "Getting Started."

2) Some fancier user interface controls of Vista and Windows 7 are now enabled.
Under Windows XP you should see no significant differences.

3) A new menu option, "Preferences > Preferences for this document > Save and
restore Layers window size", preserves the size and column widths of the Layers
window in the NTL file. Also, when you move between multiple open documents by
clicking on the corresponding map or image, the Layers/Properties window resizes
to match the active document.

4) Revised the image previewer so that an image opened indirectly via the "double
extension" feature can optionally be added as a layer if it's compatible. A large,
rarely-used DOQ might be opened this way. Documentation for the previewer is
now included in "Getting Started."

5) Improved dialogs: (a) When opening a project for which some layers are missing
you're offered the option to skip the remaining missing files without additional
warnings. (b) The dialog requesting an editor's name that pops up when one
attempts, for the first time, to edit a shapefile with timestamp fields, has the
checkbox option to make the name persistent across program invocations.

6) Bug fix: Some file timestamps were wrongly converted to UTC in exported ZIP
files.

* Build 7/30/2012

1) Updated online help by adding new topics and detailing previously-available
operations. Additional info for most (but still not all) dialogs can be accessed by
pressing F1 when the dialog is open. The Available Background Imagery topic
has a link to a another DEM/DRG NTI file set for West Texas, this one covering
Terrell and eastern Pecos counties (63 7.5' quads).

2) The list of up to 8 text strings (with matching criteria) that were last used
successfully when searching for shapefile records is now a project setting that's
optionally saved in the NTL file. Enable this option by checking "Save and restore
search history" in the "Preferences > Preferences for this document" menu.

3) Fixed a bug reported by George Veni. Pasting rich text into a memo field already
containing plain text would not function as designed, the program inserting a plain
text version without offering to switch the editing mode to rich text.

4) Miscellaneous improvements: The Image Opacity dialog has a "Preview" check box.
KML was added to the list of media types that can be opened from the image preview
window. You can toggle a shapefile's editable mode in some situations where a
record is already open in one or more windows. Some unhelpful options in edit box
context menus were removed.

* Build 4/22/2012

1) The "Copy to layer" functions no longer require that the source and destination
shapefiles have the same attribute structure. Each can have fields not present in
the other. Also, fields with matching names can have different lengths, with
conversions between memo and fixed-length text fields possibly occurring. The user
is warned when structures are different and when data might be (or have been)
truncated.

2) The image viewer now recognizes non-raster graphics and media files, but only if
they exist alongside a suitable raster image file. For example, if you wanted to
link a pdf document named "Hueco Tanks.pdf", you would actually link a specially-
named raster representation of it, say "Hueco Tanks.pdf.png." Then, whenever this
png image is being previewed, the "Open File" button will open the corresponding
pdf file in the associated application -- most likely, Adobe Reader.

3) Double-clicking a thumbnail in the image viewer is now equivalent to clicking
the "Open File" button. Except for the special case described above (item 2), this
causes the previewed raster image to be opened as another document in the instance
of WallsMap you're running.

* Build 12/31/2011

At least the prior build (12/14/2011) contains a Windows 7 dependency that causes it
to display an error message, "entry point not found...", when run under Windows XP.

* Build 12/14/2011

1) A new toolbar icon (cross-hair symbol) opens a dialog where you can enter
coordinates for centering the map. The dialog also appears when "Center on point"
is chosen from the view's right-click menu, in which case the coordinate boxes are
initialized to the position clicked on instead of the previous centered-on
position. A cross-shaped placemark is optionally produced that remains in place
until the next centering operation.

2) Common formats for lat/long coordinates, such as N 30 25' 30.5" W 97 15' 2" (or
simply 30 25 30.5 -97 15 2) can be pasted or entered directly into the Selected
Points and Center on Point coordinate boxes. As you type, validity is indicated by
an enabled "Relocate" (or "OK") button.

3) Opening projects (NTL files) at a network location is supported.

* Build 10/27/2011

1) The NTI/NTE elevation layer format is improved so that the visible NTI component
can have a higher resolution than the associated DEM component (NTE file). When made
partially transparent (see item 5) it can provide an attractive 3D shading for an
underlying USGS topo map. Also, elevations retrieved from the DEM are of maximum
possible accuracy and no longer affected by the screen map's zoom level.

2) The downloadable NTI imagery was updated with new sets of DEM/DRG layers that
allow the DEM components to produce a shading effect for the topos. The number of
7.5' quads covered has increased from 245 to 506. The links to these and other
NTI layers for regions of Texas are in the help file topic "Available Background
Imagery", which was moved from the TSS project section to the public section.

3) Spreadsheets (xls files) compatible with Microsoft Excel and OpenOffice.org can
now be created directly, thanks in large part to code developed by Yap Chun Wei and
Martin Fuchs. This export option for shapefiles is a right-click menu choice for
the selected items in a table view. Worth noting is the fact that memo field text
can be exported without truncation as long as it's no longer than Excel's 32K cell
size limit. Also, rich text (RTF) is optionally transferred intact.

4) To greatly simplify proofing of data in ranges of shapefile records, memo fields
are presented differently in table views, with up to 80 characters of field content
displayed to the right of the button that opens the editor or image viewer.

5) Raster image transparency is supported, which allows the setting of both overall
opacity and a specific color to be treated as transparent. For hiding the margins
of lossy-compressed images, an error tolerance for matching the transparent color
can be specified. These layer-specific settings are accessed by clicking an
"Opacity" button in the Layers window, or by right-clicking a map image and
selecting "Image opacity" from the pop-up menu.

6) Exploring a location at a web mapping site is more efficient. From the context
menus you can either open the default site or select one of five suggested URLs and
tweak its parameters. The default is optionally changed in the process. The
suggested URLs are for Acme Mapper (my preference), Gmap4, Google Maps, Bing Maps,
and ArcGIS Online.

7) Minor fixes: Saving new shapefile records with empty label fields is now
easier, requiring only confirmation. Changing marker and label visibility sometimes
failed to update all windows properly.

* Build 06/12/2011

Miscellaneous improvements: Thumbnail captions in the image preview window can be
edited in place, with the default captions for new images more likely to be
acceptable. Certain rarely-used characters in labels could cause Google Earth to
reject a KML file without explanation. This was fixed with proper formatting.

* Build 05/08/2011

1) The "Locate in Google Earth" feature now works for multiple point features, such
as a branch in the Selected Points window or all selected items in a table view.
When you choose "Options" instead of "Fly Direct", you can specify the attribute
fields (if any) to be used for constructing the descriptions that appear in GE when
you click on a placemark icon. The placemark symbols for multi-point exports and
single-point exports can be specified via two URLs. See "Preferences | Google Earth
(KML) launch options" on the menu.

2) WallsMap uses the Geospatial Data Abstraction Library (http://ww.gdal.org) to
display many types of raster imagery. This build incorporates the latest release,
1.8.0, instead of 1.7.2.

* Build 04/22/2011

1) There's a new section in the help file, "Available Background Imagery",
containing descriptions and download links for sources of image data suitable for
adding to the TSS_Data project. Apart from the public sources, the WallsMap site
now has a starting set of 8 images comprised of 245 USGS 7.5' topos at full
resolution, along with a corresponding set of DEM images for retrieving elevations
in those regions. These cover more than 60% of the caves, sinks, and shelters now
in the database. There are also links to some 6-inch resolution photography for the
GCSNA and Spring Branch areas.

2) Miscellaneous fixes: Text can be copied from fields in shapefiles that have
been set read-only; the text editor has a checkbox for toggling toolbar display
when in rich text mode; a certain kind of JPEG file would fail to open on Vista
or Windows 7 when located on the system drive. The user is prompted only once
when multiple NTI files requiring overview generation are added to a project.

* Build 02/25/2011

1) Paired with the ability to launch Google Earth at a selected location is a new
option to open a web mapping application. This can be a quicker alternative when
the goal is to explore the geography surrounding a single placemark, possibly to
retrieve new coordinates. An application named Gmap4, for example, can display
relief-shaded USGS 7.5' topographic maps in addition to Google's standard map
types. The web site accessed and type of map initially shown are determined by a
Preferences menu setting labeled "URL for web map navigation." The help file has
more informtion about it under "Configuration Settings."

2) Miscellaneous fixes: Field descriptions display as tooltips on table header;
Linked local files, not just images, are tested for validity and included in
exported archives. Layer context menu option to close a shapefile and delete its
files; more coordinate pair formats can be copied and pasted; option to abort
opening a project instead of skipping each layer file that can't be found;
memo field links to local PDF files now work properly; better handling of
corrupted shapefiles; some incompatibilities with non-standard numeric field types
used by other SW removed; recognition of Global Mapper prj files.

* Build 10/10/2010

A bug was introduced in the previous build that caused a "Decompressing tile..."
error when accessing 256-color NTI images. This was fixed, along with an error
in the function that converts from Lat/Long to UTM where latitude is negative.

* Build 08/03/2010

1) This build was created with a new compiler version (Microsoft VC++ 2010),
requiring that a DLL for reading MrSID (.sid) image files be distributed with the
executable. This module from LizardTech (www.lizardtech.com) is loaded only when
required.

2) The newest versions of supporting libraries Kakadu (v6.4) and GDAL (v1.72)
have been incorporated, one result being that certain JPEG2000 (.jp2) files are
now displayed with the correct colors.

3) The function that copies shapefile records to a destination shapefile was broken
in build 06/27/2010. This is fixed starting with build 07/05/2010.

4) Miscellaneous improvements and fixes, such as the ability to delete or
undelete shapefile records in a table view. (The "Reload" function now optionally
includes the deleted records.) The timestamp of an edited DBF component was not
updated in earlier builds.

* Build 06/27/2010

A number of obscure or display related issues were addressed. Occasionally 2D point
coordinates in a shapefile would change unnecessarily due to round-off errors during
coordinate system transformations. The About box now contains a live link to a web
page from where updates can be downloaded. The zip archiving function offers more
options for choosing which layers to include. The setup and uninstall programs won't
proceed when WallsMap is running. The help file, still incomplete, has some new
sections.

* Build 04/19/2010

1) The display of shapefiles has been improved through the use of Cairo, an open
source 2D graphics library that can render visually appealing shapes and lines. The
various symbol dialogs have new options and controls. NOTE: Since the marker symbol
size units have changed, a project saved with this build will appear with oversized
markers in an older program version. A project saved with an earlier version,
however, should look approximately the same.

2) The PNG export function, useful for creating map images for printing, has new
options for controlling the sizes of rendered shapefile features (text, symbols, and
line widths), for including a scale bar, and for immediately opening the image as a
georeferenced document.

3) The logic for determining layer compatibility when coordinate system info is
incomplete has been overhauled.

4) The beginnings of an online help file is provided with this build.

* Build 01/03/2010

1) The File menu now has an "Export project as ZIP archive" option. The archive
stores a project script, <ZIP name>.NTL, along with all associated layer files, with
hidden (unchecked) layers optionally excluded. There's also an option to include the
targets of links in memo fields, whether they be image files accessed by the viewer
or local files (such as PDFs) targeted by links in ordinary text memos. A file won't
be included, however, if the corresponding link is broken (obviously), or if it
resides outside the directory branch whose root is the parent of the NTL file's
folder. If a report of such instances isn't displayed when the archive is created,
the project will be fully self-contained and can be extracted to any folder and
opened. This feature, by the way, complements the "Fix broken links" dialog (accessed
by right-clicking a memo field name in a table view) in that it can be used to
discover orphaned files you may still want to add to the project (or delete). Just
extract the ZIP to a new location and do a directory comparison.

2) Field descriptions can be stored in a shapefile's template component (.tmpshp
file). These will display as multi-line tooltips when the mouse pointer hovers over a
field name in the Selected Points dialog.

3) Minor fixes and improvements. The layers list did not update properly when layers
were added via drag-and-drop or cut-and-paste. The "Fix Broken Links" dialog now has
a "Select Record" button, which is useful when more than the path portion of a link
needs to be repaired.

* Build 12/14/2009

1) Editing of attributes can be controlled by options specified in a template file
(extension .tmpshp) that might exist alongside the shapefile's other components. You
can make specific fields read-only, have alternative choices appear in drop-down list
boxes, and specify how fields in new records are to be initialized. The initialized
value can be a fixed string or, depending on where you right-clicked, a value such as
a quadrangle name retrieved from a polygon shapefile or an elevation (converted to
feet or meters) retrieved from an NTI/NTE image layer. To view or edit the template
for a shapefile that has one, right-click the shapefile's title in the Layers
window and select "Open (name).tmpshp" from the context menu.

2) There is a new dialog for finding and optionally fixing bad image links in memo
fields, particularly those for files that have moved relative to the shapefile's
location. To access it, open a table view of the shapefile, right-click on a memo
field's name in the header and select "Fix broken image links...". This makes it
convenient to store images comprising a future project update in a separate folder to
minimize the size of the archive that would need to be distributed.

3) It's no longer necessary to have either the Layers window or the Selected Points
dialog open to add new point features. Just right-click on the map and appropriate
options will be presented in the pop-up context menu. Similarly, to copy items from
one shapefile to another it's only necessary to have the items to be copied present
in the Selected Points window. The right-click context menus for both tree branches
and individual records now have a "Copy to shapefile..." option, with the suitable
target shapefiles listed in a submenu. Unlike the drag-and-drop method, the copied
items in the source shapefile remain selected, allowing them to be easily deleted to
complete a move operation. As before, the source and target shapefiles must have
identical attribute structures.

4) Miscellaneous improvements. Checkboxes now represent Yes/No attribute fields.
Tooltips are enabled for the memo field buttons in a table view. It's now
possible to add layers to a project by dragging or copying file names in Windows
Explorer and dropping or pasting them in the Layers window. To simplify file
management, including deletion, there is a new "Open containing folder" option in
the Layers window context menu. Units conversion is now possible when exporting
via a template.

5) UTM zone numbers are handled more flexibly in the Selected Points dialog,
making it easier to enter UTM coordinates in projects covering multiple zones.

6) Among the bugs fixed is the potential mishandling of deleted records in a
shapefile export. The other issues fixed were less serious.

=========================================================================
<EOF>
