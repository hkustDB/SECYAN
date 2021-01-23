

add_library(secyan
    OEP.cpp
    relation.cpp
    MurmurHash3.cpp
    PSI.cpp
    poly.cpp
    RNG.cpp
    party.cpp
    OT.cpp
)

target_link_libraries(secyan INTERFACE
        ABY::aby
        libOTe
        relic_s
        )


target_include_directories(secyan PUBLIC
        ../../extern/ABY/src/abycore
        ../../extern/ABY/extern/ENCRYPTO_utils/src
        ../../extern/libOTe/cryptoTools
        ../../extern/libOTe/
        )