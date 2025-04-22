# Project Name
TARGET = Cosmos

# Sources
CPP_SOURCES = Cosmos.cpp Filter.cpp

# Library Locations
# run make in these folders first
LIBDAISY_DIR = ./libDaisy
DAISYSP_DIR = ./DaisySP

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
