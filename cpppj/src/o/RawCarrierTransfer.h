// 미등록187 캐리어의 주기적 Halt와 너겟 전달 명령의 실제 호출 순서를 복원한다.
#pragma once
#include "o/RawSquidFinder.h"
#include "o/RawFormPacket.h"
#include <functional>
namespace netstorm::o {
// 세 DWORD는 모든 미등록187 캐리어가 공유하는 원본 전역이며 부모별 타이머가 아니다.
struct CarrierTransferState {
    std::uint32_t haltCounter{},cooldown{},lastDonor{}; // 54542c/545424/545420의 현재 값이다.
    std::uint32_t emptyType{187},nuggetType{154}; // 효과 뒤 다시 읽는 현재 타입 전역이다.
    FormPacketHeader packet{0x60,1,10}; // 40acf0의 BYTE/BYTE/WORD 원문이며 길이를4 이상으로 보정하지 않는다.
};
// 작은 전달 수신기는 실제 Form 가상 호출을 사용하고 큰 경제/경로 선택은 명시 경계다.
struct CarrierTransferHooks {
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> find; // 현재4ac9f0의 내용물 조회다.
    std::function<void(Sid,std::uint32_t)> detach; // Form 가상+48(flags0)이다.
    std::function<void(Sid,Sid,std::uint32_t)> attach; // 같은 Form 가상+50(parent,0)이다.
    std::function<void(Sid)> preview; // 426080의 실제491 예약 입구다.
    std::function<void(std::span<const std::uint8_t>)> broadcast; // 실제4e2e30이며 로컬 수신 뒤 호출한다.
    std::function<std::uint32_t(std::uint32_t)> movementType; // 현재488fe0(type,0,1)이다.
    std::function<void(Sid,std::uint32_t)> removeMovement; // 현재4b1a90(parent,type,0,1)의 실제 삭제다.
    std::function<Sid(Sid)> chooseGeyser; // 429420의 현재 후보 선택이다.
    std::function<void(Sid,Sid)> goGeyser; // 427600의 실제 이동 명령이다.
    std::function<void(Sid)> goHome; // 428fd0의 현재 귀환 요청이다.
    std::function<std::uint32_t(Sid)> commonHalt; // 42a5f0 전체 몸체이며 주기 분기에서만 호출한다.
};
// 같은 풀/TYPE/공간과 현재 권한·편집 모드·로컬 owner를 빌린다.
class RawCarrierTransfer {
public:
    RawCarrierTransfer(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        const bool& boss,const std::uint32_t& editor,const std::uint32_t& localOwner,
        CarrierTransferState& state,CarrierTransferHooks hooks); // 모든 참조는 처리기보다 오래 살아야 한다.
    std::uint32_t Transfer(Sid parent) const; // 실제42a0d0: 전달 성공12, 나머지10이다.
    std::uint32_t Halt(Sid parent) const; // 실제42ac60: 감긴 signed 나머지0일 때만 전달한다.
    void Receive(std::uint32_t owner,std::uint32_t flags,std::span<const std::uint8_t> packet) const; // 실제426830은 두 선행 인자와 packet+8을 읽지 않는다.
    const SidPool& Pool() const; // 다른 세션의 Form/공간 혼합을 검사한다.
private:
    const SidPool& pool_; // 부모/기증자/내용물의 같은 물리 저장소다.
    const SquidHash& hash_; // 실제 일반 점 finder의 네 단계 버킷이다.
    std::span<const RiftTypeRecord> types_; // 현재 내용물 WORD로 다시 조회하는 TYPE 표다.
    const bool& boss_; // 최초 권한 검사에서 현재 값을 읽는다.
    const std::uint32_t& editor_; // 첫 내용물 조회 뒤 현재 편집 모드를 읽는다.
    const std::uint32_t& localOwner_; // 로컬 수신 직전의 원본 owner다.
    CarrierTransferState& state_; // 모든 부모가 공유하는 카운터/타입/헤더다.
    CarrierTransferHooks hooks_; // 선택된 실제 하위만 요구한다.
};
}
