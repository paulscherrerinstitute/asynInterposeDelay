# AsynInterposeDelay

This is an interpose layer for asyn octet drivers.
It adds the feature to have a delay after every chunk of characters
written to the device.
The default chunk size is 1.

## Usage

In the startup script, after loading the `asynInterposeDelay` driver and
setting up an asyn octet port,
call `asynInterposeDelay` to add this interpose layer to an existing
asyn octet port/address.

At run-time, delay and chunk size can be examined or changed using the
asyn option interface with the `"delay"` or `"chunksize"` option, respectively.

The delay is a floating point number in seconds.

The chunk size is an unsigned integer. 0 is treated as 1.

```
asynInterposeDelay "port", address, "delay", chunksize
asynShowOption "port", address, "delay"
asynShowOption "port", address, "chunksize"
asynSetOption "port", address, "delay", delay
asynSetOption "port", address, "chunksize", chunksize
```

## Example startup script

```
drvAsynSerialPortConfigure "port", "/dev/ttyS0"
asynInterposeDelay "port", 0, 0.001
```

## Example interactive delay display and change

```
asynShowOption "port", 0, "delay"
asynSetOption "port", 0, "delay", 0.05
asynShowOption "port", 0, "chunksize"
asynSetOption "port", 0, "chunksize", 32
```
