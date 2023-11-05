
print("Hello from basic test script")
print("atla.types is ", type(atla.types))

min_typeid = 8000

foo_typeid = min_typeid + 1
vec2_typeid = min_typeid + 2
vec3_typeid = min_typeid + 3

test_type = atla.create_type(foo_typeid, "foo", 6)
print("test_type = atla.create_type(foo_typeid, 'foo') is ", tostring(test_type))
print("atla.types['foo'] is ", tostring(atla.types["foo"]))
print("atla.types.foo is ", tostring(atla.types.foo))
-- TODO -- print("atla.types[foo_typeid] is ", tostring(atla.types[foo_typeid]))
test_nested_type = atla.create_type(vec2_typeid, "vec2", 1)
vec3_type = atla.create_type(vec3_typeid, "vec3", 1)

-- Yield to allow other scripts to run and create types
coroutine.yield()
-- other scripts will have run so can 

atla.field(atla.types.foo, 1, 1, "field_int8", atla.int8)
atla.field(test_type, 1, 2, "field_int16", atla.int16)
atla.field(test_type, 1, 3, "field_int32", atla.int32)
atla.field(test_type, 1, 4, "field_int64", atla.int64)
atla.field(test_type, 1, 5, "field_uint8", atla.uint8)
atla.field(test_type, 1, 6, "field_uint16", atla.uint16)
atla.field(test_type, 1, 7, "field_uint32", atla.uint32)
atla.field(test_type, 1, 8, "field_uint64", atla.uint64)
atla.deprecated_field(test_type, 1, 3, 9, "field_float", atla.float)
atla.field(test_type, 3, 10, "someFloatData", atla.float)
atla.field(test_type, 3, 11, "someOtherFloatData", atla.float)
atla.field(test_type, 1, 12, "field_double", atla.double)
atla.field(test_type, 2, 13, "field_nest", test_nested_type)
atla.field_ref(test_type, 4, 14, "position", vec3_type)
atla.deprecated_field_ref(test_type, 4, 6, 15, "id", atla.uint64)
atla.field_ref(test_type, 6, 16, "id", atla.float);
atla.deprecated_field_darray(test_type, 5, 6, 17, "normals", vec3_type, atla.deprecated_field(test_type, 5, 6, 18, "normalCount", atla.uint32))
atla.field_darray(test_type, 6, 19, "positions", vec3_type, atla.field(test_type, 6, 20, "posCount", atla.uint32))
atla.deprecated_field_array(test_type, 5, 6, 21, "rect", test_nested_type, 4)
atla.field_array(test_type, 6, 22, "rectv2", test_nested_type, 4)

test_nested_type:field(1, 1, "x", atla.float)
test_nested_type:field(1, 2, "y", atla.float)

atla.field(vec3_type, 1, 1, "x", atla.float)
atla.field(vec3_type, 1, 2, "y", atla.float)
atla.field(vec3_type, 1, 3, "z", atla.float)
