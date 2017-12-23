#include <stdexcept>
#include <intrin.h>

#include <chrono>

#include "CRC.h"
#include "UdpServer.h"

#pragma comment(lib, "Ws2_32.lib")

UdpServer::UdpServer()
{
  WSADATA w;
  if (WSAStartup(0x0101, &w) != 0)
  {
    throw std::runtime_error("Could not open Windows connection");
  }

  socket_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_ == INVALID_SOCKET)
  {
    WSACleanup();
    throw std::runtime_error("Could not create socket");
  }
  //u_long iMode = 1;
  //ioctlsocket(socket_, FIONBIO, &iMode);

  addr_.sin_family = AF_INET;
  addr_.sin_port = htons(26760);
  addr_.sin_addr.S_un.S_un_b.s_b1 = 127;
  addr_.sin_addr.S_un.S_un_b.s_b2 = 0;
  addr_.sin_addr.S_un.S_un_b.s_b3 = 0;
  addr_.sin_addr.S_un.S_un_b.s_b4 = 1;

  if (bind(socket_, (SOCKADDR*)&addr_, sizeof(SOCKADDR_IN)))
  {
    closesocket(socket_);
    WSACleanup();
    throw std::runtime_error("Could not bind name to socket");
  } 

  serverId_ = 1337;

  packetCounter_ = 0;

  gamePad_.PadState = DsState::CONNECTED;
  gamePad_.Model = DsModel::DS4;
  gamePad_.ConnectionType = DsConnection::USB;
  gamePad_.PadMacAddress.byte1 = 0;
  gamePad_.PadMacAddress.byte2 = 0;
  gamePad_.PadMacAddress.byte3 = 1;
  gamePad_.BatteryStatus = DsBattery::HIGH;
  gamePad_.IsActive = 0;

  gamePadNone_.PadState = DsState::DISCONNECTED;
  gamePadNone_.Model = DsModel::NONE;
  gamePadNone_.ConnectionType = DsConnection::NONE;
  gamePadNone_.PadMacAddress.byte1 = 0;
  gamePadNone_.PadMacAddress.byte2 = 0;
  gamePadNone_.PadMacAddress.byte3 = 0;
  gamePadNone_.BatteryStatus = DsBattery::NONE;
  gamePadNone_.IsActive = 0;
}

void UdpServer::receive()
{
  memset(buffer_, 0, BufferLength);
  int slen = sizeof(SOCKADDR_IN);

  unsigned long l;
  ioctlsocket(socket_, FIONREAD, &l);
  if (l > 0)
  {
    int ret = recvfrom(socket_, buffer_, BufferLength, 0, (SOCKADDR*)&remoteAddr_, &slen);
    if (ret == SOCKET_ERROR)
    {
      throw std::runtime_error("Stuff");
    }

    processIncoming(ret);
  }
}

