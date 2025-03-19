// image_processor.h
#pragma once

#include "shared_memory_manager.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <functional>

// 定義處理結果的結構
struct ProcessedObject {
    int id;
    cv::Rect boundingBox;
    double area;
    std::vector<cv::Point> contour;
};

// 回調函數定義，用於通知處理結果
using ProcessResultCallback = std::function<void(const cv::Mat&, const std::vector<ProcessedObject>&)>;

class ImageProcessor {
public:
    // 建構函數
    ImageProcessor(const std::string& shm_name);
    
    // 設置處理參數
    void setMinObjectArea(double area) { min_object_area_ = area; }
    void setBlurSize(int size) { blur_size_ = size; }
    void setShowWindows(bool show) { show_windows_ = show; }
    
    // 設置結果回調
    void setResultCallback(ProcessResultCallback callback) { result_callback_ = callback; }
    
    // 啟動處理（阻塞式）
    void processOnce();
    
    // 啟動處理循環（非阻塞式）
    void startProcessingLoop();
    
    // 停止處理循環
    void stopProcessingLoop();
    
    // 處理單張圖像
    std::vector<ProcessedObject> processImage(const cv::Mat& image, cv::Mat& result);

private:
    std::unique_ptr<SharedMemoryManager> shm_manager_;
    double min_object_area_ = 500.0;
    int blur_size_ = 5;
    bool show_windows_ = true;
    bool running_ = false;
    std::thread processing_thread_;
    ProcessResultCallback result_callback_;
    
    // 內部處理循環
    void processingLoop();
};

