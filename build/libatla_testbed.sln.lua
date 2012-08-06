
SlnName = "libatla_testbed"
DebugCfgName = "Debug"
ReleaseCfgName = "Release"
SlnOutput = "built_projects/".._ACTION.."/"..SlnName.."/"
SlnDir = SlnOutput
ProjectDir = "../"..SlnDir.."../projects/"
TargetDir = "../"..SlnDir.."../lib/"

PlatformDefines={"WIN32","_WIN32","WINDOWS","_WINDOWS"}
CommonDefines= {{PlatformDefines},"_CRT_SECURE_NO_WARNINGS"}
DebugDefines={"_DEBUG","DEBUG"}
ReleaseDefines={"NDEBUG","RELEASE"}

PostBuildStr="IF NOT EXIST ..\\..\\..\\..\\..\\lib\\$(ConfigurationName)\\ MKDIR ..\\..\\..\\..\\..\\lib\\$(ConfigurationName)\\ \ncopy /Y $(TargetPath) ..\\..\\..\\..\\..\\lib\\$(ConfigurationName)\\"

solution (SlnName)
    location (SlnDir)
    configurations ({DebugCfgName, ReleaseCfgName})
    
    dofile "project_scripts/libatla.proj.lua" --1st project becomes the start up project

