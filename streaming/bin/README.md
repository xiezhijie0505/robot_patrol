# bin/

此处放板端可执行文件 **MediaMTX（linux arm64）**。

- 不要把体积很大的二进制强行提交 Git（可用 Git LFS，或只在板子上保留）。
- 下载：https://github.com/bluenviron/mediamtx/releases  
  选 `mediamtx_*_linux_arm64.tar.gz`，解压出 `mediamtx` 放到本目录，板端执行：

```sh
chmod +x bin/mediamtx
```

Windows 克隆仓库后，把该文件 `scp` 到板子对应路径即可。
