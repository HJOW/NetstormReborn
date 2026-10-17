//489ff0 Halt 안의 운반 대상 사망 분기를 현재 경로/내용물/선택기에 연결한다.
#pragma once
#include "o/RawPathRuntime.h"

namespace netstorm::o {
// 큰 조회·선택·새 이동은 이미 복원한 실제 몸체를 공급한다.
struct PathRetargetHooks {
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> find; //4ac9f0(parent,20h,현재nuggetType,0)의 현재 내용물 조회다.
    std::function<Sid(Sid)> chooseReturn,chooseGeyser; //428ea0/429420의 실제 현재 후보 선택이다.
    std::function<void(Sid,Sid)> goReturn,goGeyser; //4276a0/427600이며 호출 중 기존 Runtime이 삭제될 수 있다.
};
// 전체 Halt의 권한/부모 가상/살아 있음 검사는 기존 RawPathProcess가 수행한다.
class RawPathRetarget {
public:
    RawPathRetarget(const SidPool& pool,const std::uint32_t& nuggetType,PathRetargetHooks hooks); // PATCH의 물리 폭과 현재 getter 수명을 확인한다.
    void Update(RawPathRuntime& runtime,Sid initialParent) const; // 부모 Halt 전 SID와 현재 Runtime+8 SID의 서로 다른 역할을 보존한다.
    const SidPool& Pool() const; // 다른 월드의 경로/조회 연결을 거부하기 위한 같은 풀이다.
private:
    Sid Number(std::uint32_t value) const; // 실제로 조회할 때만 DWORD SID의 물리 범위를 확인한다.
    bool Dead(std::uint32_t value) const; // 최초 보관한 대상 SID의 현재 dead2만 읽으며 free1은 별개다.
    const SidPool& pool_; // 부모와 보관한 두 대상의 같은 실제 raw 저장소다.
    const std::uint32_t& nuggetType_; // 각 Find 직전에 다시 읽는5412c0의 현재 값이다.
    PathRetargetHooks hooks_; // 현재 내용물/후보/이동의 큰 실제 경계다.
};
}
