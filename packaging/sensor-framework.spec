#sbs-git:slp/pkgs/s/sensor-framework sensor-framework 0.2.5 f585f766aa864c3857e93c776846771899a4fa41
Name:       sensor-framework
Summary:    Sensor framework
Version: 0.2.9
Release:    1
Group:      TO_BE/FILLED_IN
License:    LGPL
Source0:    %{name}-%{version}.tar.gz

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
cmake . -DCMAKE_INSTALL_PREFIX=/usr

make %{?jobs:-j%jobs}

%install
%make_install

%post 
vconftool set -t int memory/private/sensor/10001 0 -i
vconftool set -t int memory/private/sensor/10002 0 -i
vconftool set -t int memory/private/sensor/10004 0 -i
vconftool set -t int memory/private/sensor/10008 0 -i
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
vconftool set -t int memory/private/sensor/80001 0 -i
vconftool set -t int memory/private/sensor/80002 0 -i
vconftool set -t int memory/private/sensor/poweroff 0 -i

mkdir -p /etc/rc.d/rc3.d
mkdir -p /etc/rc.d/rc4.d
ln -s /etc/rc.d/init.d/sfsvc /etc/rc.d/rc3.d/S40sfsvc
ln -s /etc/rc.d/init.d/sfsvc /etc/rc.d/rc4.d/S40sfsvc

%postun
rm -f /etc/rc.d/rc3.d/S40sfsvc
rm -f /etc/rc.d/rc4.d/S40sfsvc

%files
%defattr(-,root,root,-)
/usr/bin/sf_server
%{_sysconfdir}/rc.d/init.d/sfsvc
%attr(0644,root,root)/usr/etc/sf_data_stream.conf
%attr(0644,root,root)/usr/etc/sf_filter.conf
%attr(0644,root,root)/usr/etc/sf_processor.conf
%attr(0644,root,root)/usr/etc/sf_sensor.conf

