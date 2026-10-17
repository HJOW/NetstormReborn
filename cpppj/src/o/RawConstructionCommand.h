// 실제 로컬 건설 요청0x3e의25바이트와 서버 수신444af0을 복원한다.
#pragma once
#include "o/RawConstructionLocal.h"
#include "o/RawPacketDispatch.h"
namespace netstorm::o {
// PATCH0052ede6/7/8의 실제 메시지/프로토콜/선언 길이다.
struct ConstructionCommandHeader {
    std::uint8_t message{0x3e},protocol{1}; // 생성자는 protocol의 상위 비트만 제거한다.
    std::uint16_t length{25}; // 실제40a7c0→4df230은 현재 WORD를 그대로 기록한다.
};
// 40a7c0/4433b0의 비정렬 원본 필드와0시각을 직렬화한다. 뒤의 호스트 버퍼는 보존한다.
std::size_t WriteConstructionCommand(const ConstructionLocalMessage& message,std::span<std::uint8_t> packet,ConstructionCommandHeader header={});
// 현재 서버 수신이 실제 읽는 전역이다. 원본 주소는 순서대로558e74/594fa4/540c70이다.
struct ConstructionCommandState {
    std::uint32_t blocked{}; // 비영이면 타입/패킷 본문을 읽기 전에 반환한다.
    bool chargeBlocked{}; // 비용 선검사를 생략한다. 후속 Request는 별도 원본 판단을 수행한다.
    std::uint32_t localPlayer{1}; // sender 전체 DWORD와 비교하며 BYTE로 줄이지 않는다.
};
// 기존 실제 Money/ChargeShortfall/Request/Reject에 연결할 게임 몸체다.
struct ConstructionCommandHooks {
    std::function<float(std::uint32_t)> money; // 실제40ef30의 현재 잔액이다.
    std::function<void(const ConstructionConfirmRequest&)> shortfall; // 실제442b50이며 좌표/인자/방향을 현재 패킷에서 다시 읽는다.
    std::function<bool(const ConstructionConfirmRequest&)> request; // 실제444760의 전체 MayPlace/충돌/확정이다.
    std::function<void(const ConstructionConfirmRequest&)> reject; // Request 실패 뒤 실제442af0의 환불/취소다.
};
// 패킷의 시각8바이트를 읽지 않고 원본처럼 Request에0/0을 전달한다.
class RawConstructionCommand {
public:
    // 타입/현재 상태/실제 하위는 이 객체보다 오래 살아야 한다. PATCH188개 타입을 요구한다.
    RawConstructionCommand(std::span<const RiftTypeRecord> types,const ConstructionCommandState& state,ConstructionCommandHooks hooks);
    // 실제444af0의 비용 선검사→Request→실패 환불 순서다. type/flags는 수신 범위를 임의 제한하지 않는다.
    void Receive(std::uint32_t sender,std::span<const std::uint8_t> packet) const;
private:
    // 호출 경계가 원본 버퍼를 바꿨으면 후속 호출은 현재 필드를 다시 읽는다.
    static ConstructionConfirmRequest Read(std::uint32_t sender,std::span<const std::uint8_t> packet);
    std::span<const RiftTypeRecord> types_; // 현재 TYPE 비용을 Money 호출 뒤 다시 읽는다.
    const ConstructionCommandState& state_; // callback 뒤에도 같은 현재 전역이다.
    ConstructionCommandHooks hooks_; // 계산을 대신하는 고정 성공값을 공급하지 않는다.
};
// 실제0x3e의 빈 stdcall observer/프로토콜1/정책0/수신 처리를 현재 분배기에 등록한다.
// prepare는 처리기 호출 직전 Session의 같은 현재 전역을 공급한다.
void RegisterConstructionCommand(RawPacketDispatch& dispatch,RawConstructionCommand& command,std::function<void()> prepare={});
}
