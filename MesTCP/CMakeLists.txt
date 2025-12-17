cmake_minimum_required(VERSION 3.16 FATAL_ERROR)  # Минимальная версия для Qt6
project(ChatClient VERSION 1.0 LANGUAGES CXX)

# 1. Настройка C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Чистый C++17 без расширений компилятора

# 2. Указание путей для поиска Qt и Boost
set(CMAKE_PREFIX_PATH 
    "D:/Programs/Qt/6.10.1/mingw_64"  # Путь к Qt
    "D:/Boost/lib/cmake"              # Путь к CMake-модулям Boost
)

# 3. Поиск Qt6 с нужными модулями
find_package(Qt6 COMPONENTS 
    Core 
    Network 
    Widgets 
    REQUIRED
)

# 4. Поиск Boost (header-only режим, так как бинарные файлы не используются)
#    Boost.Asio и Boost.System работают в header-only при условии:
#    - #define BOOST_ASIO_NO_DEPRECATED
#    - #define BOOST_BEAST_NO_DEPRECATED
#    - Использование C++17
set(BOOST_ROOT "D:/Boost")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include/boost-1_90")
find_package(Boost 1.90 REQUIRED)  # Проверка наличия заголовков

# 5. Добавление исполняемого файла клиента
add_executable(ChatClient 
    client/src/Client.cpp
)

# 6. Подключение заголовков
target_include_directories(ChatClient PRIVATE
    client/include    # Для #include "net_client.h"
    include           # Для общих заголовков (net_common.h и др.)
    ${BOOST_INCLUDEDIR}  # Путь к заголовкам Boost
)

# 7. Автоматическая обработка Qt (MOC для сигналов/слотов)
set_target_properties(ChatClient PROPERTIES
    AUTOMOC ON
    AUTOMOC_COMPILER_PREDEFINES OFF  # Важно для MinGW
)

# 8. Линковка библиотек
target_link_libraries(ChatClient PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
    # Boost не требует линковки в header-only режиме
)

# 9. Настройка для Windows (создание консольного приложения)
if(WIN32)
    set_target_properties(ChatClient PROPERTIES
        WIN32_EXECUTABLE FALSE  # FALSE = с консолью, TRUE = без консоли
    )
endif()