// shared_memory.h - 共享記憶體定義
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <opencv2/opencv.hpp>

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