#
# include the standard macros
#
include ../../../NCSNightlyBuild/makefile.inc

#
# Add additional source dirs
#
VPATH=../shared_src

#
# Link to these NCS libraries
#
NCS_LIBS= -lNCSUtil

#
# Name of this library (release target)
#
TARGET= $(NCS_LIB_PATH)/libNCSCNet

#
# Source files for this library
#
SRC=	NCScnet.cpp \
	NCSGetPasswordDlg.cpp \
	NCSPostRequest.cpp \
	NCSSocket.cpp \
	NCSProxy.cpp \
	NCSGetRequest.cpp \
	NCSRequest.cpp

#
# Additional system libs for this target
#
ifeq ($(NCS_PLATFORM), solaris)
LIBS+=  -lsocket -lnsl
endif

#
# include the lib make target macros
#
include ../../../NCSNightlyBuild/makelibs.inc
