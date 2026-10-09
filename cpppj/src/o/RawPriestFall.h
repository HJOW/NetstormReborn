// 사제 낙하 요청과 0x25b 이벤트의 조건/효과 순서를 복원한다.
#pragma once
#include "o/RawPriestState.h"
#include "o/RawPriestShield.h"
#include "o/RawPathAnimation.h"
#include "o/SquidProcess.h"

namespace netstorm::o {
// 낙하 처리기가 선택하는 실제 이벤트 번호다. 원본: 004941f0 / CD 0040c880.
inline constexpr std::uint32_t kPriestFallEvent=0x25b;
// 타입별 프레임 선택 입력이다. 실제 공간을 바꾸는 프레임 설정은 훅으로 연결한다.
struct PriestFallFrames {
    std::int32_t falling{}; // 타입 +0x154의 낙하 시작 프레임이다.
    std::int32_t landed{}; // 타입 +0x140의 지면 도착 프레임이다.
};
// 낙하 요청이 권한 효과 직전에 다시 읽는 전역이다.
struct PriestFallState { bool authority{true}; }; // 00540bc4 / CD 00540a2c.
// 미복원 공간/소리 효과와 실제 프로세스/보호막을 연결하는 호출 경계다.
struct PriestFallHooks {
    std::function<void(Sid)> ensureShield;
    std::function<bool()> reserveRegular; // new(0x28)의 실패를 공급할 수 있다.
    std::function<void(Sid,std::uint32_t,float)> addSharedRegular;
    std::function<void(Sid,std::uint32_t)> unpop,repop;
    std::function<void(Sid,float,float)> sound; // 현재 좌표의 priestFall.wav, flags 0이다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 004acee0 / CD 004acbc0.
    std::function<void(Sid,std::uint32_t,std::uint32_t)> advanceFrame; // 004afc90 / CD 004acce0.
    std::function<void(Sid)> clearShield;
};
// 같은 월드에서 낙하 요청과 주기 이벤트를 실행한다. 일반 사제 Pop 지원을 바꾸지 않는다.
class RawPriestFall {
public:
    // 풀/상태/프레임/지도는 이 객체보다 오래 살아야 하며 지도는 256×256 WORD다.
    RawPriestFall(SidPool& pool,const RawPriestState& priest,std::span<const RiftTypeFrames> frames,
        std::span<const PriestFallFrames> choices,std::span<const std::uint16_t> surfaces,PriestFallState& state,PriestFallHooks hooks);
    // 보호막→공유 이벤트 예약→권한 Unpop(0)/Repop(0x800)→현재 좌표 소리 순서다.
    // 원본: 004941f0 / CD 0040c880. 기존 낙하 예약 중복은 검색하지 않는다.
    void Begin(Sid sid) const;
    // count/payload는 읽지 않는다. 지면이 있으면 0, 없으면 0.1f를 반환한다.
    // 원본: 00494580→00493e60 / CD 0040ccd0→0040c430.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // 분배기/효과 어댑터의 같은 월드 연결 검사에 사용한다.
    const SidPool& Pool() const;
private:
    // 원본의 거의 올림 좌표를 유지한다. 지도 밖은 지면 없음이며 비유한 입력은 진단한다.
    bool SurfaceAt(Sid sid) const;
    SidPool& pool_;
    const RawPriestState& priest_;
    std::span<const RiftTypeFrames> frames_;
    std::span<const PriestFallFrames> choices_;
    std::span<const std::uint16_t> surfaces_;
    PriestFallState& state_;
    PriestFallHooks hooks_;
};
// 실제 사제 vtable의 0x25b를 처리하고 다른 사건은 명시 fallback에 넘긴다.
RegularHandler MakePriestFallHandler(const SidPool& pool,const RawPriestFall& fall,RegularHandler fallback={});
// 실제 공유 Regular/보호막 생성·조회와 공급된 공간/소리 훅을 합친다.
PriestFallHooks MakePriestFallHooks(SquidProcessHost& host,const RawPriestShield& shield,
    const RawPriestForcefield& lookup,std::function<void(Sid,std::uint32_t)> destroy,PriestFallHooks hooks);
}
