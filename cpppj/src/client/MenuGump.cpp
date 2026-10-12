// x87 중간값의 float32 저장 위치와 콜백 이후 현재 필드 재조회를 보존한다.
#include "client/MenuGump.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 가로/장식/가로 확장 방향 비트와 장식 여백 상수다.
constexpr std::uint32_t kHorizontal=0x40000,kFancy=0x800000,kExpandWidth=0x80000;
constexpr double kFancyWidth=32.0,kFancyHeight=48.0,kFancyTop=32.0;
// 원본 _ftol의 64비트 절삭 결과에서 signed 하위 DWORD를 취한다. 무효 정수 결과의 하위 DWORD는 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
// 파생 메뉴 조회와 기본 저장소 조회는 필수이며 가상 콜백은 실제 도달 시 검사한다.
MenuGump::MenuGump(GumpCreateState& create,MenuGumpState& state,const GumpLifetime& lifetime,MenuGumpHooks hooks)
    :create_(create),state_(state),lifetime_(lifetime),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.baseNode) throw std::invalid_argument("메뉴/기본 Gump 저장소 필요");
}
// 표시/수명/파생 메뉴가 복사본이 아닌 같은 기본 노드를 공유하는지 확인한다.
MenuGumpNode& MenuGump::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.baseNode(token)) throw std::runtime_error("메뉴 공유 저장소 없음");
    return *node;
}
// fancy/title은 메뉴 클래스가 아닌 기본 Gump 파생 저장소도 허용한다.
GumpCreateNode& MenuGump::BaseNode(std::uint32_t token) const {
    auto* node=token ? hooks_.baseNode(token) : nullptr;
    if (!node) throw std::runtime_error("메뉴 장식/제목 저장소 없음");return *node;
}
// 장식 token/title/가상 Attach는 Push와 크기 변경을 마친 시점의 현재 값이다.
void MenuGump::AttachFancy(std::uint32_t token) const {
    const auto& menu=Node(token);
    const auto y=static_cast<float>(Ftol(-static_cast<double>(BaseNode(menu.title).baseHeight)-kFancyTop));
    if (!hooks_.attach) throw std::invalid_argument("메뉴 장식 Attach 경계 필요");
    hooks_.attach(menu.fancy,-16.0F,y,token,0);
}
// fancy의 기본 폭 저장 뒤에도 x87 중간 합계를 유지해 배율 폭을 계산한다.
void MenuGump::SetWidth(std::uint32_t token,float width) const {
    auto& menu=Node(token);menu.baseWidth=width;menu.width=static_cast<float>(static_cast<double>(width)*menu.scale);
    if (!(menu.menuFlags&kFancy)) return;
    if (!hooks_.push) throw std::invalid_argument("메뉴 장식 Push 경계 필요");hooks_.push(menu.fancy);
    const double size=static_cast<double>(menu.baseWidth)+kFancyWidth;
    auto& fancy=BaseNode(menu.fancy);fancy.baseWidth=static_cast<float>(size);fancy.width=static_cast<float>(size*fancy.scale);
    AttachFancy(token);
}
// 제목 높이+현재 메뉴 높이+48의 중간 합계를 float32로 먼저 좁히지 않는다.
void MenuGump::SetHeight(std::uint32_t token,float height) const {
    auto& menu=Node(token);menu.baseHeight=height;menu.height=static_cast<float>(static_cast<double>(height)*menu.scale);
    if (!(menu.menuFlags&kFancy)) return;
    if (!hooks_.push) throw std::invalid_argument("메뉴 장식 Push 경계 필요");hooks_.push(menu.fancy);
    const double size=static_cast<double>(BaseNode(menu.title).baseHeight)+menu.baseHeight+kFancyHeight;
    auto& fancy=BaseNode(menu.fancy);fancy.baseHeight=static_cast<float>(size);fancy.height=static_cast<float>(size*fancy.scale);
    AttachFancy(token);
}
// 장식의 유무는 menuFlags와 무관하다. 장식 콜백 후 기본 요청도 항상 호출한다.
void MenuGump::Redraw(std::uint32_t token,bool erase) const {
    const auto fancy=Node(token).fancy;
    if (fancy) { if (!hooks_.redraw) throw std::invalid_argument("메뉴 장식 Redraw 경계 필요");hooks_.redraw(fancy,erase); }
    if (!hooks_.baseRedraw) throw std::invalid_argument("메뉴 기본 Redraw 경계 필요");hooks_.baseRedraw(token,erase);
}
// 원본의 세로 offset 배열 범위를 벗어나는 호스트 입력은 실제 조회 직전에 거부한다.
std::array<float,2> MenuGump::ItemPosition(std::uint32_t token,std::int32_t index) const {
    const auto& menu=Node(token);
    if (menu.menuFlags&kHorizontal) return {static_cast<float>(static_cast<double>(index)*menu.itemWidth),menu.height};
    if (index<0 || static_cast<std::size_t>(index)>=menu.offsets.size()) throw std::out_of_range("메뉴 항목 offset 범위 오류");
    return {menu.width,menu.offsets[static_cast<std::size_t>(index)]};
}
// SetWidth의 콜백 이후 목표 높이를 다시 읽는다. idle 등록 이후 expanding을 쓴다.
void MenuGump::StartExpand(std::uint32_t token,std::uint32_t flags) const {
    auto& menu=Node(token);
    if (flags&kExpandWidth) { SetWidth(token,menu.targetWidth);SetHeight(token,0); }
    else { SetWidth(token,0);SetHeight(token,menu.targetHeight); }
    lifetime_.SetIdle(token,true);menu.expanding=1;
}
// 최초 비교는 x87 unordered도 확장 분기로 들어간다. Push 뒤 CD는 unordered도 크기를 지정한다.
void MenuGump::Idle(std::uint32_t token) const {
    auto& menu=Node(token);if (!menu.expanding) return;
    if (menu.height>=menu.targetHeight && menu.width>=menu.targetWidth) { menu.expanding=0;return; }
    if (!hooks_.push || !hooks_.pop) throw std::invalid_argument("메뉴 확장 Push/Pop 경계 필요");
    hooks_.push(token);
    const bool patch=create_.collection.edition==o::OriginalEdition::Patch1078;
    if (patch ? menu.height<menu.targetHeight : !(menu.height>=menu.targetHeight)) SetHeight(token,menu.targetHeight);
    if (patch ? menu.width<menu.targetWidth : !(menu.width>=menu.targetWidth)) SetWidth(token,menu.targetWidth);
    hooks_.pop(token);
}
// DWORD index의 signed 음수도 배열 한도를 넘으므로 접근 전에 거부한다.
std::size_t MenuGump::Index() const {
    if (state_.index>=state_.entries.size()) throw std::out_of_range("현재 메뉴 index 범위 오류");return state_.index;
}
// 메뉴가 없으면 0을 반환한다. 원본처럼 항목 수 조회와 달리 null 자체는 허용한다.
std::uint32_t MenuGump::Current() const { return state_.entries[Index()]; }
// 유효 현재 메뉴의 DWORD 항목 수를 그대로 반환한다.
std::uint32_t MenuGump::ItemCount() const { return Node(Current()).itemCount; }
// 패치는 칸 clear→index 감소, CD는 index 감소→이전 칸 clear 순서다.
void MenuGump::DropCurrent() const {
    const auto index=Index();if (!index) throw std::out_of_range("메뉴 예약 칸 꺼내기 오류");
    if (create_.collection.edition==o::OriginalEdition::Patch1078) { state_.entries[index]=0;--state_.index; }
    else { --state_.index;state_.entries[state_.index+1]=0; }
}
// 반환 토큰은 clear 전에 읽고 이후 index/칸 변경을 수행한다.
std::uint32_t MenuGump::TakeCurrent() const { const auto token=Current();DropCurrent();return token; }
// float32로 변환한 두 정수 좌표를 전달한다. 콜백 뒤 변경된 index의 칸을 지운다.
void MenuGump::AttachCurrent(std::int32_t x,std::int32_t y,std::uint32_t parent) const {
    const auto token=Current();Node(token);
    if (!hooks_.attach) throw std::invalid_argument("현재 메뉴 Attach 경계 필요");
    hooks_.attach(token,static_cast<float>(x),static_cast<float>(y),parent,0);DropCurrent();
}
// 실제 객체 SID를 가상 +3c에 넘긴다. 단순 화면 상대 이동으로 바꾸지 않는다.
void MenuGump::AttachObjectCurrent(std::uint32_t object,std::int32_t x,std::int32_t y) const {
    const auto token=Current();Node(token);
    if (!hooks_.attachObject) throw std::invalid_argument("현재 메뉴 객체 Attach 경계 필요");
    hooks_.attachObject(token,object,static_cast<float>(x),static_cast<float>(y),create_.entries[0],0);DropCurrent();
}
// 호출 시 현재 분배기를 조회하여 순환 의존 처리기 생성 이후에도 연결할 수 있다.
void BindMenuGumpFrame(GumpFrameHooks& hooks,std::function<const MenuGump*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("메뉴 프레임 분배 조회 필요");
    const auto idle=hooks.idle;const auto redraw=hooks.redraw;
    hooks.idle=[resolve,idle](std::uint32_t token) {
        if (const auto* menu=resolve(token)) menu->Idle(token);
        else { if (!idle) throw std::invalid_argument("메뉴 외 idle 경계 필요");idle(token); }
    };
    hooks.redraw=[resolve,redraw](std::uint32_t token,bool erase) {
        if (const auto* menu=resolve(token)) menu->Redraw(token,erase);
        else { if (!redraw) throw std::invalid_argument("메뉴 외 Redraw 경계 필요");redraw(token,erase); }
    };
}
}
