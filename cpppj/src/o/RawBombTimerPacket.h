// 10.78 상태 타이머의 writer/누적 읽기/신규 수신 및 예약 부모 선택을 복원한다.
#pragma once
#include "o/RawBombEffectTimer.h"

namespace netstorm::o {
// 41e280은 현재 선언 길이를 헤더에 쓰며 실제 반환 길이로 헤더를 덮어쓰지 않는다.
std::uint32_t WriteBombTimerPacket(const BombEffectTimerFields& fields,Sid form,Sid parent,std::span<std::uint8_t> packet,std::uint16_t declaredLength,std::uint8_t protocol=1,std::uint8_t message=0x7d);
// 41e230은 flags를 먼저 쓰고 현재 한도까지 기존 목록에 WORD 대상을 추가한 뒤 기한 비트를 복사한다.
void ReadBombTimerPacket(BombEffectTimerFields& fields,std::span<const std::uint8_t> packet);
// 41e0a0은 상태 필터 없이 현재 목록의 TYPE genus40000000인 첫 물리 SID를 반환한다.
Sid BombTimerParent(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint32_t> members);
// 큰 factory/진단/부착은 같은 Session의 실제 소비자에 맡긴다.
struct BombTimerReceiveHooks {
    std::function<void(std::uint32_t)> assertion; // boss 진단cc와 점유 Form 진단e6이다. 반환하면 원문 흐름을 계속한다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> create; // 현재 TYPE+1d0(0,0)의 새 객체 토큰이다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> read; // 새 객체에 실제41e230 전체를 적용한다.
    std::function<void(std::uint32_t,std::uint32_t,Sid,Sid,std::uint32_t)> attach; // 현재 종류/패킷 부모/요청 Form/flags0의41bd20이다.
};
// 41e320의 신규 Form 수신이다. 진단/생성/읽기 뒤 현재 자료를 원본 위치에서 다시 조회한다.
void ReceiveBombTimerPacket(const SidPool& pool,const std::uint32_t& boss,const std::uint32_t& processType,std::span<const std::uint8_t> packet,const BombTimerReceiveHooks& hooks);
}
