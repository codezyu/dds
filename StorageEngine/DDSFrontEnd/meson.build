project('DDSFrontEnd', 'cpp',
  version     : '1.0',
  default_options : ['cpp_std=c++11']
)

# 获取 C++ 编译器
cc = meson.get_compiler('cpp')

# 包含目录
common_inc = include_directories(
  '../../Common/Include',
  '../../Common/Include/Host',
  '../DDSFrontEnd',
  '../../NDSPI',
)

# 查找并链接后端库（假设编译输出位于 ../BackEndLib）
# backendlib = cc.find_library('BackEndLib', dirs: ['../BackEndLib'], required: false)

# 源文件列表
sources = [
  'DDSBackEndBridge.cpp',
  'DDSDir.cpp',
  'DDSFile.cpp',
  'DDSFrontEnd.cpp'
]

# 生成可执行文件
executable('DDSFrontEnd', sources,
  include_directories : common_inc,
#  link_with            : [backendlib]
)
