# QNX8 Image

The qnx image is built in this repository. Most of the configs and settings are
copied from the qnx8 [reference_integration](https://github.com/eclipse-score/reference_integration/tree/v0.5.0-beta/qnx_qemu/build).


## Bazel

`qnx_ifs` is bazel rule that wraps the qnx
[mkifs](https://www.qnx.com/developers/docs/8.0/com.qnx.doc.neutrino.utilities/topic/m/mkifs.html)
utility, and generates the image.


## Image 

### Networking

The VM is configured with a static IP in the `network_setup.sh` script.


### Test Binaries

The `reference_integration` builds the test binaries into the image however
here the test binaries shall be copied over network to lesten the time it takes
to run tests.

