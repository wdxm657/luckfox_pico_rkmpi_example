## 测评汇总
https://bbs.eeworld.com.cn/thread-1271012-1-1.html

## RV1126 RK3566
https://wiki.fanconn.com/docs_rv1126/rv1126_03
https://docs.radxa.com/
## 编译
+ 设置环境变量
    ```
    # uclibc
    export LUCKFOX_SDK_PATH=/home/wdxm/code/luck_fox/luckfox-pico
    ```
    **注意**：使用绝对地址。
+ 获取仓库源码并设置自动编译脚本执行权限
    ```
    chmod a+x ./build.sh
    ./build.sh
    ```
+ 执行 `./build.sh` 后选择 libc 类型
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

## 运行
+ 编译完成后会在 install 文件夹下生成对应的部署文件夹
    ```
    luckfox_pico_rtsp_opencv_demo  
    luckfox_pico_rtsp_opencv_capture_demo  
    luckfox_pico_rtsp_retinaface_demo
    luckfox_pico_rtsp_retinaface_osd_demo
    luckfox_pico_rtsp_yolov5_demo 
    ```
+ 将生成的部署文件夹完整上传到 Luckfox Pico 上 (可使用adb ssh等方式) ，板端进入文件夹运行
    ``` bash
    sudo scp build/luckfox_pico_rtsp_opencv root@192.168.30.88:/root
    sudo scp build/luckfox_pico_rtsp_opencv_capture root@192.168.30.88:/root
    sudo scp build/luckfox_pico_rtsp_retinaface root@192.168.30.88:/root
    sudo scp build/luckfox_pico_rtsp_retinaface_osd root@192.168.30.88:/root
    sudo scp build/luckfox_pico_rtsp_yolov5  root@192.168.30.88:/root
    sudo scp build/luckfox_pico_rtsp_yolov8  root@192.168.30.88:/root
    ```
    ```
    # 在 Luckfox Pico 板端运行，<Demo Target> 是部署文件夹中的可执行程序
    chmod a+x <Demo Target>
    ./<Demo Target>
    ```
+ 使用 VLC 打开网络串流 `rtsp://192.168.30.88/live/0`（按实际情况修改 IP 地址拉取图像）