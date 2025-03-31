# GlobalPlatform Unix socket connection plugin

This is a Unix socket connection plugin for the GlobalPlatform Library (https://github.com/kaoh/globalplatform). It is based on the upstream PC/SC connection plugin, see https://github.com/kaoh/globalplatform/tree/master/gppcscconnectionplugin , but replaces PC/SC with Unix sockets and thus defers the actual transport of the APDUs to whatever system is backing the sockets.

Local sockets were deliberately chosen, as this plugin does not handle any control message or protocol negotiation, or any encryption. The socket backing middleware should implement this as a proxy, and then relay / wrap the APDUs as needed, e.g. to an actual PC/SC smartcard or to a WebSocket / TCP stream to a remote system.

## Building

Make sure you have a C compilation toolchain, `cmake`, and `pkg-config` installed. In addition, the `globalplatform` and `pcsclite` libraries need to be installed.

```
mkdir -p build
cd build
cmake ..
make
make install
```

If you use Nix, a `flake.nix` is provided.

## Installation

Modify your systems library search path, e.g. `LD_LIBRARY_PATH` to include the directory where you placed the compiled `libgpsocketconnectionplugin.so` library. This way your installation of `globalplatform.so` is able to use `dlopen()` to find and execute the plugin.

## Usage

Communication is done by by transmitting the APDUs over a socket of type `AF_UNIX` with `SOCK_SEQPACKET` framing. The plugin uses the reader name as the file descriptor ID of the socket, e.g. the string `4`.

This kind of socket can be created by e.g. `socketpair()`, with the transport implementation listening on the other socket on the other side. All calls are blocking and wait and expect complete records, which means the counterpart of the socket needs to be operated in a separate thread.

The plugin will try to read the ATR from the socket when a card connection is requested, so make sure that data is already written to the socket, otherwise the ATR will be empty.
