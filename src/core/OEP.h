#pragma once
#include <vector>
#include <cstdint>

namespace SECYAN
{
    // oblivious permutation
    std::vector<uint32_t> SenderPermute(std::vector<uint32_t> &values);
    // values & permutedValues form the arithmetic share
    // pass permutedValues as a vector filled with 0 if necessary (when values are fully known)
    std::vector<uint32_t> PermutorPermute(std::vector<uint32_t> &indices, std::vector<uint32_t> &permutorValues);

    // oblivious extended permutation (M values to N values)
    std::vector<uint32_t> SenderExtendedPermute(std::vector<uint32_t> &values, uint32_t N);
    std::vector<uint32_t> PermutorExtendedPermute(std::vector<uint32_t> &indices, std::vector<uint32_t> &permutorValues);

    // aggregate (sum) neighbor values according to aggBits
    std::vector<uint32_t> SenderAggregate(std::vector<uint32_t> &values);
    std::vector<uint32_t> PermutorAggregate(std::vector<uint32_t> &aggBits, std::vector<uint32_t> &permutorValues);

} // namespace SECYAN