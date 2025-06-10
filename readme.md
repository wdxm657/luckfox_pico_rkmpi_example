## 测评汇总
https://bbs.eeworld.com.cn/thread-1271012-1-1.html

## RV1126 RK3566
https://wiki.fanconn.com/docs_rv1126/rv1126_03
https://docs.radxa.com/

## 驱动开发
https://wiki.lckfb.com/zh-hans/tspi-rk3566/project-case/fat-little-cell-phone/device-tree.html
https://blog.csdn.net/fengli1995/article/details/138192557
https://bbs.eeworld.com.cn/thread-1305930-1-1.html
## 编译
+ 设置环境变量
    ```
    # uclibc
    export LUCKFOX_SDK_PATH=/home/xh/luckfox-pico
    ```
    **注意**：使用绝对地址。
+ 获取仓库源码并设置自动编译脚本执行权限
    ```
    chmod a+x ./build.sh
    ./build.sh
    ```
---
+ 执行 `./build.sh` 后选择 ulibc 类型编译example下的程序
    ```
    1) uclibc
    2) glibc
    Enter your choice [1-2]:
    ```
+ 选择编译的例程
    ```
    1) luckfox_pico_rtsp_opencv
    2) luckfox_pico_rtsp_opencv_capture
    3) luckfox_pico_rtsp_retinaface
    4) luckfox_pico_rtsp_retinaface_osd
    5) luckfox_pico_rtsp_yolov5
    Enter your choice [1-5]:
    ```
---
+ 执行 `./build-linux.sh` 编译examples下的程序
    ``` bash
    ./build-linux.sh -t rv1106 -a armv7l -d yolov8
    ```

## 运行
+ 编译完成后会在 install 文件夹下生成对应的部署文件夹
    ```
    luckfox_pico_rtsp_yolov5_demo 
    ```
+ 将生成的部署文件夹完整上传到 Luckfox Pico 上 (可使用adb ssh等方式) ，板端进入文件夹运行
    ``` bash
    # 例：
    sudo scp -r install/rv1106_linux_armv7l/rknn_yolov8_demo root@192.168.20.117:/root
    ```
    ```
    # 在 Luckfox Pico 板端运行，<Demo Target> 是部署文件夹中的可执行程序
    chmod a+x <Demo Target>
    ./<Demo Target>
    ```
+ 使用 VLC 打开网络串流 `rtsp://192.168.20.117/live/0`（按实际情况修改 IP 地址拉取图像）