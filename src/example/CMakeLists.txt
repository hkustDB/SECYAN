add_executable(benchmark
    TPCH.cpp
    benchmark.cpp
)

add_executable(secyandemo
    TPCH.cpp
    secyandemo.cpp
)

target_link_libraries(benchmark 
    PUBLIC secyan
    PUBLIC ENCRYPTO_utils::encrypto_utils
    PUBLIC Boost::program_options)

target_link_libraries(secyandemo 
    PUBLIC secyan)