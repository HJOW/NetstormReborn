// 일반 Dude 작업 번호와 현재 Regular 시각을 실제 원본 순서로 갱신한다.
#pragma once
#include "o/SquidProcess.h"

namespace netstorm::o {
// 같은 월드의 시계와 task WORD 뒤 표면 통지에 쓰는 원본 타입 전역이다.
struct DudeTaskState {
    SquidProcessState& clock; // 새 작업의 float 지연에 더하는 현재 double 게임 시각이다.
    std::uint32_t bridgeType{82}; // task WORD 쓰기 뒤 실제 표면 통지를 받는 다리 타입이다.
};
// 현재 Host Regular와 미복원 표면 효과의 명시적 연결점이다.
struct DudeTaskHooks {
    std::function<RegularProcess*(Sid)> taskRegular; // mask fff0/value5100의 현재 Regular 조회다.
    std::function<RegularProcess*(Sid,std::uint32_t)> addTaskRegular; // event/payload5/flags50의 실제 Regular 생성이다.
    std::function<void(Sid)> notifySurface; // 다리의 task WORD 변경 뒤 실제 표면 통지를 수행한다.
};
// 00450670/CD004e8ba0의 일반 task 설정이며 Dude 생성과 이동은 별도 소비자다.
class RawDudeTask {
public:
    // 시계와 훅은 같은 SID 풀/Host의 현재 저장소를 참조해야 한다.
    RawDudeTask(SidPool& pool,DudeTaskState state,DudeTaskHooks hooks);
    // event는 Regular DWORD와 raw 하위 WORD에 쓰고 delay는 원본 float 폭으로 더한다.
    // CD는 SID0/free/dead에서 반환하고 FALL을 유지한다. 패치 FALL 교체는 원본 진단 경계다.
    void Set(Sid sid,std::uint32_t event,float delay) const;
    // 실제 factory/드롭 소비자가 다른 월드 풀을 혼용하지 않도록 현재 풀을 제공한다.
    const SidPool& Pool() const;
private:
    SidPool& pool_; // task WORD와 현재 프로세스 Form이 공유하는 raw 객체 저장소다.
    DudeTaskState state_; // 같은 double 시계와 현재 bridge 타입 전역이다.
    DudeTaskHooks hooks_; // 실제 Regular 조회/생성과 표면 통지 연결이다.
};
// 일반 task Regular의 실제 조회/생성만 같은 Host에 연결한다.
DudeTaskHooks MakeDudeTaskHostHooks(const SidPool& pool,SquidProcessHost& host,DudeTaskHooks hooks);
}
