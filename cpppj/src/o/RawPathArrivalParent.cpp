// 원문 SID 진단과 도착 결과를 Host 등록 여부/무조건 예외로 바꾸지 않는다.
#include "o/RawPathArrivalParent.h"
#include "o/RawSquidFinder.h"
#include <cmath>
#include <stdexcept>
namespace netstorm::o { // 선택한 실제 장치만 요구한다.
namespace { // 공통 경계 검사/원문 진단 위치를 내부에서 공유한다.
template<class Function> const Function& NeedArrival(const Function& function) { if (!function) throw std::logic_error("공통 도착의 선택된 실제 경계 미연결");return function; } // 앞선 부분 효과는 유지한다.
constexpr std::string_view kPathSource="\\Ns\\O\\Pathprocess.cpp",kBaseSource="\\Ns\\O\\Baseprocess.cpp"; // 서로 다른 원문 파일/진단 위치다.
void ArrivalAssert(const PathArrivalParentHooks& hooks,std::string_view expression,std::uint32_t line) { NeedArrival(hooks.assertion)(expression,kPathSource,line); } // 진단이 돌아오면 caller가 다음 원문 효과를 선택한다.
}
// 기존 Cell의 양수 지도 절삭/0.9999f bias와 물리 범위 경계를 유지한다.
bool PathArrivalBlocked(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const std::uint8_t> spots,const std::uint32_t& blockingGenus,RawPathPoint point) {
    const auto cell=[](float value,bool biased) { const double adjusted=static_cast<double>(value)+(biased ? 0.9999F : 0.0);if (!std::isfinite(adjusted) || adjusted<0 || adjusted>=256) throw std::out_of_range("경로 지도 좌표");return static_cast<int>(adjusted); }; // 원문 CRT 절삭이 조회할 유효 지도 칸이다.
    const int x=cell(point.x,true),y=cell(point.y,true);if ((spots[y*256+x]&8)==0) return false;
    RawSquidFinder finder(pool,hash,types);auto candidate=finder.Begin({cell(point.x,false),cell(point.y,false),cell(point.x,false),cell(point.y,false)});
    while (candidate.value!=0) { if ((types[pool.Slot(candidate)[10]].flags2&blockingGenus)!=0) return true;candidate=finder.Next(); } // 원문의 첫 현재 부류 일치에서 끝낸다.
    return false;
}
bool RunPathBaseRobust(const PathBaseRobustHooks& hooks) { // 물리 풀 base는 고정이며 SID0은 원문처럼 허용한다.
    const auto form=NeedArrival(hooks.form)();auto capacity=NeedArrival(hooks.capacity)();
    if (form && form>=capacity) { NeedArrival(hooks.assertion)("formSid == 0 || form.isValid()",kBaseSource,0x69);capacity=NeedArrival(hooks.capacity)(); }
    const auto parent=NeedArrival(hooks.parent)();if (parent && parent>=capacity) NeedArrival(hooks.assertion)("attachToSid == 0 || s.isValid()",kBaseSource,0x6c);
    return true;
}
void RunPathArrivalParent(RawCheckingFlag checking,const PathArrivalParentHooks& hooks) { // 살아 있는 현재 객체와 삭제 뒤 안전한 외부 장치를 분리한다.
    const auto parent=NeedArrival(hooks.parent)();
    if (!NeedArrival(hooks.validAlive)(parent)) ArrivalAssert(hooks,"walker.isValid() && !walker.isDead() && !walker.isFree()",0x25c);
    if (!NeedArrival(hooks.alive)()) return;
    const auto point=NeedArrival(hooks.step)();if (!NeedArrival(hooks.alive)()) return;
    NeedArrival(hooks.unpop)(parent);if (!NeedArrival(hooks.alive)()) return;
    NeedArrival(hooks.pop)(parent,point);if (!NeedArrival(hooks.alive)()) return;
    if (checking) NeedArrival(hooks.robust)();if (!NeedArrival(hooks.alive)()) return;
    NeedArrival(hooks.robust)();if (!NeedArrival(hooks.alive)()) return; // 0041be80의 반환은 항상1이므로 원문 false 분기는 미도달이다.
    const bool blocked=NeedArrival(hooks.blocked)(point);
    if (NeedArrival(hooks.halt)()) { if (blocked) NeedArrival(hooks.clear)(parent);return; }
    if (!NeedArrival(hooks.alive)()) { if (blocked) NeedArrival(hooks.clear)(parent);return; }
    NeedArrival(hooks.finished)(true);if (checking) NeedArrival(hooks.robust)();if (!NeedArrival(hooks.alive)()) return;
    if (NeedArrival(hooks.deadOrFree)(parent)) ArrivalAssert(hooks,"!walker.isDead() && !walker.isFree()",0x286);
    if (!NeedArrival(hooks.alive)()) return;
    if (NeedArrival(hooks.watchPresent)()) ArrivalAssert(hooks,"!deadFlag",0x289);
    if (!NeedArrival(hooks.alive)()) return;
    NeedArrival(hooks.armWatch)();const auto result=NeedArrival(hooks.arrival)(parent);
    if (result!=1000 && result!=1001 && result!=1002) ArrivalAssert(hooks,"returnCode == pathKILL || returnCode == pathALREADY_DEAD || returnCode == pathLINGER",0x28d);
    if (!NeedArrival(hooks.alive)()) {
        if (checking && result!=1001) ArrivalAssert(hooks,"returnCode == pathALREADY_DEAD",0x290);
        if (blocked) NeedArrival(hooks.clear)(parent);return;
    }
    NeedArrival(hooks.clearWatch)();if (checking) NeedArrival(hooks.robust)();
    if (NeedArrival(hooks.alive)()) { if (result==1002) NeedArrival(hooks.finished)(false);else NeedArrival(hooks.kill)(); }
    if (blocked) NeedArrival(hooks.clear)(parent);
}
}
