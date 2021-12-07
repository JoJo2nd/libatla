#include "getopt.h"
#include "minfs.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdlib.h>

#define ATLA_MAX_PATH (1024)

#ifdef PLATFORM_WINDOWS
#define strdup local_strdup
static char* local_strdup(char const* s) {
  char* sd = malloc(strlen(s) + 1);
  strcpy(sd, s);
  return sd;
}
#endif

enum {
  atla_type_none = 0,

  atla_type_int8 = 0x1000,
  atla_type_char,
  atla_type_int16,
  atla_type_int32,
  atla_type_int64,

  atla_type_uint8,
  atla_type_uint16,
  atla_type_uint32,
  atla_type_uint64,

  atla_type_float,
  atla_type_double,

  atla_type_enum,

  atla_type_user,
};

enum {
  atla_field_deprecated = 0x01,
  atla_field_pointer = 0x02,
  atla_field_dynamic_array = 0x04,
  atla_field_fixed_array = 0x08,
  atla_field_enum_type = 0x10,
};

typedef struct atlatype_t {
  char const* name;
  uint32_t    id;
  int         builtIn;
  int         lastInc;
  uint32_t    firstField;
  uint32_t    lastField;
  // enum only! i.e. builtIn == atla_type_enum
  char const* prefix;
} atlatype_t;

typedef struct atlafield_t {
  char const* name;
  union {
    int      type;
    uint32_t enumValue;
  };
  uint32_t    id, vAdd, vRem, owner, nextField, sizeField, flags;
} atlafield_t;
/*
typedef struct atlaenum_t {
  char const* name;
  char const* prefix;
  uint32_t    id;
  uint32_t    firstField;
  uint32_t    lastField;
} atlaenum_t;
*/

typedef atlatype_t atlaenum_t;

typedef struct atlalc_t {
  uint32_t writeVersion, fieldCount, fieldRes, typeCount, typeRes,
    firstIntType, lastIntType;
  atlatype_t*  types;
  atlafield_t* fields;
} atlalc_t;

static atlalc_t atla;

static uint32_t alc_add_type(atlalc_t* a) {
  uint32_t type = a->typeCount;
  a->typeCount++;
  if (a->typeCount == a->typeRes) {
    a->typeRes += 1024;
    a->types = realloc(a->types, sizeof(atlatype_t) * a->typeRes);
  }
  return type;
}

static uint32_t alc_add_enum(atlalc_t* a) {
  return alc_add_type(a);
}

static uint32_t alc_add_field(atlalc_t* a) {
  uint32_t type = a->fieldCount;
  a->fieldCount++;
  if (a->fieldCount == a->fieldRes) {
    a->fieldRes += 1024;
    a->types = realloc(a->types, sizeof(atlafield_t) * a->fieldRes);
  }
  return type;
}

static int l_create_type(lua_State* l) {
  // expected args: id, type name
  size_t      type_name_len;
  char const* type_name = luaL_checklstring(l, 2, &type_name_len);
  uint32_t    id = luaL_checkunsigned(l, 1);

  uint32_t* ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_type(&atla);
  // fprintf(stdout, "Adding type %s\n", type_name);
  atlatype_t* atype = atla.types + *ud_idx;
  atype->firstField = atype->lastField = 0;
  atype->builtIn = atla_type_user;
  atype->lastInc = -1;
  atype->id = id;
  atype->name = strdup(type_name);
  atype->prefix = NULL;
  luaL_getmetatable(l, "atla.type"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: user-data
  return 1;
}

static int l_create_enum(lua_State* l) {
  // expected args: id, type name, enum prefix
  size_t      type_name_len;
  uint32_t    id = luaL_checkunsigned(l, 1);
  char const* type_name = luaL_checklstring(l, 2, &type_name_len);
  char const* prefix = luaL_checklstring(l, 3, &type_name_len);

  uint32_t* ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_enum(&atla);
  // fprintf(stdout, "Adding type %s\n", type_name);
  atlaenum_t* atype = atla.types + *ud_idx;
  atype->firstField = atype->lastField = 0;
  atype->id = id;
  atype->builtIn = atla_type_enum;
  atype->prefix = strdup(prefix);
  atype->name = strdup(type_name);
  luaL_getmetatable(l, "atla.enum"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: user-data
  return 1;
}

static int l_enum_value(lua_State* l) {
  // expected args: owner:alta.enum, version:int, fieldname:string,
  // value:int
  uint32_t*   owner_idx = luaL_checkudata(l, 1, "atla.enum");
  uint32_t    version = luaL_checkunsigned(l, 2);
  char const* field_name = luaL_checkstring(l, 3);
  uint32_t    value = luaL_checkunsigned(l, 4);

  uint32_t* ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_field(&atla);
  atlaenum_t*  otype = atla.types + *owner_idx;
  atlafield_t* afield = atla.fields + *ud_idx;
  // fprintf(stdout, "Adding field %s\n", field_name);
  afield->name = strdup(field_name);
  afield->enumValue = value;
  afield->vAdd = version;
  afield->vRem = 0;
  afield->owner = *owner_idx;
  afield->sizeField = 0;
  afield->flags = 0;
  afield->nextField = 0;
  if (otype->lastField) {
    atla.fields[otype->lastField].nextField = *ud_idx;
  }
  otype->lastField = *ud_idx;
  if (!otype->firstField) otype->firstField = *ud_idx;
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);            // s: user-data
  lua_pop(l, 1);
  return 0;
}

static int l_enum_deprecated_value(lua_State* l) {
  // expected args: owner:alta.enum, version:int, rem_version:int, id:int, fieldname:string,
  // value:int
  uint32_t*   owner_idx = luaL_checkudata(l, 1, "atla.enum");
  uint32_t    version = luaL_checkunsigned(l, 2);
  uint32_t    rem_version = luaL_checkunsigned(l, 3);
  uint32_t    id = luaL_checkunsigned(l, 4);
  char const* field_name = luaL_checkstring(l, 5);
  uint32_t    value = luaL_checkunsigned(l, 6);

  uint32_t* ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_field(&atla);
  atlaenum_t*  otype = atla.types + *owner_idx;
  atlafield_t* afield = atla.fields + *ud_idx;
  // fprintf(stdout, "Adding field %s\n", field_name);
  afield->name = strdup(field_name);
  afield->id = id;
  afield->enumValue = value;
  afield->vAdd = version;
  afield->vRem = rem_version;
  afield->owner = *owner_idx;
  afield->sizeField = 0;
  afield->flags = atla_field_deprecated;
  afield->nextField = 0;
  if (otype->lastField) {
    atla.fields[otype->lastField].nextField = *ud_idx;
  }
  otype->lastField = *ud_idx;
  if (!otype->firstField) otype->firstField = *ud_idx;
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);            // s: user-data
  lua_pop(l, 1);
  return 0;
}

