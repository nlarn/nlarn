Name: nlarn
Version: 0.7.0
Release: 1
Summary: A remake of the roguelike game Larn

Group: Amusements/Games
License: GPL v3
URL: http://nlarn.sourceforge.net
Source: http://downloads.sourceforge.net/project/nlarn/nlarn/%{version}/nlarn-%{version}.tar.gz

BuildRequires: glib2-devel lua-devel ncurses-devel zlib-devel 

%description

%prep
%setup
premake4 gmake

%build
make

%install
mkdir -p $RPM_BUILD_ROOT/usr/games
mkdir -p $RPM_BUILD_ROOT/usr/share/games/nlarn
install -g games -o games -m 2755 nlarn $RPM_BUILD_ROOT/usr/games
install lib/fortune lib/maze lib/nlarn.hlp lib/nlarn.msg lib/monsters.lua $RPM_BUILD_ROOT/usr/share/games/nlarn
touch $RPM_BUILD_ROOT/usr/share/games/nlarn/highscores

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%attr(2755, root, games) /usr/games/nlarn
/usr/share/games/nlarn/*
%config(noreplace) %attr (0664,root,games) /usr/share/games/nlarn/highscores
%doc LICENSE README.txt Changelog.txt nlarn.ini-sample lib/maze_doc.txt

%changelog
* Sat Oct 23 2010 Joachim de Groot <jdegroot@web.de>
  - updated for version 0.7.0
  - added maze_doc.txt to documentation
* Sun Sep 19 2010 Joachim de Groot <jdegroot@web.de>
  - updated for version 0.6.1
  - added missing dependency for lua
* Sun Apr 25 2010 Joachim de Groot <jdegroot@web.de> 
  - updated for version 0.6
* Sat Apr 03 2010 Joachim de Groot <jdegroot@web.de> 
  - added Changelog.txt
  - added highscore file
  - fixed file permissions
  - updated for version 0.5.4
* Sat Jan 30 2010 Joachim de Groot <jdegroot@web.de> 
  - updated for version 0.5.3
* Sun Nov 22 2009 Joachim de Groot <jdegroot@web.de> 
  - added README.txt
  - updated for version 0.5.2
* Wed Nov 11 2009 Joachim de Groot <jdegroot@web.de> 
  - removed BUILD.txt
  - Updated for version 0.5.1
* Tue Sep 22 2009 Joachim de Groot <jdegroot@web.de>
  - Initial spec file
