// image_reader.cpp - 讀取圖像的進程
#include "shared_memory.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <圖像路徑>" << std::endl;
        return -1;
    }
    
    std::string image_path = argv[1];
    
    try {
        // 創建或打開共享記憶體
        bip::shared_memory_object shm(
            bip::create_only,           // 創建
            "image_processing_shm",     // 名稱
            bip::read_write             // 讀寫權限
        );
        
        // 設置共享記憶體大小 (結構體大小 + 最大圖像大小)
        // 假設最大圖像為 1920x1080, 3通道
        const size_t max_image_size = 1920 * 1080 * 3;
        const size_t shm_size = sizeof(SharedImageData) + max_image_size;
        shm.truncate(shm_size);
        
        // 映射整個共享記憶體區域
        bip::mapped_region region(shm, bip::read_write);
        
        // 獲取指向共享記憶體的指針並初始化
        void* addr = region.get_address();
        SharedImageData* shared_data = new (addr) SharedImageData;
        shared_data->new_image_ready = false;
        shared_data->processing_done = true;
        
        // 讀取圖像文件
        cv::Mat frame = cv::imread(image_path);
        if (frame.empty()) {
            std::cerr << "無法讀取圖像: " << image_path << std::endl;
            return -1;
        }
        
        std::cout << "成功讀取圖像: " << image_path << std::endl;
        std::cout << "圖像尺寸: " << frame.cols << "x" << frame.rows << std::endl;
        
        // 獲取鎖
        std::unique_lock<bip::interprocess_mutex> lock(shared_data->mutex);
        
        // 更新共享記憶體中的圖像信息
        shared_data->width = frame.cols;
        shared_data->height = frame.rows;
        shared_data->channels = frame.channels();
        shared_data->data_size = frame.total() * frame.elemSize();
        
        std::cout << "複製圖像到共享記憶體 (" << shared_data->data_size << " bytes)" << std::endl;
        
        // 複製圖像數據到共享記憶體
        std::memcpy(shared_data->image_data, frame.data, shared_data->data_size);
        
        // 設置標誌並通知處理進程
        shared_data->new_image_ready = true;
        shared_data->processing_done = false;
        std::cout << "通知處理進程開始工作" << std::endl;
        shared_data->new_image_cond.notify_one();
        
        // 顯示讀取的圖像
        cv::imshow("Original Image", frame);
        
        // 等待處理進程完成
        std::cout << "等待處理進程完成..." << std::endl;
        while (!shared_data->processing_done) {
            shared_data->processing_done_cond.wait(lock);
        }
        
        std::cout << "處理完成!" << std::endl;
        
        // 等待按鍵退出
        cv::waitKey(0);
        
    } catch (const std::exception& ex) {
        std::cerr << "錯誤: " << ex.what() << std::endl;
        return -1;
    }
    
    // 清理共享記憶體
    std::cout << "清理共享記憶體..." << std::endl;
    bip::shared_memory_object::remove("image_processing_shm");
    return 0;
}