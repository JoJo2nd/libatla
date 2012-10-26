    project "unittest"
        location (ProjectDir)
        kind "ConsoleApp"
        language "C"
        files {"../../test/**.h","../../test/**.c"}
        defines {CommonDefines,SharedLibDefines}
		includedirs {"../../include"}
        links{"libatla"}
        
    configuration (DebugCfgName)
        targetdir (TargetDir..DebugCfgName)
        defines {{DebugDefines}}
        flags {"Symbols"}
    configuration (ReleaseCfgName)
        targetdir (TargetDir..ReleaseCfgName)
        defines {{ReleaseDefines}}
        flags {"Optimize"}