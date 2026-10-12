// 메뉴의 선택 통지·하위 메뉴 열기/닫기·명령과 부모 통지를 같은 트리/수명 저장소에 연결한다.
#pragma once
#include "client/MenuGump.h"
#include "client/GumpEvents.h"
#include "client/GumpInput.h"
#include "client/GumpTree.h"
#include <string_view>

namespace netstorm::client {
// +20 command의 signed 메뉴 식별 바이트와 동적 하위 메뉴 요청/닫힘 사건 전역이다. 실제 초기화 값을 공급한다.
struct MenuGumpInputState { std::int8_t alignment{26};std::uint32_t requestEvent{},closedEvent{}; };
// 가상 통지/입력/Push/Attach/Redraw와 명령 큐·이름별 소리·변경 영역의 경계다.
// 콜백은 현재 선택/항목/등록표를 바꿀 수 있다. node/base는 같은 안정된 객체를 공급해야 한다.
struct MenuGumpInputHooks {
    std::function<MenuGumpNode*(std::uint32_t)> node;
    std::function<GumpCreateNode*(std::uint32_t)> base;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> broadcast;
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> handle;
    std::function<void(std::uint32_t)> push;
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
    std::function<void(std::uint32_t,bool)> redraw;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> command;
    std::function<void(std::string_view)> sound;
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
    GumpCreateState& create_;
    MenuGumpInputState& state_;
    const MenuGump& menu_;
    const GumpCollection& collection_;
    const GumpEvents& events_;
    const GumpLifetime& lifetime_;
    const GumpTree& tree_;
    MenuGumpInputHooks hooks_;
};
// 이벤트 선택/캡처/프레임 통지에서 현재 메뉴만 분배하고 기존 다른 파생 처리기를 보존한다.
void BindMenuGumpInput(GumpEventHooks& events,GumpInputHooks& input,GumpFrameHooks& frame,
    std::function<const MenuGumpInput*(std::uint32_t)> resolve);
class SoundPlayer;
// openSubGump.wav/openGump.wav를 기존 실제 SoundPlayer의 원본 인자 모두 0으로 재생한다.
void BindMenuGumpInputSound(MenuGumpInputHooks& hooks,SoundPlayer& sound);
}
