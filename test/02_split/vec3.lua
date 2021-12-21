min_typeid = 8000

foo_typeid = min_typeid + 1
vec2_typeid = min_typeid + 2
vec3_typeid = min_typeid + 3

local vec3_type = atla.create_type(vec3_typeid, "vec3", 1)

-- Yield to allow other scripts to run and create types
coroutine.yield()
-- other scripts will have run so can 

vec3_type:field(1, 1, "x", atla.float)
vec3_type:field(1, 2, "y", atla.float)
atla.field(vec3_type, 1, 3, "z", atla.float)