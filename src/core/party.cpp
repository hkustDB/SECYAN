#include <vector>
#include <iostream>
#include "party.h"
#include "RNG.h"

using namespace osuCrypto;

namespace SECYAN
{
	Party gParty;

	void Party::Init(std::string address, uint16_t port, e_role role)
	{
		if(role != SERVER && role != CLIENT)
		{
			std::cerr << "Party initialization error (role error)!" << std::endl;
			std::exit(1);
		}
		this->comm_cost = 0;
		this->address = address;
		this->port = port;
		this->role = role;
		this->abyparty = new ABYParty(role, address, port, LT, 32, 1);
		this->abyparty->ConnectAndBaseOTs();
		gPRNG.SetSeed(osuCrypto::sysRandomSeed());
		sess.start(ios, address, port + 1, role == SERVER ? SessionMode::Server : SessionMode::Client);
		chl = sess.addChannel();
		ot.Init(chl, role == SERVER);
		this->initialized = true;
	}

	void Party::CheckInit()
	{
		if(!initialized)
		{
			std::cerr << "Party not initialized yet!" << std::endl;
			exit(1);
		}
	}

	std::string Party::GetAddress()
	{
		CheckInit();
		return address;
	}

	uint16_t Party::GetPort()
	{
		CheckInit();
		return port;
	}

	ABYParty *Party::GetABYParty()
	{
		CheckInit();
		return abyparty;
	}

	e_role Party::GetRole()
	{
		CheckInit();
		return role;
	}

	e_role Party::GetRevRole()
	{
		CheckInit();
		return (e_role)(1 - role);
	}

	Circuit *Party::GetCircuit(e_sharing sharingType)
	{
		CheckInit();
		std::vector<Sharing *> &sharings = abyparty->GetSharings();
		return sharings[sharingType]->GetCircuitBuildRoutine();
	}

	void Party::ExecCircuit()
	{
		CheckInit();
		abyparty->ExecCircuit(); // comm cost updated here
		auto abysetup = abyparty->GetSentData(P_SETUP) + abyparty->GetReceivedData(P_SETUP);
		auto abyonline = abyparty->GetSentData(P_ONLINE) + abyparty->GetReceivedData(P_ONLINE);
		this->comm_cost += abysetup + abyonline;
	}

	void Party::Reset()
	{
		CheckInit();
		abyparty->Reset();
	}

	void Party::OTSend(std::vector<uint64_t> &msg0, std::vector<uint64_t> &msg1)
	{
		CheckInit();
		ot.Send(msg0, msg1);
	}

	std::vector<uint64_t> Party::OTRecv(std::vector<uint32_t> &selectBits)
	{
		CheckInit();
		return ot.Recv(selectBits);
	}

	std::vector<std::vector<uint64_t>> Party::OPRFSend(std::vector<std::vector<uint64_t>> &inputs)
	{
		CheckInit();
		return ot.OPRFSend(inputs);
	}

	std::vector<uint64_t> Party::OPRFRecv(std::vector<uint64_t> &inputs)
	{
		CheckInit();
		return ot.OPRFRecv(inputs);
	}

	int64_t Party::Tick(std::string name)
	{
		auto it = tick_table.find(name);
		if (it != tick_table.end())
		{
			auto duration = std::chrono::system_clock::now() - it->second;
			tick_table.erase(it);
			int64_t elaspe = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
			if (printTickTime)
				std::cout << name << ": " << elaspe << "ms" << std::endl;
			return elaspe;
		}
		tick_table[name] = std::chrono::system_clock::now();
		return 0;
	}

	uint64_t Party::GetCommCostAndResetStats()
	{
		// In terms of bytes
		auto total_cost = chl.getTotalDataSent() + chl.getTotalDataRecv() + this->comm_cost;
		chl.resetStats();
		this->comm_cost = 0;
		return total_cost;
	}

} // namespace SECYAN