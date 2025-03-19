// image_reader.h
#pragma once

#include "shared_memory_manager.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <functional>

// 定義結果回調函數類型
using ImageReadyCallback = std::function<void(const cv::Mat&)>;

class ImageReader {
public:
    // 建構函數
    ImageReader(const std::string& shm_name, size_t max_image_size = 1920 * 1080 * 3);
    
    // 解構函數
    ~ImageReader();
    
    // 讀取圖像文件並送到共享記憶體
    bool readImageFile(const std::string& image_path);
    
    // 讀取攝像頭並送到共享記憶體
    bool startCamera(int camera_id = 0, bool continuous = false);
    
    // 停止攝像頭
    void stopCamera();
    
    // 等待處理完成
    bool waitForProcessing(int timeout_ms = -1);
    
    // 設置回調函數，當讀取到新圖像時呼叫
    void setImageReadyCallback(ImageReadyCallback callback) { image_ready_callback_ = callback; }
    
    // 獲取最後一次處理的圖像
    cv::Mat getLastProcessedImage() const { return last_image_; }

private:
    std::unique_ptr<SharedMemoryManager> shm_manager_;
    cv::Mat last_image_;
    bool camera_running_ = false;
    std::thread camera_thread_;
    ImageReadyCallback image_ready_callback_ = nullptr;
    
    // 攝像頭捕獲循環
    void cameraLoop(int camera_id, bool continuous);
};

