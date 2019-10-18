# libcast

libcast is an open source implementation of the Cast procotol supporting Cast
applications and streaming to Cast-compatible devices.

## Using the standalone implementations

To run the standalone sender and receivers together, first you need to install
the following dependencies: FFMPEG, LibVPX, LibOpus, LibSDL2, as well as their
headers (frequently in a seperate -dev package). From here, you need to generate
a RSA private key and create a self signed certificate with that key.

From there, after building Open Screen the `cast_sender` and `cast_receiver`
executables should be ready to use:
```
  $ /path/to/out/Default/cast_sender -s <certificate> <path/to/video>
  ...
  $ /path/to/out/Default/cast_receiver <interface> -p <private_key> -s <certificate>
```

When running on Mac OS X, also pass the `-x` flag to the cast receiver to
disable DNS-SD/mDNS, since Open Screen does not currently integrate with
Bonjour.

When connecting to a receiver that's not running on the loopback interface
(typically `lo` or `lo0`), pass the `-r <receiver IP endpoint>` flag to the
`cast_sender` binary.

An archive containing test running scripts, a video, and a generated RSA
key and certificate is available from google storage. Note that it may require
modification to work on your specific work environment:

https://storage.googleapis.com/openscreen_standalone/cast_streaming_demo.tar.gz