static int l_field(lua_State* l) {
  // pointer is handled separately, fixed array is handled separately
  // expected args: owner:alta.type, version:int, id:int, fieldname:string,
  // fieldtype:atla.type, [isPtr:bool, ptrcount:atla.field]
  uint32_t*   owner_idx = luaL_checkudata(l, 1, "atla.type");
  uint32_t    version = luaL_checkunsigned(l, 2);
  uint32_t    id = luaL_checkunsigned(l, 3);
  char const* field_name = luaL_checkstring(l, 4);
  int         is_using_enum = luaL_testudata(l, 5, "atla.enum") ? 1 : 0;
  uint32_t*   type_idx = is_using_enum
                         ? luaL_checkudata(l, 5, "atla.enum")
                         : luaL_checkudata(l, 5, "atla.type");
  uint32_t*   ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_field(&atla);
  atlatype_t*  otype = atla.types + *owner_idx;
  atlafield_t* afield = atla.fields + *ud_idx;
  // fprintf(stdout, "Adding field %s\n", field_name);
  afield->name = strdup(field_name);
  afield->id = id;
  afield->type = *type_idx;
  afield->vAdd = version;
  afield->vRem = 0;
  afield->owner = *owner_idx;
  afield->sizeField = 0;
  afield->flags = 0;
  afield->nextField = 0;
  if (otype->lastField) {
    atla.fields[otype->lastField].nextField = *ud_idx;
  }
  otype->lastField = *ud_idx;
  if (!otype->firstField) otype->firstField = *ud_idx;
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);            // s: user-data
  return 1;
}

static int l_field_ref(lua_State* l) {
  // pointer is handled separately, fixed array is handled separately
  // expected args: owner:alta.type, version:int, int: id, fieldname:string,
  // fieldtype:atla.type, [isPtr:bool, ptrcount:atla.field]
  uint32_t*   owner_idx = luaL_checkudata(l, 1, "atla.type");
  uint32_t    version = luaL_checkunsigned(l, 2);
  uint32_t    id = luaL_checkunsigned(l, 3);
  char const* field_name = luaL_checkstring(l, 4);
  uint32_t*   type_idx = luaL_checkudata(l, 5, "atla.type");
  uint32_t*   ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_field(&atla);
  atlatype_t*  otype = atla.types + *owner_idx;
  atlafield_t* afield = atla.fields + *ud_idx;
  // fprintf(stdout, "Adding field %s\n", field_name);
  afield->name = strdup(field_name);
  afield->id = id;
  afield->type = *type_idx;
  afield->vAdd = version;
  afield->vRem = 0;
  afield->owner = *owner_idx;
  afield->sizeField = 0; // as this isn't a dynamic array, no size field
  afield->flags = atla_field_pointer;
  afield->nextField = 0;
  if (otype->lastField) {
    atla.fields[otype->lastField].nextField = *ud_idx;
  }
  otype->lastField = *ud_idx;
  if (!otype->firstField) otype->firstField = *ud_idx;
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);            // s: user-data
  return 1;
}

static int l_field_array(lua_State* l) {
  // pointer is handled separately, fixed array is handled separately
  // expected args: owner:alta.type, version:int, id:int, fieldname:string,
  // fieldtype:atla.type, arraysize:int
  uint32_t*   owner_idx = luaL_checkudata(l, 1, "atla.type");
  uint32_t    version = luaL_checkunsigned(l, 2);
  uint32_t    id = luaL_checkunsigned(l, 3);
  char const* field_name = luaL_checkstring(l, 4);
  uint32_t*   type_idx = luaL_checkudata(l, 5, "atla.type");
  uint32_t    array_size = luaL_checkunsigned(l, 6);
  uint32_t*   ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  atlatype_t* otype = atla.types + *owner_idx;
  if (array_size == 0) {
    return luaL_error(l,
                      "Array size of zero for field %s.%s is not allowed\n",
                      otype->name,
                      field_name);
  }
  *ud_idx = alc_add_field(&atla);
  atlafield_t* afield = atla.fields + *ud_idx;
  // fprintf(stdout, "Adding field %s\n", field_name);
  afield->name = strdup(field_name);
  afield->id = id;
  afield->type = *type_idx;
  afield->vAdd = version;
  afield->vRem = 0;
  afield->owner = *owner_idx;
  afield->sizeField =
    array_size; // as this is a array, size means actual number count
  afield->flags = atla_field_fixed_array;
  afield->nextField = 0;
  if (otype->lastField) {
    atla.fields[otype->lastField].nextField = *ud_idx;
  }
  otype->lastField = *ud_idx;
  if (!otype->firstField) otype->firstField = *ud_idx;
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);            // s: user-data
  return 1;
}

static int l_field_darray(lua_State* l) {
  // pointer is handled separately, fixed array is handled separately
  // expected args: owner:alta.type, version:int, id:int, fieldname:string,
  // fieldtype:atla.type, lenfield:atla.field
  uint32_t*    owner_idx = luaL_checkudata(l, 1, "atla.type");
  uint32_t     version = luaL_checkunsigned(l, 2);
  uint32_t    id = luaL_checkunsigned(l, 3);
  char const*  field_name = luaL_checkstring(l, 4);
  uint32_t*    type_idx = luaL_checkudata(l, 5, "atla.type");
  uint32_t*    len_field = luaL_checkudata(l, 6, "atla.field");
  uint32_t*    ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  atlatype_t*  otype = atla.types + *owner_idx;
  atlafield_t* lfield = atla.fields + *len_field;
  if (lfield->owner != *owner_idx) {
    return luaL_error(
      l,
      "Lenght %s field for field %s must belong to the type %s\n",
      lfield->name,
      field_name,
      otype->name);
  }
  if (!(lfield->type >= atla.firstIntType && lfield->type < atla.lastIntType)) {
    return luaL_error(l,
                      "Lenght %s field for field %s must be an integer type\n",
                      lfield->name,
                      field_name);
  }
  if (lfield->flags & atla_field_deprecated) {
    return luaL_error(l,
                      "Lenght %s field for field %s must not be deprecated\n",
                      lfield->name,
                      field_name);
  }
  *ud_idx = alc_add_field(&atla);
  atlafield_t* afield = atla.fields + *ud_idx;
  // fprintf(stdout, "Adding field %s\n", field_name);
  afield->name = strdup(field_name);
  afield->id = id;
  afield->type = *type_idx;
  afield->vAdd = version;
  afield->vRem = 0;
  afield->owner = *owner_idx;
  afield->sizeField = *len_field;
  afield->flags = atla_field_pointer | atla_field_dynamic_array;
  afield->nextField = 0;
  if (otype->lastField) {
    atla.fields[otype->lastField].nextField = *ud_idx;
  }
  otype->lastField = *ud_idx;
  if (!otype->firstField) otype->firstField = *ud_idx;
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);            // s: user-data
  return 1;
}

