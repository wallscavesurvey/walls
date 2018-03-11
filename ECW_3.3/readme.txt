ECW JPEG 2000 SDK 
Version 3.3
Date : 23-06-2006
readme.txt
----------------------

(Set word wrap on your text viewer for best results
 when reading this file.)

Please read the rest of the information in this file.


REQUIREMENTS (for the binary distribution):

	O/S:		Windows 98 second edition
			Windows NT 4 SP5 (or later)
			Windows 2000
			Windows XP
			Windows 2000 Server
			Windows 2003 Server

	PLATFORM:	INTEL PENTIUM

	SOFTWARE:	Visual Studio 6.0 and 7.1

REQUIREMENTS (general):

ER Mapper has made available an approximately platform-independent C/C++ source code distribution for the ECW JPEG 2000 SDK (see www.ermapper.com for details).  Build files for Solaris, Linux and MacOS X are included, and support on Linux and Solaris has been tested to work correctly.  There are still problems with compression support under MacOS X.

See the ER Mapper website for the download page for the source distribution.

This release of the binary distribution also includes DLLs built under Visual Studio .NET 2003 for compatibility with your Visual C++ 7.1 projects and others.  The Visual Studio .NET 2003 versions of the binary files are contained in the /vc71 subdirectory of the /bin, /lib and /redistributable directories respectively.

GETTING STARTED

There are nine Visual C/C++ projects (five compression examples and four decompression examples) included in the SDK. The installation is set up to install and build without you having to do any setup. However, when setting up your own projects you should be aware of the following:

1. Your application will be using the several dlls which the setup program will have placed in your ECW SDK target directory (typically C:\Program Files\Earth Resource Mapping\Image Web Server\Client).

2. You will need to make sure that the dlls are in the same directory as your executable (or is in a directory that is included in the PATH environment variable). There are copies of these files in both the "bin" directory and the "redistributable" directory.

3. The example project files are stored in a workspace file called examples.dsw (VC6.0) and a solution file called examples.sln (VC7.1) in the "examples" subdirectory of the distribution.  All examples can be built at once using the "buildall" projects, but see the section on the VC7.1 binaries below before attempting to build the VC71 examples.

4. If you want to run the examples from a command prompt, note that they need one argument being a ECW file name or an ecwp URL. E.g.
	dexample1.exe ..\..\..\testdata\rgbimage.ecw
	dexample2.exe ..\..\..\testdata\rgbimage.ecw
or
	dexample1.exe ecwp://www.earthetc.com/images/world/gtopo30.ecw
	dexample2.exe ecwp://www.earthetc.com/images/world/gtopo30.ecw

USING THE VISUAL STUDIO .NET 2003 BINARY FILES

The distribution ships configured to build the sample code using the visual Studio 6.0 binaries, so you will need to backup the contents of "bin" and "lib" and replace those files with the contents of "bin/vc71" and "lib/vc71" to use the Visual Studio .NET 2003 binaries.

REDISTRIBUTING THE REDISTRIBUTABLE FILES

If you plan to redistribute the redistributable files as part of your application, the files in the "redistributable" directory need to be installed with your application.  Note that for Windows 2000 compatibility, these files should no longer be installed in the "Windows\System" (W9x) or "Windows\System32" (WinNT/2000/XP). See the documentation for more details.

DOCUMENTATION

There is a PDF help file called ECW_JPEG_2000_SDK.pdf in the installation directory.  It can also be accessed from the Start menu. The default location for this is Start->Programs->Earth Resource Mapping->ECW JPEG 2000 SDK.  This manual contains documentation on the purpose and suggested usage of the SDK, a partial API reference targeting the most useful related programming paradigms, discussion of issues such as enabling unlimited compression and controlling georeferencing information, and example code in C and C++ together with discussion.

You will need the Adobe Acrobat Reader to view the PDF file. This is available for download from: www.adobe.com. 

CHANGES FROM ECW JPEG 2000 SDK v3.1/3.2

Numerous bug fixes, including:

1.  functional ECWP on platforms other than Windows 
2.  a fix to lossless JPEG 2000 compression 
3.  change to headers to allow overloading new and delete on Windows 
4.  correct calculation of compression throughput 
5.  large file support issues fixed 
6.  fix to UNICODE support making it orthogonal to the use of UNICODE #define in client code 
7.  fixes to the platform-dependent code for MacOS X 
8.  fix to a bug where scanline reading was not reset between views in some situations 
9.  fix to a bug where RGB reads were corrupt when reading bands one by one 
10. fix to ensure EOC (end of codestream) markers always written to JPEG 2000 output 
11. fix to ensure consistency of view information when reading in tiled mode 
12. fix to a bug where progressive (refresh callback) read mode was broken for tiled views 
13. missing documentation items restored, and documentation updated
14. fix to a bug caused by gcc optimisation manifesting as resampling errors on Linux
15. fix to many potential race conditions in threading code, improving stability

Product enhancements, including:

1.  no more restrictions on the distribution of 'Server Software' under the GPL-style Public Use License Agreement 
2.  optional and runtime configurable auto-scaling of low bitdepth JPEG 2000 data to fit the output buffer data range 
3.  unrestricted view sizes for views obtained via ECWP in blocking read mode 
4.  runtime configurable maximum size for views set in progressive read mode 
5.  automated support for single bit opacity bands providing a null cell mechanism 
6.  functional static library builds for NCSEcw, NCSUtil, and NCScnet and for all SDK code as a single linkable object 
7.  a new build structure called 'libecwj2' providing single-library shared and static builds 
8.  libecwj2 makefiles for gcc/g++ on Solaris, Linux and Mac OS X 
9.  inclusion of the sample code that formerly shipped with the binary distribution
10. Unicode filename support on Windows

