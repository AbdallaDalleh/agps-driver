TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += agps

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install agps.dbd into <top>/dbd
DBD += agps.dbd

# specify all source files to be compiled and added to the library
agps_SRCS += AGPSController.cpp

agps_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

