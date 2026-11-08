// Meta 전용 Source 없이 같은 Client의 장기 고정 수신 본문을 연결한다.
#pragma once
#include "o/RawPacketDispatch.h"

namespace netstorm::client { //World/Session/지난 Meta wrapper를 수신 F에 저장하지 않는다.
class Client; //실제 장기 데이터/현재 UI의 owner다.
struct NativeMetaSeatHooks;struct NativeMetaSailHooks; //현재 실제 SHP 포구/항해 장치의 반환 형식이다.
void RefreshClientMetaParticipantCore(Client& client); //같은 기존 Native 본문이며 선택된 하위에서만 현재 장치를 요구한다.
void ConstructClientMetaParticipant(Client& client,std::span<std::uint8_t> record,std::span<const std::uint8_t> packet); //같은 기존 Native 본문이며 선택된 하위에서만 현재 장치를 요구한다.
void ReceiveClientMetaParticipantAdd(Client& client,std::span<const std::uint8_t> packet); //같은 기존 Native 본문이며 선택된 하위에서만 현재 장치를 요구한다.
void ReceiveClientMetaParticipantMove(Client& client,std::span<const std::uint8_t> packet); //같은 기존 Native 본문이며 선택된 하위에서만 현재 장치를 요구한다.
void ExplodeClientMetaBattleSlot(Client& client,std::uint32_t battle,std::uint32_t slot); //같은 기존 Native 본문이며 선택된 하위에서만 현재 장치를 요구한다.
void ReceiveClientMetaBattleOptions(Client& client,std::uint32_t sender,std::span<const std::uint8_t> packet); //같은 기존 Native 본문이며 선택된 하위에서만 현재 장치를 요구한다.
NativeMetaSeatHooks MakeClientMetaSeatShapeDevices(Client& client); //같은 기존 Native 본문이며 선택된 하위에서만 현재 장치를 요구한다.
NativeMetaSailHooks MakeClientMetaSailDevices(Client& client); //같은 기존 Native 본문이며 선택된 하위에서만 현재 장치를 요구한다.
void SendClientMetaData(Client& client,std::uint32_t key,std::span<const std::uint8_t> data,std::uint32_t count); //같은 기존 Native 본문이며 선택된 하위에서만 현재 장치를 요구한다.
std::uint8_t ClientMetaCoreWeight(Client& client,std::span<const std::uint8_t> core); //42b210의 현재 TYPE/기술 비트와 Core 합계이며 Session을 요구하지 않는다.
void ReceiveClientMetaSailError(Client& client,std::span<const std::uint8_t> packet); //4b47e0의 조기 모드 반환/CS 진단과 전체42c7f0을 직접 caller도 공유한다.
void InstallClientMetaPacketReceivers(Client& client,o::RawPacketDispatch& dispatch); //20/21/22/23/24/25/67/88의 actual Native F와 장기 observer/version/length를 같은 Client 기본표에 설치한다.
}
