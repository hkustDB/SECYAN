#include "OT.h"
#include "party.h"
#include <array>
#include "RNG.h"
#include "cryptoTools/Common/BitVector.h"

using namespace osuCrypto;

namespace SECYAN
{

    void OT::Init(Channel &chl, bool isServer)
    {
        this->chl = chl;
        kkrtsender.configure(false, 40, 64);
        kkrtreceiver.configure(false, 40, 64);
        if (isServer)
        {
            GenBaseOTs1();
            GenBaseOTs2();
        }
        else
        {
            GenBaseOTs2();
            GenBaseOTs1();
        }
    }

    void OT::GenBaseOTs1()
    {
        auto count = kkrtreceiver.getBaseOTCount();
        std::vector<std::array<block, 2>> msgs(count);
        iknpsender.send(msgs, gPRNG, chl);
        kkrtreceiver.setBaseOts(msgs, gPRNG, chl);
    }

    void OT::GenBaseOTs2()
    {
        auto count = kkrtsender.getBaseOTCount();
        std::vector<block> msgs(count);
        BitVector bv(count);
        bv.randomize(gPRNG);
        iknpreceiver.receive(bv, msgs, gPRNG, chl);
        kkrtsender.setBaseOts(msgs, bv, chl);
    }

    void OT::Send(std::vector<uint64_t> &msg0, std::vector<uint64_t> &msg1)
    {
        // The number of OTs.
        auto n = msg0.size();

        // Choose which messages should be sent.
        std::vector<std::array<block, 2>> sendMessages(n);
        for (int i = 0; i < n; i++)
            sendMessages[i] = {toBlock(msg0[i]), toBlock(msg1[i])};

        // Send the messages.
        iknpsender.sendChosen(sendMessages, gPRNG, chl);
    }

    std::vector<uint64_t> OT::Recv(std::vector<uint32_t> &selectBits)
    {
        // The number of OTs.
        auto n = selectBits.size();

        // Choose which messages should be received.
        BitVector choices(n);
        for (int i = 0; i < n; i++)
            choices[i] = selectBits[i];

        // Receive the messages
        std::vector<block> messages(n);
        iknpreceiver.receiveChosen(choices, messages, gPRNG, chl);

        // messages[i] = sendMessages[i][choices[i]];
        std::vector<uint64_t> out(n);
        for (int i = 0; i < n; i++)
            out[i] = (uint64_t)_mm_cvtsi128_si64x(messages[i]);
        return out;
    }

    std::vector<std::vector<uint64_t>> OT::OPRFSend(std::vector<std::vector<uint64_t>> &inputs)
    {
        auto outputs = inputs;
        auto n = inputs.size();
        kkrtsender.init(n, gPRNG, chl);

        kkrtsender.recvCorrection(chl, n);

        for (uint32_t i = 0; i < n; i++)
        {
            for (uint32_t j = 0; j < inputs[i].size(); j++)
            {
                kkrtsender.encode(i, &inputs[i][j], &outputs[i][j], sizeof(uint64_t));
            }
        }

        return outputs;
    }

    std::vector<uint64_t> OT::OPRFRecv(std::vector<uint64_t> &inputs)
    {
        auto outputs = inputs;
        auto n = inputs.size();
        kkrtreceiver.init(n, gPRNG, chl);

        for (size_t i = 0; i < n; i++)
        {
            kkrtreceiver.encode(i, &inputs[i], &outputs[i], sizeof(uint64_t));
        }

        kkrtreceiver.sendCorrection(chl, n);
        return outputs;
    }

} // namespace SECYAN