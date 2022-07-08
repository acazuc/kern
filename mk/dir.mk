all: $(DIRS)

$(DIRS):
	make -C $@

clean:

.PHONY: $(DIRS)
