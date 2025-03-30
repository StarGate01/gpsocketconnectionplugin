# GlobalPlatform Unix socket connection plugin

This is a Unix socket connection plugin for the GlobalPlatform Library (https://github.com/kaoh/globalplatform).

It is based on the upstream PC/SC connection plugin, see https://github.com/kaoh/globalplatform/tree/master/gppcscconnectionplugin .

## Building

Make sure you have a C compilation toolchain, `cmake`, and `pkg-config` installed. In addition, the `globalplatform` an `pcsclite` libraries needs to be installed.

```
mkdir -p build
cd build
cmake ..
make
```
