#
# Man page makefile for the CUPS commands.
#
# Copyright © 2022 by OpenPrinting.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

include ../Makedefs


#
# Man pages...
#

MAN1	=	cancel.1 \
		cups.1 \
		lp.1 \
		lpoptions.1 \
		lpq.1 \
		lprm.1 \
		lpr.1 \
		lpstat.1
MAN8	=	cupsaccept.8 \
		cupsenable.8 \
		lpadmin.8 \
		lpmove.8 \
		lpc.8


#
# Dummy all target...
#

all:


#
# Dummy unit test target...
#

test:


#
# Dummy clean target...
#

clean:


#
# Dummy depend target...
#

depend:


#
# Install all targets...
#

install:
	echo Installing man pages to $(BUILDROOT)$(mandir)/man1...
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(mandir)/man1
	for file in $(MAN1); do \
		$(INSTALL_MAN) $$file $(BUILDROOT)$(mandir)/man1; \
	done
	echo Installing man pages to $(BUILDROOT)$(mandir)/man8...
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(mandir)/man8
	for file in $(MAN8); do \
		$(INSTALL_MAN) $$file $(BUILDROOT)$(mandir)/man8; \
	done
	$(RM) $(BUILDROOT)$(mandir)/man8/cupsdisable.8
	$(LN) cupsenable.8 $(BUILDROOT)$(mandir)/man8/cupsdisable.8
	$(RM) $(BUILDROOT)$(mandir)/man8/cupsreject.8
	$(LN) cupsaccept.8 $(BUILDROOT)$(mandir)/man8/cupsreject.8


#
# Uninstall files...
#

uninstall:
	echo Uninstalling man pages from $(BUILDROOT)$(mandir)/man1...
	for file in $(MAN1); do \
		$(RM) $(BUILDROOT)$(mandir)/man1/$$file; \
	done
	-$(RMDIR) $(BUILDROOT)$(mandir)/man1
	echo Uninstalling man pages from $(BUILDROOT)$(mandir)/man8...
	for file in $(MAN8) cupsenable.8 cupsreject.8; do \
		$(RM) $(BUILDROOT)$(mandir)/man8/$$file; \
	done
	-$(RMDIR) $(BUILDROOT)$(mandir)/man8
