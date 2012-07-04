tell application "Finder"
	tell disk "##VOLNAME##"
		open
		set current view of container window to icon view
		set toolbar visible of container window to false
		set statusbar visible of container window to false
		set the bounds of container window to {400, 100, 800, 430}
		set theViewOptions to the icon view options of container window
		set arrangement of theViewOptions to not arranged
		set icon size of theViewOptions to 48
		set background picture of theViewOptions to file ".background:dmg_background.png"
		make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
		set position of item "NLarn" of container window to {100, 120}
		set position of item "Applications" of container window to {300, 115}
		set position of item "Changelog.txt" of container window to {100, 250}
		set position of item "README.txt" of container window to {200, 250}
		set position of item "License" of container window to {300, 250}
		update without registering applications
		delay 5
	end tell
end tell
