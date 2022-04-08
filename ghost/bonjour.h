#ifndef BONJOUR_H
#define BONJOUR_H

#include "includes.h"
#include "dns_sd.h"
#include <vector>
#include <tuple>

//
// CBonjour
//

class CBonjour
{
	std::vector<std::tuple<DNSServiceRef, uint16_t, uint32_t, DNSRecordRef>> games;
public:
	DNSServiceRef client = NULL;
	DNSServiceRef admin = NULL;
	std::string interf;
	CBonjour(std::string interfs);
	~CBonjour();
	void Broadcast_Info(bool TFT, unsigned char war3Version, BYTEARRAY mapGameType, BYTEARRAY mapFlags, BYTEARRAY mapWidth, BYTEARRAY mapHeight, std::string gameName, std::string hostName, uint32_t hostTime, std::string mapPath, BYTEARRAY mapCRC, uint32_t slotsTotal, uint32_t slotsTaken, uint16_t port, uint32_t hostCounter, uint32_t entryKey, BYTEARRAY mapSHA1);
};
#endif