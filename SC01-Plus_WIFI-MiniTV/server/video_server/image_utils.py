import cv2

# 调整封面大小
def resize_cover(frame, target_size):
    target_width, target_height = target_size
    frame_height, frame_width = frame.shape[:2]

    # 计算纵横比
    aspect_ratio = frame_width / frame_height

    # 计算新尺寸
    new_width = int(target_height * aspect_ratio)
    new_height = target_height

    if new_width < target_width:
        new_width = target_width
        new_height = int(target_width / aspect_ratio)

    # 调整框架大小
    frame_resized = cv2.resize(frame, (new_width, new_height))

    # 裁剪至目标尺寸
    x_offset = (new_width - target_width) // 2
    y_offset = (new_height - target_height) // 2

    frame_cropped = frame_resized[y_offset:y_offset + target_height, x_offset:x_offset + target_width]

    return frame_cropped