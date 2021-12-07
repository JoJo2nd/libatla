

QUIET?=@
ATLA_SMD_UTIL_DIR?=../smd
ATLALC?= atlac
PSEP?=/

src = $(ATLA_SRC_PREFIX)src/atla.c $(ATLA_SRC_PREFIX)src/hashtable.c
obj = $(src:.c=.o)
dep = $(obj:.o=.d)  # one dependency file for each source

altalc_src = $(ATLA_SRC_PREFIX)src/atlalc/atlalc.c
altalc_obj = $(altalc_src:.c=.o)
altalc_dep = $(altalc_obj:.o=.d)  # one dependency file for each source

INCLUDES = -I $(ATLA_SRC_PREFIX)include -I $(ATLA_SMD_UTIL_DIR) -I $(ATLA_SRC_PREFIX)external/lua-5.2.4/src

LUAPRFIX = $(ATLA_SRC_PREFIX)external/lua-5.2.4/src/

LUA_A=	$(LUAPRFIX)liblua.a
CORE_O=	$(LUAPRFIX)lapi.o $(LUAPRFIX)lcode.o $(LUAPRFIX)lctype.o $(LUAPRFIX)ldebug.o $(LUAPRFIX)ldo.o $(LUAPRFIX)ldump.o $(LUAPRFIX)lfunc.o $(LUAPRFIX)lgc.o $(LUAPRFIX)llex.o \
	$(LUAPRFIX)lmem.o $(LUAPRFIX)lobject.o $(LUAPRFIX)lopcodes.o $(LUAPRFIX)lparser.o $(LUAPRFIX)lstate.o $(LUAPRFIX)lstring.o $(LUAPRFIX)ltable.o \
	$(LUAPRFIX)ltm.o $(LUAPRFIX)lundump.o $(LUAPRFIX)lvm.o $(LUAPRFIX)lzio.o
LIB_O=	$(LUAPRFIX)lauxlib.o $(LUAPRFIX)lbaselib.o $(LUAPRFIX)lbitlib.o $(LUAPRFIX)lcorolib.o $(LUAPRFIX)ldblib.o $(LUAPRFIX)liolib.o \
	$(LUAPRFIX)lmathlib.o $(LUAPRFIX)loslib.o $(LUAPRFIX)lstrlib.o $(LUAPRFIX)ltablib.o $(LUAPRFIX)loadlib.o $(LUAPRFIX)linit.o
BASE_O= $(CORE_O) $(LIB_O) $(MYOBJS)

LUA_T=	lua
LUA_O=	$(LUAPRFIX)lua.o

LUAC_T=	luac
LUAC_O=	$(LUAPRFIX)luac.o

ALL_O= $(BASE_O) $(LUA_O) $(LUAC_O)
ALL_T= $(LUA_A) $(LUA_T) $(LUAC_T)
ALL_A= $(LUA_A)

	
all: $(ATLALC) $(LIBATLA)

ifeq ($(OS), Windows_NT)

$(ATLALC): $(altalc_obj) $(LUA_A) $(LIBSMD)
	@echo $(mkfile_path) Linking $@
	 $(LD) -subsystem:console -out:$@ $(ATLA_LDFLAGS) $^ 

$(LIBATLA): $(obj) 
	@echo Linking $@
	$(QUIET) $(LD) /lib -out:$@ $(ATLA_LDLIBFLAGS) $^

$(LUA_A): $(BASE_O)
	@echo Linking $@
	$(QUIET) $(LD) /lib -out:$@ $(ATLA_LDLIBFLAGS) $^

else

$(ATLALC): $(altalc_obj) $(LUA_A) $(LIBSMD)
	@echo $(mkfile_path) Linking $@
	$(QUIET) $(CC) $(ATLA_LDFLAGS) $^ -o $@	

$(LIBATLA): $(obj) 
	@echo Linking $@
	$(QUIET) $(AR) -r $@ $(ATLA_LDLIBFLAGS) $^

$(LUA_A): $(BASE_O)
	@echo Linking $@
	$(QUIET) $(AR) -r $@ $(ATLA_LDLIBFLAGS) $^

endif	
.PHONY: clean all

