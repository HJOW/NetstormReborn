// 사제/Carrier의 HP·지면·extra 기반 이동 불가 상태 조회를 복원한다.
#pragma once
#include "o/SquidReward.h"

namespace netstorm::o {
// 사제 체력 좌표 모드 동작을 처리하는 자료형이다.
struct PriestHitPointMode {
    bool authority{true}; // 00540bc4 / CD 00540a2c: HP 경계 전환을 실제 공간에 반영하는 권한이다.
    std::array<bool,kPlayerCount+1> recoveryAllowed{}; // Player +0x7c / CD +0x74의 0 아님. 절반 HP로 회복하는 전환의 허용 조건이다.
    const bool* sharedAuthority{}; // 실제 Client의 boss를 빌린다. 독립 Native fixture만 위 값형 입력을 사용한다.
    bool Authority() const { return sharedAuthority ? *sharedAuthority : authority; } // HP·중립 회복의 원문 조회 위치에서 현재 권한을 읽는다.
};
// 사제 체력 좌표 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct PriestHitPointHooks {
    std::function<void(Sid,std::uint32_t)> unpop; // 실제 사제 vtable +0x48. 변경된 HP를 이미 볼 수 있다.
    std::function<void(Sid,std::uint32_t)> repop; // 실제 사제 vtable +0x4c. 저HP이면 0.99999f 배율로 정수 위치를 보정한 뒤 부른다.
};
// 사제/Carrier의 HP·지면·extra 기반 이동 불가 상태 조회를 복원한다.
class RawPriestState {
public:
    // 같은 월드의 HP 모듈·타입·spot을 받는다. 타입은 복사하며 나머지 참조는 더 오래 살아야 한다.
    RawPriestState(SidPool& pool,const SquidReward& hp,std::span<const RiftTypeRecord> types,
        std::span<const std::uint8_t> spots);
    // 패치 DWORD/CD signed WORD의 현재 HP를 읽는다. 최대 HP 모드와 별개인 raw 값이다.
    std::int32_t CurrentHitPoints(Sid sid) const;
    // 후속 회복 처리기의 같은 풀 연결을 검사한다.
    const SidPool& Pool() const;
    // 00492090 / CD 0040d7b0: genus 0x200000, HP/2 경계, 거의 올림한 지면 spot & 6과 extra를 읽는다.
    bool GroundImmobile(Sid sid) const;
    // 00427030 / CD 004e5150: extra 0x20이면 HP/좌표를 조회하지 않고 바로 참을 돌려준다.
    bool Immobile(Sid sid) const;
    // 004942b0 / CD 0040ca40: 절반 HP 전환의 권한/소유자 허용, Unpop→좌표 거의 올림→Repop을 복원한다.
    // 전체 사제 Pop을 대신하지 않으며 두 가상 공간 효과는 호출자가 공급한다.
    void SetHitPoints(Sid sid,std::int32_t value,const PriestHitPointMode& mode,const PriestHitPointHooks& hooks) const;
private:
    // 정상 자산 SID/타입을 검증한다. free/contained를 쓰기 없는 예외로 거부한다.
    std::span<const std::uint8_t> Raw(Sid sid) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // hp_: 객체의 HP를 조회·변경하는 처리기 또는 HP 값.
    const SquidReward& hp_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots_;
};
}
