#sbs-git:slp/pkgs/s/sensor-framework sensor-framework 0.2.5 f585f766aa864c3857e93c776846771899a4fa41
Name:       sensor-framework
Summary:    Sensor framework
Version: 0.2.26
Release:    1
Group:      TO_BE/FILLED_IN
License:    Apache 2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    sensor-framework.service

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

%description
Sensor framework

%prep
%setup -q

%build
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DPLATFORM_ARCH=%{_archtype}

make %{?jobs:-j%jobs}

%install
%make_install

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/
ln -s ../sensor-framework.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/sensor-framework.service

# FIXME: remove initscripts after we start using systemd
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc3.d
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc4.d
ln -s ../init.d/sfsvc %{buildroot}%{_sysconfdir}/rc.d/rc3.d/S40sfsvc
ln -s ../init.d/sfsvc %{buildroot}%{_sysconfdir}/rc.d/rc4.d/S40sfsvc

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

%files
%manifest sensor-framework.manifest
%attr(0755,root,root) %{_sysconfdir}/rc.d/init.d/sfsvc
%{_sysconfdir}/rc.d/rc3.d/S40sfsvc
%{_sysconfdir}/rc.d/rc4.d/S40sfsvc
%{_bindir}/sf_server
%attr(0644,root,root)/usr/etc/sf_data_stream.conf
%attr(0644,root,root)/usr/etc/sf_filter.conf
%attr(0644,root,root)/usr/etc/sf_processor.conf
%attr(0644,root,root)/usr/etc/sf_sensor.conf
%{_libdir}/systemd/system/sensor-framework.service
%{_libdir}/systemd/system/multi-user.target.wants/sensor-framework.service
/usr/share/license/%{name}

