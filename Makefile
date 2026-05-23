.DEFAULT_GOAL := all

.PHONY: all check benchmark plots clean help

all check benchmark plots clean help:
	$(MAKE) -C lab3 $@
