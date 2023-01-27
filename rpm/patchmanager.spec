%global theme sailfish-default

# These macros should already be defined in the RPMbuild environment, see: rpm --showrc
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{!?qmake5_install:%define qmake5_install make install INSTALL_ROOT=%{buildroot}}

# Override these defines in both (!):
# - src/bin/patchmanager-daemon/patchmanagerobject.h
# - src/qml/webcatalog.h
# SERVER_URL  "https://coderus.openrepos.net"
# API_PATH    "pm2/api"
# CATALOG_URL SERVER_URL "/" API_PATH
%global catalog_server https://coderus.openrepos.net
%global catalog_api_uri pm2/api
%global catalog_defines -DSERVER_URL=%{catalog_server} -DAPI_PATH=%{catalog_api_uri}

Name:       patchmanager

Summary:    Allows to manage Patches for SailfishOS
Version:    3.2.7
Release:    1
Group:      Qt/Qt
License:    BSD-3-Clause
URL:        https://github.com/sailfishos-patches/%{name}
Source0:    %{url}/archive/%{version}/%{name}-%{version}.tar.gz
# Note that the rpmlintrc file MUST be named exactly so according to
# https://en.opensuse.org/openSUSE:Packaging_checks#Building_Packages_in_spite_of_errors
Source99:   %{name}-rpmlintrc
Requires:   unzip
Requires:   patch
Requires:   grep
Requires:   sed
Requires:   sailfish-version >= 3.4.0
Requires:   qml(Nemo.Configuration)
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


%package testcases
Summary:    Provides test cases for Patchmanager
Group:      Development
BuildArch:  noarch
Requires:   %{name}
Requires:   libsailfishapp-launcher

# This description section includes metadata for SailfishOS:Chum, see
# https://github.com/sailfishos-chum/main/blob/main/Metadata.md
# For the patchmanager-testcases, point directly to the GitHub bugtracker.
%description testcases
The package patchmanager-testcases installs a small test application and a
couple of Patches which are helpful for testing the internal functioning of
Patchmanager, e.g., when changes were made to it.

Note that utilising these test cases requires a thorough understanding how
Patchmanager internally works and hence is most likely only useful for
developers.

%if "%{?vendor}" == "chum"
PackageName: Test cases for Patchmanager
Type: addon
Categories:
 - Development
Custom:
  Repo: %{url}
Url:
  Homepage: https://openrepos.net/content/%{name}/%{name}-testcases
  Help: %{url}/discussions/232
  Bugtracker: %{url}/issues
%endif


# This description section includes metadata for SailfishOS:Chum, see
# https://github.com/sailfishos-chum/main/blob/main/Metadata.md
# - The "Help:" field here would default to GitHub discussions, which is used
#   for internal discussions among the Patchmanager maintainers, hence pointing
#   to the README, because the Wiki is not suitable for end-users.
# - The "Bugtracker:" field would default to GitHub issues; prefer to guide
#   end-users to the corresponding SFOS Forum thread, first.
%description
Patchmanager is a tool for transparently modifying installed files by the patch
utility and for managing the special patch files ("Patches") for doing so.
Since version 3.0, Patchmanager does not modify original files on mass storage,
it merely alters their content when they are loaded into RAM to be executed.

Note that Patchmanager does not install an application icon on the launcher,
but creates a new entry in SailfishOS' Settings app.

%if "%{?vendor}" == "chum"
PackageName: Patchmanager for SailfishOS
Type: desktop-application
Categories:
 - System
 - Settings
 - Utility
Custom:
  Repo: %{url}
Icon: %{url}/raw/master/src/share/patchmanager-big.png
Url:
  Homepage: https://openrepos.net/content/%{name}/%{name}
  Help: %{url}/blob/master/README.md
  Bugtracker: https://forum.sailfishos.org/t/bugs-in-patchmanager-3-1-0/8552
  Donation: https://openrepos.net/donate
%endif


%prep

%setup -q


%build

%qtc_qmake5 "PROJECT_PACKAGE_VERSION=%{version}"
%qtc_make %{?_smp_mflags} EXTRA_CFLAGS="$CFLAGS %{catalog_defines}"


%install

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
case "$1" in
1)  # Installation
  echo "Installing %{name}: %%pre section"
