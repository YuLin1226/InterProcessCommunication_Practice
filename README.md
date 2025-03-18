## Compile

```
g++ -o image_reader image_reader.cpp -lboost_system -lboost_filesystem -lpthread -lrt `pkg-config --cflags --libs opencv4`
g++ -o image_processor image_processor.cpp -lboost_system -lboost_filesystem -lpthread -lrt `pkg-config --cflags --libs opencv4`
```



## If Forgetting to Remove Shared Memory

#### Manually Remove Memory

```
# 查看現有的共享記憶體段
ls -la /dev/shm/
```

You might see this:
```
total 732
drwxrwxrwt  2 root     root          60  三  18 14:50 .
drwxr-xr-x 19 root     root        4880  三  17 08:49 ..
-rw-r--r--  1 ryanchen ryanchen 6220976  三  18 14:44 image_processing_shm
```

第四行 `-rw-r--r--  1 ryanchen ryanchen 6220976  三  18 14:44 image_processing_shm`：
這是你的共享記憶體文件

- `-rw-r--r--` 表示所有者有讀寫權限，其他人只有讀權限
- `ryanchen` 是所有者和所屬組 (你的用戶名)
- `6220976` 是文件大小 (約 6MB)，這對應於你在程式中設置的共享記憶體大小
- `三  18 14:44` 是創建時間 (3月18日 14:44)

```
# 刪除特定的共享記憶體
rm /dev/shm/image_processing_shm
```

