# Display and Input backends

## Dependencies:
### Required:
* libxenbe
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
| `WITH_MOCKBELIB` | Use test mock backend library | 

> If `WITH_DRM` and `WITH_WAYLAND` are disabled no display backend will be built.

Supported variables:

| Variable | Description |
| --- | --- |
| `CMAKE_BUILD_TYPE` | `Realease`, `Debug`, `RelWithDebInfo`, `MinSizeRel`|
| `CMAKE_INSTALL_PREFIX` | Default install path |
| `XEN_INCLUDE_PATH` | Path to Xen tools includes if they are located in non standard place |
| `XENBE_INCLUDE_PATH` | Path to libxenbe includes if they are located in non standard place |
| `IF_INCLUDE_PATH` | Path to the interface headers if they are located in non standard place |
| `XENBEMOCK_INCLUDE_PATH` | Path to the mock headers if they are located in non standard place |
| `DRMZCOPY_INCLUDE_PATH` | Path to DRM zero copy header if it is located in non standard place |
| `XEN_LIB_PATH` | Path to Xen tools libraries if they are located in non standard place |
| `XENBE_LIB_PATH` | Path to libxenbe if it is located in non standard place |
| `XENBEMOCK_LIB_PATH` | Path to libxenbemock if it is located in non standard place |

Example:
```
// Debug build without Wayland framework and without input backend
cmake ${PATH_TO_SOURCES} -DWITH_WAYLAND=OFF -DWITH_INPUT=OFF -DCMAKE_BUILD_TYPE=Debug
```

## How to build:
```
cd ${BUILD_DIR}
make     // build sources
make doc // build documentation
```
## How to install
```
make install // to default location
make DESTDIR=${PATH_TO_INSTALL} install //to other location
```
## Configuration:

### Display

Backend configuration is done in domain configuration file. See vdispl on http://xenbits.xen.org/docs/unstable-staging/man/xl.cfg.5.html#Devices

In order to match virtual connector with real one, connector id field is used.

For DRM mode, connector id specifies DRM connector on backend side. For example: HDMI-A-1, VGA-1 etc.

Domain configuration for vdispl in DRM mode example:
```
vdispl = [ 'backend=DomD,be-alloc=0,connectors=HDMI-A-1:1920x1080;VGA-1:768x1024' ]
```
Backend will provide HDMI-A-1 and VGA-1 DRM connectors for the configured domain.

For Wayland mode, in case of IVI extension connector id specifies id of surface which will be created to serve this virtual connector. This id can be used by [DisplayManager](https://github.com/xen-troops/DisplayManager), for example, to adjust surface layout. Without IVI extension, connector id is ignored by backend.

Domain configuration for vdispl in Wayland mode example:
```
vdispl = [ 'backend=DomD,be-alloc=0,connectors=1000:1920x1080;1001:768x1024' ]
```
Backend will create surfaces with id 1000 and 1001 for the configured domain.

### Input

Backend configuration is done in domain configuration file. See vkb on http://xenbits.xen.org/docs/unstable-staging/man/xl.cfg.5.html#Devices

In order to match virtual input device with real one, unique-id field is used.

One input frontend handler serves 3 devices: keyboard, pointer and touch.
The backend can redirect input events either from real input devices or from Wayland surfaces input events. For this reason `unique-id` field has following format: `K:<id>;P:<id>;T<id>`, where K stands for keyboard, P for pointer and T for touch. Not used devices can be omitted. `<id>` - can specify either real device as `/dev/input/event0` or Wayland IVI Extension surface ID.

Domain configuration for vkb:
```
vkb = [ 'backend=DomD,backend-type=linux,multi-touch-width=1920,multi-touch-height=1080,multi-touch-num-contacts=10,id=T:1000' ]
```
The backend will redirect touch events from the surface with id 1000 to the configured domain.

```
vkb = [ 'backend=DomD,backend-type=linux,multi-touch-width=1920,multi-touch-height=1080,multi-touch-num-contacts=10,id=K:/dev/input/event0;T:1000' ]
```
The backend will redirect keyboard events from /dev/input/event0 device and touch events from the surface with id 1000 to the configured domain.

## How to run:
```
disple_be -m{MODE} -d{DRM_DEVICE} -l{LOG_FILE} -v${LOG_MASK}
```
Example:

```
disple_be -v *:Debug
```