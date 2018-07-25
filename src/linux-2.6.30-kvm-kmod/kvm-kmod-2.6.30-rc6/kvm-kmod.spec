%define kmod_name kvm

Name:           kvm-kmod
Version:        0.0
Release:        0
Summary:        %{kmod_name} kernel module

Group:          System Environment/Kernel
License:        GPL
URL:            http://www.qumranet.com
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}

ExclusiveArch: i386 x86_64 ia64

%description
This kernel module provides support for virtual machines using hardware support
(Intel VT-x&VT-i or AMD SVM).

%prep

%build

rm -rf %{buildroot}

%install

%define kverrel unknown
%define moddir /lib/modules/%{kverrel}/extra
mkdir -p %{buildroot}/%{moddir}
cp %{objdir}/%{kmod_name}.ko %{objdir}/%{kmod_name}-*.ko %{buildroot}/%{moddir}
chmod u+x %{buildroot}/%{moddir}/%{kmod_name}*.ko

%post 

depmod %{kverrel}

%postun

depmod %{kverrel}

%clean
%{__rm} -rf %{buildroot}

%files
%{moddir}/%{kmod_name}.ko
%ifarch i386 x86_64
%{moddir}/%{kmod_name}-amd.ko
%endif
%{moddir}/%{kmod_name}-intel.ko


%changelog
