# $Id: id3lib.spec,v 1.1 2007/11/12 21:32:37 pindakaasmod Exp $

%define name    id3lib
%define	version	3.8.3
%define	release	1
%define	prefix	/usr

Name:		%{name}
Version:	%{version}
Release:	%{release}
Summary:	A software library for manipulating ID3v1 and ID3v2 tags.
Source:		http://download.sourceforge.net/id3lib/%{name}-%{version}.tar.gz
URL:		http://id3lib.sourceforge.net
Group:		System Environment/Libraries
BuildRoot:	%{_tmppath}/%{name}-buildroot
Copyright:	LGPL
Prefix:         %{_prefix}
Docdir:		%{prefix}/doc
Requires:       zlib

%description
This package provides a software library for manipulating ID3v1 and ID3v2 tags.
It provides a convenient interface for software developers to include 
standards-compliant ID3v1/2 tagging capabilities in their applications.  
Features include identification of valid tags, automatic size conversions, 
(re)synchronisation of tag frames, seamless tag (de)compression, and optional
padding facilities. Additionally, it can tell mp3 header info, like bitrate etc.

%package	devel
Summary:	Headers for developing programs that will use id3lib
Group:		Development/Libraries
Requires:       %{name}

%description	devel
This package contains the headers that programmers will need to develop
applications which will use id3lib, the software library for ID3v1 and ID3v2
tag manipulation.

%package        doc
Summary:	Documentation for developing programs that will use id3lib
Group:		Documentation

%description	doc
This package contains the documentation of the id3lib API that programmers will
need to develop applications which will use id3lib, the software library for ID3v1
and ID3v2 tag manipulation.

%package        examples
Summary:	Example applications that make use of the id3lib library
Group:		Applications/File
Requires:       %{name}

%description	examples
This package contains simple example applications that make use of id3lib, a 
software library for ID3v1 and ID3v2 tag manipulation.

%prep
%setup -q

%build

CXXFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%prefix --enable-debug=no

%ifnarch noarch

uname -a|grep SMP && make -j 2 || make

%endif

%install
rm -rf $RPM_BUILD_ROOT

%ifnarch noarch

make prefix=$RPM_BUILD_ROOT%{prefix} install

%else

make docs
 
# strip down the doc and examples directories so we can copy w/impunity
for i in doc/ examples/; do \
  find $i                   \
  \(  -name 'Makefile*' -or \
      -name '*.ps.gz'   -or \
      -name '*.pdf'         \
  \)  -exec rm {} \; ; done

%endif

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%ifnarch noarch

%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog HISTORY NEWS README THANKS TODO
%{prefix}/lib/*.so.*

%files devel
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog HISTORY NEWS README THANKS TODO
%{prefix}/include/id3*.h
%{prefix}/include/id3
%{prefix}/lib/*.la
%{prefix}/lib/*.a
%{prefix}/lib/*.so

%files examples
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog HISTORY NEWS README THANKS TODO
%{prefix}/bin/id3*

%else

%files doc
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog HISTORY NEWS README THANKS TODO
%doc doc/*.* doc/api examples

%endif

%changelog
* Sat Sep 08 2001 Cedric Tefft <cedric@earthling.net> 3.8.0pre2
- Version 3.8.0pre2

* Mon Nov 20 2000 Scott Thomas Haug <scott@id3.org> 3.8.0pre1-1
- Version 3.8.0pre1

* Thu Sep 14 2000 Scott Thomas Haug <scott@id3.org> 3.7.13-1
- Version 3.7.13

* Sat Aug 26 2000 Scott Thomas Haug <scott@id3.org> 3.7.12-2
- Removed -mpreferred-stack-boundary option from RPM_OPT_FLAGS for RedHat 6.2

* Fri Jul 07 2000 Scott Thomas Haug <scott@id3.org> 3.7.12-1
- Version 3.7.12

* Fri Jul 05 2000 Scott Thomas Haug <scott@id3.org> 3.7.11-1
- Version 3.7.11

* Fri Jun 23 2000 Scott Thomas Haug <scott@id3.org> 3.7.10-1
- Version 3.7.10

* Wed May 24 2000 Scott Thomas Haug <scott@id3.org> 3.7.9-1
- Version 3.7.9

* Wed May 10 2000 Scott Thomas Haug <scott@id3.org> 3.7.8-1
- Version 3.7.8

* Wed May 10 2000 Scott Thomas Haug <scott@id3.org> 3.7.7-1
- Version 3.7.7

* Wed May 03 2000 Scott Thomas Haug <scott@id3.org> 3.7.6-1
- Version 3.7.6

* Fri Apr 28 2000 Scott Thomas Haug <scott@id3.org> 3.7.5-1
- Version 3.7.5

* Wed Apr 26 2000 Scott Thomas Haug <scott@id3.org> 3.7.4-1
- Version 3.7.4

* Mon Apr 24 2000 Scott Thomas Haug <scott@id3.org> 3.7.3-1
- Version 3.7.3
- Added explicit RPM_OPT_FLAGS def based on arch, since -fno-exceptions and
  -fno-rtti are part of the default flags in rpmrc and we need both exceptions
  and rtti (exceptions uses rtti)

* Fri Apr 21 2000 Scott Thomas Haug <scott@id3.org> 3.7.2-1
- Version 3.7.2
- More conditional blocks for noarch
- More thorough cleaning of files for documentation
- Updated html directory

* Thu Apr 20 2000 Scott Thomas Haug <scott@id3.org> 3.7.1-2
- Fixed date of changelog entry for 3.7.1-1
- Added conditional blocks so docs only get built for noarch target

* Wed Apr 19 2000 Scott Thomas Haug <scott@id3.org> 3.7.1-1
- Version 3.7.1
- Removed zlib-devel requirement from devel
- Added doc package to distribute documentation
- Added examples package to distribute binary examples (id3tag, id3info, ...)
- Moved doc/ and examples/ source files from devel to doc package

* Mon Apr 17 2000 Scott Thomas Haug <scott@id3.org> 3.7.0-1
- First (s)rpm build
