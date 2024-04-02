# Copyright 2011 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(ihf): When X on Nyan is gone, simplify this makefile,
#            specifically the *_PORTABLE and *_X_ONLY bits.

USE_X = $(filter X, $(USE))
ifeq ($(GRAPHICS_BACKEND),OPENGLES)
    USE_GLES = y
else
    USE_GLES = $(filter opengles, $(USE))
endif

ifneq ($(USE_X),)
    ifneq ($(USE_GLES),)
        PLATFORM = PLATFORM_X11_EGL
    else
        PLATFORM = PLATFORM_GLX
    endif
else
    PLATFORM = PLATFORM_NULL
endif

SOURCES_GL_BENCH = main.cc yuvtest.cc testbase.cc
SOURCES_GL_BENCH += glinterfacetest.cc contexttest.cc swaptest.cc
SOURCES_GL_BENCH += readpixeltest.cc
SOURCES_GL_BENCH += attributefetchtest.cc varyingsandddxytest.cc cleartest.cc
SOURCES_GL_BENCH += texturetest.cc texturereusetest.cc textureupdatetest.cc
SOURCES_GL_BENCH += textureuploadtest.cc trianglesetuptest.cc fillratetest.cc
SOURCES_GL_BENCH += windowmanagercompositingtest.cc drawsizetest.cc
SOURCES_GL_BENCH += texturerebind.cc
SOURCES_GL_BENCH += md5.cc png_helper.cc utils.cc waffle_stuff.cc filepath.cc
SOURCES_GL_BENCH += bufferstreamingtest.cc
SOURCES_GL_BENCH += bufferuploadtest.cc bufferuploadsubtest.cc
SOURCES_GL_BENCH += querygettest.cc

SOURCES_COMPOSITORTEST = compositortest.cc workload.cc

SOURCES_WINDOWMANAGERTEST = windowmanagertest.cc utils.cc waffle_stuff.cc \
    filepath.cc

PKG_CONFIG ?= pkg-config
PC_DEPS = libpng
PC_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PC_DEPS))
PC_LIBS := $(shell $(PKG_CONFIG) --libs $(PC_DEPS))

CXXFLAGS = -g -Wall -Werror -std=gnu++11
CPPFLAGS += $(PC_CFLAGS)
LDLIBS = $(PC_LIBS) -lgflags

GL_BENCH = ../glbench
COMPOSITORTEST = ../compositortest
WINDOWMANAGERTEST = ../windowmanagertest

PLATFORM_PKGS = waffle-1 epoxy
ifeq ($(PLATFORM),PLATFORM_GLX)
    PLATFORM_PKGS += x11
endif
ifeq ($(PLATFORM),PLATFORM_X11_EGL)
    PLATFORM_PKGS += egl
endif
PLATFORM_CFLAGS = -DPLATFORM=$(PLATFORM)
PLATFORM_CFLAGS += $(shell $(PKG_CONFIG) --cflags $(PLATFORM_PKGS))
PLATFORM_LIBS = $(shell $(PKG_CONFIG) --libs $(PLATFORM_PKGS))

ifneq ($(USE_GLES),)
CPPFLAGS += -DUSE_OPENGLES
LDLIBS += -lGLESv2
else
CPPFLAGS += -DUSE_OPENGL
LDLIBS += -lGL
endif

CPPFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

SOURCES_ALL = $(sort $(SOURCES_GL_BENCH) \
                     $(SOURCES_COMPOSITORTEST) \
                     $(SOURCES_WINDOWMANAGERTEST))

OBJS_GL_BENCH = $(SOURCES_GL_BENCH:.cc=.o)
OBJS_COMPOSITORTEST = $(SOURCES_COMPOSITORTEST:.cc=.o)
OBJS_WINDOWMANAGERTEST = $(SOURCES_WINDOWMANAGERTEST:.cc=.o)
OBJS_ALL = $(SOURCES_ALL:.cc=.o)
DEPS_ALL = $(SOURCES_ALL:.cc=.d)

.PHONY: all clean

EXE_PORTABLE = $(GL_BENCH) $(COMPOSITORTEST) $(WINDOWMANAGERTEST)
OBJ_PORTABLE = $(sort $(OBJS_GL_BENCH) \
                      $(OBJS_COMPOSITORTEST) \
                      $(OBJS_WINDOWMANAGERTEST))

all:: $(EXE_PORTABLE)
ifneq ($(USE_X),)
all:: $(EXE_X_ONLY)
endif

$(GL_BENCH): $(OBJS_GL_BENCH)
$(COMPOSITORTEST): $(OBJS_COMPOSITORTEST)
$(WINDOWMANAGERTEST): $(OBJS_WINDOWMANAGERTEST)

clean:
	$(RM) $(GL_BENCH) $(COMPOSITORTEST) $(WINDOWMANAGERTEST)
	$(RM) $(OBJS_ALL) $(DEPS_ALL)
	$(RM) *.o *.d .version

$(EXE_PORTABLE):
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS) $(PLATFORM_LIBS)

$(EXE_X_ONLY):
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS) -lX11 -lrt

$(OBJ_PORTABLE): %.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(PLATFORM_CFLAGS) -c $< -o $@ -MMD

$(OBJS_X_ONLY): %.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@ -MMD

-include $(DEPS_ALL)