void UdpServer::newReportIncoming(const Procon::ExpandedPadState& state)
{
  for (auto& client : clients_)
  {
    std::vector<char> outData(100);
    int outIdx = beginPacket(outData, eMessageType::DSUS_PadDataRsp, 1001);

    ProtocolPacket* packet = (ProtocolPacket*)(outData.data());

    packet->Metadata = gamePad_;
    packet->Metadata.IsActive = 1;
    packet->PacketCounter = packetCounter_++;

    packet->LeftKeys = 0x00;
    packet->RightKeys = 0x00;
    packet->PS = 0x00;
    packet->TouchButton = 0x00;
    packet->LeftStick = 0x0000;
    packet->RightStick = 0x0000;
    packet->DpadLeft = 0x00;
    packet->DpadDown = 0x00;
    packet->DpadRight = 0x00;
    packet->DpadUp = 0x00;
    packet->KeyY = 0x00;
    packet->KeyB = 0x00;
    packet->KeyA = 0x00;
    packet->KeyX = 0x00;
    packet->KeyR = 0x00;
    packet->KeyL = 0x00;
    packet->KeyZR = 0x00;
    packet->KeyZL = 0x00;
    packet->TPad0.IsActive = 0;
    packet->TPad0.Id = 0;
    packet->TPad0.X = 0x0000;
    packet->TPad0.Y = 0x0000;
    packet->TPad1.IsActive = 0;
    packet->TPad1.Id = 1;
    packet->TPad1.X = 0x0000;
    packet->TPad1.Y = 0x0000;
    packet->MotionTimestamp = state.lastStatus.time_since_epoch().count() / 1000;

    float tmpAZ = -state.accel.z;
    packet->AccelX = *(uint32_t*)(&state.accel.y);
    packet->AccelY = *(uint32_t*)(&tmpAZ);
    packet->AccelZ = *(uint32_t*)(&state.accel.x);

    float tmpGX = state.gyro.x;
    float tmpGY = -state.gyro.y;
    float tmpGZ = -state.gyro.z;
    packet->GyroPitch = *(uint32_t*)(&tmpGY); // y
    packet->GyroYaw = *(uint32_t*)(&tmpGZ); // -z
    packet->GyroRoll = *(uint32_t*)(&tmpGX); // x

    finishPacket(outData);

    if (sendto(socket_, outData.data(), (uint32_t)outData.size(), 0, (SOCKADDR*)&client, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
    {
      throw std::runtime_error("Something went wrong");
    }
  }
}

void UdpServer::processIncoming(int recv_len)
{
  int currIdx = 0;
  if (buffer_[0] != 'D' || buffer_[1] != 'S' || buffer_[2] != 'U' || buffer_[3] != 'C')
    return;
  else
    currIdx += 4;

  uint16_t protocolVersion = *(uint16_t*)(buffer_ + currIdx);
  currIdx += 2;
  uint16_t packetSize = *(uint16_t*)(buffer_ + currIdx);
  currIdx += 2;
  uint32_t crcValue = *(uint32_t*)(buffer_ + currIdx);
  *(uint32_t*)(buffer_ + currIdx) = 0;
  uint32_t expectedCrc = CRC::Calculate(buffer_, recv_len, CRC::CRC_32());
  currIdx += 4;
  uint32_t clientId = *(uint32_t*)(buffer_ + currIdx);
  currIdx += 4;
  eMessageType messageType = *(eMessageType*)(buffer_ + currIdx);
  currIdx += 4;

  if (messageType == eMessageType::DSUC_VersionReq)
  {
    std::vector<char> outData(4);
    int outIdx = 0;
    // Is inside the header now
    //*(uint32_t*)(outData.data() + outIdx) = (uint32_t)eMessageType::DSUS_VersionRsp;
    //outIdx += 4;

    *(uint16_t*)(outData.data() + outIdx) = MaxProtocolVersion;
    outIdx += 2;
    outData[++outIdx] = 0;
    outData[++outIdx] = 0;

    sendPacket(outData, eMessageType::DSUS_VersionRsp, 1001);
  }
  else if (messageType == eMessageType::DSUC_ListPorts)
  {
    int32_t numPadRequests = *(int32_t*)(buffer_ + currIdx);
    currIdx += 4;

    int requestsIdx = currIdx;
    for (int i = 0; i < numPadRequests; ++i)
    {
      uint8_t currRequest = buffer_[requestsIdx + i];
    }

    std::vector<char> outData(12);
    for (int i = 0; i < numPadRequests; ++i)
    {
      uint8_t currRequest = buffer_[requestsIdx + i];
      
      int outIdx = 0;
      // Is inside the header now
      //*(uint32_t*)(outData.data() + outIdx) = (uint32_t)eMessageType::DSUS_PortInfo;
      //outIdx += 4;

      GamePadMeta* gamePad = (GamePadMeta*)(outData.data() + outIdx);
      if (i == 0)
      {
        *gamePad = gamePad_;
      }
      else
      {
        *gamePad = gamePadNone_;
      }
      gamePad->PadId = i;

      sendPacket(outData, eMessageType::DSUS_PortInfo, 1001);
    }
  }
  else if (messageType == eMessageType::DSUC_PadDataReq)
  {
    uint8_t regFlags = *(uint8_t*)(buffer_ + currIdx);
    currIdx += 1;
    uint8_t idToReg = *(uint8_t*)(buffer_ + currIdx);
    currIdx += 1;
    PhysicalAddress macToReg = *(PhysicalAddress*)(buffer_ + currIdx);
    currIdx += 6;

    //if (std::find(clients_.begin(), clients_.end(), remoteAddr_) == clients_.end())
    //{
    //  clients_.push_back(remoteAddr_);
    //}
    clients_[remoteAddr_] = macToReg;
  }
}

int UdpServer::beginPacket(std::vector<char>& packetData, UdpServer::eMessageType messageType, uint16_t reqProtocolVersion)
{
  PacketHeader* packetHeader = (PacketHeader*)(packetData.data());
  packetHeader->Header[0] = 'D';
  packetHeader->Header[1] = 'S';
  packetHeader->Header[2] = 'U';
  packetHeader->Header[3] = 'S';
  packetHeader->Version = reqProtocolVersion;
  packetHeader->Length = (uint16_t)(packetData.size() - 16);
  packetHeader->Crc32 = 0;
  packetHeader->ServerId = serverId_;
  packetHeader->MessageType = (uint32_t)messageType;

  return sizeof(PacketHeader);
}

void UdpServer::finishPacket(std::vector<char>& packetData)
{
  uint32_t crc32 = CRC::Calculate(packetData.data(), packetData.size(), CRC::CRC_32());
  PacketHeader* packetHeader = (PacketHeader*)(packetData.data());
  packetHeader->Crc32 = crc32;
}

void UdpServer::sendPacket(std::vector<char>& buffer, UdpServer::eMessageType messageType, uint16_t reqProtocolVersion)
{
  packetData_.resize(buffer.size() + sizeof(PacketHeader));
  memset(packetData_.data(), 0, buffer.size() + sizeof(PacketHeader));
  int curIdx = beginPacket(packetData_, messageType, reqProtocolVersion);
  memcpy(packetData_.data() + curIdx, buffer.data(), buffer.size());
  finishPacket(packetData_);

  if (sendto(socket_, packetData_.data(), (uint32_t)packetData_.size(), 0, (SOCKADDR*)&remoteAddr_, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
  {
    throw std::runtime_error("Something went wrong");
  }
}