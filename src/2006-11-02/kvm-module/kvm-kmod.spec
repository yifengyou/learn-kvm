%define kmod_name kvm

Name:           kvm-kmod
Version:        0.0
Release:        0
Summary:        %{kmod_name} kernel module

Group:          System Environment/Kernel
License:        GPL
URL:            http://www.qumranet.com
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}

ExclusiveArch:  i586 i686 x86_64

%description
This kernel module provides support for virtual machines using hardware support
(Intel VT or AMD SVM).

%prep

%build

rm -rf %{buildroot}

%install

%define moddir /lib/modules/%{kverrel}/extra
mkdir -p %{buildroot}/%{moddir}
cp %{objdir}/%{kmod_name}.ko %{buildroot}/%{moddir}
chmod u+x %{buildroot}/%{moddir}/%{kmod_name}.ko

%post 

depmod %{kverrel}

%postun

depmod %{kverrel}

%clean

%files
%{moddir}/%{kmod_name}.ko


%changelog
