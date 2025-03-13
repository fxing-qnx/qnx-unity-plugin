# ntoaarch64-g++ "Assets/bbqnx/Plugin/QNX800WindowMapperPlugin.cpp" -shared -o "Assets/bbqnx/QNX800WindowMapperPlugin.so" -fPIC -lscreen -lqh -lEGL -lGLESv2 -std=c++11 -D_QNX_SOURCE
ntoaarch64-g++ "demo/test.cxx" -o "test" -fPIC -lscreen -lqh -lEGL -lGLESv2 -std=c++11 -D_QNX_SOURCE
