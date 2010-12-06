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

-- simple function to determine Linux distribution 
-- will not work if lsb_release is not available 
-- (most modern distribution should provide it)
function get_linux_distribution()
	fh = io.popen("lsb_release -is")
	distribution = fh:read()
	fh:close()

	return distribution
end

solution "NLarn"
	configurations { "Debug", "Release" }

	project "nlarn"
		kind "ConsoleApp"
		language "C"
		files { "inc/*.h", "src/*.c" }
		includedirs { "inc" }
		defines { "G_DISABLE_DEPRECATED" }

		links { "glib-2.0", "m", "z" }

		-- Debian and Ubuntu have a specific naming convention for the lua package 
		-- fortunately it can be configured with pkg-config
		if os.is("linux") and (get_linux_distribution() == "Debian" 
				or get_linux_distribution() == "Ubuntu")
			then
			includedirs { get_dirs("include", "lua5.1") }
			links { "lua5.1" }
			libdirs { get_dirs("lib", "lua5.1") }
		else
			links { "lua" }
		end

		configuration "Debug"
			defines { "DEBUG", "LUA_USE_APICHECK" }
			flags { "Symbols", "ExtraWarnings" }

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize" }

		configuration "bsd"
			includedirs { "/usr/local/include/lua51" }
			libdirs { "/usr/local/lib/lua51" }

		configuration "windows" 
			-- do not include unnecessary header files
			defines { "WIN32_LEAN_AND_MEAN", "NOGDI" }
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

