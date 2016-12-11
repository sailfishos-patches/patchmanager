Name:       patchmanager

%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

Summary:    Patchmanager allows you to manage Sailfish OS patches
Version:    2.0.0
Release:    1
Group:      Qt/Qt
License:    TODO
URL:        https://github.com/sailfishos-patches/patchmanager
Source0:    %{name}-%{version}.tar.bz2
Requires:   ausmt
Requires:   unzip
Requires:   jolla-settings-%{name} = %{version}-%{release}
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  pkgconfig(sailfishapp) >= 0.0.10

%description
patchmanager allows managing system patch
on your SailfishOS device easily. This package
contains the system daemon.

%package -n jolla-settings-%{name}
Summary:    Jolla settings plugin for Patchmanager
Group:      Qt/Qt
Requires:   %{name} = %{version}-%{release}
Conflicts:  %{name}-ui

%description -n jolla-settings-%{name}
Patchmanager allows managing system patch
on your SailfishOS device easily. This package
contains the Jolla settings plugin.


%prep
%setup -q -n %{name}-%{version}

%build

%qtc_qmake5 

%qtc_make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install

%preun
if [ -d /var/lib/patchmanager/ausmt/patches/sailfishos-patchmanager-unapplyall ]; then
/usr/sbin/patchmanager -u sailfishos-patchmanager-unapplyall || true
fi

dbus-send --system --type=method_call \
--dest=org.SfietKonstantin.patchmanager /org/SfietKonstantin/patchmanager \
org.SfietKonstantin.patchmanager.quit

%post
dbus-send --system --type=method_call \
--dest=org.freedesktop.DBus / org.freedesktop.DBus.ReloadConfig

%postun
dbus-send --system --type=method_call \
--dest=org.freedesktop.DBus / org.freedesktop.DBus.ReloadConfig

%files
%defattr(-,root,root,-)
%{_sbindir}/%{name}
%{_datadir}/%{name}/tools
%{_datadir}/dbus-1/
%{_sysconfdir}/dbus-1/system.d/
%{_datadir}/patchmanager/patches/sailfishos-patchmanager-unapplyall/patch.json
%{_datadir}/patchmanager/patches/sailfishos-patchmanager-unapplyall/unified_diff.patch

%files -n jolla-settings-%{name}
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/org/SfietKonstantin/%{name}
%{_datadir}/%{name}/data
%{_datadir}/jolla-settings/pages/%{name}
%{_datadir}/jolla-settings/entries/%{name}.json
%{_datadir}/%{name}/icons/icon-m-patchmanager.png
%attr(644,nemo,nemo) %ghost /home/nemo/.config/patchmanager2.conf
