#
# include the standard macros
#
include ../../NCSNightlyBuild/makefile.inc


#
# Name of this library (release target)
#
TARGET=	$(NCS_LIB_PATH)/libNCSUtil

#
# Source files for this library
#
SRC=	NCSThread.cpp \
	NCSBase64.cpp \
  	log.cpp \
  	NCSLog.cpp \
  	NCSObject.cpp \
  	NCSMutex.cpp \
	CNCSBase64Coder.cpp \
	NCSEvent.cpp \
	NCSError.cpp \
	dynamiclib.c \
	error.c \
	file.c \
	malloc.c \
	mutex.c \
	pool.c \
	queue.c \
	NCSPrefs.cpp \
	NCSPrefsXML.cpp \
	thread.c \
	util.c \
	NCSString.cpp \
	NCSPoint.cpp \
	NCSWorldPoint.cpp \
	NCSScreenPoint.cpp

LIBS += -lpthread
#
# include the lib make target macros
#
include ../../NCSNightlyBuild/makelibs.inc
