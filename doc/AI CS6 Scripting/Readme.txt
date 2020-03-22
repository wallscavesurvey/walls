Adobe Illustrator CS6 Scripting
ReadMe File

______________________________________________________________________

This file has the following sections:
  1. Introduction
  2. Documentation
  3. Sample Scripts
  4. Known Issues
  5. SDK Support and Adobe Partner Programs


***********************************************************
1. Introduction
***********************************************************
To find out what's new in Illustrator CS6 scripting, see
"Adobe Illustrator CS6 Scripting Guide."

To find the latest available information on scripting
Illustrator, go to:

  http://www.adobe.com/devnet/illustrator/


***********************************************************
2. Documentation
***********************************************************

Adobe Illustrator CS6 Scripting Guide
  File: Illustrator Scripting Guide.pdf

Adobe Illustrator CS6 Scripting Reference: AppleScript
  File: Illustrator Scripting Reference - AppleScript.pdf

Adobe Illustrator CS6 Scripting Reference: JavaScript
  File: Illustrator Scripting Reference - JavaScript.pdf

Adobe Illustrator CS6 Scripting Reference: VBScript
  File: Illustrator Scripting Reference - VBScript.pdf

Adobe Introduction to Scripting
  File: Adobe Intro to Scripting.pdf

JavaScript Tools Guide
  File: JavaScript Tools Guide CS6.pdf


***********************************************************
3. Sample Scripts
***********************************************************
Sample scripts are included in the product installer for Illustrator
CS6. When Illustrator is installed to its default location, the path
to the sample scripts is as follows:

  Windows:
    C:\Program Files\Adobe\Adobe Illustrator CS6\Scripting\Sample Scripts\

  Mac OS:
    /Applications/Adobe Illustrator CS6/Scripting/Sample Scripts/

Look in these folders and try the supplied samples. The Illustrator
AppleScript, JavaScript, and VBScript reference documents contain
many other examples.


***********************************************************
4. Known Issues
***********************************************************
Text selection not set reliably in AppleScript (1436010)

  Affects: AppleScript

  Problem:
    Setting a text selection using AppleScript does not work reliably.
    For example, suppose your script selects a word and subsequently
    checks for a text selection before applying some styling to the
    selected text:

      select word 1 of story 1 of current document
      -- Double the font size of selected text
      set selectedText to selection of current document
      if class of selectedText is text then
        set fontSize to size of selectedText
        set size of selectedText to 2 * fontSize
      end if

    The "class of selectedText is text" test in this script does
    not work reliably. To reproduce the problem, create text in
    a document, click the Selection Tool, then run the script.

  Workaround:
    Apply styling to text directly, without using selection:

      -- Double the font size of the first word in the first story
      set fontSize to size of word 1 of story 1 of current document
      set size of word 1 of story 1 of current document to 2 * fontSize

_____________________________________________________________________________

Cannot switch hyphenation of a paragraph from off to on (1437421)

  Affects: AppleScript, JavaScript, VBScript

  Problem:
    The samples below fail to turn on hyphenation, if it is already off.

    AppleScript:
      set hyphenation of paragraph 1 of story 1 of document 1 to true

    JavaScript:
      app.activeDocument.textFrames[0].paragraphs[0].hyphenation = true;

    VBScript:
      Set appRef = CreateObject("Illustrator.Application")
      appRef.ActiveDocument.TextFrames(1).Paragraphs(1).ParagraphAttributes.Hyphenation = true

  Workaround:
    None.

_____________________________________________________________________________

Punctuation causes iteration of words to be incorrect (1451863)

  Affects: AppleScript, JavaScript, VBScript

  Problem:
    This JavaScript iterates the words in a sentence containing some
    punctuation:

      var docRef = app.documents.add ();
      var tfRef = docRef.textFrames.add();
      tfRef.contents = "This is : a ( test ) of words"
      wordCount = tfRef.words.length;
      for (i = 0; i < wordCount; ++i ) {
	$.writeln (app.activeDocument.textFrames[0].words[i].contents);
      }

    The words that should be reported are:

      This
      is
      :
      a
      (
      test
      )
      of
      words

    The words that are reported are:

      This
      i
      :

      tes

      of
      words

  Workaround:
    None.

_____________________________________________________________________________

