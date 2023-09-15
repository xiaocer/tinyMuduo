#!/usr/bin/bash

# 大部分情况下，执行出错则退出程序
set -e

# 如果没有build目录则创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build  &&
    cmake ..    &&
    make

# 回到项目根目录
cd ..

# 如果不存在/usr/include/tinyMuduo目录则创建它
if [ ! -d /usr/include/tinyMuduo ]; then
    mkdir /usr/include/tinyMuduo
fi
# 将头文件拷贝到/usr/include/tinyMuduo，so库文件拷贝到/usr/lib
for header in `ls *.h`
do
    cp $header /usr/include/tinyMuduo
done

cp `pwd`/lib/libtinyMuduo.so /usr/lib

# 搜寻动态库
ldconfig