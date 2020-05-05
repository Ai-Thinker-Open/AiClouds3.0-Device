# 关于  xClouds 开源架构

&nbsp;&nbsp;&nbsp;&nbsp; 当我们日复一日年复一年的搬砖的时候，你是否曾想过提升一下开发效率，如果一个  **IOT智能家具物联网**  模板的项目摆在你的面前，你还会选择自己搭架构么？

 &nbsp;&nbsp;&nbsp;&nbsp; 但是搭建出一个好的架构并非易事，有多少人愿意选择去做，还有多少人选择努力去做好，可能寥寥无几，但是你今天看到的，正是你所想要的，一个真正能解决你开发新项目时最大痛点的架构工程，你不需要再麻木 ``Copy`` 原有旧项目的代码，只需改动少量代码就能得到想要的效果，你会发现开发新项目其实是一件很快乐的事；

&nbsp;&nbsp;&nbsp;&nbsp; 很幸运的是，在2018年底时候，我已经自学打通了天猫精灵音箱和小爱同学音箱同时控制一个 ESP8266，那时候我就开始专研服务器开发，自学攻破了数据库、php语言，再到了微信公众号的生态，我打通了这一整个的控制；

&nbsp;&nbsp;&nbsp;&nbsp;  直到目前为止，我依然在走这条道路，但是随着开源其趋势不断影响，很多组织或个人奋力地做一些开源，源源不断的架构和代码封装，加快了我们程序员开发的效率，比如前端的三大框架的 **Angular**，**React**，**Vue**：


&nbsp;&nbsp;&nbsp;&nbsp;  国内AI智能音箱一大崛起，也有很多优秀的企业一步一步崛起，像涂鸦就是典型的例子，一站式开发，对于很多小型家电传统公司接入AI音箱等智能控制，无疑就是一个 “菩萨” 般存在；

&nbsp;&nbsp;&nbsp;&nbsp;  话说回来，类似这种平台，对于我们开发者有什么值得学习的地方呢？ 最熟悉宣传语就是：**全球部署，一接入可对接多语音平台。**

&nbsp;&nbsp;&nbsp;&nbsp;  于是乎，我有个大胆的想法能否单独做这样的架构出来并开源出来：

- **跨平台、开发快、全开源**的宗旨；
- 微信公众号内 `airkiss` 配网和 `MQTT` 控制；
- 微信小程序内 `smartConfig` 配网和 `MQTT` 控制；
- 设备端要求：支持 `airkiss` 配网和 `MQTT` 协议即可；
- 服务器端：有微信公众号业务和各云平台对接的业务，支持但不限于 天猫精灵、小爱同学、小度音箱、Alexa音箱等；
- 扩展性强，支持用户二次开发，可私定义协议；
- 三端开源：设备端、服务器端、前端；

&nbsp;&nbsp;&nbsp;&nbsp;  现在，我有能力地公开这个架构，我想对于那些想学系统化的，或者电子diy爱好者，抑或是在校大学生，可参考学习，我的愿景就是：

&nbsp;&nbsp;&nbsp;&nbsp; **xClouds能在国内物联网一枝独秀，助国内物联网发展；**
&nbsp;&nbsp;&nbsp;&nbsp; **但愿人长久，搬砖不再有！**



# xClouds-devices for  ESP8266  开发框架


&nbsp;&nbsp;&nbsp;&nbsp;  认识我的人都知道，我和乐鑫 ESP8266 芯片有一个解不开的缘分，我通过此芯片认识很多志同道合的的小伙伴，也让我逐渐地找到了自己的人生价值，所以，我会以自己擅长的 Wi-Fi模块二次开发，带领大家快速体验和商业化使用此物美价廉的芯片；


&nbsp;&nbsp;&nbsp;&nbsp;  



### 1. Master branch
The master branch is an integration branch where bug fixes/features are gathered for compiling and functional testing.

### 2. Release branch
The release branch is where releases are maintained and hot fixes (with names like *release/v2.x.x*) are added.
Please ensure that all your production-related work are tracked with the release branches.

With this new model, we can push out bug fixes more quickly and achieve simpler maintenance.

## Roadmap
*ESP8266_RTOS_SDK*'s framework is quite outdated and different from the current *[esp-idf](https://github.com/espressif/esp-idf)* and we are planning to migrate *ESP8266_RTOS_SDK* to *esp-idf* eventually after *v2.0.0*.

