#include "OEP.h"

#include <iostream>
#include "RNG.h"
#include "party.h"
#include <cassert>

namespace SECYAN
{
    // "On Arbitrary Waksman Networks and their Vulnerability"

    uint8_t gateNumMap[] = {0, 0, 1, 3, 5, 8, 11, 14, 17, 21, 25, 29, 33, 37,
                            41, 45, 49, 54, 59, 64, 69, 74, 79, 84, 89, 94, 99};
    // return sum(ceil(log2(i))) for i=1 to N
    uint32_t ComputeGateNum(int N)
    {
        if (N < sizeof(gateNumMap) / sizeof(uint8_t))
            return gateNumMap[N];
        int power = floor_log2(N) + 1;
        return power * N + 1 - (1 << power);
    }

    inline uint64_t pack(uint32_t a, uint32_t b)
    {
        return (uint64_t)a << 32 | b;
    }

    inline void unpack(uint64_t c, uint32_t &a, uint32_t &b)
    {
        a = c >> 32;
        b = c & 0xffffffff;
    }

    struct Label
    {
        uint32_t input1;
        uint32_t input2;
        uint32_t output1;
        uint32_t output2;
    };

    struct GateBlinder
    {
        uint32_t upper;
        uint32_t lower;
    };

    void GenSelectionBits(const uint32_t *permuIndices, int size, bool *bits)
    {
        if (size == 2)
            bits[0] = permuIndices[0];
        if (size <= 2)
            return;

        uint32_t *invPermuIndices = new uint32_t[size];
        for (int i = 0; i < size; i++)
            invPermuIndices[permuIndices[i]] = i;

        bool odd = size & 1;

        // Solve the edge coloring problem

        // flag=0: non-specified; flag=1: upperNetwork; flag=2: lowerNetwork
        char *leftFlag = new char[size]();
        char *rightFlag = new char[size]();
        int rightPointer = size - 1;
        int leftPointer;
        while (rightFlag[rightPointer] == 0)
        {
            rightFlag[rightPointer] = 2;
            leftPointer = permuIndices[rightPointer];
            leftFlag[leftPointer] = 2;
            if (odd && leftPointer == size - 1)
                break;
            leftPointer = leftPointer & 1 ? leftPointer - 1 : leftPointer + 1;
            leftFlag[leftPointer] = 1;
            rightPointer = invPermuIndices[leftPointer];
            rightFlag[rightPointer] = 1;
            rightPointer = rightPointer & 1 ? rightPointer - 1 : rightPointer + 1;
        }
        for (int i = 0; i < size - 1; i++)
        {
            rightPointer = i;
            while (rightFlag[rightPointer] == 0)
            {
                rightFlag[rightPointer] = 2;
                leftPointer = permuIndices[rightPointer];
                leftFlag[leftPointer] = 2;
                leftPointer = leftPointer & 1 ? leftPointer - 1 : leftPointer + 1;
                leftFlag[leftPointer] = 1;
                rightPointer = invPermuIndices[leftPointer];

                rightFlag[rightPointer] = 1;
                rightPointer = rightPointer & 1 ? rightPointer - 1 : rightPointer + 1;
            }
        }
        delete[] invPermuIndices;

        // Determine bits on left gates
        int halfSize = size / 2;
        for (int i = 0; i < halfSize; i++)
            bits[i] = leftFlag[2 * i] == 2;

        int upperIndex = halfSize;
        int uppergateNum = ComputeGateNum(halfSize);
        int lowerIndex = upperIndex + uppergateNum;
        int rightGateIndex = lowerIndex + (odd ? ComputeGateNum(halfSize + 1) : uppergateNum);
        // Determine bits on right gates
        for (int i = 0; i < halfSize - 1; i++)
            bits[rightGateIndex + i] = rightFlag[2 * i] == 2;
        if (odd)
            bits[rightGateIndex + halfSize - 1] = rightFlag[size - 2] == 1;

        delete[] leftFlag;
        delete[] rightFlag;

        // Compute upper network
        uint32_t *upperIndices = new uint32_t[halfSize];
        for (int i = 0; i < halfSize - 1 + odd; i++)
            upperIndices[i] = permuIndices[2 * i + bits[rightGateIndex + i]] / 2;
        if (!odd)
            upperIndices[halfSize - 1] = permuIndices[size - 2] / 2;
        GenSelectionBits(upperIndices, halfSize, bits + upperIndex);
        delete[] upperIndices;

        // Compute lower network
        int lowerSize = halfSize + odd;
        uint32_t *lowerIndices = new uint32_t[lowerSize];
        for (int i = 0; i < halfSize - 1 + odd; i++)
            lowerIndices[i] = permuIndices[2 * i + 1 - bits[rightGateIndex + i]] / 2;
        if (odd)
            lowerIndices[halfSize] = permuIndices[size - 1] / 2;
        else
            lowerIndices[halfSize - 1] = permuIndices[2 * halfSize - 1] / 2;
        GenSelectionBits(lowerIndices, lowerSize, bits + lowerIndex);
        delete[] lowerIndices;
    }

