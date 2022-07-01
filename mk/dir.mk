all: $(DIRS)

$(DIRS):
	make -C $@

.PHONY: $(DIRS)
