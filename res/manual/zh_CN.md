## 引言

- 对于日渐重复且复杂的工作流程，比如：手动上传和下载文件、修改文件名、手动执行各种命令，以及**混乱的项目文件**
- 在工作上有非常多的项目，在不同的项目之间希望能够快速切换下载目录，致力于一键实现全流程自动化调试软件的能力
- 对于新知识的探索与实践，学会如何**管理软件版本**、**流水线**以及开源项目的**贡献流程**

* 于是就有了这个项目，尽可能的解决工作流程上的各种重复性动作，解放您的双手便于**Touch Fish**
* **第一次使用本程序也许您会有很多疑惑，别急，烦请您耐心阅读完教程，相信这会给您带来许多帮助**

## 快速开始

#### 准备环境

1. 文件隔离器的**账号**和**密码**

2. 最好有使用过**网页端**文件隔离器的经验，这对您后边的操作流程有很大的帮助

3. 使用Windows操作系统环境

#### 开始使用

##### 登录

1. 在登录窗口输入**账号**和**密码**后点击**登录按钮**(或者按下回车键)，**Website(服务器)**使用默认的即可

2. 不出意外的话，登录成功就能看到**主窗口**了
   - 主窗口左侧显示当前文件隔离器上的文件信息
   - 窗口中间为工程目录操作区，工程信息都将在这里显示
   - 窗口右侧为上传文件配置区，用于监听文件的变化

##### 下载文件

1. 下面我们来尝试创建一个新项目，并尝试下载一个文件到这个目录
   - 在主窗口中间上边点击**添加**按钮，比如项目名称叫：`Test Project`
   - 点击OK按钮后会进入**项目信息**配置界面，在这里可以修改您的工程信息
   - 我们**选择**好一个工程目录，后续下载文件时会默认下载到这个目录下
   - 点击OK按钮后**主窗口**中间表格会多出一列工程信息，我们用鼠标选中它，此时表格上方会显示刚才输入的**项目名称**
   - 到这里我们的项目信息就选择好了，接下来在主窗口左侧的文件列表中选择好需要下载的文件
   - 点击左下角的**下载**按钮，此时文件会开始从服务器上开始下载下来，按钮右侧会显示当前下载进度条信息
   - 正常情况此时已经下载完成文件了，可以在资源管理器相对应的目录找到下载好的文件

##### 上传文件

* 既然有下载，那肯定是可以上传文件的，下面我们尝试上传一个文件

  - 在**主窗口**左下角位置**点击上传按钮**，在弹出的提示框中选择需要上传的文件即可，文件默认会上传至当前浏览目录
  - 另一个比较懒人化的方案就是在右侧添加需要监听的文件
  - 我们看向**主窗口**右侧的上传文件配置区，我们点击配置区上方的**添加按钮**，选择完成后可以看到配置区出现需要监听的文件了
  - 此时我们**勾选**刚选择好的文件，再点击右上角的**文件监听器按钮**开启监听所选文件
  - 每当文件被创建或被修改时，程序会自动上传文件到当前浏览目录当中
* 如果需要移除监听文件呢？
  - 我们在列表中选中需要删除的文件（**需要注意的是，并非删除复选框选中的文件，鼠标单击文件后背景颜色会变深高亮，此时文件条目就是选中状态，反之非选中状态，不会被删除**），选好文件后点击上边的**移除按钮**即可删除正在监听文件的状态

##### 覆盖文件

- 在**主窗口**工程表格区右上角有个`Override`或者`覆盖`按钮，我们有时候下载文件时需要一直保存至同一个文件，这个时候我们就需要**选中覆盖或Override按钮**，以保证每次下载都会覆盖掉原有文件
- 当我们需要对下载的文件进行归档操作时，我们可以**取消选中按钮**，这样在每次下载文件时就会重新生成一个带有`(数字)`结尾的下载文件名

#### 设置

##### 常用界面

* 刷新间隔

  长时间未与服务器进行交互后，服务器会主动退出登录状态，在网页端长时间不操作的话同样也会退出登录状态。所以我们需要一个经常**主动刷新**的保活机制，以秒为单位，默认300s会主动刷新一次文件列表信息

* 下载位置

  - 有时候我们需要经常将文件放在一个固定的目录，比如`C:/Users/{Username}/Downloads`。在**主窗口**的**项目表格**区中，我们有两种下载模式可以选择，`默认模式`和`工程模式`。

  默认模式：会将下载所有文件统一下载至`C:/Users/{Username}/Downloads`目录

  工程模式：下载至所选工程的工作目录下

* 通知气泡

  目前有两种通知方式，一种是调用系统API接口，一种是以独立弹窗的形式，根据需求开启对应**弹窗权限**即可

* 语言

  选择语言，现支持中文和英文

* 关闭主窗口

  按需求设置即可

* 启动

  按需求设置即可

* 文件监听器抖动延迟

  监听上传文件时会产生非常多的系统消息，此处用于设置文件收到变化消息后延迟多久触发自动上传的时间

* 网络代理

  按需求设置即可

##### 调试

- 没事儿别来这个界面，非常规操作引起的问题一律不予理会

## 高级用法

#### 下载后自动执行脚本

* 在**项目信息**页面中找到**下载脚本**栏目，选择好脚本文件的路径，并勾选使能按钮，然后确认修改

