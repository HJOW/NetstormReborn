// 00446060 ↔ CD 0043da30: 커서 조각의 미완성 칸과 kind 4 두 레이어를 보존한다.
#include "client/PreviewCollection.h"
#include "client/RawCursorPlacement.h"
#include "o/RawBridgeConnect.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 독립fixture입력과실제전역DWORD는원래값을유지한채소비직전에만연결한다.
std::uint32_t PreviewSidNumber(o::Sid sid) { return sid.value; }
std::uint32_t PreviewSidNumber(std::uint32_t sid) { return sid; }
// 원본 배치 칸의 +0.9999 단정도 비트와 SP 강조 기준, 타입 깊이 우선 비트다.
constexpr float kSpotBias=std::bit_cast<float>(0x3f7ff972U),kSpThreshold=7000.0f;
constexpr std::uint32_t kSurface=4,kIsland=0x400000;
// raw little endian DWORD를 주변 바이트와 분리해 읽는다.
std::uint32_t ReadPreview(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t result=0;
    // 정렬에 의존하지 않고 네 바이트를 모은다.
    for (std::size_t i=0;i<4;++i) result|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return result;
}
}
// 작업 기간 동안 외부 저장소의 현재 값을 읽고, spot/포인터 대리 범위를 생성 때 검증한다.
PreviewCollection::PreviewCollection(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
    std::span<const std::uint8_t> spots,PreviewCollectionState& state,PreviewCollectionHooks hooks,std::uint32_t tokenBase)
    :pool_(pool),types_(types),spots_(spots),state_(state),hooks_(std::move(hooks)),tokenBase_(tokenBase) {
    if (types.empty() || spots.size()!=65536) throw std::invalid_argument("Preview collection tables");
    if (static_cast<std::uint64_t>(tokenBase)+static_cast<std::uint64_t>(pool.Capacity())*pool.Layout().stride>0x100000000ULL)
        throw std::invalid_argument("Preview collection token range");
}
// 다른 풀이나 CD 전역을 PATCH 참조 배치에 연결하지 않는다.
void PreviewCollection::BindCursor(CursorPreviewSource source) {
    if (&source.pool!=&pool_ || pool_.Edition()!=o::OriginalEdition::Patch1078)
        throw std::invalid_argument("Preview collection cursor pool/edition");
    cursor_.emplace(source);
}
// 비유한/64비트 범위 밖 입력은 CRT 불확정 정수의 하위 0을 사용한다.
std::int32_t PreviewCollection::SpotCoordinate(float coordinate) {
    const double value=static_cast<double>(coordinate)+kSpotBias;std::int64_t integer=0;
    if (std::isfinite(value) && value>=-0x1p63 && value<0x1p63) integer=static_cast<std::int64_t>(value);
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(integer));
}
// SP DWORD를 float 비트로 해석하고 ordered >7000만 강조한다. 카운터는 실제 제출 쌍마다 한 번 감소한다.
void PreviewCollection::PatchFeedback(std::uint32_t owner) const {
    const auto now=[this] { return hooks_.now ? hooks_.now() : state_.now; }; // 현재 float 시각의 DWORD 비트를 그대로 복사한다.
    const auto offset=hooks_.scrambledOffset ? hooks_.scrambledOffset() : state_.scrambledOffset;
    const auto copy=hooks_.copiedOffset ? hooks_.copiedOffset() : state_.copiedOffset;
    if (offset!=copy) state_.highlight=now();
    if (state_.countdown) {
        if (hooks_.querySp ? hooks_.querySp() : state_.querySp) {
            if (!hooks_.readSp) throw std::invalid_argument("Preview collection SP hook");
            if (std::bit_cast<float>(hooks_.readSp(owner))>kSpThreshold) state_.highlight=now();
        }
        --state_.countdown;
    }
    if (!(hooks_.firstPool ? hooks_.firstPool() : state_.firstSpPoolPresent)) state_.highlight=now();
    if ((hooks_.firstIndex ? hooks_.firstIndex() : state_.firstSpIndex)==-1) state_.highlight=now();
}
// 원본은 view 인수를 읽지 않는다. 배치 판정 실패의 부분 쓰기와 성공 쌍의 원래 깊이를 구분한다.
std::int32_t PreviewCollection::Collect(o::RenderWorldRect view,std::span<o::RenderCollectionEntry> output,std::int32_t limit) const {
    (void)view;
    // 기존 독립 상태와 실제 전역 참조는 같은 본체에서 매 접근 시 현재 값을 읽는다.
    const auto collect=[&](const auto& c) -> std::int32_t {
    if (c.lockDepth) return 0;std::int32_t count=0;
        // count와 조각 SID는 각 반복에서 현재 상태를 읽으며 실패 후보는 output 위치를 전진하지 않는다.
        for (std::int32_t i=0;i<c.pieceCount;++i) {
            if (limit<1) return count;
            if (static_cast<std::size_t>(i)>=c.pieces.size()) throw std::out_of_range("Preview collection list");
            const auto sid=CursorPreviewSid(PreviewSidNumber(c.pieces[static_cast<std::size_t>(i)]));if (!sid.value) throw std::out_of_range("Preview collection zero SID");
            const auto raw=pool_.Slot(sid);
            if (static_cast<std::size_t>(count)>=output.size()) throw std::length_error("Preview collection output");
            const float positionX=std::bit_cast<float>(ReadPreview(raw,14)),positionY=std::bit_cast<float>(ReadPreview(raw,18));
            auto& first=output[static_cast<std::size_t>(count)];first.x=positionX;first.y=positionY;
            first.token=tokenBase_+sid.value*pool_.Layout().stride;
            if (c.type>=types_.size()) throw std::out_of_range("Preview collection type");
            const auto& type=types_[c.type];std::uint32_t depth=static_cast<std::uint32_t>(type.zOrder);
            if (c.parentGump) {
                if (!hooks_.gumpDepth) throw std::invalid_argument("Preview collection parent depth");
                depth=hooks_.gumpDepth(c.parentGump)-9U;
            }
            if (type.flags2&kSurface) depth=0xffffffffU;
            if (type.flags2&kIsland) depth=static_cast<std::uint32_t>(-20);
            first.depth=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(depth));
            if (raw[10]==c.specialTypeA || raw[10]==c.specialTypeB) {
                if (!c.legal) continue;
                const auto x=static_cast<std::uint32_t>(SpotCoordinate(c.x)),y=static_cast<std::uint32_t>(SpotCoordinate(c.y));
                const auto index=x+(y<<8);
                if (index>=spots_.size()) throw std::out_of_range("Preview collection spot address");
                if (spots_[index]&2) continue;
                first.depth=20;
            }
            if (static_cast<std::size_t>(count)+2>output.size()) throw std::length_error("Preview collection pair capacity");
            if (pool_.Edition()==o::OriginalEdition::Patch1078) PatchFeedback(c.localPlayer);
            first.kind=4;first.tail=0;
            const auto current=CursorPreviewSid(PreviewSidNumber(c.pieces.at(static_cast<std::size_t>(i))));
            if (!current.value || current.value>=pool_.Capacity()) throw std::out_of_range("Preview collection current SID");
            auto& second=output[static_cast<std::size_t>(count)+1];second={positionX,positionY,tokenBase_+current.value*pool_.Layout().stride,
                std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(depth-1U)),4,2};
            count+=2;limit-=2;
        }
        return count;
    };
    if (!cursor_) return collect(state_);
    const auto& source=*cursor_;
    // 목록은 span 복사로 고정하지 않고 실제 배열 자체를 참조한다.
    struct Input {
        const std::uint32_t &lockDepth,&type,&parentGump,&specialTypeA,&specialTypeB,&legal; // 현재 잠금·타입·표시 판정이다.
        const std::int32_t& pieceCount; // 각 반복에서 현재 개수를 다시 읽는다.
        const std::array<std::uint32_t,30>& pieces; // SP 경계 재진입 뒤 바뀐 SID도 다시 읽는다.
        const float &x,&y; // 특수 받침의 배치 칸 좌표다.
        const std::uint32_t& localPlayer; // 실제 SP 읽기와 색 표의 로컬 소유자다.
    };
    return collect(Input{source.drop.lockDepth,source.lifecycle.abstractType,source.lifecycle.currentGump,
        source.surfaces.islandType,source.surfaces.stalagType,source.placement.placementAllowed,
        source.lifecycle.previewCount,source.lifecycle.previews,source.drop.x,source.drop.y,source.drop.localPlayer});
}
// 같은 풀의 유효 SID만 반환한다. 호스트 포인터로 대리 토큰을 해석하지 않는다.
o::Sid PreviewCollection::Object(std::uint32_t token) const {
    if (token<tokenBase_) throw std::out_of_range("Preview collection token");
    const auto offset=token-tokenBase_,stride=pool_.Layout().stride;
    if (!offset || offset%stride || offset/stride>=pool_.Capacity()) throw std::out_of_range("Preview collection token SID");
    return {static_cast<std::uint16_t>(offset/stride)};
}
// 부모 깊이 경계는 유지하고 SP 읽기만 복원한 실제 저장소에 연결한다.
PreviewCollectionHooks MakePreviewSpHooks(o::ScrambledSpStore& store,PreviewCollectionHooks hooks) {
    if (!store.Initialized()) throw std::invalid_argument("Preview collection prepared SP store required");
    hooks.readSp=[&store](auto owner) { return store.Get(owner); };return hooks;
}
// 미리보기는 공통 목록의 마지막 수집기로 클립 전 영역을 그대로 받는다.
void BindPreviewCollection(DrawCollectionHooks& hooks,const PreviewCollection& collector) {
    hooks.preview=[&collector](auto view,auto output,auto limit) { return collector.Collect(view,output,limit); };
}
}
