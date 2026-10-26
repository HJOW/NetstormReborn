// 실제 연결표와 수신 깊이의 원문 정리 순서만 처리한다.
#pragma once
#include "o/RawPacketDispatch.h"

namespace netstorm::o { //연결 숫자 토큰은 호출자가 같은 실제 연결 저장소에서 해석한다.
constexpr std::size_t kTransceiverSlots=22; //005e4a18..005e4a6c의 현재 연결 수다.
constexpr std::size_t kPendingConnectionSlots=32; //005e46f0..005e476c이며0칸도 전체 정리에 포함된다.
// 같은 장기 표·현재 동기화 토큰·pending 개수를 빌리며 새0표를 만들어 대체하지 않는다.
struct ConnectionCleanupGlobals {
    std::span<std::uint32_t> transceivers; //현재22연결이며 버퍼표와 별개다.
    std::span<std::uint32_t> buffers; //현재22송신버퍼이며 전체 연결 정리는 이 표를 지우지 않는다.
    std::span<std::uint32_t> pending; //현재32pending이며 음수 ID는1..31칸만 선택한다.
    std::uint32_t& synchronization; //005c8bf4의 실제 현재 동기화 연결 토큰이다.
    std::uint32_t& pendingCount; //00543604의 실제 DWORD이며 감소는 unsigned 감김을 유지한다.
};
// 노드 조회는 순수 주소 조회이며 고정 소멸/CRT free/진단만 외부 장치다.
struct ConnectionCleanupHooks {
    std::function<PacketConnection&(std::uint32_t)> node; //같은 실제 flags/depth 필드를 반환한다.
    std::function<void(std::uint32_t)> destroy; //004e03d0의 고정 연결 소멸 몸체다.
    std::function<void(std::uint32_t)> release; //004e4354의 연결 또는 별도 송신버퍼 free다.
    std::function<void(std::uint32_t)> assertion; //004e0620의 원문 줄250 진단이다.
};
//004df440: 진입 깊이 보관→flags|40→깊이0에서 소멸→반납이며 소멸 뒤 깊이를 재조회하지 않는다.
void DeferConnectionClose(PacketConnection& connection,const std::function<void()>& destroy,const std::function<void()>& release);
//004df520: -31..-1의 실제 pending만 종료하고 현재 개수 감소→선택칸0→개수 store를 수행한다.
void ClosePendingConnection(std::int32_t id,ConnectionCleanupGlobals globals,const ConnectionCleanupHooks& hooks);
//004e2940: 음수 pending 위임 또는 양수 연결/현재 송신버퍼의 개별 종료다.
void DisconnectTransceiver(std::int32_t id,ConnectionCleanupGlobals globals,const ConnectionCleanupHooks& hooks);
//004e29c0: 현재0..21을 한 번 순회하며 동기화/연결만 정리하고 버퍼표는 보존한다.
void CloseAllTransceivers(ConnectionCleanupGlobals globals,const ConnectionCleanupHooks& hooks);
//004e2a10: 진입 server가 비영일 때만1..8연결을 종료하며 각 현재 슬롯을 다시 읽는다.
void EndPlayerConnections(std::uint32_t server,ConnectionCleanupGlobals globals,const ConnectionCleanupHooks& hooks);
//004df4a0/004e2a00: 현재0..31 pending을 종료하고 개수/동기화/양수표는 쓰지 않는다.
void CloseAllPendingConnections(ConnectionCleanupGlobals globals,const ConnectionCleanupHooks& hooks);
}
