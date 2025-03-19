// shared_memory_manager.cpp
#include "shared_memory_manager.h"
#include <iostream>
#include <chrono>

SharedMemoryManager::SharedMemoryManager(const std::string& name, SharedMemoryMode mode, size_t max_image_size)
    : name_(name), max_image_size_(max_image_size), is_creator_(mode == SharedMemoryMode::CREATE) {
    
    try {
        if (mode == SharedMemoryMode::CREATE) {
            // 創建新的共享記憶體
            shm_ = bip::shared_memory_object(
                bip::create_only,       // 創建
                name.c_str(),           // 名稱
                bip::read_write         // 讀寫權限
            );
            
            // 設置共享記憶體大小 (結構體大小 + 最大圖像大小)
            const size_t shm_size = sizeof(SharedImageData) + max_image_size_;
            shm_.truncate(shm_size);
            
            // 映射整個共享記憶體區域
            region_ = bip::mapped_region(shm_, bip::read_write);
            
            // 獲取指向共享記憶體的指針並初始化
            void* addr = region_.get_address();
            shared_data_ = new (addr) SharedImageData;
            shared_data_->new_image_ready = false;
            shared_data_->processing_done = true;
            
            std::cout << "創建共享記憶體: " << name << " (" << shm_size << " bytes)" << std::endl;
        } else {
            // 打開已存在的共享記憶體
            shm_ = bip::shared_memory_object(
                bip::open_only,         // 打開現有
                name.c_str(),           // 名稱
                bip::read_write         // 讀寫權限
            );
            
            // 映射共享記憶體區域
            region_ = bip::mapped_region(shm_, bip::read_write);
            
            // 獲取指向共享數據的指針
            shared_data_ = static_cast<SharedImageData*>(region_.get_address());
            
            std::cout << "連接到共享記憶體: " << name << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "共享記憶體錯誤: " << ex.what() << std::endl;
        throw;
    }
}

SharedMemoryManager::~SharedMemoryManager() {
    if (is_creator_) {
        std::cout << "清理共享記憶體: " << name_ << std::endl;
        remove(name_);
    }
}

bool SharedMemoryManager::writeImage(const cv::Mat& image) {
    if (image.empty()) {
        std::cerr << "無法寫入空圖像" << std::endl;
        return false;
    }
    
    size_t data_size = image.total() * image.elemSize();
    if (data_size > max_image_size_) {
        std::cerr << "圖像太大，無法寫入共享記憶體 (" << data_size << " > " << max_image_size_ << ")" << std::endl;
        return false;
    }
    
    // 獲取鎖
    std::unique_lock<bip::interprocess_mutex> lock(shared_data_->mutex);
    
    // 更新共享記憶體中的圖像信息
    shared_data_->width = image.cols;
    shared_data_->height = image.rows;
    shared_data_->channels = image.channels();
    shared_data_->data_size = data_size;
    
    // 複製圖像數據到共享記憶體
    std::memcpy(shared_data_->image_data, image.data, data_size);
    std::cout << "複製圖像到共享記憶體 (" << data_size << " bytes)" << std::endl;
    
    return true;
}

cv::Mat SharedMemoryManager::readImage() {
    // 獲取鎖
    std::unique_lock<bip::interprocess_mutex> lock(shared_data_->mutex);
    
    if (shared_data_->width == 0 || shared_data_->height == 0 || shared_data_->data_size == 0) {
        return cv::Mat();
    }
    
    // 從共享記憶體創建cv::Mat對象
    cv::Mat image(
        shared_data_->height,
        shared_data_->width,
        CV_8UC3,  // 假設彩色圖像，如需支援其他格式，可根據channels值確定
        shared_data_->image_data
    );
    
    return image.clone(); // 返回複製以確保安全
}

void SharedMemoryManager::notifyNewImage() {
    std::unique_lock<bip::interprocess_mutex> lock(shared_data_->mutex);
    shared_data_->new_image_ready = true;
    shared_data_->processing_done = false;
    std::cout << "通知處理進程開始工作" << std::endl;
    shared_data_->new_image_cond.notify_one();
}

bool SharedMemoryManager::waitForNewImage(int timeout_ms) {
    std::unique_lock<bip::interprocess_mutex> lock(shared_data_->mutex);
    
    std::cout << "等待新圖像..." << std::endl;
    
    if (timeout_ms < 0) {
        // 無限等待
        while (!shared_data_->new_image_ready) {
            shared_data_->new_image_cond.wait(lock);
        }
        return true;
    } else {
        // 有超時限制的等待
        auto now = std::chrono::system_clock::now();
        auto end_time = now + std::chrono::milliseconds(timeout_ms);
        
        while (!shared_data_->new_image_ready) {
            if (shared_data_->new_image_cond.timed_wait(lock, end_time)) {
                if (shared_data_->new_image_ready) {
                    return true;
                }
            } else {
                // 超時
                return false;
            }
        }
        return true;
    }
}

void SharedMemoryManager::notifyProcessingDone() {
    std::unique_lock<bip::interprocess_mutex> lock(shared_data_->mutex);
    shared_data_->new_image_ready = false;
    shared_data_->processing_done = true;
    std::cout << "通知讀取進程處理完成" << std::endl;
    shared_data_->processing_done_cond.notify_one();
}

bool SharedMemoryManager::waitForProcessingDone(int timeout_ms) {
    std::unique_lock<bip::interprocess_mutex> lock(shared_data_->mutex);
    
    std::cout << "等待處理完成..." << std::endl;
    
    if (timeout_ms < 0) {
        // 無限等待
        while (!shared_data_->processing_done) {
            shared_data_->processing_done_cond.wait(lock);
        }
        return true;
    } else {
        // 有超時限制的等待
        auto now = std::chrono::system_clock::now();
        auto end_time = now + std::chrono::milliseconds(timeout_ms);
        
        while (!shared_data_->processing_done) {
            if (shared_data_->processing_done_cond.timed_wait(lock, end_time)) {
                if (shared_data_->processing_done) {
                    return true;
                }
            } else {
                // 超時
                return false;
            }
        }
        return true;
    }
}

bool SharedMemoryManager::remove(const std::string& name) {
    return bip::shared_memory_object::remove(name.c_str());
}
