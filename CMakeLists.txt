include_directories(include)
add_library(log-visor
            lib/log.cpp
            include/log-visor/log.hpp)

set(LOG_VISOR_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE PATH "log-visor include path" FORCE)

install(DIRECTORY include/log-visor DESTINATION include/log-visor)
