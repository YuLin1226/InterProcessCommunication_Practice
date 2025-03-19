// image_reader.cpp
#include "image_reader.h"
#include <iostream>
#include <thread>

ImageReader::ImageReader(const std::string& shm_name, size_t max_image_size) {
    try {
        // 創建共享記憶體
        shm_manager_ = std::make_unique<SharedMemoryManager>(
            shm_name, 
            SharedMemoryMode::CREATE, 
            max_image_size
        );
    } catch (const std::exception& ex) {
        std::cerr << "無法創建共享記憶體: " << ex.what() << std::endl;
        throw;
    }
}

ImageReader::~ImageReader() {
    stopCamera();
}

bool ImageReader::readImageFile(const std::string& image_path) {
    try {
        // 讀取圖像文件
        cv::Mat frame = cv::imread(image_path);
        if (frame.empty()) {
            std::cerr << "無法讀取圖像: " << image_path << std::endl;
            return false;
        }
        
        std::cout << "成功讀取圖像: " << image_path << std::endl;
        std::cout << "圖像尺寸: " << frame.cols << "x" << frame.rows << std::endl;
        
        // 保存最後讀取的圖像
        last_image_ = frame.clone();
        
        // 如果有回調，執行回調
        if (image_ready_callback_) {
            image_ready_callback_(last_image_);
        }
        
        // 寫入圖像到共享記憶體
        if (!shm_manager_->writeImage(frame)) {
            std::cerr << "寫入圖像到共享記憶體失敗" << std::endl;
            return false;
        }
        
        // 通知處理進程
        shm_manager_->notifyNewImage();
        
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "讀取圖像文件時出錯: " << ex.what() << std::endl;
        return false;
    }
}

bool ImageReader::startCamera(int camera_id, bool continuous) {
    if (camera_running_) {
        std::cerr << "攝像頭已經在運行中" << std::endl;
        return false;
    }
    
    camera_running_ = true;
    camera_thread_ = std::thread(&ImageReader::cameraLoop, this, camera_id, continuous);
    return true;
}

void ImageReader::stopCamera() {
    camera_running_ = false;
    if (camera_thread_.joinable()) {
        camera_thread_.join();
    }
}

bool ImageReader::waitForProcessing(int timeout_ms) {
    return shm_manager_->waitForProcessingDone(timeout_ms);
}

void ImageReader::cameraLoop(int camera_id, bool continuous) {
    try {
        // 打開攝像頭
        cv::VideoCapture cap(camera_id);
        if (!cap.isOpened()) {
            std::cerr << "無法打開攝像頭" << std::endl;
            camera_running_ = false;
            return;
        }
        
        std::cout << "成功打開攝像頭" << std::endl;
        
        cv::Mat frame;
        while (camera_running_) {
            // 讀取一幀
            cap >> frame;
            if (frame.empty()) {
                std::cerr << "讀取攝像頭幀失敗" << std::endl;
                break;
            }
            
            // 保存最後讀取的圖像
            last_image_ = frame.clone();
            
            // 如果有回調，執行回調
            if (image_ready_callback_) {
                image_ready_callback_(last_image_);
            }
            
            // 寫入圖像到共享記憶體
            if (!shm_manager_->writeImage(frame)) {
                std::cerr << "寫入攝像頭幀到共享記憶體失敗" << std::endl;
                continue;
            }
            
            // 通知處理進程
            shm_manager_->notifyNewImage();
            
            // 等待處理完成，如果是連續模式
            if (continuous) {
                if (!shm_manager_->waitForProcessingDone(1000)) {
                    std::cerr << "等待處理完成超時" << std::endl;
                }
            } else {
                // 非連續模式只處理一幀
                camera_running_ = false;
                break;
            }
            
            // 短暫延遲，減少CPU使用率
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // 關閉攝像頭
        cap.release();
        std::cout << "攝像頭已關閉" << std::endl;
        
    } catch (const std::exception& ex) {
        std::cerr << "攝像頭捕獲時出錯: " << ex.what() << std::endl;
    }
    
    camera_running_ = false;
}
