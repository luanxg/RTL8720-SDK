set -e

rm -rf out
source ./build/envsetup.sh

##linux_x86-democonfig
lightduerconfig linux_x86-democonfig
#NOTE: why use this make?? long story !!! <_>
/usr/bin/make

