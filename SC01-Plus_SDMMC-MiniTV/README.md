# 本项目参考

https://github.com/moononournation/MiniTV

https://youtu.be/2TOVohmUqOE?si=TeJfgpuY36PCbGmO

# 视频演示

https://youtu.be/Y63FC2zGdZ8?si=6SZcHhU8UXoEGSb4

# ffmpeg转换代码

```
ffmpeg -i input.mp4 -ar 44100 -ac 1 -ab 24k -filter:a loudnorm -filter:a "volume=-5dB" 44100.aac
```

```
ffmpeg -i input.mp4 -vf "fps=20,scale=-1:272:flags=lanczos,crop=480:in_h:(in_w-480)/2:0" -q:v 9 480_20fps.mjpeg
```

# 注意事项

>+ 启动PSRAM