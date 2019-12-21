#DCS3.0 linux demo使用说明

—————————————————————————————————————————————————

###1. 编译环境搭建：

#### gstreamer1.0 ：
	sudo apt-get install gstreamer1.0*

`注：`这个demo是使用在Ubuntu系统下的，64位Ubuntu安装gstreamer可能失败，请使用32位的Ubuntu或自行安装gstreamer1.0-hybris.

#### alsa ：

	sudo apt-get install alsa-base alsa-utils libasound2-dev


###2. 编译方式：

在工程文件下运行以下命令：

1. `source build/envsetup.sh`
2. `lightduerconfig`
 此时选择 linux_x86-dcs3linuxdemoconfig.
3. `make`


###3. 运行方式：

运行编译生成的可执行文件`dcs3-linux-demo`， -p `<路径>/profile`, -r `<路径>/闹钟铃声文件`。
-r是可选参数，没有闹钟功能是不需要加此选项。
例如：

	out/linux/x86/dcs3linuxdemoconfig/modules/dcs3-linux-demo/dcs3-linux-demo -p ../0a540000000008 -r ../ring.mp3

###4. 按键说明：

运行成功后，确保键盘不在大写锁定状态，所有按键都不需要长按。

 - q ： 退出
 - w ： 增加音量
 - s ： 减小音量
 - a ： 上一曲
 - d ： 下一曲
 - e ： 静音
 - z ： 暂停/开始 （播放）
 - x ： 开始录音
 - c ： 切换语音交互模式 （0：普通模式，1：中翻英，2：英翻中）
