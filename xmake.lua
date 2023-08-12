add_rules("mode.debug", "mode.release")
set_languages("c99", "c++11")

target("demo")    
    set_kind("binary")
    add_files("*.cpp")
    add_linkdirs("frida")
    add_links("frida-gum")
    if is_plat("linux") then
        add_links("dl", "pthread", "resolv")
    elseif is_plat("windows") then 
        add_defines("NOMINMAX")
        add_cxxflags("/utf-8")
        add_cxxflags("/Zc:__cplusplus")
        add_links("Advapi32", "Ole32", "Shell32", "User32")
    end
    add_cxxflags("-O0", "-g", "-fno-lto", "-fno-inline-functions", "-fno-inline")

