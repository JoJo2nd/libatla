
min_typeid = 8000

foo_typeid = min_typeid + 1
vec2_typeid = min_typeid + 2
vec3_typeid = min_typeid + 3

atla.create_type(vec2_typeid, "vec2")

-- Yield to allow other scripts to run and create types
coroutine.yield()
-- other scripts will have run so can 

atla.types.vec2:field(1, 1, "x", atla.float)
atla.types.vec2:field(1, 2, "y", atla.float)
