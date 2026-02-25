### 引言

本项目引用[SecAssistPro](https://gitee.com/time--chicken/SecAssistPro)重新开发，在SecAssistUp的基础上完善了大部分功能

### 完善功能

* 完善文件上传逻辑，引入大文件分片传输的上传文件逻辑
* 去Qt Quick化，不依赖QML的运行库
* 针对需要上传的文件增加了文件监听功能，文件新增或修改后会自动上传至当前目录
* 新增开机自启动项配置(默认关闭)
* 上传文件成功气泡提示
* 支持国际化

### 参考项目

[SecAssistPro](https://gitee.com/time--chicken/SecAssistPro)

[FluentUI](https://github.com/zhuzichu520/FluentUI)