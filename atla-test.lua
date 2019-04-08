

test_type = atla.create_type("foo")
test_nested_type = atla.create_type("vec2")
vec3_type = atla.create_type("vec3")

atla.field(test_type, 1, "field_int8", atla.int8)
atla.field(test_type, 1, "field_int16", atla.int16)
atla.field(test_type, 1, "field_int32", atla.int32)
atla.field(test_type, 1, "field_int64", atla.int64)
atla.field(test_type, 1, "field_uint8", atla.uint8)
atla.field(test_type, 1, "field_uint16", atla.uint16)
atla.field(test_type, 1, "field_uint32", atla.uint32)
atla.field(test_type, 1, "field_uint64", atla.uint64)
atla.deprecated_field(test_type, 1, 3, "field_float", atla.float)
atla.field(test_type, 3, "someFloatData", atla.float)
atla.field(test_type, 3, "someOtherFloatData", atla.float)
atla.field(test_type, 1, "field_double", atla.double)
atla.field(test_type, 2, "field_nest", test_nested_type)
atla.field_ref(test_type, 4, "position", vec3_type)
atla.deprecated_field_ref(test_type, 4, 6, "id", atla.uint64)
atla.field_ref(test_type, 6, "id", atla.float);
atla.deprecated_field_darray(test_type, 5, 6, "normals", vec3_type, atla.deprecated_field(test_type, 5, 6, "normalCount", atla.uint32))
atla.field_darray(test_type, 6, "positions", vec3_type, atla.field(test_type, 6, "posCount", atla.uint32))
atla.deprecated_field_array(test_type, 5, 6, "rect", test_nested_type, 4)
atla.field_array(test_type, 6, "rectv2", test_nested_type, 4)

test_nested_type:field(1, "x", atla.float)
test_nested_type:field(1, "y", atla.float)

atla.field(vec3_type, 4, "x", atla.float)
atla.field(vec3_type, 4, "y", atla.float)
atla.field(vec3_type, 4, "z", atla.float)
