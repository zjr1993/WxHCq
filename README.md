# WxHCq
1. 基于Excel 2013 SDK开发的加载项编写框架，为用户提供C++编程环境。
2. 设计并实现了高性能自定义函数，满足用户在Excel中的特定需求。
3. 通过此框架，用户可以轻松地使用C++语言编写并加载自定义函数，提升Excel使用体验。
# XLL加载项开发模板

## 1、XLL加载项是什么？

XLL加载项是一种**Excel插件**，它本质上就是一个DLL<sup><i style="font-size:0.7rem;font-weight:100">动态链接库</i></sup> ，因此开发流程也和写DLL类似。特别之处是，在XLL中我们<span alt="rainbow">必须实现几个预定义的</span>**注册函数**，然后才能写我们自己的函数并将其导出。XLL加载项是通过C API和Excel进行数据交互和命令执行的，它是最接近底层的一种插件开发模式。

🧀<font>Excel插件</font>

> excel是一个历史悠久，发展至今功能庞杂的软件。但即便如此，定制个性化功能的需求依然很广泛，因此微软也相继推出了好几种插件开发的模式来满足用户功能定制的需求。
>
> 1. 宏表(xlm)函数，通过这些函数可以调用大部分excel功能，从而实现自动化脚本。不过在excel4.0后逐步淘汰(指官方不在维护更新，也不推荐使用)
> 2. VBA (visual basic for application) 是宏表函数的继任者，也是windows系统上的标配脚本语言，其语法和vb是类似的。
> 3. COM接口，利用COM技术发展起来的一种插件技术，可以利用C#，F#，VB.NET等语言开发，可调用的功能非常丰富，并且可以自定义excel的界面。
> 4. web Api是利用JavaScript开发的一种在线插件(插件本体需要布署到服务器上去)，这种插件开发模式是微软最新推出的，可以很好的结合web技术和excel功能。
> 5. XLL加载项是最古老的插件开发模式，能实现的功能最少，但也是效率最高的方式。

Excel支持的四种开发模式的对比

|    模式     |     开发语言     |                             优点                             |                             缺点                             |
| :---------: | :--------------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
| COM托管模式 | C#、F#、VB.NET等 | 功能最全面，效率高，可以全方位定制excel的界面，操作等等，市面上售卖的插件都是这类。 |               有一定的效率损耗，开发门槛也较高               |
|   web API   |    JavaScript    |  在线插件能方便维护和更新，借助web技术可以实现很炫酷的功能   | 功能支持还不完善，异步调用的编程范式让人很不适应，运行效率一般 |
|     VBA     |   visual basic   |      有操作录制功能，开发效率高，功能全面，可自定义界面      | 缺乏好用的编辑器，语言本身比较过时，写大型项目比较困难，运行效率低 |
|  XLL加载项  |      C/C++       |    性能最高，接近原生程序，适合开发复杂计算类的高性能函数    |                    编程难度最高，很难上手                    |

如果需要实现一些**计算量很大**的函数，那么首选就是**XLL加载项**。

🧙‍♂️<font title="yellow">注册函数</font>

> 注册函数有：xlAutoOpen，xlAutoClose，xlAutoRegister12，xlAutoAdd，xlAutoRemove，xlAutoFree和xlAddInManagerInfo12。

## 2、XLL加载项相关资料

开发包下载：[最新版SDK下载| Microsoft Docs](https://docs.microsoft.com/zh-cn/office/client-developer/excel/welcome-to-the-excel-software-development-kit)

相关书籍：[Financial application using excel add-in development in C/C++](https://baike.baidu.com/item/Financial%20Applications%20using%20Excel%20Add-in%20Development%20in%20C%2FC%2B%2B/56371067?fr=aladdin)

📌<font title="green">说明</font>

> **XLL加载项**是一项古老的技术，相关资料也非常少。书籍的话截止目前就上面列举的那一本😂，但已经过时了😓(里面的内容是针对32位程序的)，所以建议参考SDK包里面的说明文档和例子。

## 3、此模板使用

这个模板是一个真实的工作案例，我在此项目中将 Jieba 的分词功能和 rapidfuzzy 的模糊匹配功能移植到了excel中。采用的方法就是将库函数打包进一个**XLL加载项**中，并以函数的形式导出供Excel用户调用(效果和使用Excel内建函数SUM、AVERGE、CELL完全一样)，项目构建使用的是CMAKE。

如果你要定义自己的函数，请将其写到文件src/function.cpp中，并将函数相关信息(提示、函数名称，接受参数类型，返回参数类型)注册到文件register.h中。