;;
[2-9])  # Update
  echo "Updating %{name}: %%pre section"
  # Disable and deactivate ("unapply") all Patches if Patchmanager 2.x is installed
  if [ -d /var/lib/patchmanager/ausmt/patches/ ]
  then
    /usr/sbin/patchmanager --unapply-all
    if [ -n "$(ls -A /var/lib/patchmanager/ausmt/patches/)" ]
    then
      echo "Do disable ("unapply") all Patches: %{name} must not be updated unless all Patches are inactive!"
      exit 1  # An exit N with N ≠ 0 in a %%pre scriptlet might not constitute an
              # appropriate way to terminate an update (here: from PM2 to PM3), see e.g.,
              # https://docs.fedoraproject.org/en-US/packaging-guidelines/Scriptlets/#_syntax
              # OTOH, how else might one achieve that?!?
    fi
  fi
;;
*)
  echo "Case $1 is not handled in %%pre section of %{name}!"
;;
esac
exit 0


%post
export NO_PM_PRELOAD=1
case "$1" in
1)  # Installation
  echo "Installing %{name}: %%post section"
;;
[2-9])  # Update
  echo "Updating %{name}: %%post section"
;;
*)
  echo "Case $1 is not handled in %%post section of %{name}!"
;;
esac
sed -i '/libpreload%{name}/ d' /etc/ld.so.preload
echo '%{_libdir}/libpreload%{name}.so' >> /etc/ld.so.preload
/sbin/ldconfig
if ! grep -qsF 'include whitelist-common-%{name}.local' /etc/firejail/whitelist-common.local
then
  echo 'include whitelist-common-%{name}.local' >> /etc/firejail/whitelist-common.local
fi
dbus-send --system --type=method_call --dest=org.freedesktop.DBus / org.freedesktop.DBus.ReloadConfig
systemctl daemon-reload
systemctl-user daemon-reload
systemctl restart dbus-org.SfietKonstantin.patchmanager.service
systemctl restart checkForUpdates-org.SfietKonstantin.patchmanager.timer
exit 0


%preun
export NO_PM_PRELOAD=1
case "$1" in
0)  # Removal ("uninstallation")
  echo "Removing %{name}: %%preun section"
  systemctl stop checkForUpdates-org.SfietKonstantin.patchmanager.timer
  systemctl stop dbus-org.SfietKonstantin.patchmanager.service
;;
1)  # Update
  echo "Updating %{name}: %%preun section"
;;
*)
  echo "Case $1 is not handled in %%preun section of %{name}!"
;;
esac
exit 0


%postun
export NO_PM_PRELOAD=1
case "$1" in
0)  # Removal ("uninstallation")
  echo "Removing %{name}: %%postun section"
  sed -i '/whitelist-common-%{name}.local/ d' /etc/firejail/whitelist-common.local
  sed -i '/libpreload%{name}/ d' /etc/ld.so.preload
  /sbin/ldconfig
  rm -rf /tmp/patchmanager
  rm -f /tmp/patchmanager-socket
;;
1)  # Update
  echo "Updating %{name}: %%postun section"
;;
*)
  echo "Case $1 is not handled in %%postun section of %{name}!"
;;
esac
dbus-send --system --type=method_call --dest=org.freedesktop.DBus / org.freedesktop.DBus.ReloadConfig
systemctl daemon-reload
systemctl-user daemon-reload
exit 0


%files testcases
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/org/SfietKonstantin/patchmanagertests
%{_datadir}/%{name}/patches/pm-test-case-*
%{_datadir}/applications/patchmanager-testcase.desktop
%{_datadir}/patchmanager-testcase
%{_datadir}/patchmanager-test/testfile


%files
%defattr(-,root,root,-)

%{_bindir}/%{name}-dialog
%{_sbindir}/%{name}
%dir %{_datadir}/%{name}/patches
%{_datadir}/%{name}/tools
%{_datadir}/dbus-1/*/org.SfietKonstantin.patchmanager.*
%config %{_sysconfdir}/dbus-1/system.d/org.SfietKonstantin.patchmanager.conf
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
%config(noreplace) %{_sysconfdir}/%{name}/manglelist.conf
%config(noreplace) %{_sysconfdir}/firejail/whitelist-common-%{name}.local

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

%changelog
* Thu Sep  9 1999 SailfishOS Patches <sailfishos-patches@users.noreply.github.com> - 99.99.99
- See %{url}/releases

