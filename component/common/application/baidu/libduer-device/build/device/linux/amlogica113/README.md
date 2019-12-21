# How TO
## Install the environment
```
$ tar vxzf /d/Users/suhao/Downloads/gcc-linaro-6.4.1-2017.11-x86_64_arm-linux-gnueabihf.tgz -C /opt
$ export AMLOGIC_A113_TOOLCHAIN_PATH=/opt/gcc-linaro-6.4.1-2017.11-x86_64_arm-linux-gnueabihf
```
## Configure the environment
- `cd` to The `libduer-device` work path.
- Run `. build/envsetup.sh`
- Select the `linux_amlogica113-sdkconfig` with command `lightduerconfig`
- `make` and generate the `libduer-device.a`
```
$ cd libduer-device
$ . build/envsetup.sh
$ lightduerconfig 6
$ make
```