function get_dirs(which, lib)
  local paths = { }

  local query = ""
  local sep = ""

  if which == "lib" then
    query = "--libs-only-L"
    sep = "-L"
  elseif which == "include" then
    query = "--cflags-only-I"
    sep = "-I"
  end

  fh = io.popen("pkg-config " .. query .. " " .. lib)
  out = string.explode(fh:read(), " ")
  fh:close()

  for i=1,#out do
      if string.startswith(out[i], sep) then
        local path = out[i]:gsub(sep,"")
      table.insert(paths, path)
      end
  end

  return paths
end

-- Debian and Ubuntu have a specific naming convention for the lua
-- package; fortunately it can be configured with pkg-config
if os.is("linux") and os.isfile("/etc/debian_version") then
  lua = "lua5.1"
else
  lua = "lua"
end

solution "NLarn"
  configurations { "Debug", "Release" }

  project "nlarn"
    kind "ConsoleApp"
    language "C"
    files { "src/*.c" }
    includedirs { "inc", get_dirs("include", "glib-2.0") }
    libdirs { get_dirs("lib", "glib-2.0") }
    links { "glib-2.0", lua, "m", "z" }
    defines { "G_DISABLE_DEPRECATED" }
    -- assume gcc or clang
    buildoptions { "-std=c99", "-Wextra" }

    configuration "Debug"
      defines { "DEBUG", "LUA_USE_APICHECK" }
      flags { "Symbols", "ExtraWarnings" }

    configuration "Release"
      defines { "G_DISABLE_ASSERT" }
      flags { "Optimize" }

    configuration "windows"
      -- do not include unnecessary header files
      defines { "WIN32_LEAN_AND_MEAN", "NOGDI" }
      links { "pdcurses" }

    configuration "not windows"
      includedirs { get_dirs("include", lua) }
      libdirs { get_dirs("lib", lua) }
      links { "ncurses", "panel" }
