// 같은 생성 저장소에서 원본 Gump 부착·밀어내기·정렬·깊이·이동 순서를 복원한다.
#pragma once
#include "client/GumpCreate.h"

namespace netstorm::client {
// 원본 지도 화면 영역과 패치판의 밀어내기 링크 진단 전역이다.
struct GumpTreeState {
    // mapView: 지도 출력과 연결된 Gump 토큰.
    ScreenRect mapView{};
    std::uint32_t pushDebugMode{}; // 3/4는 원본 패치판의 의도적인 링크 변경 분기다.
};
// 가상 +8/+0x18/+0x20과 삭제 요청/월드 투영을 실제 상위 처리기에 전달하는 경계다.
// 각 콜백은 같은 저장소/현재 전역을 바꿀 수 있다. 처리기 몸체와 객체 삭제는 별도 복원 범위다.
struct GumpTreeHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> node;
    // redraw: 대상의 화면 갱신을 요청하는 콜백.
    std::function<void(std::uint32_t,bool)> redraw;
    // push: 현재 대상을 처리 목록에 넣는 콜백.
    std::function<void(std::uint32_t)> push;
    // attach: 대상을 부모 또는 처리 목록에 부착하는 콜백.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
    // kill: 대상을 UI 삭제 대기 목록에 넣는 콜백.
    std::function<void(std::uint32_t)> kill;
    // project: 로컬 좌표를 출력 좌표로 투영하는 콜백.
    std::function<ScreenPoint(std::uint32_t)> project;
};
// 00466750/CD 00493070의 머리 삽입과 원본의 후속 정렬/현재 자식 재배치를 담당한다.
class GumpTree {
public:
    // 생성 상태/트리 전역/안정된 노드 저장소는 트리 처리기보다 오래 살아야 한다.
    GumpTree(GumpCreateState& state,GumpTreeState& tree,GumpTreeHooks hooks);
    // parent=0이면 기존 부모를 재사용한다. mode bit 1은 화면 제한 생략, bit 2는 정렬 적용이다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
    // 형제/부모에서 분리해 void 비트를 세우고 가상 redraw(true)를 호출한다. 자신의 링크는 남긴다.
    void Push(std::uint32_t token) const;
    // 현재 부모 기본 크기/화면 크기로 크기 종속·가운데/끝/지도 중심을 순서대로 계산한다.
    // 계산된 위치만 반환하며 자신의 x/y는 직접 바꾸지 않는다. 종속 크기 필드는 갱신한다.
    std::array<float,2> Align(std::uint32_t token) const;
    // 모든 자식을 먼저 처리한 뒤 자신의 DWORD 깊이를 더하거나 지정한다. visible/dead 필터는 없다.
    void AddDepth(std::uint32_t token,std::uint32_t delta) const;
    void SetDepth(std::uint32_t token,std::uint32_t depth) const;
    // 드래그 기준점 보정→현재 void 판정의 가상 Push→가상 Attach(x,y,parent,0) 순서다.
    void Move(std::uint32_t token,float x,float y,std::uint32_t parent) const;
    // 현재 부모와 reference가 다르면 기준 위치를 더하고 anchor bit 8/4로 폭/높이를 더한다.
    void MoveRelative(std::uint32_t token,std::uint32_t reference,float x,float y,std::uint8_t anchor,std::uint32_t parent) const;
private:
    // 유효 토큰의 같은 생성/표시 저장소를 조회한다. 없는 노드는 예외로 거부한다.
    GumpCreateNode& Node(std::uint32_t token) const;
    // 한 경로의 링크 순환을 거부하며 원본 자식 우선 재귀 쓰기 순서를 유지한다.
    void Depth(std::uint32_t token,std::uint32_t value,bool add,std::vector<std::uint32_t>& active) const;
    // state_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& state_;
    // tree_: 원본 지도 화면 영역과 패치판의 밀어내기 링크 진단 전역이다 (GumpTreeState).
    GumpTreeState& tree_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (GumpTreeHooks).
    GumpTreeHooks hooks_;
};
}
