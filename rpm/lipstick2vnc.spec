Name:           lipstick2vnc
Summary:        A VNC server for Sailfish OS QA
Version:        0.1
Release:        1
License:        GPLv2+
URL:            https://github.com/mer-qa/lipstick2vnc
Source0:        %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(libvncserver)
BuildRequires:  pkgconfig(libsystemd)
BuildRequires:  libjpeg-turbo-devel
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  qt5-qtwayland-wayland_egl-devel
BuildRequires:  oneshot
Requires:       oneshot
Requires:       jolla-sessions-qt5 >= 1.2.7
Requires(pre):  sailfish-setup
%{_oneshot_requires_post}


%description
%{summary}.

Don't install this package if you care about security.
There is no protection in this VNC server.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 CONFIG+=release
%make_build

%install
%qmake5_install

# systemd integration
mkdir -p %{buildroot}%{_userunitdir}/sockets.target.wants/
ln -s ../vnc.socket %{buildroot}%{_userunitdir}/sockets.target.wants/vnc.socket

%post
systemctl-user daemon-reload || :
systemctl-user stop vnc.service || :
%{_bindir}/add-oneshot 20-lipstick2vnc-configurator || :

%preun
if [ "$1" == 0 ]
then
    systemctl-user stop vnc.service vnc.socket || :
fi

%postun
if [ "$1" == 0 ]
then
    systemctl-user daemon-reload || :
fi

%files
%attr(2755, root, privileged) %{_bindir}/%{name}
%{_datadir}/lipstick2vnc/cursor_empty.png
%{_datadir}/lipstick2vnc/cursor_pointer.png
%{_datadir}/lipstick2vnc/cursor_pointer_touch.png
%attr(755, root, root) %{_oneshotdir}/20-lipstick2vnc-configurator
%{_userunitdir}/vnc.socket
%{_userunitdir}/vnc.service
%{_userunitdir}/sockets.target.wants/vnc.socket
