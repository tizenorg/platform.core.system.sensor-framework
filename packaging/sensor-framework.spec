#sbs-git:slp/pkgs/s/sensor-framework sensor-framework 0.2.5 f585f766aa864c3857e93c776846771899a4fa41
Name:       sensor-framework
Summary:    Sensor framework
Version: 0.2.31
Release:    1
Group:      Framework/system
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	sensor-framework.manifest

%ifarch %{arm}
  #arm build
  %define	 _archtype arch_arm
%else
  #ix86 build
  %if 0%{?simulator}
   #emul build target
   %define	 _archtype arch_sdk
  %else
   #IA build target
   %define	 _archtype arch_ia
  %endif
%endif
Requires(post): /usr/bin/vconftool

BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(sf_common)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(libsystemd-daemon)
%{?systemd_requires}

%description
Sensor framework

%prep
%setup -q
cp %{SOURCE1001} .

%build
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DPLATFORM_ARCH=%{_archtype}

make %{?jobs:-j%jobs}

%install
%make_install

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
mkdir -p %{buildroot}%{_libdir}/systemd/system/sockets.target.wants
ln -s ../sensor-framework.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/sensor-framework.service
ln -s ../sensor-framework.socket  %{buildroot}%{_libdir}/systemd/system/sockets.target.wants/sensor-framework.socket

%post 
vconftool set -t int memory/private/sensor/10001 0 -i
vconftool set -t int memory/private/sensor/10002 0 -i
vconftool set -t int memory/private/sensor/10004 0 -i
vconftool set -t int memory/private/sensor/10008 0 -i
vconftool set -t int memory/private/sensor/10010 0 -i
vconftool set -t int memory/private/sensor/20001 0 -i
vconftool set -t int memory/private/sensor/20002 0 -i
vconftool set -t int memory/private/sensor/20004 0 -i
vconftool set -t int memory/private/sensor/200001 0 -i
vconftool set -t int memory/private/sensor/40001 0 -i
vconftool set -t int memory/private/sensor/40002 0 -i
vconftool set -t int memory/private/sensor/800001 0 -i
vconftool set -t int memory/private/sensor/800002 0 -i
vconftool set -t int memory/private/sensor/800004 0 -i
vconftool set -t int memory/private/sensor/800008 0 -i
vconftool set -t int memory/private/sensor/800010 0 -i
vconftool set -t int memory/private/sensor/800020 0 -i
vconftool set -t int memory/private/sensor/800040 0 -i
vconftool set -t int memory/private/sensor/800080 0 -i
vconftool set -t int memory/private/sensor/800100 0 -i
vconftool set -t int memory/private/sensor/800200 0 -i
vconftool set -t int memory/private/sensor/800400 0 -i
vconftool set -t int memory/private/sensor/800800 0 -i
vconftool set -t int memory/private/sensor/80001 0 -i
vconftool set -t int memory/private/sensor/80002 0 -i
vconftool set -t int memory/private/sensor/poweroff 0 -i

systemctl daemon-reload

%files
%manifest sensor-framework.manifest
%{_bindir}/sf_server
%attr(0644,root,root)/usr/etc/sf_data_stream.conf
%attr(0644,root,root)/usr/etc/sf_filter.conf
%attr(0644,root,root)/usr/etc/sf_processor.conf
%attr(0644,root,root)/usr/etc/sf_sensor.conf
%{_libdir}/systemd/system/sensor-framework.service
%{_libdir}/systemd/system/sensor-framework.socket
%{_libdir}/systemd/system/multi-user.target.wants/sensor-framework.service
%{_libdir}/systemd/system/sockets.target.wants/sensor-framework.socket
/usr/share/license/%{name}

