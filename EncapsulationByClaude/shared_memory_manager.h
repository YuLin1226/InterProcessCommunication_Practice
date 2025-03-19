// shared_memory_manager.h
#pragma once

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <memory>

namespace bip = boost::interprocess;

// 共享記憶體中的數據結構
struct SharedImageData {
    bool new_image_ready;          // 表示有新圖像
    bool processing_done;          // 表示處理完成
    size_t width;                  // 圖像寬度
    size_t height;                 // 圖像高度
    size_t channels;               // 圖像通道數
    size_t data_size;              // 圖像數據大小
    bip::interprocess_mutex mutex; // 互斥鎖
    bip::interprocess_condition new_image_cond;   // 條件變數：有新圖像
    bip::interprocess_condition processing_done_cond;  // 條件變數：處理完成
    char image_data[0];            // 柔性數組成員，存儲實際圖像數據
};

enum class SharedMemoryMode {
    CREATE, // 創建新的共享記憶體
    OPEN    // 打開已存在的共享記憶體
};

class SharedMemoryManager {
public:
    // 建構函數
    SharedMemoryManager(const std::string& name, SharedMemoryMode mode, size_t max_image_size = 1920 * 1080 * 3);
    
    // 解構函數 - 清理資源
    ~SharedMemoryManager();
    
    // 寫入圖像到共享記憶體
    bool writeImage(const cv::Mat& image);
    
    // 從共享記憶體讀取圖像
    cv::Mat readImage();
    
    // 通知有新圖像可處理
    void notifyNewImage();
    
    // 等待新圖像
    bool waitForNewImage(int timeout_ms = -1);
    
    // 通知圖像處理完成
    void notifyProcessingDone();
    
    // 等待圖像處理完成
    bool waitForProcessingDone(int timeout_ms = -1);
    
    // 移除共享記憶體（靜態方法）
    static bool remove(const std::string& name);
    
    // 獲取共享數據指針
    SharedImageData* getData() { return shared_data_; }

private:
    std::string name_;                          // 共享記憶體名稱
    bip::shared_memory_object shm_;             // 共享記憶體物件
    bip::mapped_region region_;                 // 映射區域
    SharedImageData* shared_data_;              // 共享數據指針
    size_t max_image_size_;                     // 最大圖像大小
    bool is_creator_;                           // 是否為創建者
};