static int l_deprecated_field(lua_State* l) {
  // pointer is handled separately, fixed array is handled separately
  // expected args: owner:alta.type, version:int, removever:int,
  // id:int, fieldname:string, fieldtype:atla.type, [isPtr:bool, ptrcount:atla.field]
  uint32_t*   owner_idx = luaL_checkudata(l, 1, "atla.type");
  uint32_t    version = luaL_checkunsigned(l, 2);
  uint32_t    rem_version = luaL_checkunsigned(l, 3);
  uint32_t    id = luaL_checkunsigned(l, 4);
  char const* field_name = luaL_checkstring(l, 5);
  uint32_t*   type_idx = luaL_checkudata(l, 6, "atla.type");
  uint32_t*   ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_field(&atla);
  atlatype_t*  otype = atla.types + *owner_idx;
  atlafield_t* afield = atla.fields + *ud_idx;
  // fprintf(stdout, "Adding field %s\n", field_name);
  afield->name = strdup(field_name);
  afield->id = id;
  afield->type = *type_idx;
  afield->vAdd = version;
  afield->vRem = rem_version;
  afield->owner = *owner_idx;
  afield->sizeField = 0;
  afield->flags = atla_field_deprecated;
  afield->nextField = 0;
  if (otype->lastField) {
    atla.fields[otype->lastField].nextField = *ud_idx;
  }
  otype->lastField = *ud_idx;
  if (!otype->firstField) otype->firstField = *ud_idx;
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);            // s: user-data
  return 1;
}

static int l_deprecated_field_ref(lua_State* l) {
  // pointer is handled separately, fixed array is handled separately
  // expected args: owner:alta.type, version:int, rem_version:int,
  // id:int, fieldname:string, fieldtype:atla.type, [isPtr:bool, ptrcount:atla.field]
  uint32_t*   owner_idx = luaL_checkudata(l, 1, "atla.type");
  uint32_t    version = luaL_checkunsigned(l, 2);
  uint32_t    rem_version = luaL_checkunsigned(l, 3);
  uint32_t    id = luaL_checkunsigned(l, 4);
  char const* field_name = luaL_checkstring(l, 5);
  uint32_t*   type_idx = luaL_checkudata(l, 6, "atla.type");
  uint32_t*   ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  *ud_idx = alc_add_field(&atla);
  atlatype_t*  otype = atla.types + *owner_idx;
  atlafield_t* afield = atla.fields + *ud_idx;
  // fprintf(stdout, "Adding field %s\n", field_name);
  afield->name = strdup(field_name);
  afield->id = id;
  afield->type = *type_idx;
  afield->vAdd = version;
  afield->vRem = rem_version;
  afield->owner = *owner_idx;
  afield->sizeField = 0; // as this isn't a dynamic array, no size field
  afield->flags = atla_field_pointer | atla_field_deprecated;
  afield->nextField = 0;
  if (otype->lastField) {
    atla.fields[otype->lastField].nextField = *ud_idx;
  }
  otype->lastField = *ud_idx;
  if (!otype->firstField) otype->firstField = *ud_idx;
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);            // s: user-data
  return 1;
}

static int l_deprecated_field_array(lua_State* l) {
  // pointer is handled separately, fixed array is handled separately
  // expected args: owner:alta.type, version:int, id:int, fieldname:string,
  // fieldtype:atla.type, arraysize:int
  uint32_t*   owner_idx = luaL_checkudata(l, 1, "atla.type");
  uint32_t    version = luaL_checkunsigned(l, 2);
  uint32_t    rem_version = luaL_checkunsigned(l, 3);
  uint32_t    id = luaL_checkunsigned(l, 4);
  char const* field_name = luaL_checkstring(l, 5);
  uint32_t*   type_idx = luaL_checkudata(l, 6, "atla.type");
  uint32_t    array_size = luaL_checkunsigned(l, 7);
  uint32_t*   ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  atlatype_t* otype = atla.types + *owner_idx;
  if (array_size == 0) {
    return luaL_error(l,
                      "Array size of zero for field %s.%s is not allowed\n",
                      otype->name,
                      field_name);
  }
  *ud_idx = alc_add_field(&atla);
  atlafield_t* afield = atla.fields + *ud_idx;
  // fprintf(stdout, "Adding field %s\n", field_name);
  afield->name = strdup(field_name);
  afield->id = id;
  afield->type = *type_idx;
  afield->vAdd = version;
  afield->vRem = rem_version;
  afield->owner = *owner_idx;
  afield->sizeField =
    array_size; // as this is a array, size means actual number count
  afield->flags = atla_field_fixed_array | atla_field_deprecated;
  afield->nextField = 0;
  if (otype->lastField) {
    atla.fields[otype->lastField].nextField = *ud_idx;
  }
  otype->lastField = *ud_idx;
  if (!otype->firstField) otype->firstField = *ud_idx;
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);            // s: user-data
  return 1;
}

