min_typeid = 8000

foo_typeid = min_typeid + 1
vec2_typeid = min_typeid + 2
vec3_typeid = min_typeid + 3

local vec3_type = atla.create_type(vec3_typeid, "vec3")

-- Yield to allow other scripts to run and create types
coroutine.yield()
-- other scripts will have run so can 

vec3_type:field(4, 1, "x", atla.float)
vec3_type:field(4, 2, "y", atla.float)
atla.field(vec3_type, 4, 3, "z", atla.float)