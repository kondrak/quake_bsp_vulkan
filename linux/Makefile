help:
	@echo "make debug|release|cleandebug|cleanrelease|cleanall"

debug:
	@+make -f debug.mk

release:
	@+make -f release.mk

cleandebug:
	@make -f debug.mk clean

cleanrelease:
	@make -f release.mk clean

cleanall: cleandebug cleanrelease

.PHONY: help debug release cleandebug cleanrelease cleanall
