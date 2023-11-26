# 设置

> 确保您已安装 python3。
>
> 然后创建虚拟环境：

```
python3 -m venv venv
```

> 激活虚拟环境：

```
source venv/bin/activate
```

> 安装要求：

```
pip install -r requirements.txt
```

# 添加视频

将视频放在“movies”目录中。 视频应为 mp4 格式，并且应相当短（最多大约 2-3 分钟）。
如果您更改视频的任何参数（例如帧大小），只需删除“cache”目录，它就会在您启动服务器时重新生成。

# 运行

> 运行应用程序：

```
source venv/bin/activate
python3 app.py
```