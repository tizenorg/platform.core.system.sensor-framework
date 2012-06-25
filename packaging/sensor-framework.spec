Name:       sensor-framework
Summary:    Sensor framework
Version:    0.2.5
Release:    1
Group:      TO_BE/FILLED_IN
License:    LGPL
Source0:    %{name}-%{version}.tar.gz
Source1:    sensor-framework.service
Source1001: packaging/sensor-framework.manifest 

Requires(post): /usr/bin/vconftool

BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(sf_common)
BuildRequires:  pkgconfig(vconf)

%description
Sensor framework

%prep
%setup -q

%build
cp %{SOURCE1001} .
cmake . -DCMAKE_INSTALL_PREFIX=/usr

make %{?jobs:-j%jobs}

%install
%make_install

mkdir -p %{buildroot}%{_libdir}/systemd/user/tizen-middleware.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/user/
ln -s ../sensor-framework.service %{buildroot}%{_libdir}/systemd/user/tizen-middleware.target.wants/sensor-framework.service

# FIXME: remove initscripts after we start using systemd
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc3.d
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc4.d
ln -s ../init.d/sfsvc %{buildroot}%{_sysconfdir}/rc.d/rc3.d/S40sfsvc
ln -s ../init.d/sfsvc %{buildroot}%{_sysconfdir}/rc.d/rc4.d/S40sfsvc


%post 
vconftool set -t int memory/sensor/10001 0 -i
vconftool set -t int memory/sensor/10002 0 -i
vconftool set -t int memory/sensor/10004 0 -i
vconftool set -t int memory/sensor/10008 0 -i
vconftool set -t int memory/sensor/20001 0 -i
vconftool set -t int memory/sensor/20002 0 -i
vconftool set -t int memory/sensor/20004 0 -i
vconftool set -t int memory/sensor/200001 0 -i
vconftool set -t int memory/sensor/40001 0 -i
vconftool set -t int memory/sensor/40002 0 -i
vconftool set -t int memory/sensor/800001 0 -i
vconftool set -t int memory/sensor/800002 0 -i
vconftool set -t int memory/sensor/800004 0 -i
vconftool set -t int memory/sensor/800008 0 -i
vconftool set -t int memory/sensor/800010 0 -i
vconftool set -t int memory/sensor/800020 0 -i
vconftool set -t int memory/sensor/800040 0 -i
vconftool set -t int memory/sensor/80001 0 -i
vconftool set -t int memory/sensor/80002 0 -i


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
%{_libdir}/systemd/user/sensor-framework.service
%{_libdir}/systemd/user/tizen-middleware.target.wants/sensor-framework.service

