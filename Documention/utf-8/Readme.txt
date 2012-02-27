一）来龙去脉
原始代码来自google code上开源的libjingle，版本是0.5.8。
libjingle是google实现的基于XMPP协议的Peer to Peer(p2p)通信库。google发布的libjingle代码中有如下示例程序：
talk/examples/call   ----- 语音呼叫示例程序
talk/examples/login  ----- 登录gtalk服务器的一个示例程序
在较早的版本中还有使用p2p实现文件传输的示例程序。

libjingle本身是由C++语言编写，我把p2p和登录gtalk服务器相关的部分接口封装成了c语言接口。目录树及其说明：
libjingle
  |--------tests  测试封装后的c接口的一些代码
  |
  |--------cif    在这儿把c++接口封装成了c接口,cif的意思是c interface
  |
  |--------service 依然是用c++编写的，封装libjingle提供的基本功能
  |
  |--------talk  google实现的libjingle的核心功能
  |
  |--------build-x86 x86平台的编译目录

二）编译系统
google发布的libjingle的自从0.5版开始放弃了automake编译系统，而使用scons。我根据scons重新编写了automake的编译系统。这样可以使用熟悉的 configure; make; make install来完成编译和部署。
需要注意的是，automake编译系统中没有编译语音通话相关的模块。编译系统能够在linux下工作，或许也能在cygwin下工作，但是我没有测试。
在使用编译系统的时候，分开了编译目录和源码目录。这样可以是源码目录保持干净，同时可以同时编译不同平台上的版本，尤其是各种嵌入式平台。只要分别创建不同的编译目录，configure的时候指定正确的 --host即可。
另外google发布的libjingle把其各个内部模块分别编译成了不同的库，这样使用上不是很方便。在automake编译系统中统一编译成了一个libjingle.so共享库，但是对这个共享库的导出符号做了限制，只有添加在jingle.symbols中的符号才能导出。

三）依赖的第三方库
如果不考虑libjingle的语音呼叫功能，它还依赖：
(1) openssl SSL／TLS和各种密码、CA等相关的实现。libjingle用它来实现安全登录gtalk服务器。
(2) libexpat 解析xml。因为XMPP协议的形式是xml流，XMPP基于xml。
还请使用者自行确认有这两个库的开发环境（头文件和库以及可用的openssl命令）。他们都是开源的，可自行google之。

