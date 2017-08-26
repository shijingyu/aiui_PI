1. compile:

in aiui directory,

make clean
make build

2. run.

cd build
export LD_LIBRARY_PATH=../../../libs/linux/
./demo


Note:
1. The library is compiled in Ubuntu 16.10 x64 and Ubuntu 16.04 LTS i686.
Please compile and test the demo in the same environment.

2. If error 10116 is reported, please make sure the 'scene' in build/AIUI/cfg/aiui.cfg is
same with the scene name of your application on aiui.xfyun.cn.
