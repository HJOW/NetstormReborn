// 신전 의식이 쓰는 사제 K~P 프레임 선택과604 공유 예약을 실제 프레임 표/Host에 연결한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
class SquidFrame; // 기존 실제 frame setter를 같은 풀에 연결한다.
class SquidProcessHost; // 기존 실제 SharedRegular를 같은 Kernel에 연결한다.
// 실제 frame setter·new0x28 성공·SharedRegular 생성의 호출 경계다.
struct PriestRitualActionHooks {
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 기존 SquidFrame::Set(sid,type+0x158+stage*4,flags0)이다.
    std::function<bool()> reserveShared; // frame 요청 뒤 원본 new0x28 성공/실패를 공급한다.
    std::function<void(Sid,std::uint32_t,float)> addShared; // 같은 Host.AddSharedRegular(sid,604,0.01f)다.
};
// 00491ca0/00491cf0 ↔ CD0040cf10/0040cf60의 실제 범위/프레임/예약 순서를 복원한다.
class RawPriestRitualActions {
public:
    // 현재 타입별 원본 LetterRun 표는 참조한다. 풀/표/효과 대상은 이 객체보다 오래 살아야 한다.
    RawPriestRitualActions(const SidPool& pool,std::span<const RiftTypeFrames> frames,PriestRitualActionHooks hooks);
    // 유효1..capacity-1 번호만 현재 raw 타입의 K+stage 첫 프레임으로 바꾼다. free/dead/void를 별도 조건으로 쓰지 않는다.
    // stage0..5는 실제 타입 first-table의 K..P 구간이다. 그 밖의 손상 단계는 명시적으로 거부한다.
    void Begin(std::uint32_t sid,std::uint32_t stage) const;
    // 현재 범위 검사→Begin→new0x28→성공 시604(payload0.01) 공유 예약이다. 기존 같은 사건을 취소하지 않는다.
    void Continue(std::uint32_t sid,std::uint32_t stage) const;
    // 연결 어댑터가 같은 raw 월드를 사용하는지 검사한다.
    const SidPool& Pool() const;
private:
    // 원본 pointer 범위와 같은 정상 DWORD SID 범위다. SID0은 자연 반환한다.
    bool Valid(std::uint32_t sid) const;
    const SidPool& pool_; // 현재 부모 raw 타입을 읽는 유일한 풀이다.
    std::span<const RiftTypeFrames> frames_; // 원본 type+0x130..0x16c의 현재 첫 프레임 표를 재사용한다.
    PriestRitualActionHooks hooks_; // 같은 월드의 실제 frame setter와 공유 예약이다.
};
// 기존 SquidFrame/Host의 실제 몸체를 연결한다. 예약 확보 경계는 caller가 공급하며 비울 수 없다.
PriestRitualActionHooks MakePriestRitualActionHooks(const SidPool& pool,const SquidFrame& frame,SquidProcessHost& host,PriestRitualActionHooks hooks);
}
