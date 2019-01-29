# Shared
include ../environ.mk

src = src/atla.c src/hashtable.c
obj = $(src:.c=.o)
dep = $(obj:.o=.d)  # one dependency file for each source
INCLUDES = -I ./include

.PHONY: clean all

atla.a: $(obj)
	@echo Linking $@
	@$(LDLIB) /OUT:$@ $(LDLIBFLAGS) $^

all: atla.a

clean:
	@$(RM) $(subst /,$(PSEP),src/*.o) && $(RM) $(subst /,$(PSEP),src/*.d) && $(RM) *.a

.c.o:
	@echo $<
	@$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

-include $(dep)   # include all dep files in the makefile

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.c
	@$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) $< -MM -MT $(@:.d=.o) >$@

