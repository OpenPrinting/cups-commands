#
# Locale file makefile for the CUPS commands.
#
# Copyright © 2022 by OpenPrinting.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

include ../Makedefs


# Directory for localization files...
LANGUAGES =	\
		ca.strings \
		cs.strings \
		da.strings \
		de.strings \
		en.strings \
		es.strings \
		fr.strings \
		it.strings \
		ja.strings \
		pt_BR.strings \
		ru.strings \
		zh_CN.strings

LOCALEDIR =	$(BUILDROOT)$(datadir)/locale/cups-commands


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
	$(INSTALL_DIR) -m 755 $(LOCALEDIR)
	for loc in $(LANGUAGES) ; do \
		$(INSTALL_DATA) $$loc $(LOCALEDIR); \
	done


#
# Uninstall files...
#

uninstall:
	-for loc in $(LANGUAGES) ; do \
		$(RM) $(LOCALEDIR)/$$loc; \
	done
	$(RMDIR) $(LOCALEDIR)


#
# Update localization strings - depends on stringsutil from
# <https://github.com/michaelrsweet/stringsutil>.
#

strings:
	echo TODO
