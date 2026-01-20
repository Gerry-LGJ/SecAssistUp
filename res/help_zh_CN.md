## 漫游式引导

* 第一次打开程序时会进入**漫游式引导**，在引导中您可以了解到应用程序的基本操作界面，这对您的使用有着极大的帮助
* 如您不慎关闭**漫游式引导**，可以尝试到**设置**中帮助页面找到**重置**按钮，重置完成后需要手动重启应用程序。

## 快速开始

#### 准备环境

* **文件隔离器**的**账号**和**密码**
* 熟悉网页端**文件隔离器**的使用方案
* 使用**Windows10**或**Windows11**操作系统

#### 开始使用

* 输入**账号**和**密码**后点击**登录按钮**，**Website(网站)**使用**默认**的即可

  <img src="screenshots/loginpage.png" style="zoom: 67%;" />

* 在**工程表格**视图中点击**添加工程**按钮添加一个新的工程

  ![](screenshots/newproject_step1.png)

* 在**新建工程**页面中输入**工程名称**和**工作目录**，然后点击页面顶部的**确认**按钮

  ![](screenshots/newproject_step2.png)

* 在**工程表格**中**单击鼠标**选择刚才创建的新工程

* 在主页左侧的文件列表中**单击鼠标**选择需要下载的文件

* **鼠标单击**文件列表下方的**下载按钮**

  <img src="screenshots/newproject_step3.png" alt="New Project Step 3" style="zoom:67%;" />

* 打开文件资源管理器在**工作目录**下可以找到刚才下载好的文件

  ![](screenshots/newproject_step4.png)

## 高级用法

#### 下载后自动执行脚本

* 在**新建工程**或**修改工程**页面中找到**下载脚本**栏目，在输入框内输入脚本文件路径，并勾选使能按钮，然后确认修改

  ![](screenshots/advance_step1.png)

* 当文件下载完成后将会自动执行用户配置的**脚本文件**，切记在Windows平台下仅支持bat脚本，请勿使用其他类型的脚本文件，这可能会导致不可预料的错误发生。

* 在**工程表格**中的**"RDS(下载脚本)"**一栏可以快速设置下载后是否自动执行脚本文件

  ![](screenshots/advance_step2.png)

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

#### 上传前自动执行脚本

* 在**新建工程**或**修改工程**页面中找到**上传文件配置**栏目，添加需要上传的文件，并勾选使能按钮，然后确认修改
* 当文件上传前会自动执行用户配置的**脚本文件**，切记在Windows平台下仅支持bat脚本，请勿使用其他类型的脚本文件，这可能会导致不可预料的错误发生。
* 在**工程表格**中的**RUS**一栏可以快速设置上传前是否自动执行脚本文件，请参考**上传脚本**文件内容进行自定义编写操作逻辑

## 写在最后

* 程序设计之初是为了在多项目之间能够快速切换下载目录，从而达到一键实现全流程自动化调试的能力。
* 在下载脚本中，如果有冷重启(reboot)的需求，与[Uart2Relay](https://gitee.com/time--chicken/uart2relay)项目配合可以达到硬件层的冷重启效果。
* 如本程序对您有帮助，请到项目仓库给一个免费的Star，或给予本项目更多的技术支持，在此感激不尽。
* 感谢 [GitHub - zhuzichu520/FluentUI: FluentUI for QML](https://github.com/zhuzichu520/FluentUI) 提供的技术框架支持，有条件的可以支持一下这个项目。
* **请勿将本程序用于职场内卷范畴，否则本项目将不再给予任何技术支持。**

