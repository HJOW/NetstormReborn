// 원본 회수 요청44c420·일반 삭제 송신4437c0·54h 수신4436f0의 순서를 보존한다.
#pragma once
#include "o/RawFormPacket.h"
#include <functional>
#include <string_view>

namespace netstorm::o {
// 현재 미션/Player/제단 전역은 복제하지 않고 같은 저장소를 계속 참조한다.
struct SalvageCommandState {
    const std::uint32_t& localOwner; // 540c70의 전체DWORD 로컬 번호다.
    const std::int32_t& denySalvage; // 595078의 현재 회수 금지 값이다.
    const bool& checking; // 5e4794의 비용 일치 진단이다.
    const bool& mission; // 594fa4가 비영이면 서버의 소유자 비교를 생략한다.
    const bool& boss; // 540bc4의 현재 서버 권한이며 진단 뒤 다시 읽는다.
    const std::uint32_t& altarType; // 5412e8의 현재 제단 타입이다.
    const std::uint32_t& daisType; // 5412ec의 현재 다이스 타입이다.
    const std::uint32_t& nuggetType; // 5412c0의 회수 금지 예외 종류다.
    FormPacketHeader request{0x54,1,8}; // 호출마다 읽는 BYTE/BYTE/WORD 머리다.
};
// 이미 복원한 세계/환불/전송과 현재 가상 삭제만 연결한다.
struct SalvageCommandHooks {
    std::function<Sid(Sid)> daisAt; // 448bb0: 제단과 같은 위치의 실제 다이스다.
    std::function<void(std::string_view)> tell; // 4cf960의 DenySalvage 안내다.
    std::function<void(std::uint32_t)> assertion; // 비용295/서버권한563 원본 진단의 행 번호다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; // 4e2dc0(9,8바이트) 전송 요청이다.
    std::function<void(Sid,std::uint32_t,bool)> refund; // 44c2f0(sid,현재owner,1)의 실제 환불이다.
    std::function<void(Sid,std::uint32_t)> destroy; // 환불 뒤의 현재 VT+10과 보관한 flags를 사용한다.
};
// 요청에서 세계를 삭제하지 않고 실제 서버 수신에서 환불과 삭제를 실행한다.
class RawSalvageCommand {
public:
    RawSalvageCommand(const SidPool& pool,std::span<const RiftTypeRecord> types,SalvageCommandState& state,SalvageCommandHooks hooks); // 같은10.78 풀/타입/전역을 빌린다.
    void Request(std::uint32_t sid) const; // 44c420: 제단 반복 조회와 현재 회수 금지 조건이다.
    void Send(std::uint32_t sid,std::uint32_t flags) const; // 4437c0: SID 하위WORD/flags 상위WORD를 직렬화한다.
    void Receive(std::uint32_t sender,std::span<const std::uint8_t> packet) const; // 4436f0: 미션/소유자/권한 뒤 환불과 가상 삭제다.
private:
    const SidPool& pool_; // 모든 상태를 읽는 같은50BYTE 풀이다.
    std::span<const RiftTypeRecord> types_; // 비용 진단이 현재 읽는188개 타입이다.
    SalvageCommandState& state_; // 값형 헤더와 실제 전역 참조를 함께 보유한다.
    SalvageCommandHooks hooks_; // 도달한 실제 경계만 호출하며 누락은 예외로 거부한다.
};
}
