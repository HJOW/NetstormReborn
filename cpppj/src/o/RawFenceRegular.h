// 썬 바리케이드의 실제 Handle00457a50에서 프레임/연결 변경 순서를 보존한다.
#pragma once
#include "o/SquidFrame.h"
#include "o/SquidProcess.h"
#include <array>

namespace netstorm::o {
// 각 선택 위치에서 현재 실제 소비자를 호출하며 누락은 성공으로 취급하지 않는다.
struct FenceRegularHooks {
    std::function<Sid(Sid,std::uint32_t)> neighbor; //00456220의 방향0/2/4/6 최근접 검색이다.
    std::function<void(Sid,Sid)> clearMarkers; //004566d0의 두 끝점 사이 기존 fencemark 삭제다.
    std::function<void(std::array<float,4>)> clearLine; //00455b20(mode0)의 현재 연결선 삭제다.
    std::function<std::uint32_t(Sid)> connections; //004573d0의 현재 활성 연결 수다. 변경 시 두 번 조회한다.
    std::function<bool()> reserveRegular; //각 new(0x28)의 독립 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; //같은 Kernel에 사건2와0.1f payload를 예약한다.
    std::function<void(Sid)> refreshNeighbors; //004578f0의 현재 이웃 스냅샷/재Pop 갱신이다.
    std::function<void(Sid)> rebuild; //00457460의 현재 동맹 이웃과 연결선/mark 재생성이다.
    RegularHandler damageable; //차단 비트20 또는 다른 사건은 실제 Damageable Handle로 넘긴다.
};
// 같은 원본 Pool/TYPE 프레임/지정기와 현재 연결 소비자를 참조하는 처리기다.
class RawFenceRegular {
public:
    RawFenceRegular(SidPool& pool,std::span<const RiftTypeFrames> frames,const SquidFrame& setter,FenceRegularHooks hooks); //PATCH188TYPE/같은 풀만 허용한다.
    bool Handles(Sid sid) const; //실제00509470 가상 표 기록값만 분배한다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const; //현재 프레임/소유자/연결수의 읽기·쓰기·호출 순서다.
    RegularHandler Handler(RegularHandler fallback={}) const; //다른 실제 부모는 기존 처리기로 전달한다.
private:
    void ClearNeighbors(Sid sid) const; //네 방향을 모두 모은 뒤 현재 owner와 기존 mark/선을 갱신한다.
    void Schedule(Sid sid) const; //개별 할당 실패는 그 예약만 생략한다.
    SidPool& pool_; //현재 frame/owner/+2a/+2e를 소유한 단일 raw 풀이다.
    std::span<const RiftTypeFrames> frames_; //원본TYPE+170의 현재 글자 구간 수다.
    const SquidFrame& setter_; //같은 풀의 실제 프레임/표시 지정기다.
    FenceRegularHooks hooks_; //선택된 하위의 실제 소비자이며 이 객체가 호출 수명을 소유한다.
};
}
