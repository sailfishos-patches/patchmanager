## This is an example .spec file for a patchmanager spec
Name:      patch-example

BuildArch: noarch

Summary:    Patch to hack the Gibson
# three-digit version recommended, as patchmanager uses such version identifiers
Version:    0.1.0
Release:    1
Group:      Qt/Qt
License:    BSD-3-Clause
Source0:    %{name}-%{version}.tar.gz
Requires:   patchmanager
# add SailfishOS release and/or the app package this patch modifies as dependencies
# do not use greater-than here, require known working versions
Requires:   sailfishos-version = 4.0.1
Requires:   sailfish-browser = 2.0.18

%description
Patch for the Gibson to be more easily hacked

%prep
%setup -q -n %{name}-%{version}

%build

%install
rm -rf %{buildroot}
install -p -D patch-files/unified_diff.patch %{buildroot}/usr/share/patchmanager/patches/%{name}/unified_diff.patch
install -p -D patch-files/patch.json %{buildroot}/usr/share/patchmanager/patches/%{name}/patch.json

%pre
if [ -d /tmp/patchmanager3/patches/%{name} ]; then
/usr/sbin/patchmanager -u %{name} || true
fi

%preun
if [ -d /tmp/patchmanager3/patches/%{name} ]; then
/usr/sbin/patchmanager -u %{name} || true
fi

%files
%defattr(-,root,root,-)
%{_datadir}/patchmanager/patches/%{name}
