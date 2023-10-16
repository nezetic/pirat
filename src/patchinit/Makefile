# Project Name
TARGET = PRat_patchinit

# Sources
CPP_SOURCES += $(wildcard *.cpp ../prat/*.cpp ../noisegate/*.cpp)

# Flags
CPP_STANDARD=-I../commons -I../prat -I../noisegate -DUSE_DAISYSP -std=gnu++14

# Library Locations
LIBDAISY_DIR ?= ../../../libDaisy
DAISYSP_DIR ?= ../../../DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
