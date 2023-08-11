add_rules("mode.debug", "mode.release")
set_languages("c99", "c++11")

target("demo")    
    set_kind("binary")
    add_files("*.cpp")
    add_linkdirs("frida")
    add_links("frida-gum")
    if is_plat("linux") then
        add_links("dl", "pthread", "resolv")
    end
    add_cxxflags("-O0", "-g", "-fno-lto", "-fno-inline-functions", "-fno-inline")

