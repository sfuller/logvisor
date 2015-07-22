include_directories(include)
add_library(LogVisor
            lib/LogVisor.cpp
            include/LogVisor/LogVisor.hpp)
install(DIRECTORY include/LogVisor DESTINATION include/LogVisor)
