// 원본 부분 초기화/조건부 항목/현재 전역 재조회와 판본별 x87 저장 위치를 유지한다.
#include "client/MenuGumpWidgets.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 두 가상 표와 정상 생성/입력/방향/장식 비트다. 주소는 호스트에서 실행하지 않는다.
constexpr std::uint32_t kVtable=0x0050c6f0,kCdVtable=0x00501fa0,kBackground=0x03e00000;
constexpr std::uint32_t kText=0x10000,kMenu=0x20000,kHorizontal=0x40000,kFancy=0x800000;
// 객체 연결 helper의 기본 flags 비트와 Form 생성 인자다. 화면 투영 비트 0x2000과 구분한다.
constexpr std::uint32_t kObjectBinding=0x40,kObjectFormMode=2;
// 실제 세 파생 객체의 원본 할당 크기다. 호스트 클래스 sizeof와 혼용하지 않는다.
constexpr std::size_t kMenuBytes=0xb90,kDecorationBytes=0xc4,kTitleBytes=0xd8;
// 원본 _ftol의 64비트 절삭에서 하위 signed DWORD를 반환한다. 무효 정수 결과 하위값은 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 좌표 차이를 signed DWORD 감김으로 계산한다.
std::int32_t Subtract(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::uint32_t(first)-std::uint32_t(second)); }
// 두 판본의 min은 unordered에서 요청값을 선택한다. 패치만 FILD/add 요청을 float32로 먼저 저장한다.
std::int32_t Limit(double request,double maximum,bool narrow) {
    if (narrow) request=static_cast<float>(request);return Ftol(maximum<request ? maximum : request);
}
}
// 같은 메뉴/기본 저장소만 필수이며 할당/선택 통지/가상 부착은 실제 도달할 때 요구한다.
MenuGumpWidgets::MenuGumpWidgets(GumpCreateState& create,MenuGumpState& menus,MenuGumpWidgetState& state,
    const GumpBackgroundCreate& background,const GumpDecoration& decoration,const ScrollingTextGumpWidgets& title,
    const MenuGumpLayout& layout,const StyleText& style,const GumpLifetime& lifetime,const GumpCollection& collection,
    const GumpTree& tree,MenuGumpWidgetHooks hooks)
    :create_(create),menus_(menus),state_(state),background_(background),decoration_(decoration),title_(title),layout_(layout),
    style_(style),lifetime_(lifetime),collection_(collection),tree_(tree),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.base) throw std::invalid_argument("메뉴 생성 공유 노드 필요");
}
// 기존 메뉴 크기/레이아웃/표시 수명과 같은 주소여야 한다.
MenuGumpNode& MenuGumpWidgets::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("메뉴 생성 공유 노드 없음");return *node;
}
// 자식 ID/visible/깊이와 장식 크기는 실제 기본 저장소를 사용한다.
GumpCreateNode& MenuGumpWidgets::Base(std::uint32_t token) const {
    auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::runtime_error("메뉴 생성 자식/부모 노드 없음");return *node;
}
// 원본 항목 배열 밖 쓰기는 호스트 오류로 거부하고 이미 수행한 생성/통지는 되돌리지 않는다.
MenuGumpItem& MenuGumpWidgets::Item(std::uint32_t token,std::uint32_t index) const {
    if (index>=kMenuItemCapacity) throw std::out_of_range("메뉴 생성 항목 한도 초과");return Node(token).items[index];
}
// 콜백이 바꾼 index도 읽는다. 예약 0번은 시작 시만 별도로 거부하며 이후 원본 쓰기 순서대로 허용한다.
std::size_t MenuGumpWidgets::Index() const {
    if (menus_.index>=menus_.entries.size()) throw std::out_of_range("controlled 메뉴 생성 칸 범위 초과");return menus_.index;
}
// 부착할 실제 저장소를 검사하고 각 원본 자식의 가상 +20 경계로 전달한다.
void MenuGumpWidgets::AttachChild(std::uint32_t token,float x,float y,std::uint32_t parent) const {
    Base(token);if (!hooks_.attach) throw std::invalid_argument("메뉴 생성 자식 Attach 경계 필요");hooks_.attach(token,x,y,parent,0);
}
// 생성 중 문자열 항목은 Append와 달리 매번 Reflow하지 않는다. 조건 false 항목도 command/parameter를 먼저 쓴다.
void MenuGumpWidgets::Entry(std::uint32_t token,const MenuGumpConstructionEntry& entry) const {
    const auto flags=entry.flags;
    if (flags&1U) Item(token,Node(token).itemCount).command=entry.command;
    if (flags&0x80U) Item(token,Node(token).itemCount).parameter=entry.parameter;
    const bool enabled=!(flags&0x100U) || entry.condition!=0;
    if (flags&kText) {
        if (flags&kMenu) throw std::invalid_argument("메뉴 생성 문자열/자식 플래그 중복");
        if (enabled) {
            const auto* text=std::get_if<std::string>(&entry.source);if (!text) throw std::invalid_argument("메뉴 생성 실제 문자열 필요");
            const auto index=Node(token).itemCount;Item(token,index).flags=flags;const auto context=Node(token).textContext;
            layout_.SetText(token,index,text->c_str(),context);++Node(token).itemCount;
        }
        if (std::bit_cast<std::int32_t>(Node(token).itemCount)>=static_cast<std::int32_t>(kMenuItemCapacity))
            throw std::length_error("메뉴 생성 numItems < 40 계약 위반");
    } else {
        if (!(flags&kMenu)) throw std::invalid_argument("메뉴 생성 문자열 또는 자식 플래그 필요");
        if (enabled) {
            const auto* child=std::get_if<std::uint32_t>(&entry.source);if (!child) throw std::invalid_argument("메뉴 생성 실제 자식 토큰 필요");
            Base(*child).flags&=~1U;AttachChild(*child,0,0,token);
            if (std::bit_cast<std::int32_t>(Node(token).itemCount)>0) {
                Item(token,Node(token).itemCount-1).flags|=0x20U;
                Item(token,Node(token).itemCount-1).childId=Base(*child).id;
            }
        }
    }
}
// 장식과 제목은 별도 등록 객체이며 제목 자신의 버퍼/Slider/overlay는 기존 실제 생성자가 만든다.
void MenuGumpWidgets::Fancy(std::uint32_t token) const {
    if (Node(token).menuFlags&kHorizontal) throw std::invalid_argument("장식 메뉴는 가로 플래그를 사용할 수 없음");
    if (!hooks_.allocate) throw std::invalid_argument("메뉴 장식/제목 할당 경계 필요");
    const auto decoration=hooks_.allocate(kDecorationBytes);
    Node(token).fancy=decoration ? decoration_.Construct(decoration,1,1) : 0;
    AttachChild(Node(token).fancy,0,0,token);Base(Node(token).fancy).secondaryDepth=1;
    const auto title=hooks_.allocate(kTitleBytes);
    Node(token).title=title ? title_.Construct(title,0,0,0,state_.foreground) : 0;
    AttachChild(Node(token).title,16,16,Node(token).fancy);Base(Node(token).title).secondaryDepth=0xffffffffU;
    Node(token).backgroundFlags|=0x40000U;
}
// 원본이 쓰지 않은 offsets/목표 크기는 초기화하지 않으며 Reflow의 방향 분기가 실제 목표 값을 결정한다.
std::uint32_t MenuGumpWidgets::Construct(std::uint32_t token,std::span<const MenuGumpConstructionEntry> entries,std::uint32_t flags) const {
    Node(token);background_.Construct(token,0,0,kBackground,0x201,state_.backgroundType,state_.backgroundFrame);
    auto& node=Node(token);node.vtable=create_.collection.edition==o::OriginalEdition::Patch1078 ? kVtable : kCdVtable;
    node.fancy=0;node.title=0;node.selectionCallback=0;node.textContext[1]=0;
    node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.alignment));node.textContext[2]=state_.foreground;
    node.textContext[7]=0;node.textContext[5]=state_.shadow;node.textContext[6]=1;node.textContext[0]|=2U;
    ++menus_.instances;node.expanding=0;node.itemWidth=0;node.menuFlags=0;node.itemCount=0;
    node.flags|=0x20U;node.depth=0xffffffc4U;node.selection=-1;node.heights.fill(0);node.items={};
    // 호스트 입력 순서를 유지하며 조건부 항목 콜백 뒤 현재 항목 개수를 다시 읽는다.
    for (const auto& entry:entries) Entry(token,entry);
    Node(token).menuFlags=flags;if (flags&kFancy) Fancy(token);
    layout_.Reflow(token);Node(token).flags|=0x100U;return token;
}
// 스택 칸 증가는 assert/할당보다 앞이다. 생성/idle 콜백 이후의 현재 index로 칸/메뉴를 다시 읽는다.
std::uint32_t MenuGumpWidgets::Begin(std::uint32_t flags,bool idle,bool extra,std::uint32_t backgroundFlags) const {
    ++menus_.index;if (!menus_.index) throw std::out_of_range("controlled 메뉴 시작 예약 칸 금지");
    if (menus_.entries[Index()]) throw std::invalid_argument("controlled 메뉴 시작 칸이 비어 있어야 함");
    if (!hooks_.allocate) throw std::invalid_argument("controlled 메뉴 할당 경계 필요");
    const auto token=hooks_.allocate(kMenuBytes);const auto constructed=token ? Construct(token,{},flags) : 0;
    menus_.entries[Index()]=constructed;
    if (idle) lifetime_.SetIdle(menus_.entries[Index()],true);
    if (extra) Node(menus_.entries[Index()]).backgroundFlags|=backgroundFlags;
    return menus_.entries[Index()];
}
// 원본 가로 controlled 메뉴는 생성 이후 별도 idle 표에 등록한다.
std::uint32_t MenuGumpWidgets::BeginHorizontal() const { return Begin(0x140000,true,false,0); }
// 원본 장식 메뉴는 세로/장식 플래그로 생성하되 별도 idle 등록 없이 반환한다.
std::uint32_t MenuGumpWidgets::BeginFancy() const { return Begin(0xa80000,false,false,0); }
// 원본 세로 controlled 메뉴는 idle 뒤 현재 배경 플래그에 전달한 값을 추가한다.
std::uint32_t MenuGumpWidgets::BeginVertical(std::uint32_t backgroundFlags) const { return Begin(0x280000,true,true,backgroundFlags); }
// 기본 Attach 전에 전체 화면 기준으로 부모의 절대 위치와 장식의 범위를 보정한다.
void MenuGumpWidgets::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    if (!parent) parent=Node(token).parent;
    if (parent==create_.collection.root) Node(token).flags|=0x4000U;
    if (!(mode&1U) && !(Node(token).flags&0x20000U)) {
        const auto origin=collection_.Position(parent);const bool patch=create_.collection.edition==o::OriginalEdition::Patch1078;
        std::int32_t screenX=0,screenY=0;
        if (Node(token).menuFlags&kFancy) {
            auto& decoration=Base(Node(token).fancy);
            screenX=Ftol((static_cast<double>(origin.x)+decoration.x)+x);
            screenY=Ftol(patch ? (static_cast<double>(origin.y)+y)+decoration.y : (static_cast<double>(origin.y)+decoration.y)+y);
            if (screenX<0) screenX=0;if (screenY<0) screenY=0;
            screenX=Limit(screenX,static_cast<double>(create_.screenWidth)-decoration.width,patch);
            screenY=Limit(screenY,static_cast<double>(create_.screenHeight)-decoration.height,patch);
            screenX=Ftol(static_cast<double>(screenX)-decoration.x);screenY=Ftol(static_cast<double>(screenY)-decoration.y);
        } else {
            screenX=Limit(static_cast<double>(origin.x)+x,static_cast<double>(create_.screenWidth)-Node(token).width,patch);
            screenY=Limit(static_cast<double>(origin.y)+y,static_cast<double>(create_.screenHeight)-Node(token).height,patch);
        }
        x=static_cast<float>(Subtract(screenX,origin.x));y=static_cast<float>(Subtract(screenY,origin.y));
    }
    tree_.Attach(token,x,y,parent,mode);
}
// 선택 콜백 뒤 현재 전체 항목을 마지막부터 삭제한다. itemCount나 styled 플래그로 소멸 대상을 줄이지 않는다.
std::uint32_t MenuGumpWidgets::DeleteScalar(std::uint32_t token,std::uint32_t flags) const {
    Node(token).vtable=create_.collection.edition==o::OriginalEdition::Patch1078 ? kVtable : kCdVtable;--menus_.instances;
    if (Node(token).selectionCallback) {
        if (!hooks_.selectionNotice) throw std::invalid_argument("메뉴 소멸 선택 통지 경계 필요");
        hooks_.selectionNotice(Node(token).selectionCallback,Node(token).selection);
    }
    // 원본 vector destructor는 40개 전체를 역순으로 해제한다. 스타일 head는 같은 실제 소유 필드다.
    for (std::size_t index=kMenuItemCapacity;index>0;--index) style_.Clear(Item(token,static_cast<std::uint32_t>(index-1)).styled);
    return lifetime_.DeleteScalar(token,flags);
}
//4644e0의 Form 생성/소유 등록/연결 뒤 같은 기본 노드의 연결 필드만 쓴다.
void MenuGumpWidgets::Reference(std::uint32_t token,std::uint32_t object) const {
    if (object) {
        if (!hooks_.objectForm || !hooks_.formOwner || !hooks_.formObject) throw std::invalid_argument("메뉴 객체 부착 Form 경계 필요");
        const auto form=hooks_.objectForm(state_.objectFormType,kObjectFormMode);
        if (!form.token) throw std::runtime_error("메뉴 객체 부착 Form 생성 실패");
        hooks_.formOwner(form.token,Base(token).id);hooks_.formObject(form.token,object,0);
        if (create_.collection.edition==o::OriginalEdition::Patch1078) { Base(token).flags|=kObjectBinding;Base(token).object=object; }
        else { Base(token).object=object;Base(token).flags|=kObjectBinding; }
        Base(token).form=form.index;
    }
}
// 실제 정보 메뉴는 화면 부착 전에 같은 내용물 Form을 먼저 연결한다.
void MenuGumpWidgets::AttachObject(std::uint32_t token,std::uint32_t object,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    Reference(token,object);if (!hooks_.attach) throw std::invalid_argument("메뉴 객체 가상 Attach 경계 필요");hooks_.attach(token,x,y,parent,mode);
}
// fancy 정보 메뉴의 원본 별도465330 호출을 같은 등록표로 분배한다.
void MenuGumpWidgets::SetIdle(std::uint32_t token,bool enabled) const { lifetime_.SetIdle(token,enabled); }
// 기존 제목 Reflow 경계를 원본 RequestScroll/FitContent 몸체에 직접 연결한다.
void BindMenuTitleLayout(MenuGumpLayoutHooks& hooks,const ScrollingTextGump& text) {
    hooks.titleFit=[&text](std::uint32_t token,std::uint32_t position) { text.RequestScroll(token,std::bit_cast<std::int32_t>(position)); };
    hooks.titleLayout=[&text](std::uint32_t token,std::uint32_t width,std::uint32_t height) {
        text.FitContent(token,std::bit_cast<std::int32_t>(width),std::bit_cast<std::int32_t>(height));
    };
}
// 생성 이후에도 현재 메뉴 소속을 조회하여 실제 트리/수명 분배를 같은 처리기에 연결한다.
void BindMenuGumpWidgets(GumpTreeHooks& tree,GumpLifetimeHooks& lifetime,std::function<const MenuGumpWidgets*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("메뉴 생성/수명 분배 조회 필요");
    const auto previousAttach=tree.attach;const auto previousDelete=lifetime.deletingDestructor;
    tree.attach=[resolve,previousAttach](std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) {
        if (const auto* menu=resolve(token)) menu->Attach(token,x,y,parent,mode);
        else { if (!previousAttach) throw std::invalid_argument("메뉴 외 Attach 경계 필요");previousAttach(token,x,y,parent,mode); }
    };
    lifetime.deletingDestructor=[resolve,previousDelete](std::uint32_t token,std::uint32_t flags) {
        if (const auto* menu=resolve(token)) menu->DeleteScalar(token,flags);
        else { if (!previousDelete) throw std::invalid_argument("메뉴 외 소멸 경계 필요");previousDelete(token,flags); }
    };
}
// 객체 연결은 메뉴 토큰일 때만 새 몸체를 호출하며 실제 Form 공급과 가상 Attach는 동일 저장소를 사용한다.
void BindMenuGumpObjectAttach(MenuGumpHooks& hooks,std::function<const MenuGumpWidgets*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("메뉴 객체 부착 분배 조회 필요");const auto previous=hooks.attachObject;
    // 같은 controlled 메뉴의 가상 +3c 호출을 현재 분류 결과로 분배한다.
    hooks.attachObject=[resolve,previous](std::uint32_t token,std::uint32_t object,float x,float y,std::uint32_t parent,std::uint32_t mode) {
        if (const auto* menu=resolve(token)) menu->AttachObject(token,object,x,y,parent,mode);
        else { if (!previous) throw std::invalid_argument("메뉴 외 객체 부착 경계 필요");previous(token,object,x,y,parent,mode); }
    };
}
}
