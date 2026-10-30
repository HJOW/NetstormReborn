// 실제 내용물 SID를 사용하는 생산 격자와 항목의 원본 생성·배치·통지를 복원한다.
#pragma once
#include "client/GumpCreate.h"
#include "o/RawContainedFinder.h"
#include <functional>

namespace netstorm::client {
// 506800의 +a0~c0 필드다. 기본 생성/표시/수명 저장소를 그대로 상속한다.
struct InventoryGridNode: GumpCreateNode {
    std::int32_t minimumColumns{},minimumRows{}; // +a0/+a4: 자동 배치의 최소값 또는 고정 열/행 수다.
    float cellWidth{},cellHeight{}; // +a8/+ac: 실제 부착에서 계산한 배율 적용 칸 크기다.
    std::uint32_t contentMask{},acceptedCursor{},requireImageHit{},fixed{}; // +b0/+b4/+b8/+bc: 목록·커서·클릭·고정 배치 조건이다.
    float itemScale{}; // +c0: 비0이면 각 항목의 아이콘 배율에 그대로 전달한다.
};
// 5068f4의 +a0~bc다. +a4/+b2는 생성자가 쓰지 않는 원문을 보존한다.
struct InventoryItemNode: GumpCreateNode {
    std::uint32_t content{},reservedA4{}; // +a0의 실제 내용물 SID와 미작성 DWORD다.
    double blinkUntil{}; // +a8: 점멸 단계의 다음 실제 시각이다.
    std::uint8_t blink{},decoration{}; // +b0/+b1: 점멸 단계와 항목 Draw 장식 조건이다.
    std::uint16_t reservedB2{}; // +b2의 미작성 WORD다.
    std::uint32_t requireImageHit{},additional{}; // +b4/+b8: 실제 그림 hit 요구와 원본 초기0 필드다.
    float iconScale{}; // +bc:0이면 원본 경계에 맞추고 비0이면 지정 배율을 쓴다.
};
// 커서 허용 질의는 사건 인자가 아닌 현재 Client 전역을 읽는다.
struct InventoryGumpState {
    const ScreenPoint& cursorPoint; // 55a2f8/fc: 현재 화면 좌표다.
    const std::uint32_t& cursorFlags; // 55a2ac: 현재 내용물 커서의 전체 플래그다.
};
// 공통 트리/등록표의 실제 가상 소비자를 조립한다. 아이콘 자식 부착은 별도 원본 몸체다.
struct InventoryGumpHooks {
    std::function<InventoryGridNode*(std::uint32_t)> grid; // 안정된 격자 파생 저장소다.
    std::function<InventoryItemNode*(std::uint32_t)> item; // 안정된 항목 파생 저장소다.
    std::function<void(std::uint32_t)> clearChildren; // 464c60의 실제 자식 Kill이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> baseAttach; // 같은 GumpTree::Attach다.
    std::function<std::uint32_t(std::size_t)> allocateItem; // 0xc0 요청을 같은 등록표의 저장소에 할당한다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 현재 노드의 가상+20이다.
    std::function<void(std::uint32_t)> push; // 현재 노드의 가상+18이다.
    std::function<void(std::uint32_t,std::uint32_t)> bind; // 4644e0의 실제 부모 Form 연결이다.
    std::function<void(std::uint32_t,bool)> idle; // 같은 GumpLifetime::SetIdle이다.
    std::function<double()> now; // 460d70의 실제 시각을 초로 공급한다.
    std::function<std::uint32_t(std::uint32_t)> contentParent; // 내용물의 가상+0 부모 SID다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> forward; // 현재 부모 체인의 464e90다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseHandle; // 기본 464990 처리다.
    std::function<std::array<float,2>(std::uint32_t,std::uint32_t,ScreenPoint)> local; // 4650a0의 현재 사건 좌표 변환이다.
    std::function<bool(std::uint32_t,std::array<float,2>)> inside; // 4248e0의 로컬 경계 포함 여부다.
    std::function<void()> cancelCursor; // unmodified 왼쪽 버튼의 실제 446e70다.
    std::function<void(std::string_view)> assertion; // 원본 진단 뒤에도 정상 몸체를 계속 실행한다.
    std::function<void(std::uint32_t,bool)> redraw; // 항목의 현재 가상+8이며 숨길 때도 bit1을 지우기 전에 호출한다.
    std::function<void(std::uint32_t)> clearBlink; // 종료 Redraw 뒤 현재 content+15의bit2만 해제한다.
};
// 내용물 조회는 같은 SidPool/검색기, 생성은 기존 GumpCreate를 사용한다. 별도 표를 만들지 않는다.
class InventoryGump {
public:
    // 모든 노드/현재 전역/공통 생성기는 이 처리기와 호출보다 오래 살아야 한다.
    InventoryGump(const GumpCreateState& create,const GumpCreate& factory,const o::SidPool& pool,
        const o::ContainedFinderState& finder,InventoryGumpState state,InventoryGumpHooks hooks);
    // 43d790: 실제 기본 생성 뒤 격자 필드를 쓴다. 0열은 원본처럼 진단 뒤 계속한다.
    std::uint32_t ConstructGrid(std::uint32_t token,std::int32_t columns,std::int32_t rows,std::uint32_t mask) const;
    // 43e120: 실제 내용물 SID/크기와 flags200의 점멸 시작을 반영한다.
    std::uint32_t ConstructItem(std::uint32_t token,float width,float height,std::uint32_t content) const;
    // 43d690: 단계4→실제 idle 등록→현재 시각+0.1을 보존한다.
    void BeginBlink(std::uint32_t token) const;
    void IdleItem(std::uint32_t token) const; //43d6e0: 현재 두 시각/표시 전환 순서와 종료 시 Idle·내용물 정리를 유지한다.
    // 43e1e0: 자식 제거→기본 부착→두 번의 현재 목록 순회→실제 항목 생성·가상 부착이다.
    void AttachGrid(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
    // 43d850: 내용물 통지/커서 허용/목록mask/부모 전달/왼쪽 취소/기본 처리를 실행한다.
    std::uint32_t HandleGrid(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 43d9c0: 현재 부모 삭제 통지의 진단→Push→연결 해제→가상 재부착이다.
    void ParentGone(std::uint32_t token,std::uint32_t parent) const;
private:
    InventoryGridNode& Grid(std::uint32_t token) const; // 없는 파생 저장소는 그 호출 위치에서 거부한다.
    InventoryItemNode& Item(std::uint32_t token) const; // 기본 생성과 같은 주소를 공급한다.
    o::Sid Content(std::uint32_t sid) const; // 실제 DWORD의 물리 범위를 검사한 뒤에만 SID로 바꾼다.
    const GumpCreate& factory_; // 같은 실제 Gump 등록표/기본 생성자다.
    const o::SidPool& pool_; // 실제 내용물 flags와 slot BYTE의 유일한 저장소다.
    const o::ContainedFinderState& finder_; // 같은 현재 순회 정책이다.
    InventoryGumpState state_; // 현재 커서 전역의 안정 참조다.
    InventoryGumpHooks hooks_; // 실제 선택된 외부 효과만 호출한다.
};
}
