// 콜백 뒤 현재 선택/플래그/링크를 다시 읽고 숨김과 지연 삭제의 원본 순서를 유지한다.
#include "client/MenuGumpInput.h"
#include "client/Sound.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client {
namespace {
// 메뉴 항목의 명령/유지/강조/자식/명시 인자/비활성/동적 자식 비트다.
constexpr std::uint32_t kCommand=1,kKeep=2,kSelected=8,kChild=0x20,kParameter=0x80,kDisabled=0x200,kDynamic=0x800;
// 메뉴 방향/루트 닫기/명령 뒤 삭제/compact/외부 누름 삭제와 기본 표시/팝업/dead 비트다.
constexpr std::uint32_t kHorizontal=0x40000,kVertical=0x80000,kRootMenu=0x100000,kDeleteCommand=0x200000;
constexpr std::uint32_t kCompact=0x1000000,kDeleteOutside=0x4000000,kVisible=1,kPopup=0x4000,kDead=0x10000;
// 마우스 사건의 종류 마스크·왼쪽/오른쪽 누름·release·선택 해제 비트다.
constexpr std::uint32_t kEventMask=0x03ffffff,kLeft=0x10000,kRight=0x20000,kRelease=0x80000000,kOutside=0x4000000;
// CRT _ftol의 64비트 절삭에서 signed 하위 DWORD만 취한다. 무효 정수의 하위 DWORD는 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 화면 끝점 합산의 signed DWORD 감김을 정의된 unsigned 덧셈으로 표현한다.
std::int32_t Add(std::int32_t first,std::int32_t second) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(first)+static_cast<std::uint32_t>(second));
}
// 한 재귀 경로의 순환만 거부한다. 유효 원본 트리에서는 추가 동작을 하지 않는다.
void Enter(std::vector<std::uint32_t>& active,std::uint32_t token) {
    if (std::find(active.begin(),active.end(),token)!=active.end()) throw std::runtime_error("메뉴 재귀 링크 순환");
    active.push_back(token);
}
}
// 같은 기본/파생 저장소는 필수이고 실제 가상 호출 경계는 도달 시에만 요구한다.
MenuGumpInput::MenuGumpInput(GumpCreateState& create,MenuGumpInputState& state,const MenuGump& menu,
    const GumpCollection& collection,const GumpEvents& events,const GumpLifetime& lifetime,
    const GumpTree& tree,MenuGumpInputHooks hooks)
    :create_(create),state_(state),menu_(menu),collection_(collection),events_(events),lifetime_(lifetime),tree_(tree),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.base) throw std::invalid_argument("메뉴 입력 공유 저장소 필요");
}
// 기본 조회는 메뉴 이외의 부모/팝업도 허용한다.
GumpCreateNode& MenuGumpInput::Base(std::uint32_t token) const {
    auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::runtime_error("메뉴 입력 기본 노드 없음");return *node;
}
// 같은 객체의 현재 선택/항목 상태를 반환한다.
MenuGumpNode& MenuGumpInput::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=&Base(token)) throw std::runtime_error("메뉴 입력 공유 노드 없음");return *node;
}
// 원본은 itemCount와 별개로 현재 선택의 40칸 주소에 접근한다. 사용하지 않는 유효 칸은 허용한다.
MenuGumpItem& MenuGumpInput::Item(std::uint32_t token,std::int32_t index) const {
    if (index<0 || static_cast<std::size_t>(index)>=kMenuItemCapacity) throw std::out_of_range("메뉴 입력 항목 범위 오류");
    return Node(token).items[static_cast<std::size_t>(index)];
}
// 항목의 childId는 Gump 토큰이 아니라 현재 등록표의 ID다.
std::uint32_t MenuGumpInput::Child(std::uint32_t id) const {
    if (id>=create_.entries.size() || !create_.entries[id]) throw std::runtime_error("메뉴 자식 등록 ID 없음");return create_.entries[id];
}
// CD는 float 비트 <=80000000으로 -0을 허용한다. 양수 NaN은 ordered 상한에서 제외한다.
bool MenuGumpInput::Contains(std::uint32_t token,std::array<float,2> local) const {
    const bool patch=create_.collection.edition==o::OriginalEdition::Patch1078;
    const bool x=patch ? local[0]>=0.0F : std::bit_cast<std::uint32_t>(local[0])<=0x80000000U;
    const bool y=patch ? local[1]>=0.0F : std::bit_cast<std::uint32_t>(local[1])<=0x80000000U;
    return x && local[0]<Base(token).width && y && local[1]<Base(token).height;
}
// 메뉴 아닌 자식은 하위 탐색도 생략한다. 표시 여부는 자신의 경계 검사에만 적용한다.
bool MenuGumpInput::Covered(std::uint32_t token,ScreenPoint point,std::vector<std::uint32_t>& active) const {
    Enter(active,token);std::unordered_set<std::uint32_t> siblings;
    // 원본은 재귀 뒤 현재 next를 읽는다. ScreenToLocal의 투영 콜백이 바꾼 링크도 반영한다.
    for (auto child=Base(token).child;child;child=Base(child).next) {
        if (!siblings.insert(child).second) throw std::runtime_error("메뉴 가림 형제 순환");
        if (Base(child).command!=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.alignment))) continue;
        const auto local=collection_.ScreenToLocal(child,point);
        if (((Base(child).flags&kVisible) && Contains(child,local)) || Covered(child,point,active)) { active.pop_back();return true; }
    }
    active.pop_back();return false;
}
// 가상 Redraw는 픽셀 출력과 별개이며 원본 인자는 항상 false다.
void MenuGumpInput::Redraw(std::uint32_t token) const {
    if (!hooks_.redraw) throw std::invalid_argument("메뉴 입력 Redraw 경계 필요");hooks_.redraw(token,false);
}
// 명시 인자 비트가 없으면 원본처럼 flags 전체 DWORD를 인자로 전달한다.
std::uint32_t MenuGumpInput::Parameter(std::uint32_t token,std::int32_t index,std::uint32_t flags) const {
    return flags&kParameter ? Item(token,index).parameter : flags;
}
// 현재 음향 경계가 없으면 해당 원본 동작을 생략하지 않고 도달 시 거부한다.
void MenuGumpInput::Sound(std::string_view name) const {
    if (!hooks_.sound) throw std::invalid_argument("메뉴 음향 경계 필요");hooks_.sound(name);
}
// 숨김 자식의 동적 삭제도 수행한다. redraw와 Kill 뒤 항목의 현재 flags를 다시 읽는다.
void MenuGumpInput::CloseChildren(std::uint32_t token,bool clearSelection,std::vector<std::uint32_t>& active) const {
    Enter(active,token);
    // signed 항목 수가 0 이하이면 생략한다. 매 반복 끝에 콜백이 바꾼 현재 개수를 반영한다.
    for (std::int32_t index=0;index<std::bit_cast<std::int32_t>(Node(token).itemCount);++index) {
        auto& item=Item(token,index);if (clearSelection) item.flags&=~kSelected;
        if (!(item.flags&kChild)) continue;
        const auto child=Child(item.childId);
        if (Base(child).flags&kVisible) {
            CloseChildren(child,clearSelection,active);Redraw(child);Base(child).flags&=~kVisible;
        }
        if (item.flags&kDynamic) { lifetime_.Kill(child);item.childId=0;item.flags&=~kChild; }
    }
    active.pop_back();
}
// 순환 검사 상태를 호출 단위로 새로 만들며 정상 재귀에서 선택 인덱스는 보존한다.
void MenuGumpInput::Close(std::uint32_t token,bool clearSelection) const {
    std::vector<std::uint32_t> active;CloseChildren(token,clearSelection,active);
}
// 루트 next는 Kill 전에 저장한다. 삭제 요청이 형제 링크를 바꾸어도 같은 방문 순서를 유지한다.
void MenuGumpInput::ClosePopups() const {
    std::unordered_set<std::uint32_t> siblings;
    // 등록표 0번이 실제 루트이며 collection override를 사용하지 않는다.
    for (auto token=Base(create_.entries[0]).child;token;) {
        if (!siblings.insert(token).second) throw std::runtime_error("메뉴 팝업 형제 순환");
        auto& node=Base(token);const auto next=node.next;
        if ((node.flags&kPopup) && !(node.flags&kDead)) lifetime_.Kill(token);token=next;
    }
}
// 높이/폭은 이미 배율 적용된 값이다. offset/행 높이에 배율을 다시 곱하지 않는다.
void MenuGumpInput::RedrawItem(std::uint32_t token,std::int32_t index) const {
    if (!(Base(token).flags&kVisible)) return;
    std::unordered_set<std::uint32_t> parents;
    // 현재 모든 부모의 visible만 확인하며 dead/void는 추가로 제외하지 않는다.
    for (auto parent=Base(token).parent;parent;parent=Base(parent).parent) {
        if (!parents.insert(parent).second) throw std::runtime_error("메뉴 행 부모 순환");if (!(Base(parent).flags&kVisible)) return;
    }
    auto point=collection_.Position(token);std::int32_t height{};
    if (Node(token).menuFlags&kCompact) height=Ftol(Node(token).height);
    else {
        Item(token,index);const auto offset=static_cast<std::size_t>(index);
        point.y=Ftol(static_cast<double>(point.y)+Node(token).offsets[offset]);height=Ftol(Node(token).heights[offset]);
    }
    const auto width=Ftol(Node(token).width);if (width<=0 || height<=0) return;
    if (!hooks_.invalidate) throw std::invalid_argument("메뉴 행 변경 영역 경계 필요");
    hooks_.invalidate({point.x,point.y,Add(point.x,width),Add(point.y,height)});
}
// 선택 변경은 old/new 행 요청 사이에 현재 선택/항목을 다시 읽는다.
void MenuGumpInput::Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    const ScreenPoint point{std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)};
    const auto local=collection_.ScreenToLocal(token,point);std::int32_t selected=-1;std::vector<std::uint32_t> active;
    if (!(event&kOutside) && Contains(token,local) && !Covered(token,point,active)) {
        if (!(Node(token).menuFlags&kVertical)) selected=Ftol(static_cast<double>(local[0])/Node(token).itemWidth);
        else {
            selected=0;
            if (!(Node(token).menuFlags&kCompact)) {
                // 원본은 마지막 항목까지 찾지 못하면 마지막 인덱스를 그대로 선택한다.
                while (selected<Add(std::bit_cast<std::int32_t>(Node(token).itemCount),-1)) {
                    Item(token,selected);const auto index=static_cast<std::size_t>(selected);
                    if (local[1]>=Node(token).offsets[index] && static_cast<double>(local[1])<static_cast<double>(Node(token).heights[index])+Node(token).offsets[index]) break;
                    ++selected;
                }
            }
        }
    }
    const auto old=Node(token).selection;if (selected==old) return;
    if (old>=0) {
        if (Node(token).menuFlags&kVertical) RedrawItem(token,old);
        Item(token,Node(token).selection).flags&=~kSelected;Node(token).selection=-1;
    }
    if (selected>=0 && !(Item(token,selected).flags&kDisabled)) {
        if (Node(token).menuFlags&kVertical) RedrawItem(token,selected);
        Node(token).selection=selected;Item(token,selected).flags|=kSelected;
    }
    if (!(Node(token).menuFlags&kVertical)) Redraw(token);
}
// 명령 큐·소리·Close가 바꾼 현재 선택은 후속 단계마다 다시 조회한다.
std::uint32_t MenuGumpInput::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    const auto kind=event&kEventMask;
    if (Node(token).expanding || (kind!=kLeft && kind!=kRight) || (event&kRelease)) return 0;
    if (!hooks_.broadcast) throw std::invalid_argument("메뉴 가상 통지 경계 필요");hooks_.broadcast(token,event,first,second);
    const auto selected=Node(token).selection;
    if (selected<0) { if (Node(token).menuFlags&kDeleteOutside) lifetime_.Kill(token);return 1; }
    auto flags=Item(token,selected).flags;
    if (flags&kCommand) {
        const auto parameter=Parameter(token,selected,flags);const auto command=Item(token,selected).command,id=Base(token).id;
        if (!hooks_.command) throw std::invalid_argument("메뉴 명령 큐 경계 필요");hooks_.command(command,id,parameter);Sound("openSubGump.wav");
    }
    Close(token,(flags&(kChild|kKeep))==0);flags=Item(token,Node(token).selection).flags;
    if ((flags&kDynamic) && !(flags&kChild)) {
        const auto parameter=Parameter(token,Node(token).selection,flags),serial=Base(token).serial,eventCode=state_.requestEvent;
        if (!hooks_.handle) throw std::invalid_argument("메뉴 동적 자식 가상 입력 경계 필요");
        const auto child=hooks_.handle(token,eventCode,serial,parameter);
        if (child) {
            Item(token,Node(token).selection).childId=Base(child).id;Base(child).flags&=~kVisible;
            if (!hooks_.attach) throw std::invalid_argument("메뉴 동적 자식 Attach 경계 필요");hooks_.attach(child,0,0,token,0);
            Item(token,Node(token).selection).flags|=kChild;flags=Item(token,Node(token).selection).flags;
        }
    }
    if (flags&kChild) {
        const auto child=Child(Item(token,Node(token).selection).childId);
        if (!hooks_.push) throw std::invalid_argument("메뉴 자식 Push 경계 필요");hooks_.push(child);Base(child).flags|=kVisible;
        const auto position=menu_.ItemPosition(token,Node(token).selection);
        if (!hooks_.attach) throw std::invalid_argument("메뉴 자식 Attach 경계 필요");hooks_.attach(child,position[0],position[1],0,0);
        tree_.AddDepth(child,0xffffffffU);menu_.StartExpand(child,(Node(token).menuFlags&kHorizontal) ? kVertical : kHorizontal);
        Sound("openGump.wav");Redraw(token);
    } else if (!(flags&kKeep)) {
        std::unordered_set<std::uint32_t> parents;auto current=token;
        // 메뉴 부모를 따라가며 루트 메뉴 경계에서 해당 가지의 자식들을 닫는다.
        while (current) {
            if (!parents.insert(current).second) throw std::runtime_error("메뉴 닫기 부모 순환");
            const auto parent=Base(current).parent;
            if (Base(parent).command!=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.alignment)) || (Node(current).menuFlags&kRootMenu)) { Close(current,true);break; }
            current=parent;
        }
    }
    if (Node(token).menuFlags&kDeleteCommand) {
        if (!(flags&kCommand)) return 1;
        lifetime_.Kill(token);const auto serial=Base(token).serial;events_.Bubble(token,state_.closedEvent,serial,serial);
    }
    if (flags&kCommand) ClosePopups();return 1;
}
// 두 입력 계층의 기존 반환 DWORD를 보존하며 현재 메뉴에만 실제 Handle을 사용한다.
void BindMenuGumpInput(GumpEventHooks& events,GumpInputHooks& input,GumpFrameHooks& frame,
    std::function<const MenuGumpInput*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("메뉴 입력 분배 조회 필요");
    // 기존 이벤트/캡처 처리기가 서로 다를 수 있으므로 각각 감싼다.
    const auto bind=[resolve](auto previous) {
        // 동적 하위 메뉴 요청도 같은 가상 경계를 통해 상위 파생 처리기에 전달할 수 있다.
        return [resolve,previous](std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second)->std::uint32_t {
            if (const auto* menu=resolve(token)) return menu->Handle(token,event,first,second);
            if (!previous) throw std::invalid_argument("메뉴 외 입력 경계 필요");return previous(token,event,first,second);
        };
    };
    events.handle=bind(events.handle);input.handle=bind(input.handle);const auto previous=frame.broadcast;
    // 등록표/visible 필터는 GumpFrame에 맡기고 실제 메뉴 통지만 교체한다.
    frame.broadcast=[resolve,previous](std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) {
        if (const auto* menu=resolve(token)) menu->Broadcast(token,event,first,second);
        else { if (!previous) throw std::invalid_argument("메뉴 외 통지 경계 필요");previous(token,event,first,second); }
    };
}
// 음향 파일 이름만 메뉴가 결정하며 채널/거리/우선순위는 원본처럼 모두 0이다.
void BindMenuGumpInputSound(MenuGumpInputHooks& hooks,SoundPlayer& sound) {
    hooks.sound=[&sound](std::string_view name) { sound.PlayByName(name,0,0,0,0,0); };
}
}