static int l_deprecated_field_darray(lua_State* l) {
  // pointer is handled separately, fixed array is handled separately
  // expected args: owner:alta.type, version:int, id:int, fieldname:string,
  // fieldtype:atla.type, lenfield:atla.field
  uint32_t*    owner_idx = luaL_checkudata(l, 1, "atla.type");
  uint32_t     version = luaL_checkunsigned(l, 2);
  uint32_t     rem_version = luaL_checkunsigned(l, 3);
  uint32_t    id = luaL_checkunsigned(l, 4);
  char const*  field_name = luaL_checkstring(l, 5);
  uint32_t*    type_idx = luaL_checkudata(l, 6, "atla.type");
  uint32_t*    len_field = luaL_checkudata(l, 7, "atla.field");
  uint32_t*    ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: user-data
  atlatype_t*  otype = atla.types + *owner_idx;
  atlafield_t* lfield = atla.fields + *len_field;
  if (lfield->owner != *owner_idx) {
    return luaL_error(
      l,
      "Lenght %s field for field %s must belong to the type %s\n",
      lfield->name,
      field_name,
      otype->name);
  }
  if (!(lfield->type >= atla.firstIntType && lfield->type < atla.lastIntType)) {
    return luaL_error(l,
                      "Lenght %s field for field %s must be an integer type\n",
                      lfield->name,
                      field_name);
  }
  *ud_idx = alc_add_field(&atla);
  atlafield_t* afield = atla.fields + *ud_idx;
  // fprintf(stdout, "Adding field %s\n", field_name);
  afield->name = strdup(field_name);
  afield->id = id;
  afield->type = *type_idx;
  afield->vAdd = version;
  afield->vRem = rem_version;
  afield->owner = *owner_idx;
  afield->sizeField = *len_field;
  afield->flags =
    atla_field_pointer | atla_field_dynamic_array | atla_field_deprecated;
  afield->nextField = 0;
  if (otype->lastField) {
    atla.fields[otype->lastField].nextField = *ud_idx;
  }
  otype->lastField = *ud_idx;
  if (!otype->firstField) otype->firstField = *ud_idx;
  luaL_getmetatable(l, "atla.field"); // s: user-data, meta-tbl
  lua_setmetatable(l, -2);            // s: user-data
  return 1;
}

int luaopen_atla(lua_State* l) {
  struct luaL_Reg const lib[] = {
    {"create_type", l_create_type},
    {"enum", l_create_enum},
    {"field", l_field},
    {"field_ref", l_field_ref},
    {"field_array", l_field_array},
    {"field_darray", l_field_darray},
    {"deprecated_field", l_deprecated_field},
    {"deprecated_field_ref", l_deprecated_field_ref},
    {"deprecated_field_array", l_deprecated_field_array},
    {"deprecated_field_darray", l_deprecated_field_darray},
    {NULL, NULL}};
  struct luaL_Reg const typelib_m[] = {
    {"field", l_field},
    {"field_ref", l_field_ref},
    {"field_array", l_field_array},
    {"field_darray", l_field_darray},
    {"deprecated_field", l_deprecated_field},
    {"deprecated_field_ref", l_deprecated_field_ref},
    {"deprecated_field_array", l_deprecated_field_array},
    {"deprecated_field_darray", l_deprecated_field_darray},
    {NULL, NULL}};
  static luaL_Reg const enumlib_m[] = { 
    {"value", l_enum_value},
                                       {"deprecated_value", l_enum_deprecated_value},
    {NULL,NULL}
  };
  uint32_t*   ud_idx;
  atlatype_t* atype;

  // Initialise the atla data
  atla.fieldRes = 1024;
  atla.typeRes = 1024;
  atla.typeCount = 1;
  atla.fieldCount = 1;
  atla.types = malloc(sizeof(atlatype_t) * atla.typeRes);
  atla.fields = malloc(sizeof(atlafield_t) * atla.fieldRes);
  // create the null entries
  memset(atla.types, 0, sizeof(atlatype_t));
  memset(atla.fields, 0, sizeof(atlafield_t));

  atla.firstIntType = atla.typeCount;
  luaL_newmetatable(l, "atla.type");
  // duplicate the metatable
  lua_pushvalue(l, -1);
  lua_setfield(l, -2, "__index");
  luaL_setfuncs(l, typelib_m, 0);

  luaL_newmetatable(l, "atla.enum");
  lua_pushvalue(l, -1);
  lua_setfield(l, -2, "__index");
  luaL_setfuncs(l, enumlib_m, 0);

  luaL_newmetatable(l, "atla.field");
  luaL_newlib(l, lib);
  // Add common constants to our library table
  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  // init type data
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_int8;
  atype->name = strdup("int8_t");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "int8");       // s: lib-tbl

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_char;
  atype->name = strdup("char");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "char");       // s: lib-tbl

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_int16;
  atype->name = strdup("int16_t");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "int16");      // s: lib-tbl

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_int32;
  atype->name = strdup("int32_t");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "int32");      // s: lib-tbl

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_int64;
  atype->name = strdup("int64_t");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "int64");      // s: lib-tbl

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_uint8;
  atype->name = strdup("uint8_t");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "uint8");      // s: lib-tbl

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_uint16;
  atype->name = strdup("uint16_t");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "uint16");     // s: lib-tbl

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_uint32;
  atype->name = strdup("uint32_t");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "uint32");     // s: lib-tbl

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_uint64;
  atype->name = strdup("uint64_t");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "uint64");     // s: lib-tbl
  atla.lastIntType = atla.typeCount;

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_float;
  atype->name = strdup("float");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "float");      // s: lib-tbl

  ud_idx = lua_newuserdata(l, sizeof(uint32_t)); // s: lib-tbl, user-data
  *ud_idx = alc_add_type(&atla);
  atype = atla.types + *ud_idx;
  atype->firstField = 0;
  atype->builtIn = atla_type_double;
  atype->name = strdup("double");
  luaL_getmetatable(l, "atla.type"); // s: lib-tbl, user-data, meta-tbl
  lua_setmetatable(l, -2);           // s: lib-tbl, user-data
  lua_setfield(l, -2, "double");     // s: lib-tbl

  // NOTE: this is how math add some number constants. Can use for common types,
  // etc lua_pushnumber(L, PI); lua_setfield(L, -2, "pi"); lua_pushnumber(L,
  // HUGE_VAL); lua_setfield(L, -2, "huge");

  return 1;
}

