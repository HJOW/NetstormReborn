// 메뉴의 정상 생성·장식/제목 자식·controlled 시작·가상 부착·역순 항목 소멸을 기존 처리기에 연결한다.
#pragma once
#include "client/MenuGumpLayout.h"
#include "client/GumpDecoration.h"
#include "client/GumpTree.h"
#include "client/ScrollingTextGumpWidgets.h"
#include <string>
#include <variant>

namespace netstorm::client {
// 원본 varargs의 문자열/자식 포인터와 조건부 DWORD 입력을 호스트 값으로 표현한다.
// flags의 1/80/100이 command/parameter/condition을 소비하며 조건 false에서도 명령/인자 쓰기는 수행한다.
struct MenuGumpConstructionEntry {
    std::variant<std::string,std::uint32_t> source;
    std::uint32_t flags{},command{},parameter{},condition{1};
};
// signed 정렬은 두 PE 초기값 26이다. 배경 타입/프레임·흰색/그림자 색은 실제 초기화 전역을 공급한다.
struct MenuGumpWidgetState { std::int8_t alignment{26};std::uint32_t backgroundType{},backgroundFrame{},foreground{},shadow{}; };
// 같은 메뉴/기본 객체·요청 크기별 호스트 할당·실제 가상 Attach와 소멸 선택 콜백의 경계다.
struct MenuGumpWidgetHooks {
    std::function<MenuGumpNode*(std::uint32_t)> node;
    std::function<GumpCreateNode*(std::uint32_t)> base;
    std::function<std::uint32_t(std::size_t)> allocate;
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
    std::function<void(std::uint32_t,std::int32_t)> selectionNotice;
};
// 원본 MenuGump 생성/수명과 화면 밖 부착 보정을 복원한다. 실제 메뉴 Draw/입력과 전체 Form 객체는 후속이다.
class MenuGumpWidgets {
public:
    // 모든 상태/기존 처리기와 저장소는 생성/부착/수명 분배보다 오래 살아야 한다.
    MenuGumpWidgets(GumpCreateState& create,MenuGumpState& menus,MenuGumpWidgetState& state,
        const GumpBackgroundCreate& background,const GumpDecoration& decoration,const ScrollingTextGumpWidgets& title,
        const MenuGumpLayout& layout,const StyleText& style,const GumpLifetime& lifetime,const GumpCollection& collection,
        const GumpTree& tree,MenuGumpWidgetHooks hooks);
    // 004775f0/CD 00451a70: 기본 배경/부분 초기화→조건부 항목·이전 항목의 자식 연결→최종 메뉴 flags/장식/제목→Reflow/notify다.
    // native varargs의 마지막 null 뒤 flags는 별도 인자다. 미작성 offsets/목표 크기·문맥의 다른 DWORD는 보존한다.
    std::uint32_t Construct(std::uint32_t token,std::span<const MenuGumpConstructionEntry> entries,std::uint32_t flags) const;
    // 00477a10/CD 00450e00: controlled 칸 증가→new(0xb90)/가로 0x140000→실제 idle 등록이다.
    std::uint32_t BeginHorizontal() const;
    // 00477ae0/CD 00450ed0: controlled 칸 증가→new(0xb90)/장식 0xa80000다. idle은 등록하지 않는다.
    std::uint32_t BeginFancy() const;
    // 00477bb0/CD 00450f90: 세로 0x280000 생성/idle 후 현재 메뉴의 배경 flags에 추가 DWORD를 OR한다.
    std::uint32_t BeginVertical(std::uint32_t backgroundFlags=0) const;
    // 00476be0/CD 004530d0: root 부착 비트·parent 0 재사용·일반/장식 전체 화면 보정→기존 기본 Attach다.
    // mode 1 또는 기본 0x20000이면 보정을 생략한다. 패치의 float32 중간 저장과 CD의 미저장을 유지한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
    // 00476ec0/00476f40/CD 00451650/004533a0: 파생 표/개수 감소→선택 통지→40개 조각 역순 Clear→기본 scalar다.
    // 자식 장식/제목 삭제는 기존 GumpLifetime::Destroy의 자식 우선 순서를 따른다.
    std::uint32_t DeleteScalar(std::uint32_t token,std::uint32_t flags) const;
private:
    // 동일 기본/파생 저장소와 실제 사용 항목/스택 칸을 조회한다.
    MenuGumpNode& Node(std::uint32_t token) const;
    GumpCreateNode& Base(std::uint32_t token) const;
    MenuGumpItem& Item(std::uint32_t token,std::uint32_t index) const;
    std::size_t Index() const;
    // 생성자 입력 하나를 처리한다. 명령/인자는 조건 검사보다 먼저 쓰고 자식은 앞 항목에 연결한다.
    void Entry(std::uint32_t token,const MenuGumpConstructionEntry& entry) const;
    // new(0xc4 장식)/new(0xd8 제목)의 실제 생성과 현재 자식 Attach/secondaryDepth를 수행한다.
    void Fancy(std::uint32_t token) const;
    // 세 controlled 시작 함수의 공통 칸/할당/생성/idle/추가 flags 순서다. 콜백 뒤 현재 index를 다시 읽는다.
    std::uint32_t Begin(std::uint32_t flags,bool idle,bool extra,std::uint32_t backgroundFlags) const;
    // 현재 가상 Attach 경계는 장식/제목/입력 자식이 도달할 때만 요구한다.
    void AttachChild(std::uint32_t token,float x,float y,std::uint32_t parent) const;
    GumpCreateState& create_;
    MenuGumpState& menus_;
    MenuGumpWidgetState& state_;
    const GumpBackgroundCreate& background_;
    const GumpDecoration& decoration_;
    const ScrollingTextGumpWidgets& title_;
    const MenuGumpLayout& layout_;
    const StyleText& style_;
    const GumpLifetime& lifetime_;
    const GumpCollection& collection_;
    const GumpTree& tree_;
    MenuGumpWidgetHooks hooks_;
};
// 메뉴 제목 Reflow의 기존 titleFit은 실제 RequestScroll, titleLayout은 실제 FitContent에 연결한다.
void BindMenuTitleLayout(MenuGumpLayoutHooks& hooks,const ScrollingTextGump& text);
// 메뉴 파생 Attach/scalar만 분배하며 기존 트리/장식/텍스트/버튼 처리를 보존한다.
void BindMenuGumpWidgets(GumpTreeHooks& tree,GumpLifetimeHooks& lifetime,
    std::function<const MenuGumpWidgets*(std::uint32_t)> resolve);
}
