# ★ 自动镜像脚本（2026-10-13 排障产物，长期保留）
# 背景：浏览器端烧录工具把「hello」记成了本项目的烧录目标（早期排障环境
#       [env:hello] 删除后遗留的指向），只会去 .pio/build/hello/ 取固件。
#       真实产物在 .pio/build/esp32-s3-devkitc-1/，两边路径对不上时
#       烧录就报「未找到编译产物」。
# 作用：每次固件链接完成，自动把产物镜像到 .pio/build/hello/，
#       让烧录工具永远拿到最新固件，无需手动同步。

Import("env")


def mirror_to_legacy(source, target, env):
    import os
    import shutil

    build_dir = env.subst("$BUILD_DIR")                       # .../esp32-s3-devkitc-1
    legacy_dir = os.path.normpath(os.path.join(build_dir, "..", "hello"))

    os.makedirs(legacy_dir, exist_ok=True)
    copied = []
    # 烧录实际取的 3 个文件 + 其余产物（防止以后工具多取几个）
    for name in (
        "bootloader.bin",
        "partitions.bin",
        "firmware.bin",
        "firmware.elf",
        "firmware.factory.bin",
        "boot_app0.bin",
    ):
        src = os.path.join(build_dir, name)
        if os.path.exists(src):
            shutil.copy2(src, os.path.join(legacy_dir, name))
            copied.append(name)

    print(">>> [mirror] 已镜像 %s -> .pio/build/hello/（烧录工具遗留目标目录）"
          % ", ".join(copied))


# 固件 bin 生成后触发（此时 bootloader/partitions 已先一步生成完毕）
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", mirror_to_legacy)
