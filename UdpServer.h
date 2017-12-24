#pragma once

#include <vector>
#include <unordered_map>

#define _WINSOCKAPI_
#include <WinSock2.h>

#include "Controller.hpp"

enum class DsState : uint8_t
{
	DISCONNECTED = 0x00,
	RESERVED = 0x01,
	CONNECTED = 0x02
};

enum class DsConnection : uint8_t
{
	NONE = 0x00,
	USB = 0x01,
	BLUETOOTH = 0x02
};

enum class DsModel : uint8_t
{
	NONE = 0x00,
	DS3 = 0x01,
	DS4 = 0x02,
	GENERIC = 0x03
};

enum class DsBattery : uint8_t
{
	NONE = 0x00,
	DYING = 0x01,
	LOW = 0x02,
	MEDIUM = 0x03,
	HIGH = 0x04,
	FULL = 0x05,
	CHARGING = 0xEE,
	CHARGED = 0xEF
};

#pragma pack(push, 1)
struct PhysicalAddress
{
	uint16_t byte1;
	uint16_t byte2;
	uint16_t byte3;
};

struct GamePadMeta
{
	uint8_t PadId;
	DsState PadState;
	DsModel Model;
	DsConnection ConnectionType;
	PhysicalAddress PadMacAddress;
	DsBattery BatteryStatus;
	uint8_t IsActive;
};

struct TouchPad
{
	uint8_t IsActive;
	uint8_t Id;
	uint16_t X;
	uint16_t Y;
};

struct PacketHeader
{
	char Header[4];
	uint16_t Version;
	uint16_t Length;
	uint32_t Crc32;
	uint32_t ServerId;
	uint32_t MessageType;
};

struct ProtocolPacket
{
	PacketHeader HeaderData;
	GamePadMeta Metadata;
	uint32_t PacketCounter;
	uint8_t LeftKeys;
	uint8_t RightKeys;
	uint8_t PS;
	uint8_t TouchButton;
	uint16_t LeftStick;
	uint16_t RightStick;
	uint8_t DpadLeft;
	uint8_t DpadDown;
	uint8_t DpadRight;
	uint8_t DpadUp;
	uint8_t KeyY;
	uint8_t KeyB;
	uint8_t KeyA;
	uint8_t KeyX;
	uint8_t KeyR;
	uint8_t KeyL;
	uint8_t KeyZR;
	uint8_t KeyZL;
	TouchPad TPad0;
	TouchPad TPad1;
	uint64_t MotionTimestamp;
	uint32_t AccelX;
	uint32_t AccelY;
	uint32_t AccelZ;
	uint32_t GyroPitch;
	uint32_t GyroYaw;
	uint32_t GyroRoll;
};
#pragma pack(pop)

struct SOCKADDR_IN_HASH
{
	std::size_t operator()(const SOCKADDR_IN& a) const
	{
		return (std::hash<uint32_t>()(a.sin_addr.S_un.S_addr)
			^ (std::hash<uint16_t>()(a.sin_port))
			^ (std::hash<uint16_t>()(a.sin_family)));
	}
};

struct SOCKADDR_IN_EQUAL
{
	bool operator()(const SOCKADDR_IN& a1, const SOCKADDR_IN& a2) const
	{
		return a1.sin_family == a2.sin_family
			&& a1.sin_port == a2.sin_port
			&& a1.sin_addr.S_un.S_addr == a2.sin_addr.S_un.S_addr;
	}
};

class UdpServer
{
public:
	UdpServer();

	void receive();
	void newReportIncoming(const Procon::ExpandedPadState& state);

private:
	enum class eMessageType : uint32_t
	{
		DSUC_VersionReq = 0x100000,
		DSUS_VersionRsp = 0x100000,
		DSUC_ListPorts = 0x100001,
		DSUS_PortInfo = 0x100001,
		DSUC_PadDataReq = 0x100002,
		DSUS_PadDataRsp = 0x100002,
	};

	static const constexpr uint16_t MaxProtocolVersion = 1001;
	static const constexpr uint16_t BufferLength = 512;

	void processIncoming(int recv_len);

	int beginPacket(std::vector<char>& packetData, eMessageType messageType, uint16_t reqProtocolVersion);
	void sendPacket(std::vector<char>& buffer, eMessageType messageType, uint16_t reqProtocolVersion = MaxProtocolVersion);
	void finishPacket(std::vector<char>& packetData);

	// Temporary
	GamePadMeta gamePad_;
	GamePadMeta gamePadNone_;
	uint32_t packetCounter_;
	// Temporary

	SOCKET socket_;
	SOCKADDR_IN addr_;
	SOCKADDR_IN remoteAddr_;

	std::unordered_map<SOCKADDR_IN, PhysicalAddress, SOCKADDR_IN_HASH, SOCKADDR_IN_EQUAL> clients_;

	uint32_t serverId_;

	char buffer_[BufferLength];
	std::vector<char> packetData_;
};
