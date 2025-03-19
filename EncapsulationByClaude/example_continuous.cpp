// example_continuous.cpp
// 連續處理範例
#include "image_reader.h"
#include "image_processor.h"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> running(true);

void signalHandler(int signum) {
    std::cout << "接收到信號 " << signum << "，正在停止..." << std::endl;
    running = false;
}

int main(int argc, char** argv) {
    // 註冊信號處理
    signal(SIGINT, signalHandler);
    
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <camera_id>" << std::endl;
        return -1;
    }
    
    int camera_id = std::stoi(argv[1]);
    
    try {
        // 在單一進程中同時啟動讀取者和處理者
        
        // 建立共享記憶體管理器
        SharedMemoryManager shm("continuous_processing_shm", SharedMemoryMode::CREATE);
        
        // 建立處理者和讀取者
        ImageProcessor processor("continuous_processing_shm");
        
        // 設置處理參數
        processor.setMinObjectArea(300);  // 較小的物體也檢測
        processor.setBlurSize(3);
        processor.setShowWindows(true);
        
        // 設置處理回調
        processor.setResultCallback([](const cv::Mat& result, const std::vector<ProcessedObject>& objects) {
            std::cout << "處理完成，偵測到 " << objects.size() << " 個物體" << std::endl;
        });
        
        // 啟動處理循環（非阻塞）
        processor.startProcessingLoop();
        
        // 打開攝像頭
        std::cout << "開啟攝像頭 #" << camera_id << std::endl;
        cv::VideoCapture cap(camera_id);
        if (!cap.isOpened()) {
            std::cerr << "無法開啟攝像頭" << std::endl;
            return -1;
        }
        
        std::cout << "連續處理已啟動，按 Ctrl+C 停止" << std::endl;
        
        // 主循環
        cv::Mat frame;
        while (running) {
            // 讀取一幀
            cap >> frame;
            if (frame.empty()) {
                std::cerr << "讀取攝像頭幀失敗" << std::endl;
                break;
            }
            
            // 顯示原始幀
            cv::imshow("輸入", frame);
            
            // 寫入圖像到共享記憶體
            shm.writeImage(frame);
            
            // 通知處理進程
            shm.notifyNewImage();
            
            // 等待處理完成，但有超時
            if (!shm.waitForProcessingDone(100)) {
                std::cout << "處理超時，跳過此幀" << std::endl;
            }
            
            // 檢查按鍵
            if (cv::waitKey(30) >= 0) {
                break;
            }
        }
        
        // 停止處理循環
        processor.stopProcessingLoop();
        
        // 釋放資源
        cap.release();
        cv::destroyAllWindows();
        
    } catch (const std::exception& ex) {
        std::cerr << "錯誤: " << ex.what() << std::endl;
        return -1;
    }
    
    std::cout << "程式正常退出" << std::endl;
    return 0;
}
