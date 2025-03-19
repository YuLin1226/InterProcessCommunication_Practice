// example_reader.cpp
// 讀取者程式範例
#include "image_reader.h"
#include <iostream>
#include <string>

void onImageReady(const cv::Mat& image) {
    std::cout << "新圖像已就緒，尺寸: " << image.cols << "x" << image.rows << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <圖像路徑> [camera_id]" << std::endl;
        return -1;
    }
    
    std::string image_path = argv[1];
    bool use_camera = false;
    int camera_id = 0;
    
    if (argc >= 3) {
        try {
            camera_id = std::stoi(argv[2]);
            use_camera = true;
        } catch (...) {
            std::cerr << "相機ID必須是數字" << std::endl;
            return -1;
        }
    }
    
    try {
        // 建立讀取者
        ImageReader reader("image_processing_shm");
        
        // 設置回調函數
        reader.setImageReadyCallback(onImageReady);
        
        if (use_camera) {
            // 使用攝像頭
            std::cout << "啟動攝像頭 #" << camera_id << std::endl;
            if (!reader.startCamera(camera_id, false)) {
                std::cerr << "無法啟動攝像頭" << std::endl;
                return -1;
            }
        } else {
            // 讀取圖像文件
            std::cout << "讀取圖像文件: " << image_path << std::endl;
            if (!reader.readImageFile(image_path)) {
                std::cerr << "無法讀取圖像文件" << std::endl;
                return -1;
            }
        }
        
        // 等待處理完成
        std::cout << "等待處理完成..." << std::endl;
        reader.waitForProcessing();
        
        std::cout << "處理完成!" << std::endl;
        
        // 等待用戶按鍵
        std::cout << "按任意鍵退出..." << std::endl;
        std::cin.get();
        
    } catch (const std::exception& ex) {
        std::cerr << "錯誤: " << ex.what() << std::endl;
        return -1;
    }
    
    return 0;
}