    // Inputs of the gate: v0=x0-r1, v1=x1-r2
    // Outputs of the gate: if bit==1 then v0=x1-r3, v1=x0-r4; otherwise v0=x0-r3, v1=x1-r4
    // m0=r1-r3, m1=r2-r4
    void EvaluateGate(uint32_t &v0, uint32_t &v1, GateBlinder blinder, uint8_t bit)
    {
        if (bit)
        {
            uint32_t temp = v1 + blinder.upper;
            v1 = v0 + blinder.lower;
            v0 = temp;
        }
        else
        {
            v0 += blinder.upper;
            v1 += blinder.lower;
        }
    }

    // If you want to apply the original exchange operation, set blinders to be 0;
    void EvaluateNetwork(uint32_t *values, int size, const bool *bits, const GateBlinder *blinders)
    {
        if (size == 2)
            EvaluateGate(values[0], values[1], blinders[0], bits[0]);
        if (size <= 2)
            return;

        int odd = size & 1;
        int halfSize = size / 2;

        // Compute left gates
        for (int i = 0; i < halfSize; i++)
            EvaluateGate(values[2 * i], values[2 * i + 1], blinders[i], bits[i]);
        bits += halfSize;
        blinders += halfSize;

        // Compute upper subnetwork
        uint32_t *upperValues = new uint32_t[halfSize];
        for (int i = 0; i < halfSize; i++)
            upperValues[i] = values[i * 2];
        EvaluateNetwork(upperValues, halfSize, bits, blinders);
        int uppergateNum = ComputeGateNum(halfSize);
        bits += uppergateNum;
        blinders += uppergateNum;

        // Compute lower subnetwork
        int lowerSize = halfSize + odd;
        uint32_t *lowerValues = new uint32_t[lowerSize];
        for (int i = 0; i < halfSize; i++)
            lowerValues[i] = values[i * 2 + 1];
        if (odd) // the last element
            lowerValues[lowerSize - 1] = values[size - 1];
        EvaluateNetwork(lowerValues, lowerSize, bits, blinders);
        int lowergateNum = odd ? ComputeGateNum(lowerSize) : uppergateNum;
        bits += lowergateNum;
        blinders += lowergateNum;

        // Deal with outputs of subnetworks
        for (int i = 0; i < halfSize; i++)
        {
            values[2 * i] = upperValues[i];
            values[2 * i + 1] = lowerValues[i];
        }
        if (odd) // the last element
            values[size - 1] = lowerValues[lowerSize - 1];

        // Compute right gates
        for (int i = 0; i < halfSize - 1 + odd; i++)
            EvaluateGate(values[2 * i], values[2 * i + 1], blinders[i], bits[i]);

        delete[] upperValues;
        delete[] lowerValues;
    }