static int               verbose_flag;
static struct gop_option cmd_opts[] = {
  {.name = "verbose",
   .usage = "Enable verbose output.",
   .has_arg = gop_no_argument,
   .flag = &verbose_flag,
   .val = 1,
   .short_name = 'v'},
  {.name = "output",
   .usage = "Output destination folder for generated C files.",
   .has_arg = gop_required_argument,
   .flag = NULL,
   .val = 0,
   .short_name = 'o'},
  {.name = "version",
   .usage = "Overwrite the version for serialization.",
   .has_arg = gop_required_argument,
   .flag = NULL,
   .val = 0,
   .short_name = 'o'},
  {0}};

int main(int argc, char** argv) {
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);
  luaL_requiref(L, "atla", luaopen_atla, 1);
  lua_pop(L, 1); // remove lib

  // parse all inputs
  int         error = 0;
  char const* output_dir = NULL;

  {
    struct gop_ctx args;
    int            d;
    gop_init(&args, argc, argv, cmd_opts);
    while ((d = gop_next(&args)) != -1) {
      switch (d) {
      case 'w': atla.writeVersion = atoi(args.optarg); break;
      case 'o': output_dir = strdup(args.optarg); break;
      case '?':
      default: exit(1);
      }
    }

    for (int i = args.optind; i < argc; ++i) {
      // fprintf(stderr, "Expected argument after options\n");
      // int error = luaL_loadfile(L, argv[i]) || lua_pcall(L, 0, 0, 0);
      if (luaL_loadfile(L, argv[i]) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        ++error;
      }
    }
  }

  if (!output_dir) {
    fprintf(stderr, "No output path given\n");
    exit(1);
  }

  if (error) {
    fprintf(stderr, "%d errors encountered.\n", error);
    exit(1);
  }

  // Dump all enums
  int mfs_ret;
  for (uint32_t i = 0; i < atla.typeCount; ++i) {
    atlaenum_t* type = atla.types + i;

    if (type->builtIn != atla_type_enum) continue;

    char dest_path[ATLA_MAX_PATH], dest_name[ATLA_MAX_PATH];
    sprintf(dest_name, "%s.enum.gen.h", type->name);
    mfs_ret = minfs_path_join(output_dir, dest_name, dest_path, ATLA_MAX_PATH);
    if (FS_FAILED(mfs_ret)) {
      fprintf(stderr, "Failed to build output path for type %s\n", type->name);
      exit(1);
    }
    FILE* dstf = fopen(dest_path, "wb");

    fprintf(dstf,
            "//\n// %s.h generated file by atlalc. Do not "
            "modifiy.\n//\n\n#pragma once\n\n"
            "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif // __cplusplus\n\n",
            type->name);

    fprintf(dstf,
            "typedef enum %s {\n",
            type->name);

    uint32_t enum_count = 0;
    for (uint32_t j = type->firstField; j;) {
      ++enum_count;
      atlafield_t* field = atla.fields + j;
      if (field->flags & atla_field_deprecated) {
        fprintf(dstf, "//");
      }
      fprintf(dstf, "  %s%s = %d, // Added in version %d", type->prefix, field->name, field->enumValue, field->vAdd);
      if (field->flags & atla_field_deprecated) {
        fprintf(dstf, "  deprecated in version %u", field->vRem);
      } 
      fprintf(dstf, "\n");
      j = field->nextField;
    }
    fprintf(dstf, "} %s;\n\n", type->name);
    fprintf(dstf, "#define %s_num_of_entries (%u)\n", type->name, enum_count);
    fprintf(dstf, "char const* %s_as_string(%s v);\n", type->name, type->name);

    fprintf(dstf, 
            "#ifdef __cplusplus\n"
            "} // extern \"C\"\n"
            "#endif // __cplusplus\n\n");
    fclose(dstf);
  }

  // Dump all structs
  for (uint32_t i = 1; i < atla.typeCount; ++i) {
    atlatype_t* type = atla.types + i;
    if (type->builtIn != atla_type_user) continue;

    char dest_path[ATLA_MAX_PATH], dest_name[ATLA_MAX_PATH];
    sprintf(dest_name, "%s.gen.h", type->name);
    mfs_ret = minfs_path_join(output_dir, dest_name, dest_path, ATLA_MAX_PATH);
    if (FS_FAILED(mfs_ret)) {
      fprintf(stderr, "Failed to build output path for type %s\n", type->name);
      exit(1);
    }
    FILE* dstf = fopen(dest_path, "wb");

    fprintf(dstf,
            "//\n// %s.h generated file by atlalc. Do not "
            "modifiy.\n//\n\n#pragma once\n\n"
            "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif // __cplusplus\n\n",
            type->name);
    // Search for includes
    for (uint32_t j = type->firstField; j;) {
      atlafield_t* field = atla.fields + j;
      atlatype_t*  fieldtype = atla.types + field->type;
      j = field->nextField;
      if (field->flags & atla_field_deprecated) continue;
      if (fieldtype->builtIn != atla_type_user && fieldtype->builtIn != atla_type_enum) continue;
      if (field->flags & atla_field_pointer) continue;
      if (fieldtype->lastInc == i) continue;

      fieldtype->lastInc = i;
      if (fieldtype->builtIn == atla_type_enum) {
        fprintf(dstf, "#include \"%s.enum.gen.h\"\n", fieldtype->name);
      } else {
        fprintf(dstf, "#include \"%s.gen.h\"\n", fieldtype->name);
      }
    }

    // Search for prototypes
    fprintf(dstf, "\n");
    for (uint32_t j = type->firstField; j;) {
      atlafield_t* field = atla.fields + j;
      atlatype_t*  fieldtype = atla.types + field->type;
      j = field->nextField;
      if (field->flags & atla_field_deprecated) continue;
      if (fieldtype->builtIn != atla_type_user) continue;
      if (fieldtype->lastInc == i) continue;

      fieldtype->lastInc = i;
      fprintf(
        dstf, "typedef struct %s %s;\n", fieldtype->name, fieldtype->name);
    }


    fprintf(dstf, "\ntypedef struct %s {\n", type->name);
    for (uint32_t j = type->firstField; j;) {
      atlafield_t* field = atla.fields + j;
      atlatype_t*  fieldtype = atla.types + field->type;
      j = field->nextField;
      if (field->flags & atla_field_deprecated) {
        fprintf(dstf, "  //");
      } else {
        fprintf(dstf, "  ");
      }
      if (field->flags & atla_field_pointer) {
        fprintf(dstf, "%s* %s;", fieldtype->name, field->name);
      } else if (field->flags & atla_field_fixed_array) {
        fprintf(
          dstf, "%s %s[%u];", fieldtype->name, field->name, field->sizeField);
      } else {
        fprintf(dstf, "%s %s;", fieldtype->name, field->name);
      }
      if (field->flags & atla_field_deprecated) {
        fprintf(dstf, "  deprecated in version %u\n", field->vRem);
      } else {
        fprintf(dstf, "// added in version %u\n", field->vAdd);
      }
    }
    fprintf(dstf, "} %s;\n\n", type->name);
    fprintf(dstf, "typedef struct %s_delta {\n", type->name);
    for (uint32_t j = type->firstField; j;) {
      atlafield_t* field = atla.fields + j;
      //atlatype_t*  fieldtype = atla.types + field->type;
      j = field->nextField;
      if (field->flags & atla_field_deprecated) {
        fprintf(dstf, "  //");
      } else {
        fprintf(dstf, "  ");
      }
      fprintf(dstf, "uint32_t %s_present : 1;", field->name);
      if (field->flags & atla_field_deprecated) {
        fprintf(dstf, "  deprecated in version %u\n", field->vRem);
      } else {
        fprintf(dstf, "// added in version %u\n", field->vAdd);
      }
    }
    fprintf(dstf, "} %s_delta;\n\n", type->name);
    
    fprintf(dstf, "#define %s_type_id (%u)\n\n", type->name, type->id);

    for (uint32_t j = type->firstField; j;) {
      atlafield_t* field = atla.fields + j;
      //atlatype_t*  fieldtype = atla.types + field->type;
      j = field->nextField;
      if (field->flags & atla_field_deprecated) {
        fprintf(dstf, "#define deprecated_%s_%s_member_id (%u)\n", type->name, field->name, field->id);
      } else {
        fprintf(dstf, "#define %s_%s_member_id (%u)\n", type->name, field->name, field->id);
      }
    }
    fprintf(dstf,
            "\nvoid atla_serialize_%s(atAtlaSerializer_t*, void*);\n",
            type->name);
    fprintf(dstf, "extern atAtlaReflInfo_t const atla_reflection_%s;\n\n", type->name);
    fprintf(dstf, 
            "#ifdef __cplusplus\n"
            "} // extern \"C\"\n"
            "#endif // __cplusplus\n\n");

    fclose(dstf);
    sprintf(dest_name, "%s.gen.c", type->name);
    mfs_ret = minfs_path_join(output_dir, dest_name, dest_path, ATLA_MAX_PATH);
    if (FS_FAILED(mfs_ret)) {
      fprintf(stderr, "Failed to build output path for type %s\n", type->name);
      exit(1);
    }
    dstf = fopen(dest_path, "wb");
    fprintf(dstf,
            "//\n// %s.c generated file by atlalc. Do not "
            "modifiy.\n//\n\n#pragma once\n\n",
            type->name);
    fprintf(dstf, "#include \"%s.gen.h\"\n\n", type->name);

    // Add includes for any prototypes in the header
    fprintf(dstf, "\n");
    for (uint32_t j = type->firstField; j;) {
      atlafield_t* field = atla.fields + j;
      atlatype_t*  fieldtype = atla.types + field->type;
      j = field->nextField;
      //if (field->flags & atla_field_deprecated) continue; // Don't omit deprecated fields, we still need the type information
      if (fieldtype->builtIn != atla_type_user) continue;
      if (!(field->flags & (atla_field_pointer | atla_field_deprecated))) continue;
      //if (fieldtype->lastInc > type->lastField) continue; // TODO: why is this preventing some deprecated includes?

      fieldtype->lastInc = type->lastField + 1;
      fprintf(dstf, "#include \"%s.gen.h\"\n", fieldtype->name);
    }
    fprintf(dstf, "\n");

    fprintf(
      dstf,
      "void atla_serialize_%s(atAtlaSerializer_t* serializer, void* vptr) {\n",
      type->name);
    fprintf(dstf,
            "  struct %s*       ptr = (struct %s*)vptr;\n",
            type->name,
            type->name);
    fprintf(dstf,
            "  if (serializer->reading) memset(ptr, 0, sizeof(%s));\n",
            type->name);
    fprintf(dstf, "  serializer->depth++;\n");

    for (uint32_t j = type->firstField; j;) {
      atlafield_t* field = atla.fields + j;
      atlatype_t*  fieldtype = atla.types + field->type;
      int          dep = field->flags & atla_field_deprecated;
      j = field->nextField;
      if (field->flags & atla_field_pointer) {
        if (fieldtype->builtIn != atla_type_user) {
          int darr = field->flags & atla_field_dynamic_array;
          if (dep) {
            fprintf(dstf,
                    "  if (serializer->version >= %d && serializer->version < "
                    "%d) {\n",
                    field->vAdd,
                    field->vRem);
            fprintf(dstf, "    if (serializer->reading) {\n");
            fprintf(dstf, "      uint32_t a;\n");
            fprintf(dstf,
                    "      atSerializeRead(serializer, &a, sizeof(a), 1);\n");
          } else {
            fprintf(dstf, "  if (serializer->version >= %d) {\n", field->vAdd);
            fprintf(dstf, "    if (serializer->reading) {\n");
            if (darr) {
              //atlafield_t* fsize = atla.fields + field->sizeField;
              fprintf(dstf,
                      "      uint32_t a;\n"
                      "      atSerializeRead(serializer, &a, sizeof(a), 1);\n");
              fprintf(dstf,
                      "      ptr->%s = "
                      "(%s*)atSerializeReadGetBlobLocation(serializer, "
                      "a);\n",
                      field->name,
                      fieldtype->name);
            } else {
              fprintf(dstf,
                      "      uint32_t a;\n"
                      "      atSerializeRead(serializer, &a, sizeof(a), 1);\n");
              fprintf(dstf,
                      "      ptr->%s = "
                      "(%s*)atSerializeReadGetBlobLocation(serializer, a);\n",
                      field->name,
                      fieldtype->name);
            }
            fprintf(dstf, "    } else {\n");
            fprintf(dstf, "      uint32_t id = atSerializeWritePendingBlob(\n");
            if (darr) {
              atlafield_t* fsize = atla.fields + field->sizeField;
              fprintf(dstf,
                      "        serializer, ptr->%s, sizeof(%s), ptr->%s);\n",
                      field->name,
                      fieldtype->name,
                      fsize->name);
            } else {
              fprintf(dstf,
                      "        serializer, ptr->%s, sizeof(%s), 1);\n",
                      field->name,
                      fieldtype->name);
            }
            fprintf(
              dstf,
              "      atSerializeWrite(serializer, &id, sizeof(id), 1);\n");
          }
          fprintf(dstf, "    }\n");
          fprintf(dstf, "  }\n");
        } else {
          int darr = field->flags & atla_field_dynamic_array;
          if (dep) {
            fprintf(dstf,
                    "  if (serializer->version >= %d && serializer->version < "
                    "%d) {\n",
                    field->vAdd,
                    field->vRem);
            fprintf(dstf, "    if (serializer->reading) {\n");
            fprintf(dstf, "      uint32_t a;\n");
            fprintf(dstf,
                    "      atSerializeRead(serializer, &a, sizeof(a), 1);\n");
          } else {
            //atlafield_t* fsize = atla.fields + field->sizeField;
            fprintf(dstf, "  if (serializer->version >= %u) {\n", field->vAdd);
            fprintf(dstf, "    if (serializer->reading) {\n");
            fprintf(dstf,
                    "      uint32_t a;\n"
                    "      atSerializeRead(serializer, &a, sizeof(a), 1);\n");
            if (darr) {
              //atlafield_t* fsize = atla.fields + field->sizeField;
              fprintf(
                dstf,
                "      ptr->%s = (%s*)atSerializeReadTypeLocation(serializer, "
                "a, \"%s\", atla_serialize_%s);\n",
                field->name,
                fieldtype->name,
                fieldtype->name,
                fieldtype->name);
            } else {
              fprintf(dstf,
                      "      ptr->%s = "
                      "(%s*)atSerializeReadTypeLocation(serializer, "
                      "a, \"%s\", atla_serialize_%s);\n",
                      field->name,
                      fieldtype->name,
                      fieldtype->name,
                      fieldtype->name);
            }
            fprintf(dstf, "    } else {\n");
            fprintf(dstf, "      uint32_t id = atSerializeWritePendingType(\n");
            if (darr) {
              atlafield_t* fsize = atla.fields + field->sizeField;
              fprintf(dstf,
                      "        serializer, ptr->%s, \"%s\", atla_serialize_%s, "
                      "sizeof(%s), ptr->%s);\n",
                      field->name,
                      fieldtype->name,
                      fieldtype->name,
                      fieldtype->name,
                      fsize->name);
            } else {
              fprintf(dstf,
                      "        serializer, ptr->%s, \"%s\", atla_serialize_%s, "
                      "sizeof(%s), 1);\n",
                      field->name,
                      fieldtype->name,
                      fieldtype->name,
                      fieldtype->name);
            }
            fprintf(
              dstf,
              "      atSerializeWrite(serializer, &id, sizeof(id), 1);\n");
          }
          fprintf(dstf, "    }\n");
          fprintf(dstf, "  }\n");
        }
      } else {
        uint32_t count =
          (field->flags & atla_field_fixed_array) ? field->sizeField : 1;
        uint32_t isArray = field->flags & atla_field_fixed_array;
        if (fieldtype->builtIn != atla_type_user) {
          if (dep) {
            fprintf(dstf,
                    "  if (serializer->version >= %d && serializer->version "
                    "< %d) {\n",
                    field->vAdd,
                    field->vRem);
          } else {
            fprintf(dstf, "  if (serializer->version >= %d) {\n", field->vAdd);
          }
          fprintf(dstf, "    if (serializer->reading) {\n");
          if (dep) {
            fprintf(dstf,
                    "      atSerializeSkip(serializer, sizeof(%s), %u);\n",
                    fieldtype->name,
                    count);
          } else {
            fprintf(dstf,
                    "      atSerializeRead(serializer, &(ptr->%s), sizeof(%s), "
                    "%u);\n",
                    field->name,
                    fieldtype->name,
                    count);
            fprintf(dstf, "    } else {\n");
            fprintf(dstf,
                    "      atSerializeWrite(serializer, &(ptr->%s), "
                    "sizeof(%s), %u);\n",
                    field->name,
                    fieldtype->name,
                    count);
          }
          fprintf(dstf, "    }\n");
          fprintf(dstf, "  }\n");
        } else {
          if (dep) {
            fprintf(dstf,
                    "  if (serializer->version >= %d && serializer->version "
                    "< (%d)) {\n",
                    field->vAdd,
                    field->vRem);
          } else {
            fprintf(dstf, "  if (serializer->version >= %d) {\n", field->vAdd);
          }
          char const* fname = field->name;
          if (dep) {
            fprintf(dstf, "    %s dummy;\n", fieldtype->name);
            fname = "dummy";
          }
          fprintf(dstf, "    for (uint32_t i = 0; i < %u; ++i) {\n", count);
          fprintf(dstf,
                  "      atla_serialize_%s(serializer, (&(%s%s))%s);\n",
                  fieldtype->name,
                  !dep ? "ptr->" : "",
                  fname,
                  (!dep && !isArray) ? "+i" : "");
          fprintf(dstf, "    }\n");
          fprintf(dstf, "  }\n");
        }
      }
    }
    fprintf(dstf, "  if (serializer->depth == 1 && !serializer->reading)\n");
    fprintf(dstf, "    atSerializeWriteProcessPending(serializer);\n");
    fprintf(dstf, "  --serializer->depth;\n");
    fprintf(dstf, "}\n");
    fprintf(dstf, "\n");

    fprintf(dstf,"  static atAtlaReflInfo_t       atla_reflection_%s_member_fields[] = {\n", type->name);

    uint32_t reflection_count = 0;
    for (uint32_t j = type->firstField; j;) {
      atlafield_t* field = atla.fields + j;
      atlatype_t*  fieldtype = atla.types + field->type;
      atlafield_t* fsize = (field->flags & atla_field_dynamic_array) ? atla.fields + field->sizeField : NULL;
      j = field->nextField;
      fprintf(dstf,"    {\n");
      fprintf(dstf,"      .name=\"%s\",\n", field->name);
      if (field->flags & atla_field_fixed_array)
        fprintf(dstf,"      .arrayCount=%u,\n", field->sizeField);
      else if (field->flags & atla_field_dynamic_array)
        fprintf(dstf,"      .arrayCount=%u,\n", fsize->id);
      else
        fprintf(dstf,"      .arrayCount=0,\n");
      fprintf(dstf,"      .id=%u,\n", field->id);
      fprintf(dstf,"      .flags=");
      {
        int pipeop = 0; 
        if (field->flags & atla_field_deprecated) {
          if (pipeop) fprintf(dstf,"|");
          fprintf(dstf,"at_rinfo_deprecated");
          pipeop=1;
        }
        if (field->flags & atla_field_pointer) {
          if (pipeop) fprintf(dstf,"|");
          fprintf(dstf,"at_rinfo_pointer");
          pipeop=1;
        }
        if (field->flags & atla_field_dynamic_array) {
          if (pipeop) fprintf(dstf,"|");
          fprintf(dstf,"at_rinfo_dynamic_array");
          pipeop=1;
        }
        if (field->flags & atla_field_fixed_array) {
          if (pipeop) fprintf(dstf,"|");
          fprintf(dstf,"at_rinfo_fixed_array");
          pipeop=1;
        }
        if (!pipeop) fprintf(dstf,"at_rinfo_none");
        fprintf(dstf,",\n");
      }
      fprintf(dstf,"      .verAdd=%u,\n", field->vAdd);
      fprintf(dstf,"      .verRem=%u,\n", field->vRem);
      fprintf(dstf,"      .type=");
      switch (fieldtype->builtIn) {
      case atla_type_int8: fprintf(dstf, "at_rinfo_type_int8,\n"); break;
      case atla_type_char: fprintf(dstf, "at_rinfo_type_char,\n"); break;
      case atla_type_int16: fprintf(dstf, "at_rinfo_type_int16,\n"); break;
      case atla_type_int32: fprintf(dstf, "at_rinfo_type_int32,\n"); break;
      case atla_type_int64: fprintf(dstf, "at_rinfo_type_int64,\n"); break;
      case atla_type_uint8: fprintf(dstf, "at_rinfo_type_uint8,\n"); break;
      case atla_type_uint16: fprintf(dstf, "at_rinfo_type_uint16,\n"); break;
      case atla_type_uint32: fprintf(dstf, "at_rinfo_type_uint32,\n"); break;
      case atla_type_uint64: fprintf(dstf, "at_rinfo_type_uint64,\n"); break;
      case atla_type_float: fprintf(dstf, "at_rinfo_type_float,\n"); break;
      case atla_type_double: fprintf(dstf, "at_rinfo_type_double,\n"); break;
      case atla_type_user: fprintf(dstf, "at_rinfo_type_user,\n"); break;
      default: fprintf(dstf,"at_rinfo_type_none,\n"); break;
      }
      if (!(field->flags & atla_field_deprecated)) {
        fprintf(dstf,"      .offset=(ptrdiff_t)&(((%s*)NULL)->%s),\n", type->name, field->name);
      }
      if (fieldtype->builtIn == atla_type_user) {
        fprintf(dstf, "      .typeExtra = { .reflData=&atla_reflection_%s },\n", fieldtype->name);
      }
      fprintf(dstf, "    },\n");
      ++reflection_count;
    }
    fprintf(dstf, "  };\n");
    fprintf(dstf, "  atAtlaReflInfo_t const atla_reflection_%s = {\n", type->name);
    fprintf(dstf, "    .name=\"%s\",\n", type->name);
    fprintf(dstf, "    .id=%u,\n", type->id);
    fprintf(dstf, "    .type=at_rinfo_type_user,\n");
    fprintf(dstf, "    .memberCount=%u,\n", reflection_count);
    fprintf(dstf, "    .members=atla_reflection_%s_member_fields,\n", type->name);
    fprintf(dstf, "  };\n");

    fclose(dstf);
  }

  char dest_path[ATLA_MAX_PATH], dest_name[ATLA_MAX_PATH];
  strcpy(dest_name, "atla.gen.c");
  mfs_ret = minfs_path_join(output_dir, dest_name, dest_path, ATLA_MAX_PATH);
  if (FS_FAILED(mfs_ret)) {
    fprintf(stderr, "Failed to build final output path\n");
    exit(1);
  }
  FILE* dstf = fopen(dest_path, "wb");

  fprintf(dstf, "//\n//\n//\n");
  fprintf(dstf, "#include \"atla/atla.h\"\n");
  fprintf(dstf, "#include <string.h>\n\n");
  for (uint32_t i = 1; i < atla.typeCount; ++i) {
    atlatype_t* type = atla.types + i;
    if (type->builtIn == atla_type_enum) {
      fprintf(dstf, "#include \"%s.enum.gen.h\"\n", type->name);
      fprintf(
        dstf, "char const* %s_as_string(%s v) {\n", type->name, type->name);
      fprintf(dstf, "  switch (v) {\n");
      for (uint32_t j = type->firstField; j;) {
        atlafield_t* field = atla.fields + j;
        fprintf(dstf,
                "    case %s%s: return \"%s\";\n",
                type->prefix,
                field->name,
                field->name);
        j = field->nextField;
      }
      fprintf(dstf, "    default: return \"NULL\";\n  }\n", type->name);
      fprintf(dstf, "}\n");
    }
    if (type->builtIn != atla_type_user) continue;

    fprintf(dstf, "#include \"%s.gen.c\"\n", type->name);
  }
  fprintf(dstf, "\nenum atla_gen_all_type_ids {\n");
  for (uint32_t i = 1; i < atla.typeCount; ++i) {
    atlatype_t* type = atla.types + i;
    if (type->builtIn != atla_type_user) continue;
    fprintf(dstf,
            "atla_private_%s_type_id = %u,\n",
            type->name, type->id);
  }
  fprintf(dstf, "};\n");

  fprintf(dstf, "\nvoid register_all_types(atAtlaContext_t* ctx) {\n");
  for (uint32_t i = 1; i < atla.typeCount; ++i) {
    atlatype_t* type = atla.types + i;
    if (type->builtIn != atla_type_user) continue;
    // I can expand the atContextReg macro here, save some pro-processing
    // fprintf(dstf, "  atContextReg(ctx, %s);\n", type->name);
    fprintf(dstf,
            "  atContextRegisterType(ctx, \"%s\", sizeof(%s));\n",
            type->name,
            type->name);
  }
  fprintf(dstf, "}\n\n");
  fclose(dstf);
  lua_close(L);
  return 0;
}