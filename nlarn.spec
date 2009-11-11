Name: nlarn
Summary: remake of the roguelike Larn
Version: 0.5.1
Release: 1
Source: http://downloads.sourceforge.net/project/nlarn/nlarn/0.5.1/nlarn-0.5.1.tar.gz
License: GPL v3
Group: Amusements/Games

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
install -g games -o games -m 644 lib/fortune lib/maze lib/nlarn.hlp lib/nlarn.msg $RPM_BUILD_ROOT/usr/share/games/nlarn

%clean
rm -rf $RPM_BUILD_ROOT

%files
%attr(2755, games, games) /usr/games/nlarn
%attr(775, games, games) /usr/share/games/nlarn
%attr(644, games, games) /usr/share/games/nlarn/*
%doc LICENSE nlarn.ini-sample

%changelog
* Wed Nov 11 2009 Joachim de Groot <jdegroot@web.de> 
  - removed BUILD.txt
  - Updated for version 0.5.1
* Tue Sep 22 2009 Joachim de Groot <jdegroot@web.de>
  - Initial spec file
