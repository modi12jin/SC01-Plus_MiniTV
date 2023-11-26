import subprocess
import soundfile as sf
from tqdm import tqdm
import tempfile
import shutil
import numpy as np
import cv2
import hashlib
import os
from scipy.signal import resample
import pickle
import gzip

from .image_utils import resize_cover

# 提取音频
def extract_audio(video_file, sample_rate=16000):
    # 创建一个命名的临时文件来存储音频
    with tempfile.NamedTemporaryFile(suffix=".wav") as f:
        # 检查 ffmpeg 是否存在
        if not shutil.which("ffmpeg"):
            raise Exception("ffmpeg not installed!")
        # 将视频文件转换为wav格式
        status = subprocess.call(
            ["ffmpeg", "-i", video_file, "-q:a", "0", "-map", "a", f.name, "-y"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.STDOUT,
        )
        if status != 0:
            raise Exception("ffmpeg returned a non-zero status: {}".format(status))
        # 读取音频文件
        audio_data, sample_rate = sf.read(f.name)
        # 检查音频是否为立体声
        if len(audio_data.shape) == 2 and audio_data.shape[1] == 2:
            # 通过平均两个通道转换为单声道
            audio_data = np.mean(audio_data, axis=1)
        # 重新采样至 16kHz
        resampled_audio = resample(
            audio_data, int(len(audio_data) * 16000 / sample_rate)
        )
        # 标准化音频
        resampled_audio = resampled_audio / np.max(np.abs(resampled_audio))
        # 转换为有符号 8 位
        audio_8bit = np.int8(resampled_audio * 127)
        # 创建音频缓冲区
        audio_buffer = audio_8bit.tobytes()
        # 我们现在在缓冲区中有 16KHz 8 位单声道音频
        return audio_buffer

# 提取视频帧
def extract_video_frames(video_file, target_size, frame_rate=15):
    video_jpegs = []
    cap = cv2.VideoCapture(video_file)
    # 获取总帧数
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    frame_interval = 1000 / frame_rate
    # 确保我们得到第一帧
    last_frame_time = -frame_interval
    with tqdm(total=total_frames, desc=video_file) as progress:
        while cap.isOpened():
            frame_exists, frame = cap.read()
            if frame_exists:
                # 我们只需要帧率
                frame_ms = int(cap.get(cv2.CAP_PROP_POS_MSEC))
                if frame_ms - last_frame_time >= frame_interval:
                    last_frame_time = frame_ms
                    # 调整框架大小以固定我们的目标尺寸
                    frame = resize_cover(frame, target_size)
                    # 低质量以节省空间和带宽
                    encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 50]
                    _, buffer = cv2.imencode(".jpg", frame, encode_param)
                    video_jpegs.append((frame_ms, buffer.tobytes()))
                progress.update(1)
            else:
                break
    cap.release()
    return video_jpegs

# 获取视频哈希值
def get_video_hash(video_file):
    hash = hashlib.sha256(video_file.encode("utf-8")).hexdigest()
    return hash

# 获取视频数据
def get_video_data(video_file):
    hash = get_video_hash(video_file)
    cache_file_name = f"cache/{hash}.pkl.gz"
    with gzip.open(cache_file_name, "rb") as f:
        data = pickle.load(f)
        audio = data["audio"]
        frames = data["frames"]
        return audio, frames

# 处理视频文件
def process_video_file(
    video_path, video_file, target_size=(480, 320), sample_rate=16000, frame_rate=20
):
    hash = get_video_hash(video_file)
    cache_file_name = f"cache/{hash}.pkl.gz"
    # 检查我们是否已经处理了该视频
    if not os.path.exists(cache_file_name):
        # 提取音频和视频帧
        full_path = os.path.join(video_path, video_file)
        audio = extract_audio(full_path)
        frames = extract_video_frames(full_path, target_size, frame_rate)
        # 将音频和帧保存到缓存
        with gzip.open(cache_file_name, "wb") as f:
            pickle.dump({"audio": audio, "frames": frames}, f)
    else:
        audio, frames = get_video_data(video_file)
    return audio, frames

VALID_MOVIE_EXTENSTIONS = [".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm"]
def is_movie_file(file):
    return os.path.splitext(file)[1] in VALID_MOVIE_EXTENSTIONS

# 处理视频
def process_videos(
    video_path, target_size=(480, 320), sample_rate=16000, frame_rate=20
):
    # 创建缓存目录
    os.makedirs("cache", exist_ok=True)
    # 获取视频路径下的文件列表
    files = os.listdir(video_path)
    # 过滤掉非视频文件
    files = [f for f in files if is_movie_file(f)]
    # 按字母顺序排列文件
    files.sort()
    # 处理每个视频文件
    video_data = []
    for file in tqdm(files, desc="Processing videos"):
        audio, frames = process_video_file(
            video_path, file, target_size, sample_rate, frame_rate
        )
        video_data.append((audio, frames))
    return video_data
