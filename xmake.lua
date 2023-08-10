add_rules("mode.debug", "mode.release")
set_languages("c99", "c++14")

target("demo")    
    set_kind("binary")
    add_files("*.cpp")
    add_linkdirs("frida")
    add_links("frida-gum")    
    add_cxxflags("-O0", "-g", "-fno-lto", "-fno-inline-functions", "-fno-inline")

