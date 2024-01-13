workspace "TsoEngine"
    architecture "x64"
    configurations { "Debug", "Release", "Dist" }
	startproject "TsoEditor"

--��ǰ·��Ϊpremake5.lua����·��
--create outputdir macro
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

--ʹ��submodule��premake5.lua�ļ�
include "TsoEngine/third_party/GLFW"
include "TsoEngine/third_party/Glad"
include "TsoEngine/third_party/imgui"
include "TsoEngine/third_party/yaml-cpp"

project "TsoEngine"
    location "%{prj.name}" -- �����location�����ɵ�vcproj��λ��, ��targetdir��objdir��ͬ
    kind "StaticLib"
    language "C++"
	staticruntime "off"
	cppdialect "C++17"
	targetdir ("bin/" .. outputdir .. "/%{prj.name}") 
	objdir   ("bin-int/" .. outputdir .. "/%{prj.name}") 
	links {"GLFW", "opengl32.lib", "Glad", "imgui", "YAML_CPP"}

    pchheader "TPch.h"
    pchsource "%{prj.name}/src/TPch.cpp"

	defines
	{
	    "_CRT_SECURE_NO_WARNINGS", 
		"YAML_CPP_STATIC_DEFINE"
	}
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/third_party/**.hpp",
		"%{prj.name}/third_party/entt/entt.hpp",
		"%{prj.name}/third_party/stb_image/**.h",
		"%{prj.name}/third_party/stb_image/**.cpp",
		"%{prj.name}/third_party/entt/entt.hpp",
		"%{prj.name}/third_party/yaml-cpp/include/yaml-cpp/**.h"
	}

	includedirs
	{
		"%{prj.name}/third_party/spdlog/include",
		"%{prj.name}/src",
		"%{prj.name}/src/Tso",
		"%{prj.name}/third_party/GLFW/include",
		"%{prj.name}/third_party/Glad/include",
		"%{prj.name}/third_party/imgui",
		"%{prj.name}/third_party/glm",
		"%{prj.name}/third_party/stb_image",
		"%{prj.name}/third_party/entt",
		"%{prj.name}/third_party/yaml-cpp/include"
	}

	links{
		"GLFW",
		"Glad",
		"ImGui",
		"yaml-cpp",
		"opengl32.lib"
	}
	
	--filter "files:'%{prj.name}'/vendor/imguizmo/ImGuizmo.cpp"
	--filter "files:Hazel/vendor/imguizmo/ImGuizmo.cpp"
	--filter "files:**.cpp"
	--flags { "NoPCH" }

	filter { "system:windows" }
	    systemversion "latest"
		defines {"TSO_PLATFORM_WINDOWS", "GLFW_INCLUDE_NONE", "TSO_ENABLE_ASSERTS"}
		
		postbuildcommands
		{
		    -- "copy default.config bin\\project.config"
			-- copy from relative path to ... ע�������COPYǰ��û��%
		    ("{COPY} %{cfg.buildtarget.relpath} \"../bin/" ..outputdir.."/Sandbox/\"")
		}

		filter "system:macosx"
			cppdialect "C++17"
			staticruntime "On"
			systemversion "latest"

			pchheader "src/TPch.h"
			pchsource "%{prj.name}/src/TPch.cpp"

			defines{
				"TSO_PLATFORM_MACOSX",
			}

    filter { "configurations:Debug" }
        defines { "TSO_DEBUG"}
		symbols "On"
		runtime "Debug" -- ����ʱ���ӵ�dll��debug���͵�	
		filter "system:windows"
			buildoptions "/MTd"
		-- in VS2019 that is Additional Library Directories
		--libdirs
		--{
		--	"%{VULKAN_SDK}/Lib",
		--	"%{prj.name}/vendor/Mono/lib/Debug"
		--}
		
		--links
		--{
			--"spirv-cross-cored.lib",
			--"spirv-cross-glsld.lib",
			--"SPIRV-Toolsd.lib",
			--"libmono-static-sgen.lib"
		--}
		

    filter { "configurations:Release"}
	defines { "TSO_RELEASE"}
	optimize "On"
	runtime "Release" -- ����ʱ���ӵ�dll��release���͵�
		filter "system:windows"
			buildoptions "/MT"
		-- in VS2019 that is Additional Library Directories
		--libdirs
		--{
		--	"%{VULKAN_SDK}/Lib",
		--	"%{prj.name}/vendor/Mono/lib/Release"
		--}
		
		--links
		--{
			--"shaderc_shared.lib",
			--"spirv-cross-core.lib",
			--"spirv-cross-glsl.lib",
			--"SPIRV-Tools.lib",
			--"libmono-static-sgen.lib"
		--}


    filter { "configurations:Dist"}
		defines { "TSO_DIST"}
	    optimize "On"
		filter "system:windows"
			buildoptions "/MT"

project "Sandbox"
	location "%{prj.name}"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir  ("bin/"..outputdir.."/%{prj.name}")
	objdir  ("bin-int/"..outputdir.."/%{prj.name}")
    
	files { "%{prj.name}/src/**.h", "%{prj.name}/src/**.cpp"}

    includedirs
	{
        "TsoEngine/third_party/spdlog/include",
		"TsoEngine/src",
		"TsoEngine/src/Tso",
		"TsoEngine/third_party/glm",
		"TsoEngine/third_party/imgui",
		"TsoEngine/third_party/entt",
		"TsoEngine/third_party/yaml-cpp/include",
		--"Hazel/vendor/box2D/include"
	}

	links { "TsoEngine" }

    filter { "system:Windows" }
	    systemversion "latest"
		 defines { "TSO_PLATFORM_WINDOWS"}

	filter "system:macosx"
		 defines{
				 "TSO_PLATFORM_MACOSX"
		 }
		 links{
			 "Cocoa.framework",
			 "IOKit.framework",
			 "CoreVideo.framework",
			 "OpenGL.framework"
		 }

    filter { "configurations:Debug"}
        defines { "DEBUG"}
        symbols "On"
		filter "system:windows"
			buildoptions "/MTd"

    filter { "configurations:Release"}
        defines { "NDEBUG" }
        optimize "On"
		filter "system:windows"
			buildoptions "/MT"

    filter { "configurations:Dist"}
		defines { "NDEBUG"}
		optimize "On"
		filter "system:windows"
			buildoptions "/MT"


project "TsoEditor"
	location "%{prj.name}"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir  ("bin/"..outputdir.."/%{prj.name}")
	objdir  ("bin-int/"..outputdir.."/%{prj.name}")
    
	files { "%{prj.name}/src/**.h", "%{prj.name}/src/**.cpp"}

	defines
	{
		"YAML_CPP_STATIC_DEFINE",
	}

    includedirs
	{
        "TsoEngine/third_party/spdlog/include",
		"TsoEngine/src",
		"TsoEngine/src/Tso",
		"TsoEngine/third_party/glm",
		"TsoEngine/third_party/imgui",
		"TsoEngine/third_party/entt",
		"TsoEngine/third_party/yaml-cpp/include",
		--"Hazel/vendor/imguizmo",
		--"Hazel/vendor/Mono/include",
		--"Hazel/vendor/box2D/include"
	}

	links { "TsoEngine" }
	

    filter { "system:Windows" }
	    systemversion "latest"
		 defines { "TSO_PLATFORM_WINDOWS"}
		 			
		links
		{
		    -- windows needed libs for mono
			"Ws2_32.lib",
			"Bcrypt.lib",
			"Version.lib",
			"Winmm.lib"
		}

	filter "system:macosx"
		defines{
				"TSO_PLATFORM_MACOSX",
				"TSO_EDITOR"

		}
		links{
			"Cocoa.framework",
			"IOKit.framework",
			"CoreVideo.framework",
			"OpenGL.framework"
		}
		

    filter { "configurations:Debug"}
        defines { "TSO_DEBUG"}
        symbols "On"
		filter "system:windows"
			buildoptions "/MTd"
		
		--libdirs
		--{
		--	"%{VULKAN_SDK}/Lib"
		--}
		
		--links
		--{
		--	"shaderc_sharedd.lib"
		--}
		

    filter { "configurations:Release"}
        defines { "TSO_RELEASE" }
        optimize "On"
		filter "system:windows"
			buildoptions "/MT"

    filter { "configurations:Dist"}
		defines { "TSO_DIST"}
		optimize "On"
		filter "system:windows"
			buildoptions "/MT"


--project "Hazel-ScriptCore"
	--location "%{prj.name}"
	--kind "SharedLib"
	--language "C#"
	--dotnetframework "4.7.2"
	
	--targetdir ("%{prj.name}/Build")
	--objdir ("%{prj.name}/Intermediates")

	--files 
	--{
	--	"%{prj.name}/Scripts/**.cs"
	--}