include_directories(include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_library(LogVisor
            lib/LogVisor.cpp
            include/LogVisor/LogVisor.hpp)
install(DIRECTORY include/LogVisor DESTINATION include/LogVisor)
set(LOG_VISOR_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE PATH "LogVisor include dir" FORCE)