However, we will firstly provide a new version of ESP8266 SDK (*ESP8266_RTOS_SDK v3.0*), which shares the same framework with *esp-idf* (esp-idf style), as a work-around, because the multi-CPU architecture is not supported by *esp-idf* for the time being.

Actions to be taken for *ESP8266_RTOS_SDK v3.0* include the following items:

1. Modify the framework to esp-idf style
2. Restructure some core libraries including Wi-Fi libraries and libmain
3. Update some third-party libraries including FreeRTOS, lwIP, mbedTLS, noPoll, libcoap, SPIFFS, cJSON, wolfSSL, etc.
4. Update some drivers
5. Others

---

# Developing With the ESP8266_RTOS_SDK

## Get toolchain

v5.2.0

* [Windows](https://dl.espressif.com/dl/xtensa-lx106-elf-win32-1.22.0-100-ge567ec7-5.2.0.zip)
* [Mac](https://dl.espressif.com/dl/xtensa-lx106-elf-macos-1.22.0-100-ge567ec7-5.2.0.tar.gz)
* [Linux(64)](https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz)
* [Linux(32)](https://dl.espressif.com/dl/xtensa-lx106-elf-linux32-1.22.0-100-ge567ec7-5.2.0.tar.gz)

If you are still using old version SDK(< 3.0), please use toolchain v4.8.5, as following:

* [Windows](https://dl.espressif.com/dl/xtensa-lx106-elf-win32-1.22.0-88-gde0bdc1-4.8.5.tar.gz)
* [Mac](https://dl.espressif.com/dl/xtensa-lx106-elf-osx-1.22.0-88-gde0bdc1-4.8.5.tar.gz)
* [Linux(64)](https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-88-gde0bdc1-4.8.5.tar.gz)
* [Linux(32)](https://dl.espressif.com/dl/xtensa-lx106-elf-linux32-1.22.0-88-gde0bdc1-4.8.5.tar.gz)

## Get ESP8266_RTOS_SDK

Besides the toolchain (that contains programs to compile and build the application), you also need ESP8266 specific API / libraries. They are provided by Espressif in [ESP8266_RTOS_SDK](https://github.com/espressif/ESP8266_RTOS_SDK) repository. To get it, open terminal, navigate to the directory you want to put ESP8266_RTOS_SDK, and clone it using `git clone` command:

```
cd ~/esp
git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
```

ESP8266_RTOS_SDK will be downloaded into `~/esp/ESP8266_RTOS_SDK`.

## Setup Path to ESP8266_RTOS_SDK

The toolchain programs access ESP8266_RTOS_SDK using `IDF_PATH` environment variable. This variable should be set up on your PC, otherwise projects will not build. Setting may be done manually, each time PC is restarted. Another option is to set up it permanently by defining `IDF_PATH` in user profile.

For manually, the command:
```
export IDF_PATH=~/esp/ESP8266_RTOS_SDK
```

## Start a Project
Now you are ready to prepare your application for ESP8266. To start off quickly, we can use `examples/get-started/hello_world` project from `examples` directory in SDK.

Once you've found the project you want to work with, change to its directory and you can configure and build it.

## Connect

You are almost there. To be able to proceed further, connect ESP8266 board to PC, check under what serial port the board is visible and verify if serial communication works. Note the port number, as it will be required in the next step.

## Configuring the Project

Being in terminal window, go to directory of `hello_world` application by typing `cd ~/esp/ESP8266_RTOS_SDK/examples/get-started/hello_world`. Then start project configuration utility `menuconfig`:

```
cd ~/esp/ESP8266_RTOS_SDK/examples/get-started/hello_world
make menuconfig
```

In the menu, navigate to `Serial flasher config` > `Default serial port` to configure the serial port, where project will be loaded to. Confirm selection by pressing enter, save configuration by selecting `< Save >` and then exit application by selecting `< Exit >`.

> Note:
	On Windows, serial ports have names like COM1. On MacOS, they start with `/dev/cu.`. On Linux, they start with `/dev/tty`.

Here are couple of tips on navigation and use of `menuconfig`:

* Use up & down arrow keys to navigate the menu.
* Use Enter key to go into a submenu, Escape key to go out or to exit.
* Type `?` to see a help screen. Enter key exits the help screen.
* Use Space key, or `Y` and `N` keys to enable (Yes) and disable (No) configuration items with checkboxes "`[*]`"
* Pressing `?` while highlighting a configuration item displays help about that item.
* Type `/` to search the configuration items.

Once done configuring, press Escape multiple times to exit and say "Yes" to save the new configuration when prompted.

