## 打包项目所需要的链接库
set(DEP_LIB XLCALL32 MMANAGER)

## 收集项目所需要的所有源文件
file(GLOB_RECURSE XlCq_SOURCES "*.cpp")

## 将导出模板文件xxx.def添加到源文件集合中，注意任何在function.cpp中添加
## 的导出函数都需要在xxx.def文件中注册
list(APPEND XlCq_SOURCES "funcsExport.def")

## 添加链接库目录
link_directories(${XlCq_LIB_PATH})

## SHARED 表明生成动态链接库，这将生成xxx.dl文件和 xxx.lib文件
add_library(XlCq SHARED ${XlCq_SOURCES})

## 为目标添加附加目录
target_include_directories(XlCq PUBLIC ${XlCq_INCLUDE_PATH})

## 为目标添加链接库
target_link_libraries(XlCq PUBLIC ${DEP_LIB})

## 设定目标性质
set_target_properties(XlCq PROPERTIES
    CXX_STANDARD             14 
    SUFFIX                   ".xll" 
    CMAKE_LINK_DEF_FILE_FLAG "funcsExport.def"
)

