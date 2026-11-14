From [Zygisk-Il2CppDumper](https://github.com/Perfare/Zygisk-Il2CppDumper)

inject from [Adriller](https://github.com/mustime/Adrill)

## 安装与切换目标

在 ARM64、Android 8.0 或更新的测试设备上启用 Magisk Zygisk，使用 Magisk App 安装
`Zygisk-Il2CppDumper-configurable-arm64.zip` 并重启。

安装后，用设备上的 root shell 把一个目标包名或完整进程名写入模块的
`target.txt`。下面的命令用临时文件替换配置，避免启动进程时读到写了一半的内容。
把 `com.example.game` 换成目标值。

```sh
moddir=/data/adb/modules/zygisk_il2cppdumper_configurable
tmp=$(mktemp "$moddir/.target.XXXXXX") &&
    printf '%s\n' 'com.example.game' > "$tmp" &&
    chmod 0644 "$tmp" &&
    chcon u:object_r:system_file:s0 "$tmp" &&
    mv -f "$tmp" "$moddir/target.txt"
```

模块在目标进程启动前读取配置。更新配置后，完全退出目标游戏，再启动即可。
如果 Unity 使用独立进程，填写完整进程名，例如 `com.example.game:unity`。
初装时配置为空。清空文件后，新启动的进程都不会触发导出。
通过 Magisk App 更新此模块时会保留已生效的 `target.txt`。

导出文件为游戏私有数据目录下的 `files/dump.cs` 和 `files/my_dump.h`。
后者列出 C++ 字段偏移常量。可以用 `adb logcat -s '日志'` 查看导出状态。

## 编译与打包

需要 CMake、Ninja 和 Android NDK。以下 PowerShell 命令从仓库根目录执行。
将 `$ndk` 改为本机的 NDK 路径。

```powershell
$ndk = 'D:/Android/Sdk/ndk/27.0.11718014'
cmake -S . -B build-android -G Ninja "-DCMAKE_TOOLCHAIN_FILE=$ndk/build/cmake/android.toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build-android --target magisk_zip
```

ZIP 生成在 `build-android/Zygisk-Il2CppDumper-configurable-arm64.zip`。
仓库原有的 `output/libzygiskDump.so` 是旧版预编译文件，安装时使用新生成的 ZIP。