* 当文件下载完成后将会自动执行用户配置的**脚本文件**，切记在Windows平台下仅支持bat脚本，请勿使用其他类型的脚本文件，这可能会导致不可预料的错误发生。

* 这里提供一个bat脚本示例，您可以根据需求修改操作逻辑。

  ```bat
  @echo off
  setlocal enabledelayedexpansion
  
  set  IS_AOSP=0
  echo IS_AOSP=%IS_AOSP%
  
  set  IS_ONLY_EXECUTABLE=0
  echo IS_ONLY_EXECUTABLE=%IS_ONLY_EXECUTABLE%
  
  set  delay=3000
  echo delay=%delay% ms
  
  set  XXXSDK=%~n0
  echo XXXSDK=%XXXSDK%
  
  set  PLATFORM_BIN_DIR=/system/bin
  echo PLATFORM_BIN_DIR=%PLATFORM_BIN_DIR%
  
  set  XXXSDK_LIB=libXxxxXxxx.so
  echo XXXSDK_LIB=%XXXSDK_LIB%
  
  set  PLATFORM_LIB_DIR=/system/lib
  echo PLATFORM_LIB_DIR=%PLATFORM_LIB_DIR%
  
  set  PLATFORM_BLUETOOTH_JNI_LIB_DIR=/system/lib
  echo PLATFORM_BLUETOOTH_JNI_LIB_DIR=%PLATFORM_BLUETOOTH_JNI_LIB_DIR%
  
  :: :::::::::: Configuration complete ::::::::::::
  
  :: :::::::::: Start execute configuration ::::::::::::
  
  :: Compare the hash values of the files
  ::certutil -hashfile .\xxxsdk.so
  ::certutil -hashfile .\libXxxxXxxx.so
  
  if exist xxxsdk.so (
  echo Exist new xxxsdk, updating......
  :: Try to delete the old files
  del %XXXSDK%
  :: rename file name
  rename xxxsdk.so %XXXSDK%
  )
  
  adb root
  adb remount
  
  :: push file
  adb push .\%XXXSDK%             %PLATFORM_BIN_DIR%/%XXXSDK%
  if %IS_ONLY_EXECUTABLE% equ 0 (
  adb push .\%XXXSDK_LIB% 	    %PLATFORM_LIB_DIR%/%XXXSDK_LIB%
  )
  if %IS_AOSP% equ 1 (
  adb push .\libbluetooth_jni.so 	%PLATFORM_BLUETOOTH_JNI_LIB_DIR%/libbluetooth_jni.so
  )
  :: adb push .\Bluetooth.apk        /system/app/Bluetooth/Bluetooth.apk
  :: adb push .\bttest.so            /data/xxx/bttest
  
  adb shell sync
  
  ::chmod
  adb shell "chmod 777 %PLATFORM_BIN_DIR%/%XXXSDK%"
  
  ::adb shell restorecon /system/bin/xxxsdk
  
  adb shell sync
  
  echo Push done, please check is succeed? (This window will close after %delay% ms)
  ::pause
  call:sleepms %delay%
  
  adb reboot
  call:sleepms 1000
  adb wait-for-device
  adb root
  adb remount
  
  ::pause
  exit
  
  :sleepms
  ::Copy From https://blog.csdn.net/PSpiritV/article/details/125632190
  set delayms=%1
  set TotalTime=0
  set NowTime=%time%
  :delay_continue
  set /a minute1=1%NowTime:~3,2%-100
  set /a second1=1%NowTime:~-5,2%%NowTime:~-2%0-100000
  set NowTime=%time%
  set /a minute2=1%NowTime:~3,2%-100
  set /a second2=1%NowTime:~-5,2%%NowTime:~-2%0-100000
  set /a TotalTime+=(%minute2%-%minute1%+60)%%60*60000+%second2%-%second1%
  if %TotalTime% lss %delay% goto delay_continue
  goto:eof
  
  ```

- 看不懂bat脚本的话，让**AI**来解释或许是个不错的选择

#### 上传前自动执行脚本

* 在**主窗口**页面右侧找到**上传文件配置**栏目，添加需要上传的文件，并勾选使能按钮，勾选要监听的文件条目复选框
* 当文件上传前会自动执行用户配置的**脚本文件**，切记在Windows平台下仅支持bat脚本，请勿使用其他类型的脚本文件，这可能会导致不可预料的错误发生。
* 在**项目信息**中的**上传脚本**一栏可以设置上传前是否自动执行脚本文件，请参考**下载脚本**文件内容进行自定义编写操作逻辑

## 写在最后

* 本文档若有照顾不周之处，还望多多海涵，您的支持将会是本项目前进的动力。
* 在下载脚本中，如果有冷重启(reboot)的需求，与[Uart2Relay](https://gitee.com/time--chicken/uart2relay)项目配合可以达到硬件层的冷重启效果。
* 如本程序对您有帮助，请到项目仓库给一个免费的Star，或给予本项目更多的技术支持，在此感激不尽。
* 感谢 [GitHub - zhuzichu520/FluentUI: FluentUI for QML](https://github.com/zhuzichu520/FluentUI) 提供的技术框架支持，有条件的可以支持一下这个项目。
* **本程序设计之初并非为了高效内卷，只想在空闲之余多一点想摸鱼的理由。**

