// image_processor.cpp
#include "image_processor.h"
#include <iostream>
#include <thread>

ImageProcessor::ImageProcessor(const std::string& shm_name) {
    try {
        // 連接到共享記憶體
        shm_manager_ = std::make_unique<SharedMemoryManager>(shm_name, SharedMemoryMode::OPEN);
    } catch (const std::exception& ex) {
        std::cerr << "無法連接到共享記憶體: " << ex.what() << std::endl;
        throw;
    }
}

void ImageProcessor::processOnce() {
    try {
        // 等待新圖像
        if (!shm_manager_->waitForNewImage()) {
            std::cerr << "等待新圖像超時" << std::endl;
            return;
        }
        
        // 從共享記憶體讀取圖像
        cv::Mat image = shm_manager_->readImage();
        if (image.empty()) {
            std::cerr << "讀取到空圖像" << std::endl;
            return;
        }
        
        std::cout << "接收到新圖像: " << image.cols << "x" << image.rows 
                 << " (" << image.total() * image.elemSize() << " bytes)" << std::endl;
        
        // 處理圖像
        cv::Mat result;
        std::vector<ProcessedObject> objects = processImage(image, result);
        
        // 如果有回調，執行回調
        if (result_callback_) {
            result_callback_(result, objects);
        }
        
        // 通知處理完成
        shm_manager_->notifyProcessingDone();
        
        // 如果顯示窗口，等待按鍵
        if (show_windows_) {
            cv::waitKey(0);
            cv::destroyAllWindows();
        }
        
    } catch (const std::exception& ex) {
        std::cerr << "處理圖像時出錯: " << ex.what() << std::endl;
    }
}

std::vector<ProcessedObject> ImageProcessor::processImage(const cv::Mat& image, cv::Mat& result) {
    std::vector<ProcessedObject> detected_objects;
    
    // 顯示原始圖片
    if (show_windows_) {
        cv::namedWindow("原始圖片", cv::WINDOW_AUTOSIZE);
        cv::imshow("原始圖片", image);
    }
    
    // 轉換為灰階
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    
    // 套用高斯模糊以減少噪點
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(blur_size_, blur_size_), 0);
    
    // 套用二值化以分離前景和背景
    cv::Mat binary;
    cv::threshold(blurred, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    
    // 顯示二值化結果
    if (show_windows_) {
        cv::namedWindow("二值化", cv::WINDOW_AUTOSIZE);
        cv::imshow("二值化", binary);
    }
    
    // 尋找輪廓
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    std::cout << "偵測到 " << contours.size() << " 個輪廓" << std::endl;
    
    // 繪製輪廓
    result = image.clone();
    int valid_object_count = 0;
    
    for (size_t i = 0; i < contours.size(); i++) {
        // 過濾掉太小的輪廓（可能是噪點）
        double area = cv::contourArea(contours[i]);
        if (area < min_object_area_) continue;
        
        // 為每個物體建立結構
        ProcessedObject obj;
        obj.id = valid_object_count++;
        obj.area = area;
        obj.contour = contours[i];
        obj.boundingBox = cv::boundingRect(contours[i]);
        
        // 為每個物體畫輪廓
        cv::Scalar color = cv::Scalar(0, 255, 0); // 綠色
        cv::drawContours(result, contours, i, color, 2);
        
        // 計算並繪製每個物體的邊界框
        cv::rectangle(result, obj.boundingBox, cv::Scalar(0, 0, 255), 2); // 紅色
        
        // 在物體上標記編號
        cv::Point center(obj.boundingBox.x + obj.boundingBox.width/2, obj.boundingBox.y + obj.boundingBox.height/2);
        cv::putText(result, "Object " + std::to_string(obj.id), 
                    cv::Point(obj.boundingBox.x, obj.boundingBox.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
        
        // 添加到結果集
        detected_objects.push_back(obj);
    }
    
    // 顯示結果
    if (show_windows_) {
        cv::namedWindow("物體檢測結果", cv::WINDOW_AUTOSIZE);
        cv::imshow("物體檢測結果", result);
    }
    
    std::cout << "物件檢測完成，有效物體數量: " << valid_object_count << std::endl;
    
    return detected_objects;
}

void ImageProcessor::startProcessingLoop() {
    if (running_) return;
    
    running_ = true;
    processing_thread_ = std::thread(&ImageProcessor::processingLoop, this);
}

void ImageProcessor::stopProcessingLoop() {
    running_ = false;
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
}

void ImageProcessor::processingLoop() {
    while (running_) {
        try {
            // 使用非阻塞方式等待，以便可以檢查running_標誌
            if (shm_manager_->waitForNewImage(100)) {
                // 從共享記憶體讀取圖像
                cv::Mat image = shm_manager_->readImage();
                if (!image.empty()) {
                    std::cout << "處理循環中接收到新圖像" << std::endl;
                    
                    // 處理圖像
                    cv::Mat result;
                    std::vector<ProcessedObject> objects = processImage(image, result);
                    
                    // 如果有回調，執行回調
                    if (result_callback_) {
                        result_callback_(result, objects);
                    }
                    
                    // 通知處理完成
                    shm_manager_->notifyProcessingDone();
                    
                    // 顯示結果（非阻塞）
                    if (show_windows_) {
                        cv::waitKey(1);
                    }
                }
            }
        } catch (const std::exception& ex) {
            std::cerr << "處理循環中出錯: " << ex.what() << std::endl;
        }
    }
}
