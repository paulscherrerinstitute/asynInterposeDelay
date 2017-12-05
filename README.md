# AsynInterposeDelay

This is an interpose layer for asyn octet drivers.
It adds the feature to have a delay after every single character
written to the device.
Some very crappy devices need this.

## Usage

In the startup script, after loading the `asynInterposeDepay` driver and
setting up an asyn octet port,
call `asynInterposeDelay` to add this interpose layer to an existing
asyn octet port/address.

At run-time the delay can be examined or changed using the asyn option
interface with the `delay` option which is added by this interpose layer.

The delay is a floating point number in seconds.

```
require asynInterposeDelay

asynInterposeDelay port, address, delay(sec)
asynShowOption port, address, "delay"
asynSetOption port, address, "delay", delay(sec)
```

## Example
```
drvAsynIPPortConfigure "device", "server.com:40000"
asynInterposeDelay "device", 0, 0.001
asynShowOption "device", 0, "delay"
asynSetOption "device", 0, "delay", 0.05
```
