// example_processor.cpp
// 處理者程式範例
#include "image_processor.h"
#include <iostream>
#include <string>

void onResultCallback(const cv::Mat& result, const std::vector<ProcessedObject>& objects) {
    std::cout << "處理回調函數被呼叫，偵測到 " << objects.size() << " 個物體" << std::endl;
    for (const auto& obj : objects) {
        std::cout << "物體 #" << obj.id << " - 面積: " << obj.area 
                  << ", 位置: (" << obj.boundingBox.x << "," << obj.boundingBox.y 
                  << "), 大小: " << obj.boundingBox.width << "x" << obj.boundingBox.height << std::endl;
    }
}

int main() {
    try {
        // 建立處理者
        ImageProcessor processor("image_processing_shm");
        
        // 設置參數
        processor.setMinObjectArea(500);
        processor.setBlurSize(5);
        processor.setShowWindows(true);
        
        // 設置回調函數
        processor.setResultCallback(onResultCallback);
        
        std::cout << "處理者已啟動，等待圖像..." << std::endl;
        
        // 單次處理模式
        processor.processOnce();
        
        std::cout << "處理完成" << std::endl;
        
    } catch (const std::exception& ex) {
        std::cerr << "錯誤: " << ex.what() << std::endl;
        return -1;
    }
    
    return 0;
}



