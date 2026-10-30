// 디컴파일 C에 생략된 ceil/ftol과 단정도 중간 저장을 실제 명령 순서로 유지한다.
#include "client/InventoryGump.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kGridVtable=0x506800,kItemVtable=0x5068f4; // 10.78 실제 가상 표다.
constexpr std::uint32_t kContentNotice=0x02000031,kCursorQuery=0x0200003f,kMaskQuery=0x0200005f,kForward=0x0200005e; // 실제 초기 사건 DWORD다.
constexpr std::size_t kItemSize=0xc0; // 43e1e0의 실제 항목 할당 크기다.
// 도달한 실제 외부 효과만 필수로 검사한다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("생산 격자 실제 하위 미연결");return callback; }
// 원문의 비정렬 flags WORD를 읽는다.
std::uint32_t Flags(std::span<const std::uint8_t> raw) { return raw[20]|(std::uint32_t(raw[21])<<8); }
// CRT ftol의 signed64 절삭 뒤 하위 DWORD다. 비유한/범위 밖은 불확정 정수의 하위0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 다른 판본을 10.78 주소/필드로 잘못 조립하지 않도록 생성 시 검사한다.
InventoryGump::InventoryGump(const GumpCreateState& create,const GumpCreate& factory,const o::SidPool& pool,
    const o::ContainedFinderState& finder,InventoryGumpState state,InventoryGumpHooks hooks)
    :factory_(factory),pool_(pool),finder_(finder),state_(state),hooks_(std::move(hooks)) {
    if (create.collection.edition!=o::OriginalEdition::Patch1078 || pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("생산 격자 PATCH 자료 필요");
}
// 가상 분배가 공급하는 파생 노드의 안정된 주소를 읽는다.
InventoryGridNode& InventoryGump::Grid(std::uint32_t token) const {
    auto* node=token ? Need(hooks_.grid)(token) : nullptr;if (!node) throw std::out_of_range("생산 격자 저장소 없음");return *node;
}
// 실제 항목 생성 전후에 같은 저장소를 다시 읽는다.
InventoryItemNode& InventoryGump::Item(std::uint32_t token) const {
    auto* node=token ? Need(hooks_.item)(token) : nullptr;if (!node) throw std::out_of_range("생산 항목 저장소 없음");return *node;
}
// 원본의 포인터 산술에 앞서 SID의 실제 범위만 확인한다.
o::Sid InventoryGump::Content(std::uint32_t sid) const {
    if (!sid || sid>=pool_.Capacity() || sid>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("생산 격자 내용물 SID 범위 오류");return {static_cast<std::uint16_t>(sid)};
}
// +a8/+ac는 이 생성자가 쓰지 않는다. 기본 생성자도 파생 필드를 초기화하지 않는다.
std::uint32_t InventoryGump::ConstructGrid(std::uint32_t token,std::int32_t columns,std::int32_t rows,std::uint32_t mask) const {
    auto& node=Grid(token);factory_.Construct(token,0,0,0,1);node.vtable=kGridVtable;
    if (!columns) Need(hooks_.assertion)("_numSlotsX != 0");
    node.command=12;node.minimumColumns=columns;node.flags|=0xa00;node.acceptedCursor=0;node.contentMask=mask;node.fixed=0;node.itemScale=0;node.minimumRows=rows;node.requireImageHit=1;return token;
}
// flags200만 점멸 시작을 선택하며 마지막에 클릭 요구1을 쓴다.
std::uint32_t InventoryGump::ConstructItem(std::uint32_t token,float width,float height,std::uint32_t content) const {
    auto& node=Item(token);factory_.Construct(token,0,width,height,1);node.vtable=kItemVtable;node.blinkUntil=0;node.flags|=0x1a00;
    node.content=content;node.command=8;node.blink=0;node.decoration=0;node.additional=0;node.iconScale=0;
    if (Flags(pool_.Slot(Content(content)))&0x200U) BeginBlink(token);
    node.requireImageHit=1;return token;
}
// idle 소비자의 부수 효과 뒤에 현재 시각을 읽는다.
void InventoryGump::BeginBlink(std::uint32_t token) const {
    auto& node=Item(token);node.blink=4;Need(hooks_.idle)(token,true);node.blinkUntil=Need(hooks_.now)()+0.1;
}
// 표시 상태를 바꾸는 앞/뒤의 실제 가상 갱신 순서와 별도 두 번의 시각 조회를 보존한다.
void InventoryGump::IdleItem(std::uint32_t token) const {
    auto& node=Item(token);
    if (!node.blink) {
        Need(hooks_.idle)(token,false);node.flags|=1U;Need(hooks_.redraw)(token,false);Need(hooks_.clearBlink)(node.content);return;
    }
    const auto now=Need(hooks_.now)(); // 원문은 시각 조회가 반환한 뒤 현재 기한을 읽는다.
    if (node.blinkUntil<=now) {
        --node.blink;node.blinkUntil=Need(hooks_.now)()+0.1;
        if (node.flags&1U) { Need(hooks_.redraw)(token,false);node.flags&=~1U; }
        else { node.flags|=1U;Need(hooks_.redraw)(token,false); }
    }
}
// 원본은 기본 부착 뒤 현재 부모 SID/크기/내용물 목록을 읽는다.
void InventoryGump::AttachGrid(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    auto& node=Grid(token);Need(hooks_.clearChildren)(token);Need(hooks_.baseAttach)(token,x,y,parent,mode);
    o::RawContainedFinder finder(pool_,finder_);std::uint32_t maximum{};
    // 첫 순회는 mask에 맞는 slot BYTE+1의 최댓값이다. 빈 목록/불일치만 있으면0이다.
    for (auto sid=finder.Begin(node.object ? Content(node.object) : o::Sid{});sid.value;sid=finder.Next()) {
        const auto raw=pool_.Slot(sid);if (Flags(raw)&node.contentMask) maximum=std::max(maximum,std::uint32_t(raw[23])+1U);
    }
    auto columns=node.minimumColumns,rows=node.minimumRows;
    if (!node.fixed) {
        // 원본은 순회 전 스택 칸을0으로 지우며 부착 mode를 배치 계산에 사용하지 않는다.
        const auto count=static_cast<float>(maximum);
        const float root=std::sqrt(count);columns=std::max(1,Ftol(std::ceil((static_cast<double>(node.baseWidth)/node.baseHeight)*root)));
        rows=std::max(1,Ftol(static_cast<double>(count)/columns));
        if (std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(rows)*static_cast<std::uint32_t>(columns))<static_cast<std::int32_t>(maximum)) ++rows;
        if (columns==1 && rows<node.minimumRows) rows=node.minimumRows;
        if (rows==1 && columns<node.minimumColumns) columns=node.minimumColumns;
    }
    if (!columns || !rows) throw std::domain_error("생산 격자 원본 0열/행 나눗셈 경로");
    node.cellWidth=static_cast<float>(static_cast<double>(node.width)/columns);node.cellHeight=static_cast<float>(static_cast<double>(node.height)/rows);
    // 두 번째 순회는 현재 목록과 현재 mask를 다시 읽어 실제 내용물 SID로 항목을 만든다.
    for (auto sid=finder.Begin(node.object ? Content(node.object) : o::Sid{});sid.value;sid=finder.Next()) {
        const auto raw=pool_.Slot(sid);if (!(Flags(raw)&node.contentMask)) continue;const auto slot=static_cast<std::int32_t>(raw[23]);
        const auto child=Need(hooks_.allocateItem)(kItemSize);
        if (!child) throw std::bad_alloc(); // 원본은 이 뒤 null을 역참조하므로 성공으로 바꾸지 않는다.
        ConstructItem(child,node.cellWidth,node.cellHeight,sid.value);auto& item=Item(child);
        if (node.itemScale!=0) item.iconScale=node.itemScale;item.requireImageHit=node.requireImageHit;
        Need(hooks_.attach)(child,static_cast<float>((slot%columns)*static_cast<double>(node.cellWidth)),static_cast<float>((slot/columns)*static_cast<double>(node.cellHeight)),token,0);
    }
}
// 내용물 알림의 재부착은 원본처럼 부모 전달의 반환값을 무시하고 기본 처리까지 계속한다.
std::uint32_t InventoryGump::HandleGrid(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    auto& node=Grid(token);const auto code=event&0x03ffffffU;
    if (code==kContentNotice) {
        const auto content=Content(first);const auto parent=Need(hooks_.contentParent)(first);
        if (parent==node.object && (node.contentMask&Flags(pool_.Slot(content)))) {
            static_cast<void>(Need(hooks_.forward)(token,event,first,second));Need(hooks_.push)(token);Need(hooks_.attach)(token,node.x,node.y,0,0);
        }
    }
    if (code==kCursorQuery) {
        const auto point=Need(hooks_.local)(token,code,state_.cursorPoint);
        if (Need(hooks_.inside)(token,point) && (node.acceptedCursor&state_.cursorFlags)) return 1;
    }
    if (code==kMaskQuery) return node.contentMask;
    if (code==kForward) return Need(hooks_.forward)(token,event,first,second);
    if (!(event&0x80000000U) && code==0x10000U) { Need(hooks_.cancelCursor)();return 1; }
    return Need(hooks_.baseHandle)(token,event,first,second);
}
// Push/연결 해제의 부수 효과 뒤 현재 x/y를 읽어 다시 부착한다.
void InventoryGump::ParentGone(std::uint32_t token,std::uint32_t parent) const {
    auto& node=Grid(token);if (parent!=node.object) Need(hooks_.assertion)("sid == parentSid");
    Need(hooks_.push)(token);Need(hooks_.bind)(token,0);Need(hooks_.attach)(token,node.x,node.y,0,0);
}
}