    void WriteGateLabels(uint32_t *inputLabel, int size, Label *gateLabels)
    {
        if (size == 2)
        {
            gateLabels[0].input1 = inputLabel[0];
            gateLabels[0].input2 = inputLabel[1];
            gateLabels[0].output1 = gRNG.NextUInt32();
            gateLabels[0].output2 = gRNG.NextUInt32();
            inputLabel[0] = gateLabels[0].output1;
            inputLabel[1] = gateLabels[0].output2;
        }

        if (size <= 2)
            return;

        int odd = size & 1;
        int halfSize = size / 2;

        // Compute left gates
        for (int i = 0; i < halfSize; i++)
        {
            gateLabels[i].input1 = inputLabel[2 * i];
            gateLabels[i].input2 = inputLabel[2 * i + 1];
            gateLabels[i].output1 = gRNG.NextUInt32();
            gateLabels[i].output2 = gRNG.NextUInt32();
            inputLabel[2 * i] = gateLabels[i].output1;
            inputLabel[2 * i + 1] = gateLabels[i].output2;
        }
        gateLabels += halfSize;

        // Compute upper subnetwork
        uint32_t *upperInputs = new uint32_t[halfSize];
        for (int i = 0; i < halfSize; i++)
            upperInputs[i] = inputLabel[2 * i];
        WriteGateLabels(upperInputs, halfSize, gateLabels);
        int uppergateNum = ComputeGateNum(halfSize);
        gateLabels += uppergateNum;

        // Compute lower subnetwork
        int lowerSize = halfSize + odd;
        uint32_t *lowerInputs = new uint32_t[lowerSize];
        for (int i = 0; i < halfSize; i++)
            lowerInputs[i] = inputLabel[2 * i + 1];
        if (odd) // the last element
            lowerInputs[lowerSize - 1] = inputLabel[size - 1];
        WriteGateLabels(lowerInputs, lowerSize, gateLabels);
        int lowergateNum = odd ? ComputeGateNum(lowerSize) : uppergateNum;
        gateLabels += lowergateNum;

        // Deal with outputs of subnetworks
        for (int i = 0; i < halfSize; i++)
        {
            inputLabel[2 * i] = upperInputs[i];
            inputLabel[2 * i + 1] = lowerInputs[i];
        }
        if (odd) // the last element
            inputLabel[size - 1] = lowerInputs[lowerSize - 1];

        // Compute right gates
        for (int i = 0; i < halfSize - 1 + odd; i++)
        {
            gateLabels[i].input1 = inputLabel[2 * i];
            gateLabels[i].input2 = inputLabel[2 * i + 1];
            gateLabels[i].output1 = gRNG.NextUInt32();
            gateLabels[i].output2 = gRNG.NextUInt32();
            inputLabel[2 * i] = gateLabels[i].output1;
            inputLabel[2 * i + 1] = gateLabels[i].output2;
        }
        delete[] upperInputs;
        delete[] lowerInputs;
    }

    // The data sender
    std::vector<uint32_t> SenderPermute(std::vector<uint32_t> &values)
    {
        auto size = values.size();
        uint32_t gateNum = ComputeGateNum(size);
        // Sender generates blinded inputs
        Label *gateLabels = new Label[gateNum];

        std::vector<uint32_t> out = values;
        // Locally randomly writes labels for each gate
        WriteGateLabels(&out[0], size, gateLabels);
        std::vector<uint64_t> msg0(gateNum);
        std::vector<uint64_t> msg1(gateNum);

        for (uint32_t i = 0; i < gateNum; i++)
        {
            Label label = gateLabels[i];
            msg0[i] = pack(label.input1 - label.output1, label.input2 - label.output2);
            msg1[i] = pack(label.input2 - label.output1, label.input1 - label.output2);
        }
        gParty.OTSend(msg0, msg1);

        delete[] gateLabels;
        return out;
    }

