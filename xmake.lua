-- include subprojects
includes("lib/commonlibf4")

-- name and version
local plugin_name = "Fallout4RichPresence"
local plugin_version = "0.1.0"
local plugin_version_major, plugin_version_minor, plugin_version_patch = plugin_version:match("^(%d+)%.(%d+)%.(%d+)$")

-- set project constants
set_project(plugin_name)
set_version(plugin_version)
set_license("GPL-3.0")
set_languages("c++26")
set_toolchains("msvc")
set_warnings("allextra")
set_encodings("utf-8")

-- set policies
set_policy("build.optimization.lto", true)

-- add dependencies
add_requires("nlohmann_json v3.12.0")

-- add common rules
add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- add options
set_config("commonlib_toml", true)

-- commonlib pins itself to c++23; raise it so headers and sources share one _HAS_CXX26 state
for _, dep in ipairs({ "commonlib-shared", "commonlibf4" }) do
    target(dep, function()
        set_languages("c++26")
    end)
end

-- define targets
target(plugin_name, function()
    add_cxxflags("/permissive-", "/Zc:preprocessor", { public = true })

    -- add commonlibf4 plugin
    add_rules("commonlibf4.plugin", {
        name = plugin_name,
        author = "Kuz",
        description = "Discord Rich Presence for Fallout 4",
        plugin_template = "res/commonlibf4-plugin.cpp.in"
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    add_installfiles("data/F4SE/Plugins/Fallout4RichPresence.toml", { prefixdir = "F4SE/Plugins" })
    add_packages("nlohmann_json")
    set_pcxxheader("src/pch.h")

    -- pass name and version
    add_defines(
        'PLUGIN_NAME="' .. plugin_name .. '"',
        'PLUGIN_VERSION="' .. plugin_version .. '"',
        "PLUGIN_VERSION_MAJOR=" .. plugin_version_major,
        "PLUGIN_VERSION_MINOR=" .. plugin_version_minor,
        "PLUGIN_VERSION_PATCH=" .. plugin_version_patch
    )
end)

target("FormatTemplateTests", function()
    set_kind("binary")
    set_default(false)
    add_deps("commonlibf4")
    add_files("tests/FormatTemplateTests.cpp", "src/Presence/FormatTemplate.cpp")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
end)
