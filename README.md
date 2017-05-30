# Display and Input backends

## Dependencies:
### Required:
* libxenbe
* libconfigc++
### Optional:
* libdrm
* wayland-client
* wayland-ivi-extension
* Doxygen

## How to configure
```
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}
cmake ${PATH_TO_SOURCES} -D${VAR1}=${VAL1} -D{VAR2}=${VAL2} ...
```
There are variables and options. Options can be set to ON and OFF.

Supported options:

| Option | Description |
| --- | --- |
| `WITH_DOC` | Creates target to build documentation. It required Doxygen to be installed. If configured, documentation can be create with `make doc` |
| `WITH_DRM` | Builds display backend with DRM framework (libdrm) |
| `WITH_ZCOPY` | Enables zero copy functionality for DRM |
| `WITH_WAYLAND` | Builds display backend with Wyaland framework (wayland-client) |
| `WITH_IVI_EXTENSION` | Uses GENIVI IVI extension to set surface positions |
| `WITH_INPUT` | Builds input backend | 

> If `WITH_DRM` and `WITH_WAYLAND` are disabled no display backend will be built.

Supported variabels:

| Variable | Description |
| --- | --- |
| `CMAKE_BUILD_TYPE` | `Realease`, `Debug`, `RelWithDebInfo`, `MinSizeRel`|
| `CMAKE_INSTALL_PREFIX` | Default install path |
| `XEN_INCLUDE_PATH` | Path to Xen tools includes if they are located in non standard place |
| `XENBE_INCLUDE_PATH` | Path to libxenbe includes if they are located in non standard place |
| `IF_INCLUDE_PATH` | Path to the interface headers if they are located in non standard place |
| `DRMZCOPY_INCLUDE_PATH` | Path to DRM zero copy header if it is located in non standard place |
| `XEN_LIB_PATH` | Path to Xen tools libraries if they are located in non standard place |
| `XENBE_LIB_PATH` | Path to libxenbe if it is located in non standard place |

Example:
```
// Debug build without Wayland framework and without input backend
cmake ${PATH_TO_SOURCES} -DWITH_WAYLAND=OFF -DWITH_INPUT=OFF -DCMAKE_BUILD_TYPE=Debug
```

## How to build:
```
cd ${BUILD_DIR}
make     // build sources
make doc // build documentaion
```
## How to install
```
make install // to default location
make DESTDIR=${PATH_TO_INSTALL} install //to other location
```
## How to run:
```
disple_be -c ${PATH_TO_CONFIG_FILE} -v${LOG_LEVEL}
```
> For syntaxes of the config file see [displ_be.cfg](displ_be.cfg)

Example:

```
disple_be -c displ_be.cfg -v debug
```