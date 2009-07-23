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

function get_pkgconfig(query)
	fh = io.popen("pkg-config " .. query)
	out = fh:read()
	fh:close()

	return out
end

solution "NLarn"
	configurations { "Debug", "Release" }

	project "nlarn"
		kind "ConsoleApp"
		language "C"
		files { "inc/*.h", "src/*.c" }
		includedirs { "inc" }
		defines { "G_DISABLE_DEPRECATED" }

		links { "glib-2.0" }

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols", "ExtraWarnings" }

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize" }

		configuration "windows" 
			links { "pdcurses" }

		configuration "not windows"
			includedirs { "/usr/include/ncurses" } 
			links { "ncurses", "panel" }

		configuration { "gmake" }
			buildoptions { get_pkgconfig("--cflags glib-2.0") }
			linkoptions {  get_pkgconfig("--libs glib-2.0") }

		configuration { "not gmake" }
			includedirs { get_dirs("include", "glib-2.0") }
			libdirs { get_dirs("lib", "glib-2.0") }

