add_executable(secyantest
    secyantest.cpp
)

target_compile_options(secyantest PRIVATE "-Wall" "-Wextra")
target_link_libraries(secyantest 
    PUBLIC secyan
    PUBLIC ABY::aby
)