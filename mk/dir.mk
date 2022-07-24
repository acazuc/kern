all: $(DIRS)

$(DIRS):
	@make -C $@

$(addprefix clean_,$(DIRS)):
	@make -C $(@:clean_%=%) clean

clean: $(addprefix clean_,$(DIRS))

.PHONY: $(DIRS) clean $(addprefix clean_,$(DIRS))
