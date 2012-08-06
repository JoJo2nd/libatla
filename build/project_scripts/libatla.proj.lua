    project "libatla"
        location (ProjectDir)
        kind "StaticLib"
        language "C++"
        files {"../../include/**.h","../../src/**.cpp"}
        defines {CommonDefines,SharedLibDefines}
        defines {"COMPILE_LIB_ATLA"}
		includedirs {"../../include"}
        
    configuration (DebugCfgName)
        targetdir (TargetDir..DebugCfgName)
        defines {{DebugDefines}}
        flags {"Symbols"}
    configuration (ReleaseCfgName)
        targetdir (TargetDir..ReleaseCfgName)
        defines {{ReleaseDefines}}
        flags {"Optimize"}