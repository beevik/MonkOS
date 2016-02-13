#----------------------------------------------------------------------------
# MonkOS documentation makefile
#
# Makefile for the doxygen documentation.
#----------------------------------------------------------------------------

DIR_ROOT := ..

include $(DIR_ROOT)/scripts/config.mk


#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

CONFIG	:= $(DIR_DOCS)/monk.doxy

TAG	:= $(BLUE)[docs]$(NORMAL)

docs: .force $(CONFIG)
	@echo "$(TAG) Running doxygen"
	@$(DOXYGEN) $(CONFIG) > /dev/null
	@echo "$(BLUE)[docs] $(SUCCESS)"

clean:
	@rm -rf $(DIR_DOCS)/monk

.force:
