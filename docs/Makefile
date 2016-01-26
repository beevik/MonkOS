#----------------------------------------------------------------------------
# MonkOS documentation makefile
#
# Makefile for the doxygen documentation.
#----------------------------------------------------------------------------

DIR_ROOT := ..

include $(DIR_ROOT)/config.mk


#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

TAG 		:= $(BLUE)[docs]$(NORMAL)
CONFIG 		:= $(DIR_DOCS)/monk.doxy

docs: .force $(CONFIG)
	@echo "$(BLUE)[docs]$(NORMAL) Running doxygen"
	@$(DOXYGEN) $(CONFIG) > /dev/null

clean:
	@rm -rf $(DIR_DOCS)/monk
