Name:    nlarn
Version: 0.7.2
Release: 1
Summary: A remake of the roguelike game Larn
Group:   Amusements/Games
License: GPL v3
URL:     https://nlarn.github.io/
Source:  http://downloads.sourceforge.net/project/nlarn/nlarn/%{version}/nlarn-%{version}.tar.gz

BuildRequires: gcc glib2-devel lua-devel ncurses-devel zlib-devel

%description

%prep
%setup

%build
make config=release SETGID=Y

%install
mkdir -p %{buildroot}/%{_bindir}
mkdir -p %{buildroot}/%{_datadir}/%{name}
mkdir -p %{buildroot}/var/games/%{name}
install -g games -o games -m 2755 nlarn %{buildroot}/%{_bindir}
install lib/fortune lib/maze lib/nlarn.hlp lib/nlarn.msg lib/monsters.lua %{buildroot}/%{_datadir}/%{name}
touch %{buildroot}/var/games/%{name}/highscores

%files
%defattr(-,root,root,-)
%attr(2755, root, games) %{_bindir}/nlarn
%{_datadir}/%{name}/*
%config(noreplace) %attr (0664,root,games) /var/games/nlarn/highscores
%doc LICENSE README.md Changelog.txt nlarn.ini-sample lib/maze_doc.txt

%changelog
* Thu May 17 2018 Joachim de Groot <jdegroot@web.de>
  - updated for version 0.7.2
  - compile with support for shared scoreboard
  - enable optimisation
  - fix paths
* Fri May 25 2012 Joachim de Groot <jdegroot@web.de>
  - updated for version 0.7.1
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
