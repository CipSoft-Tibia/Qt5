# Lacros

## Background

Lacros is an architecture project to decouple the Chrome browser from the Chrome
OS window manager and system UI. The name comes from **L**inux **A**nd
**C**h**R**ome **OS**.

Googlers: [go/lacros](http://go/lacros) has internal docs.

## Technical details

On Chrome OS, the system UI (ash window manager, login screen, etc.) and the web
browser are the same binary. Lacros separates this functionality into two
binaries, henceforth known as ash-chrome (system UI) and lacros-chrome (web
browser).

The basic approach is to rename the existing binary to ash-chrome, with minimal
changes. We then take the linux-chrome binary, improve its Wayland support, make
it act like the web browser on Chrome OS, and ship that as the lacros-chrome
binary. This allows the two binaries to be released independently, with some
performance/resource costs. The API boundary initially will be semi-stable: it
will tolerate 1-2 milestones of version skew. We may allow larger amounts of
skew in the future.

Both binaries are built out of the chromium git repository. However, the
binaries might be built at different versions. For example, the version of
lacros built from the M-101 branch might run on top of the ash version built
from the M-100 branch.

Lacros can be imagined as "Linux chrome with more Wayland support". Lacros uses
[ozone](https://chromium.googlesource.com/chromium/src.git/+/master/ui/ozone)
as an abstraction layer for graphics and event handling. Ozone has a "backend"
with client-side support for the Wayland compositor protocol.

Chrome OS has a Wayland server implementation called
[exosphere](https://chromium.googlesource.com/chromium/src.git/+/master/components/exo).
It is used by ARC (to run Android apps) and Crostini (to run Linux apps).

Lacros will use exo as the Wayland server for graphics and event handling. Where
possible we use stable Wayland protocols. We also share Wayland protocol
extensions with ARC and Crostini (e.g.
[zaura-shell](https://chromium.googlesource.com/chromium/src.git/+/master/components/exo/wayland/protocol/aura-shell.xml).
Higher-level features (e.g. file picking) use Mojo IPC.

We call the new Mojo API surface "crosapi". It's similar in concept to Win32 or
Cocoa, but much smaller. It's also mostly asynchronous for performance reasons.
The API lives in
[//chromeos/crosapi](https://chromium.googlesource.com/chromium/src.git/+/master/chromeos/crosapi).
The ash-side implementation lives in
[//chrome/browser/chromeos/crosapi](https://chromium.googlesource.com/chromium/src.git/+/master/chrome/browser/chromeos/crosapi).

Code can be conditionally compiled into lacros via BUILDFLAG(IS_LACROS).

Lacros bugs can be filed under component: OS>LaCrOs.

## Testing

Most test suites require ash-chrome to be running in order to provide a basic
Wayland server and the crosapi implementation. This requires a special test
runner:

`./build/lacros/test_runner.py test out/lacros/browser_tests --gtest_filter=BrowserTest.Title`

If you're sshing to your desktop, please prefix the command with
`./testing/xvfb.py`.

For sheriffs: Test failures that should have been caught by the CQ should be
treated like test failures on any other platform: investigated and fixed or
disabled. Use BUILDFLAG(IS_LACROS) to disable a test just for lacros. See the
[sheriffing how-to](http://go/chrome-sheriffing-how-to#test-failed).
