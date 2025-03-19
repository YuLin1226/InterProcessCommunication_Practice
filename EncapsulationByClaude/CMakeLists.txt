cmake_minimum_required(VERSION 3.10)
project(ImageProcessingSystem)

# C++17 標準
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找尋相依套件
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# 輸出路徑設定
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 包含目錄
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 新增函式庫目標
add_library(SharedMemoryManager SHARED
    shared_memory_manager.cpp
)

add_library(ImageProcessor SHARED
    image_processor.cpp
)

add_library(ImageReader SHARED
    image_reader.cpp
)

# 設定函式庫依賴關係
target_link_libraries(SharedMemoryManager
    ${OpenCV_LIBS}
    ${Boost_LIBRARIES}
)

target_link_libraries(ImageProcessor
    SharedMemoryManager
    ${OpenCV_LIBS}
    ${Boost_LIBRARIES}
)

target_link_libraries(ImageReader
    SharedMemoryManager
    ${OpenCV_LIBS}
    ${Boost_LIBRARIES}
)

# 新增可執行檔目標
add_executable(processor_app example_processor.cpp)
add_executable(reader_app example_reader.cpp)
add_executable(continuous_app example_continuous.cpp)

# 設定可執行檔依賴關係
target_link_libraries(processor_app
    ImageProcessor
    ${OpenCV_LIBS}
    ${Boost_LIBRARIES}
)

target_link_libraries(reader_app
    ImageReader
    ${OpenCV_LIBS}
    ${Boost_LIBRARIES}
)

target_link_libraries(continuous_app
    ImageReader
    ImageProcessor
    ${OpenCV_LIBS}
    ${Boost_LIBRARIES}
)

# 安裝目標
install(TARGETS 
    SharedMemoryManager 
    ImageProcessor 
    ImageReader 
    processor_app 
    reader_app 
    continuous_app
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安裝標頭檔
install(FILES 
    shared_memory_manager.h 
    image_processor.h 
    image_reader.h
    DESTINATION include
)