    // The permutor
    std::vector<uint32_t> PermutorPermute(std::vector<uint32_t> &indices, std::vector<uint32_t> &permutorValues)
    {
        auto size = indices.size();
        std::vector<bool> flag(size, false);
        for (auto i : indices)
            flag[i] = true;
        for (auto f : flag)
            assert(f && "Not a permutation!");
        assert(size == permutorValues.size());
        uint32_t gateNum = ComputeGateNum(size);
        bool *selectBits_arr = new bool[gateNum];
        GenSelectionBits(indices.data(), size, selectBits_arr);
        std::vector<uint32_t> selectBits(selectBits_arr, selectBits_arr + gateNum);
        auto msg = gParty.OTRecv(selectBits);
        GateBlinder *gateBlinders = new GateBlinder[gateNum];
        for (uint32_t i = 0; i < gateNum; i++)
            unpack(msg[i], gateBlinders[i].upper, gateBlinders[i].lower);
        std::vector<uint32_t> out = permutorValues;
        EvaluateNetwork(&out[0], size, selectBits_arr, gateBlinders);
        delete[] gateBlinders;
        delete[] selectBits_arr;
        return out;
    }

    std::vector<uint32_t> SenderReplicate(std::vector<uint32_t> &values)
    {
        auto size = values.size();
        Label *labels = new Label[size - 1];
        std::vector<uint32_t> out(size);
        for (uint32_t i = 0; i < size - 1; i++)
        {
            labels[i].input1 = i == 0 ? values[0] : labels[i - 1].output2;
            labels[i].input2 = values[i + 1];
            out[i] = labels[i].output1 = gRNG.NextUInt32();
            labels[i].output2 = gRNG.NextUInt32();
        }
        out[size - 1] = labels[size - 2].output2;
        std::vector<uint64_t> msg0(size - 1);
        std::vector<uint64_t> msg1(size - 1);
        for (uint32_t i = 0; i < size - 1; i++)
        {
            msg0[i] = pack(labels[i].input1 - labels[i].output1, labels[i].input2 - labels[i].output2);
            msg1[i] = pack(labels[i].input1 - labels[i].output1, labels[i].input1 - labels[i].output2);
        }
        gParty.OTSend(msg0, msg1);
        delete[] labels;
        return out;
    }

    std::vector<uint32_t> PermutorReplicate(std::vector<uint32_t> &repBits, std::vector<uint32_t> &permutorValues)
    {
        auto size = repBits.size() + 1;
        assert(size == permutorValues.size());
        auto msg = gParty.OTRecv(repBits);
        std::vector<uint32_t> out(size);
        Label *labels = new Label[size - 1];
        for (uint32_t i = 0; i < size - 1; i++)
        {
            uint32_t upper, lower;
            unpack(msg[i], upper, lower);
            labels[i].input1 = i == 0 ? permutorValues[i] : labels[i - 1].output2;
            labels[i].input2 = permutorValues[i + 1];
            out[i] = labels[i].output1 = labels[i].input1 + upper;
            labels[i].output2 = (repBits[i] ? labels[i].input1 : labels[i].input2) + lower;
        }
        out[size - 1] = labels[size - 2].output2;
        delete[] labels;
        return out;
    }

    std::vector<uint32_t> SenderExtendedPermute(std::vector<uint32_t> &values, uint32_t N)
    {
        auto M = values.size();
        if (N > M)
            M = N;
        auto extendedValues = values;
        extendedValues.resize(M, 0);
        auto out = SenderPermute(extendedValues);
        out.resize(N);
        out = SenderReplicate(out);
        return SenderPermute(out);
    }