CHANGES FROM ECW JPEG 2000 SDK v3.0

1.  Full source code for the SDK is available in a separate zip file download.  See the download page on www.ermapper.com for details.
2.  Improved performance and resource usage for tiled compressions.
3.  Temporary files generated during compression are stored in a more compact and faster way.
4.  Unlimited and limited distributions merged to reflect the availability of a GPL-style license for unlimited compression to JPEG 2000 and ECW in applications built on the ECW JPEG 2000 SDK.
5.  Support for compressing to RPCL added, which is now also the default.
6.  Improved GeoJP2 header box support, and related improvements to the API for georeferencing conversions.
7.  Support for custom IO streams on JPEG 2000 read/write via an overloaded CNCSFile::Open method.  This allows developers to use in-memory sources for reading of JPEG 2000 files and in-memory destinations for JPEG 2000 output, and to write customised metadata wrappers for the JPEG 2000 format (e.g. the NITF NPJE standards).

CHANGES FROM DECOMPRESSION AND COMPRESSION SDKs 2.46

1.  The ECW JPEG2000 SDK Version 3.1 supports Compression and Decompression of JPEG2000 images.  
2.  Georeferencing information in the form of PCS/EPSG codes embedded in a JP2 UUID Box will be read and converted to standard ECW GDT projection/Datum codes.  "User-Defined" PCS/EPSG codes are NOT supported.
3.  Georeferencing information in the form of GML embedded in a JP2 XML Box will be read and converted to standard ECW GDT projection/Datum codes, and written on compression.  "User-Defined" PCS/EPSG codes are supported.  By default, GML georeferencing information takes precedence over "GeoJP2" UUID box information.
4.  The SDK now includes the ability to compress to NITF BIIF NPJE and NITF BIIF NPJE profiles.
5.  You can specify precinct size, progression order, tile dimensions, # layers and # levels on JPEG2000 compression.

NOTES ON JPEG2000

The JPEG2000 support in this SDK is intended to be transparent to the majority of software developers.  In effect this means the API is 100% compatible with the previous ECW SDK APIs, including the ability to extract view subsets, setting configuration options, and blocking/progressive image update.  The "C" based API is BINARY compatible with existing applications using the ECW SDK.  Developers requiring access to JP2 specifics will be able to access these through extended APIs, including access to all metadata boxes (XML, UUID).

The SDK implements a row-based decoder as outlined in the JPEG2000 specification section J.6, being appropriately inverted for decoding.  Thus tiled images are NOT required for decoding JPEG2000 images larger than physical memory.  The SDK will cache certain information to improve performance, the amount of memory used for caching defaults to 25% of total physical memory in the system, and can be configured using NCSecwSetConfig(NCSCFG_CACHE_MAXMEM, UINT32 nBytes).

The W9X7 pipeline has been implemented in full IEEE4 floating point rather than using fixed-point maths, allowing utilisation of numerous SSE2 IEEE4 FP optimisations.  Numerous MMX, SSE and SSE2 optimisations have been done to improve decoding performance, including ICT, RCT, DC Shifting, iDWT (W5X3 only in this beta), and YCbCr->sRGB conversion.

Currently images are always converted to sRGB space if in another color space (eg, YCbCr->sRGB).  Paletted images are always decoded to the individual channels, usually RGB, and will be reported as RGB images.  The next beta will enable the raw components to be read for applications requiring this.

The SDK will read ISO/IEC 15444-1 (Part 1) compliant codestreams from .jp2, .j2k, .jpx, .jpc, .j2c and .jpf files.  The decoder is currently Class 2 compliant on all conformance data excepting Profile 2 codestream 6.

Compression to JPEG2000 is supported using the same API as was previously available for ECW.  The SDK compression functions determine whether to compress to ECW or JPEG2000 based on output filename extension.  The target compression ratio specified during compression will have a different effect depending on which file format is is being used.  A target compression ratio of 1 when using JPEG2000 will cause the SDK compression functions to compress raster data losslessly.  ECW compression does not include a lossless option.

JPEG2000 over ECWP support is fully implemented in this version of the ECW JPEG 2000 SDK.  You can see demonstration pages at demo.ermapper.com.

Test JP2 images are available from several sources, including http://www.remotesensing.org/jpeg2000 and http://www.jpeg.org/jpeg2000, and the ER Mapper sponsored BitTorrent site http://www.geotorrent.org.

REPORTING PROBLEMS

Any problems that you encounter in using this product should be sent to your nearest region:

Release Versions (product downloaded from public website):
	Americas (Canada to Peru)		support@ermapper.com
	EAME (Europe, Africa, Middle East)	support@ermapper.co.uk
	Asia Pacific (Australia, Asia)		support@ermapper.com.au

Beta Versions (product downloaded from beta website):
	http://www.ermapper.com/support/beta/

Support queries can also be directed to our new community forums at http://forum.ermapper.com

Please include the following in your bug reports:
1) PRODUCT
2) VERSION
3) COMPUTER AND O\S DETAILS
4) SUMMARY \ DESCRIPTION
5) STEPS TO REPRODUCE THE PROBLEM
6) SAMPLE DATA (JPEGs for error snapshots, small ECW or ER Mapper files, etc)
7) THINGS THAT YOU TRIED YOURSELF TO FIX THE PROBLEM OR YOUR ANALYSIS