"An Illustrator error occurred: 1346458189 ('PARM')" alert (1459349)

  Affects: JavaScript

  Problem:
    This alert may appear when carelessly written scripts are repeatedly
    run in Illustrator from the ExtendScript Toolkit.

    Each script run is executed within the same persistent ExtendScript
    engine within Illustrator. The net effect is that the state of
    the ExtendScript engine is cumulative across all scripts that
    ran previously.

    The following issues with script code can cause this problem:

      - Reading uninitialized variables.
      - Global namespace conflicts, as when two globals from different
        scripts have the same name.

  Workaround:
    Be very careful about variable initialization and namespace conflict
    when repeatedly pushing a batch of Illustrator scripts
    for execution in Illustrator via the ExtendScript Toolkit (ESTK) in a
    single Illustrator session.

    Initialize variables before using them, and consider the scope of
    your variables carefully. For example, isolate your variables by
    wrapping them within systematically named functions. Instead of:

      var myDoc = app.documents.add();
      // Add code to process myDoc

    Wrap myDoc in a function that follows a systematic naming scheme:

      function myFeatureNameProcessDoc() {
        var myDoc = app.documents.add();
        // Add code to process myDoc
      }
	myFeatureNameProcessDoc();

_____________________________________________________________________________

AppleScript has no SymbolRegistrationPoint enumeration (2489648)

  Affects: AppleScript

  Problem:
    These enumeration values are not available in AppleScript. You cannot specify a
    registration point while creating new symbols. All new symbols use the default (center)
    registration point.
      SymbolRegistrationPoint.SymbolTopLeftPoint
      SymbolRegistrationPoint.SymbolTopMiddlePoint
      SymbolRegistrationPoint.SymbolTopRightPoint
      SymbolRegistrationPoint.SymbolMiddleLeftPoint
      SymbolRegistrationPoint.SymbolCenterPoint
      SymbolRegistrationPoint.SymbolMiddleRightPoint
      SymbolRegistrationPoint.SymbolBottomLeftPoint
      SymbolRegistrationPoint.SymbolBottomMiddlePoint
      SymbolRegistrationPoint.SymbolBottomRightPoint

  Workaround:
    Use JavaScript or VBScript.

_____________________________________________________________________________

ExtendScript $.colorPicker() function causes crash in Mac OS (2580738)

  Affects: ExtendScript in Mac OS

  Problem:
    A problem in ExtendScript can cause Illustrator to crash when an
    ExtendScript script uses the $.colorPicker() function in Mac OS.

    Because of this, the following Color Picker UI selections should
    be avoided: the Gray Scale and CMYK Sliders available under Color Sliders,
    and the Developer Palette selection under Color Palettes.

    All other Color Picker UI selections work correctly in Mac OS.
    This includes all Color Wheel selections, the RGB and HSB Sliders
    under Color Sliders, the  Web Safe Colors, Crayons, and Apple
    selections available under Color Palettes, and all Image Palettes
    and Crayons selections.

    The $.colorPicker() method works correctly in all cases in Windows.

  Workaround:
    None.

_____________________________________________________________________________

ESTK cannot connect to Illustrator if it is installed in a double-byte path and not launched (2593078)

  Affects: ExtendScript

  Problem:
    ExtendScript Toolkit (ESTK) cannot connect to Illustrator if Illustrator
    is installed in a double-byte path and not launched. An example of a
    double-byte path is C:\Program Files\Adobe\<some Chinese characters>.

  Workaround:
    Launch Illustrator first, then connect to Illustrator in ESTK.

_____________________________________________________________________________

Incorrect property type for strokeDashes in ESTK OMV and Scripting Reference
(2592968)

  Affects: ExtendScript

  Problem:
    In the ESTK Object Model Viewer and Illustrator Scripting Reference,
    the type of these 3 properties is incorrectly given as object; the
    correct type is array:

      Document.defaultStrokeDashes
      PathItem.strokeDashes
      TextPath.strokeDashes

  Workaround:
    Use array, not object. For example, to create a solid line,
    set to empty array [], rather than an empty object.

      var mydoc = app.documents[0];
      mydoc.pathItems[0].strokeDashes = [];
      mydoc.pathItems[0].strokeDashes = [10,10];


***********************************************************
5. SDK Support and Adobe Partner Programs
***********************************************************
If you require SDK support for the Illustrator CS6 SDK,
you may purchase single or multi-pack SDK support cases.
Information on purchasing SDK support cases can be found at:

  http://partners.adobe.com/public/developer/support/index.html

Information on Adobe support, in general, may be found at:

  http://www.adobe.com/support/programs/

If you are a partner who extends, markets, or sells Adobe
products or solutions, you should consider membership in
the Adobe Partner Connection Solution Partner Program.
The Solution Partner Program provides development support,
access to timely product information, as well as various
marketing benefits. To learn more about the program, point
your browser to:

  http://go.adobe.com/kb/cs_cpsid_50036_en-us

_____________________________________________________________________________
Copyright 2012 Adobe Systems Incorporated. All rights reserved.

Adobe and Illustrator are registered trademarks or trademarks of Adobe
Systems Incorporated in the United States and/or other countries. Windows
is a registered trademark or trademark of Microsoft Corporation in the
United States and/or other countries. Macintosh is a trademark of Apple
Computer, Incorporated, registered in the United States and other countries.
All other brand and product names are trademarks or registered trademarks of
their respective holders.
_____________________________________________________________________________