    std::vector<uint32_t> PermutorExtendedPermute(std::vector<uint32_t> &indices, std::vector<uint32_t> &permutorValues)
    {
        auto M = permutorValues.size();
        auto N = indices.size();
        if (N > M)
            M = N;
        std::vector<uint32_t> indicesCount(M, 0);
        for (uint32_t i = 0; i < N; i++)
        {
            assert(indices[i] < permutorValues.size());
            indicesCount[indices[i]]++;
        }
        std::vector<uint32_t> firstPermu(M);
        uint32_t dummyIndex = 0, fPIndex = 0;
        std::vector<uint32_t> repBits(N - 1);

        // We call those index with indicesCount[index]==0 as dummy index
        for (uint32_t i = 0; i < M; i++)
        {
            if (indicesCount[i] > 0)
            {
                firstPermu[fPIndex++] = i;
                for (uint32_t j = 0; j < indicesCount[i] - 1; j++)
                {
                    while (indicesCount[dummyIndex] > 0)
                        dummyIndex++;
                    firstPermu[fPIndex++] = dummyIndex++;
                }
            }
        }
        while (fPIndex < M)
        {
            while (indicesCount[dummyIndex] > 0)
                dummyIndex++;
            firstPermu[fPIndex++] = dummyIndex++;
        }

        auto out = permutorValues;
        out.resize(M);
        out = PermutorPermute(firstPermu, out);
        out.resize(N);
        for (uint32_t i = 0; i < N - 1; i++)
            repBits[i] = indicesCount[firstPermu[i + 1]] == 0;
        out = PermutorReplicate(repBits, out);
        std::vector<uint32_t> pointers(M);
        uint32_t sum = 0;
        for (uint32_t i = 0; i < M; i++)
        {
            pointers[i] = sum;
            sum += indicesCount[i];
        }
        std::vector<uint32_t> totalMap(N);
        for (uint32_t i = 0; i < N; i++)
            totalMap[i] = firstPermu[pointers[indices[i]]++];
        std::vector<uint32_t> invFirstPermu(M);
        for (uint32_t i = 0; i < M; i++)
            invFirstPermu[firstPermu[i]] = i;
        std::vector<uint32_t> secondPermu(N);
        for (int i = 0; i < N; i++)
            secondPermu[i] = invFirstPermu[totalMap[i]];
        return PermutorPermute(secondPermu, out);
    }

    std::vector<uint32_t> SenderAggregate(std::vector<uint32_t> &values)
    {
        auto size = values.size();
        Label *labels = new Label[size - 1];
        std::vector<uint32_t> out(size);
        for (uint32_t i = 0; i < size - 1; i++)
        {
            labels[i].input1 = i == 0 ? values[0] : labels[i - 1].output2;
            labels[i].input2 = values[i + 1];
            out[i] = labels[i].output1 = gRNG.NextUInt32();
            labels[i].output2 = gRNG.NextUInt32();
        }
        out[size - 1] = labels[size - 2].output2;
        std::vector<uint64_t> msg0(size - 1);
        std::vector<uint64_t> msg1(size - 1);
        for (uint32_t i = 0; i < size - 1; i++)
        {
            msg0[i] = pack(labels[i].input1 - labels[i].output1, labels[i].input2 - labels[i].output2);
            msg1[i] = pack(-labels[i].output1, labels[i].input1 + labels[i].input2 - labels[i].output2);
        }
        gParty.OTSend(msg0, msg1);
        delete[] labels;
        return out;
    }

    std::vector<uint32_t> PermutorAggregate(std::vector<uint32_t> &aggBits, std::vector<uint32_t> &permutorValues)
    {
        auto size = aggBits.size() + 1;
        assert(size == permutorValues.size());
        auto msg = gParty.OTRecv(aggBits);
        std::vector<uint32_t> out(size);
        Label *labels = new Label[size - 1];
        for (uint32_t i = 0; i < size - 1; i++)
        {
            uint32_t upper, lower;
            unpack(msg[i], upper, lower);
            labels[i].input1 = i == 0 ? permutorValues[i] : labels[i - 1].output2;
            labels[i].input2 = permutorValues[i + 1];
            labels[i].output1 = upper;
            labels[i].output2 = labels[i].input2 + lower;
            if (aggBits[i])
                labels[i].output2 += labels[i].input1;
            else
                labels[i].output1 += labels[i].input1;
            out[i] = labels[i].output1;
        }
        out[size - 1] = labels[size - 2].output2;
        delete[] labels;
        return out;
    }

} // namespace SECYAN