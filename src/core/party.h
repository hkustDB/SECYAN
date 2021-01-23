#pragma once
#include <string>
#include "aby/abyparty.h"
#include "circuit/circuit.h"
#include "sharing/sharing.h"
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Network/Session.h"
#include "cryptoTools/Network/IOService.h"
#include "OT.h"
#include <unordered_map>
#include <chrono>

namespace SECYAN
{
	class Party
	{
	public:
		void Init(std::string address, uint16_t port, e_role role);
		std::string GetAddress();
		uint16_t GetPort();
		e_role GetRole();
		e_role GetRevRole(); // Get the other role
		ABYParty *GetABYParty();
		Circuit *GetCircuit(e_sharing sharingType);
		void ExecCircuit();
		void Reset();

		template <typename T>
		void Send(const std::vector<T> &buf)
		{
			CheckInit();
			chl.send(buf);
		}
		template <typename T>
		void Send(const T *buf, size_t size)
		{
			CheckInit();
			chl.send(buf, size);
		}

		template <typename T>
		void Recv(std::vector<T> &buf)
		{
			CheckInit();
			chl.recv(buf);
		}

		template <typename T>
		void Recv(T *buf, size_t size)
		{
			CheckInit();
			chl.recv(buf, size);
		}

		void OTSend(std::vector<uint64_t> &msg0, std::vector<uint64_t> &msg1);
		std::vector<uint64_t> OTRecv(std::vector<uint32_t> &selectBits);
		std::vector<std::vector<uint64_t>> OPRFSend(std::vector<std::vector<uint64_t>> &inputs);
		std::vector<uint64_t> OPRFRecv(std::vector<uint64_t> &inputs);
		bool printTickTime = true;
		int64_t Tick(std::string name);
		uint64_t GetCommCostAndResetStats(); // Get number of bytes in all communication
	private:
		bool initialized = false;
		uint64_t comm_cost;
		std::string address;
		uint16_t port;
		e_role role;
		ABYParty *abyparty;
		Circuit *ac, *bc, *yc;
		void CheckInit();
		osuCrypto::IOService ios;
		osuCrypto::Session sess;
		osuCrypto::Channel chl;
		osuCrypto::PRNG prng;
		OT ot;
		std::unordered_map<std::string, std::chrono::system_clock::time_point> tick_table;
	};
	// A global Party
	extern Party gParty;
} // namespace SECYAN