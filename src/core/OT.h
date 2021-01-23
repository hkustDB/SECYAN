#pragma once
#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Network/Channel.h"
#include "libOTe/TwoChooseOne/IknpOtExtReceiver.h"
#include "libOTe/TwoChooseOne/IknpOtExtSender.h"
#include "libOTe/NChooseOne/Kkrt/KkrtNcoOtReceiver.h"
#include "libOTe/NChooseOne/Kkrt/KkrtNcoOtSender.h"
#include <vector>

namespace SECYAN
{

	class OT
	{
	public:
		void Init(osuCrypto::Channel &chl, bool isServer);
		void Send(std::vector<uint64_t> &msg0, std::vector<uint64_t> &msg1);
		std::vector<uint64_t> Recv(std::vector<uint32_t> &selectBits);
		std::vector<std::vector<uint64_t>> OPRFSend(std::vector<std::vector<uint64_t>> &inputs);
		std::vector<uint64_t> OPRFRecv(std::vector<uint64_t> &inputs);

	private:
		osuCrypto::Channel chl;
		osuCrypto::IknpOtExtSender iknpsender;
		osuCrypto::IknpOtExtReceiver iknpreceiver;
		osuCrypto::KkrtNcoOtSender kkrtsender;
		osuCrypto::KkrtNcoOtReceiver kkrtreceiver;
		void GenBaseOTs1();
		void GenBaseOTs2();
	};

} // namespace SECYAN