clean:
	$(QUIET) rm -f src/*.o
	$(QUIET) rm -f src/*.d
	$(QUIET) rm -f src/atlalc/*.o
	$(QUIET) rm -f src/atlalc/*.d
	$(QUIET) rm -f $(LIBATLA) 
	$(QUIET) rm -f $(ATLALC)

.c.o:
	$(QUIET) $(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@


$(LUAPRFIX)lapi.o: $(LUAPRFIX)lapi.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lapi.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lstate.h $(LUAPRFIX)lobject.h $(LUAPRFIX)ltm.h \
 $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lfunc.h $(LUAPRFIX)lgc.h $(LUAPRFIX)lstring.h $(LUAPRFIX)ltable.h $(LUAPRFIX)lundump.h \
 $(LUAPRFIX)lvm.h
$(LUAPRFIX)lauxlib.o: $(LUAPRFIX)lauxlib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h
$(LUAPRFIX)lbaselib.o: $(LUAPRFIX)lbaselib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)lbitlib.o: $(LUAPRFIX)lbitlib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)lcode.o: $(LUAPRFIX)lcode.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lcode.h $(LUAPRFIX)llex.h $(LUAPRFIX)lobject.h $(LUAPRFIX)llimits.h \
 $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)lopcodes.h $(LUAPRFIX)lparser.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)lstate.h $(LUAPRFIX)ltm.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lgc.h \
 $(LUAPRFIX)lstring.h $(LUAPRFIX)ltable.h $(LUAPRFIX)lvm.h
$(LUAPRFIX)lcorolib.o: $(LUAPRFIX)lcorolib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)lctype.o: $(LUAPRFIX)lctype.c $(LUAPRFIX)lctype.h $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)llimits.h
$(LUAPRFIX)ldblib.o: $(LUAPRFIX)ldblib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)ldebug.o: $(LUAPRFIX)ldebug.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lapi.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lstate.h $(LUAPRFIX)lobject.h \
 $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)lcode.h $(LUAPRFIX)llex.h $(LUAPRFIX)lopcodes.h $(LUAPRFIX)lparser.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)ldo.h \
 $(LUAPRFIX)lfunc.h $(LUAPRFIX)lstring.h $(LUAPRFIX)lgc.h $(LUAPRFIX)ltable.h $(LUAPRFIX)lvm.h
$(LUAPRFIX)ldo.o: $(LUAPRFIX)ldo.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lapi.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lstate.h $(LUAPRFIX)lobject.h $(LUAPRFIX)ltm.h \
 $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lfunc.h $(LUAPRFIX)lgc.h $(LUAPRFIX)lopcodes.h $(LUAPRFIX)lparser.h \
 $(LUAPRFIX)lstring.h $(LUAPRFIX)ltable.h $(LUAPRFIX)lundump.h $(LUAPRFIX)lvm.h
$(LUAPRFIX)ldump.o: $(LUAPRFIX)ldump.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lobject.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lstate.h $(LUAPRFIX)ltm.h \
 $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)lundump.h
$(LUAPRFIX)lfunc.o: $(LUAPRFIX)lfunc.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lfunc.h $(LUAPRFIX)lobject.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lgc.h \
 $(LUAPRFIX)lstate.h $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h
$(LUAPRFIX)lgc.o: $(LUAPRFIX)lgc.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)lstate.h $(LUAPRFIX)lobject.h $(LUAPRFIX)llimits.h $(LUAPRFIX)ltm.h \
 $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lfunc.h $(LUAPRFIX)lgc.h $(LUAPRFIX)lstring.h $(LUAPRFIX)ltable.h
$(LUAPRFIX)linit.o: $(LUAPRFIX)linit.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lualib.h $(LUAPRFIX)lauxlib.h
$(LUAPRFIX)liolib.o: $(LUAPRFIX)liolib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)llex.o: $(LUAPRFIX)llex.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lctype.h $(LUAPRFIX)llimits.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lobject.h \
 $(LUAPRFIX)lstate.h $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)llex.h $(LUAPRFIX)lparser.h $(LUAPRFIX)lstring.h $(LUAPRFIX)lgc.h $(LUAPRFIX)ltable.h
$(LUAPRFIX)lmathlib.o: $(LUAPRFIX)lmathlib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)lmem.o: $(LUAPRFIX)lmem.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)lstate.h $(LUAPRFIX)lobject.h $(LUAPRFIX)llimits.h \
 $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lgc.h
$(LUAPRFIX)loadlib.o: $(LUAPRFIX)loadlib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)lobject.o: $(LUAPRFIX)lobject.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lctype.h $(LUAPRFIX)llimits.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)lstate.h \
 $(LUAPRFIX)lobject.h $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lstring.h $(LUAPRFIX)lgc.h $(LUAPRFIX)lvm.h
$(LUAPRFIX)lopcodes.o: $(LUAPRFIX)lopcodes.c $(LUAPRFIX)lopcodes.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h
$(LUAPRFIX)loslib.o: $(LUAPRFIX)loslib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)lparser.o: $(LUAPRFIX)lparser.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lcode.h $(LUAPRFIX)llex.h $(LUAPRFIX)lobject.h $(LUAPRFIX)llimits.h \
 $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)lopcodes.h $(LUAPRFIX)lparser.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)lstate.h $(LUAPRFIX)ltm.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lfunc.h \
 $(LUAPRFIX)lstring.h $(LUAPRFIX)lgc.h $(LUAPRFIX)ltable.h
$(LUAPRFIX)lstate.o: $(LUAPRFIX)lstate.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lapi.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lstate.h $(LUAPRFIX)lobject.h \
 $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lfunc.h $(LUAPRFIX)lgc.h $(LUAPRFIX)llex.h $(LUAPRFIX)lstring.h \
 $(LUAPRFIX)ltable.h
$(LUAPRFIX)lstring.o: $(LUAPRFIX)lstring.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lmem.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lobject.h $(LUAPRFIX)lstate.h \
 $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h $(LUAPRFIX)lstring.h $(LUAPRFIX)lgc.h
$(LUAPRFIX)lstrlib.o: $(LUAPRFIX)lstrlib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)ltable.o: $(LUAPRFIX)ltable.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)lstate.h $(LUAPRFIX)lobject.h $(LUAPRFIX)llimits.h \
 $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lgc.h $(LUAPRFIX)lstring.h $(LUAPRFIX)ltable.h $(LUAPRFIX)lvm.h
$(LUAPRFIX)ltablib.o: $(LUAPRFIX)ltablib.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)ltm.o: $(LUAPRFIX)ltm.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lobject.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lstate.h $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h \
 $(LUAPRFIX)lmem.h $(LUAPRFIX)lstring.h $(LUAPRFIX)lgc.h $(LUAPRFIX)ltable.h
$(LUAPRFIX)lua.o: $(LUAPRFIX)lua.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lualib.h
$(LUAPRFIX)luac.o: $(LUAPRFIX)luac.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)lauxlib.h $(LUAPRFIX)lobject.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lstate.h \
 $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)lundump.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)lopcodes.h
$(LUAPRFIX)lundump.o: $(LUAPRFIX)lundump.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)lstate.h $(LUAPRFIX)lobject.h \
 $(LUAPRFIX)llimits.h $(LUAPRFIX)ltm.h $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lfunc.h $(LUAPRFIX)lstring.h $(LUAPRFIX)lgc.h $(LUAPRFIX)lundump.h
$(LUAPRFIX)lvm.o: $(LUAPRFIX)lvm.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)ldebug.h $(LUAPRFIX)lstate.h $(LUAPRFIX)lobject.h $(LUAPRFIX)llimits.h $(LUAPRFIX)ltm.h \
 $(LUAPRFIX)lzio.h $(LUAPRFIX)lmem.h $(LUAPRFIX)ldo.h $(LUAPRFIX)lfunc.h $(LUAPRFIX)lgc.h $(LUAPRFIX)lopcodes.h $(LUAPRFIX)lstring.h $(LUAPRFIX)ltable.h $(LUAPRFIX)lvm.h
$(LUAPRFIX)lzio.o: $(LUAPRFIX)lzio.c $(LUAPRFIX)lua.h $(LUAPRFIX)luaconf.h $(LUAPRFIX)llimits.h $(LUAPRFIX)lmem.h $(LUAPRFIX)lstate.h $(LUAPRFIX)lobject.h $(LUAPRFIX)ltm.h \
 $(LUAPRFIX)lzio.h
