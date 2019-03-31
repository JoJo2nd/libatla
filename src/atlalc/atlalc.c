#include "getopt_port/getopt.h"
#include "minfs.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdlib.h>

enum {
  atla_type_none = 0,

  atla_type_int8 = 0x1000,
  atla_type_int16,
  atla_type_int32,
  atla_type_int64,

  atla_type_uint8,
  atla_type_uint16,
  atla_type_uint32,
  atla_type_uint64,

  atla_type_float,
  atla_type_double,

  atla_type_user,
};

enum {
  atla_field_deprecated = 0x01,
  atla_field_pointer = 0x02,
};

typedef struct atlatype_t {
  char const* name;
  int builtIn;
  uint32_t firstField;
} atlatype_t;

typedef struct atlafield_t {
  int type;
  uint32_t vAdd,
    vRem,
    owner,
    nextField,
    sizeField,
    flags;
} atlafield_t;

typedef struct atlalc_t {
  uint32_t writeVersion,
    fieldCount, 
    fieldRes,
    typeCount,
    typeRes;
  atlatype_t* types;
  atlafield_t* fields;
} atlalc_t;

static atlalc_t atla;

static uint32_t alc_add_type(atlalc_t* a) {
  uint32_t type = a->typeCount;
  a->typeCount++;
  if (a->typeCount == a->typeRes) {
    a->typeRes += 1024;
    a->types = realloc(a->types, sizeof(atlafield_t)*a->typeRes);
  }
  return type;
}

static uint32_t alc_add_field(atlalc_t* a) {
  uint32_t type = a->fieldCount;
  a->fieldCount++;
  if (a->fieldCount == a->fieldRes) {
    a->fieldRes += 1024;
    a->types = realloc(a->types, sizeof(atlafield_t)*a->fieldRes);
  }
  return type;
}

static int l_create_type(lua_State *l) {
  // expected args: type name, 
  uint32_t* ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_type(&atla); 
  // TODO: fill out data
  luaL_getmetatable(l, "atla.type"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2); // s: user-data
  return 1;
}

static int l_add_field(lua_State *l) {
  // pointer is handled separately, fixed array is handled separately
  // expected args: owner:alta.type, version:int, fieldname:string, fieldtype:atla.type, [isPtr:bool, ptrcount:atla.field]
  uint32_t* ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_field(&atla);
  // TODO: fill out data
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2); // s: user-data
  return 1;
}

static int l_deprecated_field(lua_State *l) {
  uint32_t* ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_field(&atla);
  // TODO: fill out data
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2); // s: user-data
  return 1;
}

int luaopen_atla(lua_State *l) {
  struct luaL_Reg const lib[] = {
    {"create_type", l_create_type},
    {"add_field", l_add_field},
    {"deprecated_field", l_deprecated_field},
    {NULL, NULL}
  };
  uint32_t* ud_idx;
  atlatype_t* atype;

  // Initialise the atla data
  atla.fieldRes = 1024;
  atla.typeRes = 1024;
  atla.typeCount = 1;
  atla.fieldCount = 1;
  atla.types = malloc(sizeof(atlatype_t)*atla.typeRes);
  atla.fields = malloc(sizeof(atlafield_t)*atla.fieldRes);
  // create the null entries
  memset(atla.types, 0, sizeof(atlatype_t));
  memset(atla.fields, 0, sizeof(atlafield_t));

  luaL_newmetatable(l, "atla.type");
  luaL_newmetatable(l, "atla.field");
  luaL_newlib(l, lib);
  // Add common constants to our library table
  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  // init type data
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_int8;
  atype->name = strdup("uint8");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2); // s: lib-tbl, user-data
  lua_setfield(l, -2, "uint8"); // s: lib-tbl

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  // init type data
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_int8;
  atype->name = strdup("uint16");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2); // s: lib-tbl, user-data
  lua_setfield(l, -2, "uint16"); // s: lib-tbl
  // NOTE: this is how math add some number constants. Can use for common types, etc
  // lua_pushnumber(L, PI);
  // lua_setfield(L, -2, "pi");
  // lua_pushnumber(L, HUGE_VAL);
  // lua_setfield(L, -2, "huge");

  return 1;
}

static int verbose_flag;

static struct option long_options[] = {
  /* These options set a flag. */
  {"verbose", no_argument, &verbose_flag, 1},
  /* These options don’t set a flag. We distinguish them by their indices. */
  {"input", required_argument, 0, 'i'},
  {"output", required_argument, 0, 'o'},
  {"version", required_argument, 0, 'w'},
  {0, 0, 0, 0}
};

int main(int argc, char** argv) {
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  luaL_requiref(L, "atla", luaopen_atla, 1);
  lua_pop(L, 1);  // remove lib 

  for (;;) {
    // getopt_long stores the option index here.
    int option_index = 0;

    int c = gop_getopt_long(argc, argv, "w:", long_options, &option_index);
    if (c == -1) break;
    switch (c) {
    case 'w': atla.writeVersion = atoi(optarg); break;
    case '?':
    default: exit(1);
    }
  }

  // parse all inputs
  for (int i = optind; i < argc; ++i) {

    //fprintf(stderr, "Expected argument after options\n");
  }

  lua_close(L);
  return 0;
}