// 원본 항목은 부착 뒤 자식을 다시 만들고 실제 내용물 flags·품질·수량을 현재 시점에 읽는다.
#include "client/InventoryItemGump.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kChoose=0x02000008,kRight=0x02000018,kRepeat=0x02000107; // 실제 원본 사건 DWORD다.
constexpr std::size_t kImageBytes=0xb4,kTextBytes=0xa4; // 실제 자식 할당 크기다.
// 도달한 실제 소비자만 필수로 요구한다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("생산 항목 실제 하위 미연결");return value; }
// 내용물의 비정렬 WORD만 현재 시점에 읽는다.
std::uint32_t Word(std::span<const std::uint8_t> raw,std::size_t offset) { return raw[offset]|(std::uint32_t(raw[offset+1])<<8); }
// 원본 CRT64비트 절삭의 하위 signedDWORD다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 장식 좌표의 정수 덧셈은 DWORD로 감긴다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
}
// 같은 원본10.78 내용물 저장소를 사용한다.
InventoryItemGump::InventoryItemGump(const o::SidPool& pool,InventoryItemState state,InventoryItemHooks hooks):pool_(pool),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("생산 항목 PATCH 자료 필요");
}
// 파생 노드의 동일 주소를 현재 조회한다.
InventoryItemNode& InventoryItemGump::Node(std::uint32_t token) const {
    auto* node=token ? Need(hooks_.node)(token) : nullptr;if (!node) throw std::out_of_range("생산 항목 저장소 없음");return *node;
}
// 자식 가상 부착 전 실제 공통 필드를 쓴다.
GumpCreateNode& InventoryItemGump::Base(std::uint32_t token) const {
    auto* node=token ? Need(hooks_.base)(token) : nullptr;if (!node) throw std::out_of_range("생산 항목 자식 저장소 없음");return *node;
}
// 원본 포인터 산술 전에 실제 물리 범위를 확인한다.
o::Sid InventoryItemGump::Content(std::uint32_t sid) const {
    if (!sid || sid>=pool_.Capacity() || sid>65535U) throw std::out_of_range("생산 항목 실제 내용물SID 오류");return {static_cast<std::uint16_t>(sid)};
}
// 기존 비용표 복호 값은 원본처럼 먼저float으로 저장한다. 에디터여도 SP 조회를 생략하지 않는다.
std::uint32_t InventoryItemGump::Affordable(o::Sid content) const {
    const auto kind=Word(pool_.Slot(Content(content.value)),18);const float cost=static_cast<float>(Need(hooks_.costUnits)(kind));const float money=Need(hooks_.money)(state_.localOwner);
    return state_.editor || cost<=money ? 1U : 0U;
}
// 원본의 네 dead 진단과 삭제 큐 정리는 아이콘 계산 전에 실행한다.
void InventoryItemGump::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    auto& node=Node(token);
    // 각 효과 이후의 현재 플래그를 다시 확인한다.
    const auto alive=[&] { if (node.flags&0x10000U) Need(hooks_.assertion)("!(flags & GF_DEAD)"); };
    alive();Need(hooks_.baseAttach)(token,x,y,parent,mode);alive();Need(hooks_.clearChildren)(token);alive();Need(hooks_.cleanup)();alive();
    if (!Need(hooks_.robust)(node.content,true)) Need(hooks_.assertion)("isSidRobust( contentSid )");
    const auto sid=Content(node.content);const auto raw=pool_.Slot(sid);const auto kind=Word(raw,18);auto argument=Word(raw,12);const auto info=Need(hooks_.type)(kind);const bool explicitFrame=info.gumpFrame!=-1;
    if (explicitFrame) argument=static_cast<std::uint32_t>(Need(hooks_.type)(kind).gumpFrame);
    if (!kind) return;
    const auto shape=Need(hooks_.measure)(kind,argument,explicitFrame);const auto& bounds=shape.bounds;float scale=1;
    if (node.iconScale==0) {
        const float rx=static_cast<float>((static_cast<double>(bounds.right)-bounds.left+2.0)/node.width),ry=static_cast<float>((static_cast<double>(bounds.bottom)-bounds.top+2.0)/node.height);
        if (rx>1 || ry>1) { const auto maximum=rx<=ry ? ry : rx;scale=static_cast<float>(1.0/static_cast<double>(maximum)); }
    } else scale=node.iconScale;
    const float centerX=static_cast<float>((static_cast<double>(node.width)-(static_cast<double>(bounds.right)-bounds.left)*scale)*0.5+2.0);
    const float centerY=static_cast<float>((static_cast<double>(node.height)-(static_cast<double>(bounds.bottom)-bounds.top)*scale)*0.5+2.0);
    Need(hooks_.walk)({kind,std::bit_cast<std::int32_t>(argument),0,0,0,explicitFrame},[&](InventoryItemCell cell) {
        const float px=static_cast<float>(static_cast<double>(centerX)+static_cast<double>(cell.x)*state_.pixelsX*scale),py=static_cast<float>(static_cast<double>(centerY)+static_cast<double>(cell.y)*state_.pixelsY*scale);auto frame=cell.frame;
        if (Word(raw,18)==state_.bridgeType) {
            const auto bridge=Need(hooks_.type)(state_.bridgeType);if (!bridge.frames || frame<0 || static_cast<std::size_t>(frame)>=bridge.frames->Codes().size()) throw std::out_of_range("생산 항목 현재 다리 프레임 코드 오류");
            const auto code=bridge.frames->Codes()[static_cast<std::size_t>(frame)];const auto quality=raw[30];const auto flags=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(code.flags)))|(quality==1 ? 0x20U : quality==3 ? 0x40U : 0U));frame=bridge.frames->FindFlags(code.side,code.variant,flags);
        }
        const auto flags=Word(raw,20);const auto imageMode=(flags&0x100U) ? 1U : (flags&0x418U) && !Affordable(sid) ? 2U : 0U;
        const auto image=Need(hooks_.allocate)(kImageBytes);if (!image) throw std::bad_alloc();Need(hooks_.image)(image,kind,frame,imageMode);
        auto& child=Base(image);child.scale=scale;child.width=static_cast<float>(static_cast<double>(scale)*child.baseWidth);child.height=static_cast<float>(static_cast<double>(scale)*child.baseHeight);Need(hooks_.attach)(image,px,py,token,0);
        if (state_.stockEnabled && (Word(raw,20)&8U)) {
            const auto text=Need(hooks_.allocate)(kTextBytes);if (!text) throw std::bad_alloc();Need(hooks_.text)(text);Need(hooks_.styledText)(text,"~w~E"+std::to_string(raw[30]));Need(hooks_.fitText)(text);
            const float tx=static_cast<float>(static_cast<double>(px)+child.width),ty=static_cast<float>(static_cast<double>(py)+child.height-10.0);Need(hooks_.attach)(text,tx,ty,token,0);
        }
    });
}
// 장식은 현재 화면점+3에서 시작하며 끝점은 기본 크기+화면점을 CRT 절삭한다.
void InventoryItemGump::Draw(std::uint32_t token) const {
    const auto& node=Node(token);if (!node.decoration) return;const auto point=Need(hooks_.position)(token);
    Need(hooks_.background)({Add(point.x,3),Add(point.y,3),Ftol(static_cast<double>(point.x)+node.baseWidth),Ftol(static_cast<double>(point.y)+node.baseHeight)},0x100000,state_.backgroundType,state_.backgroundFrame);
}
// 사건을 로컬 좌표로 먼저 바꾸고 현재 지식/입력 관문을 읽는다.
std::uint32_t InventoryItemGump::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    auto& node=Node(token);const auto point=Need(hooks_.local)(token,event,first,second);if (Need(hooks_.knowledgeOpen)() || state_.inputDisabled) return 0;const auto code=event&0x03ffffffU;
    if (code==kRepeat) { if (state_.lastType==Word(pool_.Slot(Content(node.content)),18)) return Need(hooks_.forward)(token,kChoose,node.content,node.serial);return 0; }
    if ((code!=0x10000U && code!=0x20000U) || (event&0x80000000U)) return 0;
    std::unordered_set<std::uint32_t> visited;
    // 현재 자식 목록에서 실제 이미지 hit를 요구하며 실패 뒤에는 현재 next를 읽는다.
    for (auto child=node.child;child;child=Base(child).next) {
        if (!visited.insert(child).second) throw std::logic_error("생산 항목 자식 순환");const auto& current=Base(child);
        if (node.requireImageHit && (current.command!=3 || !Need(hooks_.imageHit)(child,point[0],point[1]))) continue;
        if (code==0x10000U) {
            if (!Need(hooks_.denied)(1,0x41)) { state_.lastType=Word(pool_.Slot(Content(node.content)),18);return Need(hooks_.forward)(token,kChoose,node.content,node.serial); }
            Need(hooks_.tell)("NoBridgeYet");
        } else {
            if (!Need(hooks_.forward)(token,kRight,node.content,node.serial) && !Need(hooks_.knowledgeOpen)()) return Need(hooks_.about)(token,first,second);
        }
        return 1;
    }
    return 0;
}
// 같은 실제 자산의 gumpFrame/코드·전체 패턴 측정/반복자만 연결한다.
void BindInventoryItemAssets(InventoryItemHooks& hooks,const GameAssets& assets,const PriestPlacementAssets& geometry,const o::RawCanonPixelShape& measure) {
    if (assets.Edition()!=o::OriginalEdition::Patch1078 || measure.Pool().Edition()!=assets.Edition()) throw std::invalid_argument("생산 항목 실제 자산 판본 오류");
    hooks.type=[&assets,&geometry](std::uint32_t type) {
        if (type>=geometry.frames.size()) throw std::out_of_range("생산 항목 실제TYPE 프레임 범위 오류");
        const auto frame=type<o::kFirstAssetTypeNumber ? 0 : assets.Types()[type-o::kFirstAssetTypeNumber].definition.specialFrames.gumpFrame;return InventoryItemType{frame,&geometry.frames[type].frames};
    };
    hooks.measure=[&measure](auto type,auto argument,bool explicitFrame) { return measure.Measure(type,argument,0,explicitFrame); };
    hooks.walk=[&geometry,edition=assets.Edition()](o::CanonTypeQuery query,const std::function<void(InventoryItemCell)>& visitor) {
        if (query.type>=geometry.frames.size()) throw std::out_of_range("생산 항목 실제decoder 타입 오류");const auto& meta=geometry.frames[query.type];auto decoder=o::DecodeCanonType(meta.frames,meta.defaultFrame,geometry.geometry.patternTypes,query,edition);
        // 실제 전체 패턴의 현재 칸을 전달한 뒤 Advance한다.
        while (decoder.Valid()) { visitor({decoder.Frame(),decoder.X(),decoder.Y()});decoder.Advance(); }
    };
}
}
