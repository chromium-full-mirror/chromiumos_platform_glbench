# Copyright 2019 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This Makefile is intended to be use in a normal Linux environment.

USE = "X"
GRAPHICS_BACKEND = "OPENGL"

prefix ?= /usr/local
BINDIR = $(DESTDIR)$(prefix)/bin
INSTALL ?= install

.PHONY: all clean
all clean:
	$(MAKE) -C src USE=$(USE) GRAPHICS_BACKEND=$(GRAPHICS_BACKEND) $@

.PHONY: install
install:
	mkdir -p $(BINDIR)
	mkdir -p $(BINDIR)/runtime
	$(INSTALL) glbench compositortest windowmanagertest $(BINDIR)
	$(INSTALL) -D runtime/* -t $(BINDIR)/runtime
