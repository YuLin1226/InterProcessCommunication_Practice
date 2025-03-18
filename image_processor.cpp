// image_processor.cpp - 處理圖像的進程
#include "shared_memory.h"
#include <iostream>
#include <vector>

int main() {
    try {
        std::cout << "嘗試連接到共享記憶體..." << std::endl;
        
        // 打開已存在的共享記憶體
        bip::shared_memory_object shm(
            bip::open_only,            // 打開現有
            "image_processing_shm",    // 同名
            bip::read_write            // 讀寫權限
        );
        
        std::cout << "成功連接到共享記憶體" << std::endl;
        
        // 映射共享記憶體區域
        bip::mapped_region region(shm, bip::read_write);
        
        // 獲取指向共享數據的指針
        SharedImageData* shared_data = static_cast<SharedImageData*>(region.get_address());
        
        // 獲取鎖
        std::unique_lock<bip::interprocess_mutex> lock(shared_data->mutex);
        
        std::cout << "等待新圖像..." << std::endl;
        
        // 等待新圖像
        while (!shared_data->new_image_ready) {
            shared_data->new_image_cond.wait(lock);
        }
        
        std::cout << "接收到新圖像: " << shared_data->width << "x" << shared_data->height 
                 << " (" << shared_data->data_size << " bytes)" << std::endl;
        
        // 從共享記憶體創建cv::Mat對象
        cv::Mat image(
            shared_data->height,
            shared_data->width,
            CV_8UC3,  // 假設彩色圖像
            shared_data->image_data
        );
        
        // 顯示原始圖片
        cv::namedWindow("原始圖片", cv::WINDOW_AUTOSIZE);
        cv::imshow("原始圖片", image);
        
        // 轉換為灰階
        cv::Mat gray;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        
        // 套用高斯模糊以減少噪點
        cv::Mat blurred;
        cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
        
        // 套用二值化以分離前景和背景
        cv::Mat binary;
        cv::threshold(blurred, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
        
        // 顯示二值化結果
        cv::namedWindow("二值化", cv::WINDOW_AUTOSIZE);
        cv::imshow("二值化", binary);
        
        // 尋找輪廓
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
        std::cout << "偵測到 " << contours.size() << " 個物體" << std::endl;
        
        // 繪製輪廓
        cv::Mat result = image.clone();
        for (size_t i = 0; i < contours.size(); i++) {
            // 過濾掉太小的輪廓（可能是噪點）
            double area = cv::contourArea(contours[i]);
            if (area < 500) continue;
            
            // 為每個物體畫輪廓
            cv::Scalar color = cv::Scalar(0, 255, 0); // 綠色
            cv::drawContours(result, contours, i, color, 2);
            
            // 計算並繪製每個物體的邊界框
            cv::Rect boundRect = cv::boundingRect(contours[i]);
            cv::rectangle(result, boundRect, cv::Scalar(0, 0, 255), 2); // 紅色
            
            // 在物體上標記編號
            cv::Point center(boundRect.x + boundRect.width/2, boundRect.y + boundRect.height/2);
            cv::putText(result, "Object " + std::to_string(i), 
                        cv::Point(boundRect.x, boundRect.y - 10),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
        }
        
        // 顯示結果
        cv::namedWindow("物體檢測結果", cv::WINDOW_AUTOSIZE);
        cv::imshow("物體檢測結果", result);
        
        std::cout << "物件檢測完成" << std::endl;
        std::cout << "標記處理完成並通知讀取進程" << std::endl;
        
        // 標記處理完成並通知讀取進程
        shared_data->new_image_ready = false;
        shared_data->processing_done = true;
        shared_data->processing_done_cond.notify_one();
        
        // 等待按鍵後關閉所有視窗
        cv::waitKey(0);
        cv::destroyAllWindows();
        
    } catch (const std::exception& ex) {
        std::cerr << "錯誤: " << ex.what() << std::endl;
        return -1;
    }
    
    return 0;
}