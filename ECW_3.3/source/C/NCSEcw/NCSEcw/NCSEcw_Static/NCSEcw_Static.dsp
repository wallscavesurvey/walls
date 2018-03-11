# Microsoft Developer Studio Project File - Name="NCSEcw_Static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=NCSEcw_Static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NCSEcw_Static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NCSEcw_Static.mak" CFG="NCSEcw_Static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NCSEcw_Static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "NCSEcw_Static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "NCS/Source/C/NCSEcw/NCSEcw_Static"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NCSEcw_Static - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I "../../../../include" /I "../../lcms/include" /D "NDEBUG" /D "NCSECW_STATIC_LIBS" /D "WIN32" /D "_LIB" /D "NCSECW_EXPORTS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\..\..\lib\NCSEcwS.lib"

!ELSEIF  "$(CFG)" == "NCSEcw_Static - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "../../../include" /I "../../../" /I "../../../../include" /I "../../lcms/include" /D "_DEBUG" /D "WIN32" /D "_LIB" /D "NCSECW_EXPORTS" /D "NCSECW_STATIC_LIBS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\..\..\lib\NCSEcwSd.lib"

!ENDIF 

# Begin Target

# Name "NCSEcw_Static - Win32 Release"
# Name "NCSEcw_Static - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\shared_src\build_pyr.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmscnvrt.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmserr.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmsgamma.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmsgmt.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmsintrp.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmsio1.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmslut.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmsmatsh.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmsmtrx.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmsnamed.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmspack.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmspcs.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmssamp.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmsvirt.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmswtpnt.c
# End Source File
# Begin Source File

SOURCE=..\..\lcms\src\cmsxform.c
# End Source File
# Begin Source File

SOURCE=..\..\shared_src\collapse_pyr.c
# End Source File
# Begin Source File

SOURCE=..\ComNCSRenderer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared_src\compress.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared_src\ecw_open.c
# End Source File
# Begin Source File

SOURCE=..\..\shared_src\ecw_read.c
# End Source File
# Begin Source File

SOURCE=..\..\shared_src\fileio_compress.c
# End Source File
# Begin Source File

SOURCE=..\..\shared_src\fileio_decompress.c
# End Source File
# Begin Source File

SOURCE=..\NCSAffineTransform.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSBlockFile.cpp
# End Source File
# Begin Source File

SOURCE=..\ncscbm.c
# End Source File
# Begin Source File

SOURCE=..\ncscbmidwt.c
# End Source File
# Begin Source File

SOURCE=..\ncscbmnet.c
# End Source File
# Begin Source File

SOURCE=..\ncscbmopen.c
# End Source File
# Begin Source File

SOURCE=..\ncscbmpurge.c
# End Source File
# Begin Source File

SOURCE=..\NCSEcw.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\NCSGDT2\NCSGDTEpsg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\NCSGDT2\NCSGDTEPSGKey.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\NCSGDT2\NCSGDTEpsgPcsKey.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\NCSGDT2\NCSGDTLocation.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSHuffmanCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2BitsPerComponentBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2Box.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2CaptureResolutionBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2ChannelDefinitionBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2ColorSpecificationBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2ComponentMappingBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2ContiguousCodestreamBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2DataEntryURLBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2DefaultDisplayResolutionBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2File.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2FileTypeBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2FileView.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2GMLGeoLocationBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2HeaderBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2ImageHeaderBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2IntellectualPropertyBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2PaletteBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2PCSBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2ResolutionBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2SignatureBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2SuperBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2UUIDBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2UUIDInfoBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2UUIDListBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2WorldBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJP2XMLBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCCOCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCCodeBlock.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCCodingStyleParameter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCCODMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCCOMMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCComponent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCComponentDepthType.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCCRGMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCDCShiftNode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCDump.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCEcwpIOStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCEOCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCEPHMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCFileIOStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCICCNode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCIOStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCMainHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCMCTNode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCMemoryIOStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCMQCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCNode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCNodeTiler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCPacket.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCPacketLengthType.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCPaletteNode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCPLMMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCPLTMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCPOCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCPPMMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCPPTMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCPrecinct.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCProgression.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCProgressionOrderType.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCQCCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCQCDMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCQuantizationParameter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCResample.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCResolution.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCRGNMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCSegment.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCSIZMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCSOCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCSODMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCSOPMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCSOTMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCSubBand.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCT1Coder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCTagTree.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCTilePartHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCTLMMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSJP2\NCSJPCYCbCrNode.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSOutputFile.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSRenderer.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSWorldFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared_src\pack.c
# End Source File
# Begin Source File

SOURCE=..\..\shared_src\qmf_util.c
# End Source File
# Begin Source File

SOURCE=..\..\shared_src\quantize.c
# End Source File
# Begin Source File

SOURCE=..\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared_src\unpack.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
