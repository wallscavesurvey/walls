#
# include the standard macros
# 
include ../../../NCSNightlyBuild/makefile.inc

JAVA_INC_PATH=/usr/java/j2sdk1.4.2_04/include/

#CFLAGS+= -I/opt/java1.4/include -I/opt/java1.4/include/hp-ux
#CFLAGS+= -I/usr/j2se/include -I/usr/j2se/include/solaris
CFLAGS+= -I $(NCSTOP)/../ERM/Source/LIB/ERMAPPER/GEOtiff -I $(NCSTOP)/../ERM/Source/INCLUDE -I $(NCSTOP)/Source/include -I $(NCSTOP)/../ERM/Source/INCLUDE -I $(NCSTOP)/../ERM/SSource/LIB/ERMAPPER/GEOtiff -I$(JAVA_INC_PATH) -I$(JAVA_INC_PATH)/linux -I $(NCSTOP)/Source/C/NCSEcw/lcms/include -D UNICODE -w
CXXFLAGS+= -I $(NCSTOP)/../ERM/Source/LIB/ERMAPPER/GEOtiff -I $(NCSTOP)/../ERM/Source/INCLUDE -I $(NCSTOP)/Source/include -I $(NCSTOP)/../ERM/Source/INCLUDE -I $(NCSTOP)/../ERM/Source/LIB/ERMAPPER/GEOtiff -I$(JAVA_INC_PATH) -I$(JAVA_INC_PATH)/linux -I $(NCSTOP)/Source/C/NCSEcw/lcms/include -D UNICODE -w

#
# Add additional source dirs
#
VPATH= ../shared_src ../NCSJP2 ../../NCSGDT2 ../lcms/src

#
# Link to these NCS libraries
#
NCS_LIBS= -lNCSCNet -lNCSUtil

#
# Name of this library (release target)
#
TARGET=	$(NCS_LIB_PATH)/libNCSEcw

#
# Source files for this library
#
SRC=	NCSEcw.cpp \
	NCSFile.cpp \
	NCSRenderer.cpp \
	ncscbm.c \
	ncscbmidwt.c \
	ncscbmopen.c \
	ncscbmnet.c \
	ncscbmpurge.c \
	build_pyr.c \
	collapse_pyr.c \
	ecw_open.c \
	ecw_read.c \
	fileio_compress.c \
	fileio_decompress.c \
	qmf_util.c \
	unpack.c \
	pack.c \
	quantize.c \
	NCSAffineTransform.cpp \
	NCSBlockFile.cpp \
	NCSGDTEpsg.cpp \
	NCSGDTEPSGKey.cpp \
	NCSGDTEpsgPcsKey.cpp \
	NCSGDTLocation.cpp \
	NCSHuffmanCoder.cpp \
	NCSJP2.cpp \
	NCSJP2BitsPerComponentBox.cpp \
	NCSJP2Box.cpp \
	NCSJP2CaptureResolutionBox.cpp \
	NCSJP2ChannelDefinitionBox.cpp \
	NCSJP2ColorSpecificationBox.cpp \
	NCSJP2ComponentMappingBox.cpp \
	NCSJP2ContiguousCodestreamBox.cpp \
	NCSJP2DataEntryURLBox.cpp \
	NCSJP2DefaultDisplayResolutionBox.cpp \
	NCSJP2File.cpp \
	NCSJP2FileTypeBox.cpp \
	NCSJP2FileView.cpp \
	NCSJP2GMLGeoLocationBox.cpp \
	NCSJP2HeaderBox.cpp \
	NCSJP2ImageHeaderBox.cpp \
	NCSJP2IntellectualPropertyBox.cpp \
	NCSJP2PaletteBox.cpp \
	NCSJP2PCSBox.cpp \
	NCSJP2WorldBox.cpp \
	NCSJP2ResolutionBox.cpp \
	NCSJP2SignatureBox.cpp \
	NCSJP2SuperBox.cpp \
	NCSJP2UUIDBox.cpp \
	NCSJP2UUIDInfoBox.cpp \
	NCSJP2UUIDListBox.cpp \
	NCSJP2XMLBox.cpp \
	NCSJPC.cpp \
	NCSJPCBuffer.cpp \
	NCSJPCCOCMarker.cpp \
	NCSJPCCodeBlock.cpp \
	NCSJPCCodingStyleParameter.cpp \
	NCSJPCCODMarker.cpp \
	NCSJPCCOMMarker.cpp \
	NCSJPCComponent.cpp \
	NCSJPCComponentDepthType.cpp \
	NCSJPCCRGMarker.cpp \
	NCSJPCDCShiftNode.cpp \
	NCSJPCDump.cpp \
	NCSJPCEcwpIOStream.cpp \
	NCSJPCEOCMarker.cpp \
	NCSJPCEPHMarker.cpp \
	NCSJPCFileIOStream.cpp \
	NCSJPCICCNode.cpp \
	NCSJPCIOStream.cpp \
	NCSJPCMainHeader.cpp \
	NCSJPCMarker.cpp \
	NCSJPCMCTNode.cpp \
	NCSJPCMemoryIOStream.cpp \
	NCSJPCMQCoder.cpp \
	NCSJPCNode.cpp \
	NCSJPCNodeTiler.cpp \
	NCSJPCPacket.cpp \
	NCSJPCPacketLengthType.cpp \
	NCSJPCPaletteNode.cpp \
	NCSJPCPLMMarker.cpp \
	NCSJPCPLTMarker.cpp \
	NCSJPCPOCMarker.cpp \
	NCSJPCPPMMarker.cpp \
	NCSJPCPPTMarker.cpp \
	NCSJPCPrecinct.cpp \
	NCSJPCProgression.cpp \
	NCSJPCProgressionOrderType.cpp \
	NCSJPCQCCMarker.cpp \
	NCSJPCQCDMarker.cpp \
	NCSJPCQuantizationParameter.cpp \
	NCSJPCResample.cpp \
	NCSJPCResolution.cpp \
	NCSJPCRGNMarker.cpp \
	NCSJPCSegment.cpp \
	NCSJPCSIZMarker.cpp \
	NCSJPCSOCMarker.cpp \
	NCSJPCSODMarker.cpp \
	NCSJPCSOPMarker.cpp \
	NCSJPCSOTMarker.cpp \
	NCSJPCSubBand.cpp \
	NCSJPCT1Coder.cpp \
	NCSJPCTagTree.cpp \
	NCSJPCTilePartHeader.cpp \
	NCSJPCTLMMarker.cpp \
	NCSJPCYCbCrNode.cpp \
	NCSWorldFile.cpp \
	cmsgamma.c \
	cmslut.c \
	cmspack.c \
	cmsvirt.c \
	cmscam97.c \
	cmsgmt.c \
	cmsmatsh.c \
	cmspcs.c \
	cmswtpnt.c \
	cmscnvrt.c \
	cmsintrp.c \
	cmsmtrx.c \
	cmsps2.c \
	cmsxform.c \
	cmserr.c \
	cmsio1.c \
	cmsnamed.c \
	cmssamp.c

#	ecw_jni.c \
#
# include the lib make target macros
# 
include ../../../NCSNightlyBuild/makelibs.inc
