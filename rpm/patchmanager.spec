%define theme sailfish-default

# These macros should already be defined in the RPMbuild environment, see: rpm --showrc
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{!?qmake5_install:%define qmake5_install make install INSTALL_ROOT=%{buildroot}}

Name:       patchmanager

Summary:    Patchmanager allows for managing Sailfish OS patches
Version:    3.2.1
Release:    1
Group:      Qt/Qt
License:    BSD-3-Clause
URL:        https://github.com/sailfishos-patches/patchmanager
Source0:    %{name}-%{version}.tar.bz2
Requires:   unzip
Requires:   patch
Requires:   grep
Requires:   sed
Requires:   sailfish-version >= 3.4.0
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  pkgconfig(sailfishapp) >= 0.0.10
BuildRequires:  sailfish-svg2png >= 0.1.5
BuildRequires:  pkgconfig(nemonotifications-qt5)
BuildRequires:  qt5-qtdeclarative-devel-tools
BuildRequires:  pkgconfig(systemd)
BuildRequires:  pkgconfig(libshadowutils)
BuildRequires:  qt5-qttools-linguist
BuildRequires:  pkgconfig(rpm)
BuildRequires:  pkgconfig(popt)

%description
%{summary} on your device easily.

%prep
%setup -q -n %{name}-%{version}

%build

%qtc_qmake5 "PROJECT_PACKAGE_VERSION=%{version}"
%qtc_make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%qmake5_install

mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants/
ln -s ../dbus-org.SfietKonstantin.patchmanager.service %{buildroot}%{_unitdir}/multi-user.target.wants/

mkdir -p %{buildroot}%{_unitdir}/timers.target.wants/
ln -s ../checkForUpdates-org.SfietKonstantin.patchmanager.timer %{buildroot}%{_unitdir}/timers.target.wants/

mkdir -p %{buildroot}/%{_userunitdir}/lipstick.service.wants/
ln -s ../lipstick-patchmanager.service %{buildroot}/%{_userunitdir}/lipstick.service.wants/

mkdir -p %{buildroot}%{_datadir}/%{name}/patches

%pre
export NO_PM_PRELOAD=1
case "$*" in
1)
echo "Installing %{name}: pre section"
;;
2)
echo "Updating %{name}: pre section"
# Unapply all patches if Patchmanager 2.x is installed
if [ ! -d /var/lib/patchmanager/ausmt/patches/ ]
then
    exit 0
else
    /usr/sbin/patchmanager --unapply-all || true
fi
if [ -n "$(ls -A /var/lib/patchmanager/ausmt/patches/)" ]
then
    echo "Unapply all patches before updating %{name}!"
    exit 1
fi
;;
*) echo "Case $* is not handled in pre section of %{name}!"
esac

%post
export NO_PM_PRELOAD=1
case "$*" in
1)
echo "Installing %{name}: post section"
;;
2)
echo "Updating %{name}: post section"
;;
*) echo "Case $* is not handled in post section of %{name}!"
esac
sed -i '/libpreload%{name}/ d' /etc/ld.so.preload
echo '%{_libdir}/libpreload%{name}.so' >> /etc/ld.so.preload
/sbin/ldconfig
if ! grep -qsF 'include whitelist-common-%{name}.local' /etc/firejail/whitelist-common.local; then
   echo 'include whitelist-common-%{name}.local' >> /etc/firejail/whitelist-common.local
fi
dbus-send --system --type=method_call --dest=org.freedesktop.DBus / org.freedesktop.DBus.ReloadConfig
systemctl daemon-reload
systemctl-user daemon-reload
systemctl restart dbus-org.SfietKonstantin.patchmanager.service
systemctl restart checkForUpdates-org.SfietKonstantin.patchmanager.timer

%preun
export NO_PM_PRELOAD=1
case "$*" in
0)
echo "Uninstalling %{name}: preun section"
systemctl stop dbus-org.SfietKonstantin.patchmanager.service
;;
1)
echo "Updating %{name}: preun section"
;;
*) echo "Case $* is not handled in preun section of %{name}!"
esac

%postun
export NO_PM_PRELOAD=1
case "$*" in
0)
echo "Uninstalling %{name}: postun section"
sed -i '/whitelist-common-%{name}.local/ d' /etc/firejail/whitelist-common.local
sed -i '/libpreload%{name}/ d' /etc/ld.so.preload
/sbin/ldconfig
rm -rf /tmp/patchmanager
rm -f /tmp/patchmanager-socket
;;
1)
echo "Updating %{name}: postun section"
;;
*) echo "Case $* is not handled in postun section of %{name}!"
esac
dbus-send --system --type=method_call --dest=org.freedesktop.DBus / org.freedesktop.DBus.ReloadConfig
systemctl daemon-reload
systemctl-user daemon-reload

%files
%defattr(-,root,root,-)

%doc README.md
%doc doc/example_legacy_patch.json.md
%doc doc/example_patch.json.md
%doc doc/example_patch-rpm.spec

%{_bindir}/%{name}-dialog
%{_sbindir}/%{name}
%dir %{_datadir}/%{name}/patches
%{_datadir}/%{name}/tools
%{_datadir}/dbus-1/
%{_sysconfdir}/dbus-1/system.d/
%{_unitdir}/dbus-org.SfietKonstantin.patchmanager.service
%{_unitdir}/multi-user.target.wants/dbus-org.SfietKonstantin.patchmanager.service
%{_unitdir}/checkForUpdates-org.SfietKonstantin.patchmanager.service
%{_unitdir}/checkForUpdates-org.SfietKonstantin.patchmanager.timer
%{_unitdir}/timers.target.wants/checkForUpdates-org.SfietKonstantin.patchmanager.timer
%{_sharedstatedir}/environment/patchmanager/10-dbus.conf
%{_userunitdir}/dbus-org.SfietKonstantin.patchmanager.service
%{_userunitdir}/lipstick-patchmanager.service
%{_userunitdir}/lipstick.service.wants/lipstick-patchmanager.service
%{_libdir}/libpreload%{name}.so
%{_sysconfdir}/firejail/whitelist-common-%{name}.local
%config(noreplace) %{_sysconfdir}/%{name}/manglelist.conf

%attr(0755,root,root) %{_libexecdir}/pm_apply
%attr(0755,root,root) %{_libexecdir}/pm_unapply

%{_libdir}/qt5/qml/org/SfietKonstantin/%{name}
%{_datadir}/%{name}/data
%{_datadir}/translations
%{_datadir}/jolla-settings/pages/%{name}
%{_datadir}/jolla-settings/entries/%{name}.json
%{_datadir}/%{name}/icons/icon-m-patchmanager.png

%{_datadir}/themes/%{theme}/meegotouch/z1.0/icons/*.png
%{_datadir}/themes/%{theme}/meegotouch/z1.25/icons/*.png
%{_datadir}/themes/%{theme}/meegotouch/z1.5/icons/*.png
%{_datadir}/themes/%{theme}/meegotouch/z1.5-large/icons/*.png
%{_datadir}/themes/%{theme}/meegotouch/z1.75/icons/*.png
%{_datadir}/themes/%{theme}/meegotouch/z2.0/icons/*.png

