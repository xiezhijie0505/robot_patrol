# 模型放置

板端需要一个 **已转成 RKNN** 的 YOLO 检测模型（COCO，含 `person` 类 id=0）。

推荐：

- `yolov5s.rknn`（640 输入，单输出 `[1,25200,85]` 一类常见导出）
- 或 Rockchip 模型库里的 YOLOv5s / YOLOv8n detect RKNN

放到本目录，例如：

```text
detect/models/yolov5s.rknn
```

启动时：

```sh
export YOLO_MODEL=/root/robot_patrol/detect/models/yolov5s.rknn
```

> 本仓库不提交大体积 `.rknn` 文件。若输出张量布局与默认后处理不一致，需按你的模型改 `yolo_postprocess.cpp`。
