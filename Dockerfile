FROM debian:11-slim
RUN apt-get update && apt-get install -y build-essential && rm -rf /var/lib/apt/lists/*
WORKDIR /app
CMD ["g++", "-std=c++17", "-O2", "main.cpp", "-o", "dir-watch-bin"]
