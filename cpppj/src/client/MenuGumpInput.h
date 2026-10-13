// 메뉴의 선택 통지·하위 메뉴 열기/닫기·명령과 부모 통지를 같은 트리/수명 저장소에 연결한다.
#pragma once
#include "client/MenuGump.h"
#include "client/GumpEvents.h"
#include "client/GumpInput.h"
#include "client/GumpTree.h"
#include <string_view>

namespace netstorm::client {
// +20 command의 signed 메뉴 식별 바이트와 동적 하위 메뉴 요청/닫힘 사건 전역이다. 실제 초기화 값을 공급한다.
struct MenuGumpInputState { /* alignment: 항목의 좌우·상하 정렬을 선택하는 값. */ std::int8_t alignment{26};/* requestEvent: 메뉴 동작을 요청하는 이벤트 ID. */ /* closedEvent: 창이 닫혔음을 알리는 이벤트 ID. */ std::uint32_t requestEvent{},closedEvent{}; };
// 가상 통지/입력/Push/Attach/Redraw와 명령 큐·이름별 소리·변경 영역의 경계다.
// 콜백은 현재 선택/항목/등록표를 바꿀 수 있다. node/base는 같은 안정된 객체를 공급해야 한다.
struct MenuGumpInputHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<MenuGumpNode*(std::uint32_t)> node;
    // base: 파생 처리에서 공통 동작을 위임하는 기본 객체를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> base;
    // broadcast: 대상의 이벤트를 관련 노드에 일괄 전달하는 콜백.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> broadcast;
    // handle: 대상의 이벤트 처리기를 호출하는 콜백.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> handle;
    // push: 현재 대상을 처리 목록에 넣는 콜백.
    std::function<void(std::uint32_t)> push;
    // attach: 대상을 부모 또는 처리 목록에 부착하는 콜백.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
    // redraw: 대상의 화면 갱신을 요청하는 콜백.
    std::function<void(std::uint32_t,bool)> redraw;
    // command: 메뉴 항목의 명령을 부모 처리기에 전달하는 콜백.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> command;
    // sound: 해당 동작의 소리를 재생하는 콜백.
    std::function<void(std::string_view)> sound;
    // invalidate: 변경한 영역의 재그리기를 요청하는 콜백.
    std::function<void(ScreenRect)> invalidate;
};
// 복원 메뉴의 실제 가상 입력 몸체다. OS 입력 큐와 메뉴 객체를 만드는 상위 Client는 별도 연결 대상이다.
class MenuGumpInput {
public:
    // 생성/사건/수명/트리/메뉴 상태와 모든 저장소는 처리기 및 분배 훅보다 오래 살아야 한다.
    MenuGumpInput(GumpCreateState& create,MenuGumpInputState& state,const MenuGump& menu,
        const GumpCollection& collection,const GumpEvents& events,const GumpLifetime& lifetime,
        const GumpTree& tree,MenuGumpInputHooks hooks);
    // 004772d0/CD 00452f00: 로컬 좌표/자식 메뉴 가림→행 선택→기존/새 강조와 행 또는 전체 Redraw다.
    void Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 00476820/CD 00452b50: 확장 중/비누름 제외→가상 통지→명령/닫기→동적 자식/열기→부모 통지다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 00476670/CD 00452920: 표시 중 자식을 재귀 닫고 숨긴다. 동적 자식은 숨김 여부와 무관하게 Kill한다.
    // clearSelection은 강조 비트만 지우며 선택 인덱스 자체를 -1로 바꾸지 않는다.
    void Close(std::uint32_t token,bool clearSelection) const;
    // 00466710/CD 00492c20: 실제 등록표 루트의 sticky 팝업 중 비 dead인 직계 자식에 Kill을 요청한다.
    void ClosePopups() const;
    // 00476e00/CD 004532c0: visible 부모 확인 뒤 compact 전체/일반 행의 양수 변경 영역을 요청한다.
    void RedrawItem(std::uint32_t token,std::int32_t index) const;
private:
    // 실제 도달한 기본/메뉴/40칸 항목/등록 ID만 확인한다. 원본 배열 밖 접근은 호스트 예외다.
    GumpCreateNode& Base(std::uint32_t token) const;
    MenuGumpNode& Node(std::uint32_t token) const;
    MenuGumpItem& Item(std::uint32_t token,std::int32_t index) const;
    std::uint32_t Child(std::uint32_t id) const;
    // CD의 unsigned float 비트 하한과 패치의 ordered 하한, 두 판본의 strict 상한을 유지한다.
    bool Contains(std::uint32_t token,std::array<float,2> local) const;
    // 메뉴 식별인 자식만 재귀 검사한다. 숨긴 자식의 하위 표시 메뉴도 검사하며 현재 next를 다시 읽는다.
    bool Covered(std::uint32_t token,ScreenPoint point,std::vector<std::uint32_t>& active) const;
    // 원본 호출 순서대로 현재 항목 수/자식 flags를 읽는 재귀 닫기다. 순환 링크만 거부한다.
    void CloseChildren(std::uint32_t token,bool clearSelection,std::vector<std::uint32_t>& active) const;
    // 가상 재그리기 요청과 전체 DWORD 항목 인자 선택, 원본 음향 이름 재생을 수행한다.
    void Redraw(std::uint32_t token) const;
    std::uint32_t Parameter(std::uint32_t token,std::int32_t index,std::uint32_t flags) const;
    void Sound(std::string_view name) const;
    // create_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& create_;
    // state_: +20 command의 signed 메뉴 식별 바이트와 동적 하위 메뉴 요청/닫힘 사건 전역이다. 실제 초기화 값을 공급한다 (MenuGumpInputState).
    MenuGumpInputState& state_;
    // menu_: 원본 메뉴의 정상 프레임/스택 몸체다. 레이아웃/생성/출력/입력은 같은 저장소의 별도 메뉴 처리기가 담당한다 (MenuGump).
    const MenuGump& menu_;
    // collection_: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (GumpCollection).
    const GumpCollection& collection_;
    // events_: 발생한 이벤트 또는 이벤트 처리기.
    const GumpEvents& events_;
    // lifetime_: 등록된 객체의 삭제·해제를 처리하는 수명 관리자.
    const GumpLifetime& lifetime_;
    // tree_: 00466750/CD 00493070의 머리 삽입과 원본의 후속 정렬/현재 자식 재배치를 담당한다 (GumpTree).
    const GumpTree& tree_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (MenuGumpInputHooks).
    MenuGumpInputHooks hooks_;
};
// 이벤트 선택/캡처/프레임 통지에서 현재 메뉴만 분배하고 기존 다른 파생 처리기를 보존한다.
void BindMenuGumpInput(GumpEventHooks& events,GumpInputHooks& input,GumpFrameHooks& frame,
    std::function<const MenuGumpInput*(std::uint32_t)> resolve);
class SoundPlayer;
// openSubGump.wav/openGump.wav를 기존 실제 SoundPlayer의 원본 인자 모두 0으로 재생한다.
void BindMenuGumpInputSound(MenuGumpInputHooks& hooks,SoundPlayer& sound);
}
