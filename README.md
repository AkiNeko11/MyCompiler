# MyCompiler
编译原理课设仓库

上机练习实验1、2

```bash
# 1. 构建镜像
docker build -t mycompiler .

# 2. 创建并启动容器（首次）
docker run -it --name my-compiler -v .:/app mycompiler

# 3. 退出容器（但不停止）
# 按 Ctrl+P 然后 Ctrl+Q

# 4. 进入正在运行的容器
docker exec -it my-compiler /bin/bash


# 5. 重新启动已停止的容器并进入
docker start -ai my-compiler

# 6. 停止容器
docker stop my-compiler

# 7. 删除容器
docker rm my-compiler

```

g++ lexer.cpp -o lexer

./lexer