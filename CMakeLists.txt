include_directories(include)
add_library(LogVisor
            lib/LogVisor.cpp
            include/LogVisor/LogVisor.hpp)

set(LOG_VISOR_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE PATH "LogVisor include path" FORCE)

install(DIRECTORY include/LogVisor DESTINATION include/LogVisor)
