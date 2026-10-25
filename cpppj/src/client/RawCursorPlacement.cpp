// 위치 판정의 누락된 표시 여백은 정밀 C 출력 대신 실제445627..445736 명령을 따른다.
#include "client/RawCursorPlacement.h"
#include "client/GameAssets.h"
#include "client/Renderer.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 도달한 외부 동작의 누락을 명시한다. 허용/빈 그림 기본값을 만들지 않는다.
template<class T> const T& Need(const T& callback) { if (!callback) throw std::logic_error("커서 위치 의존 동작 누락");return callback; }
// x86 DWORD의 덧셈/뺄셈 감김을 signed UB 없이 유지한다.
std::int32_t Add(std::int32_t left,std::int32_t right) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(left)+static_cast<std::uint32_t>(right)); }
std::int32_t Sub(std::int32_t left,std::int32_t right) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(left)-static_cast<std::uint32_t>(right)); }
// 실제 _ftol의 signed64 절삭 결과 하위 DWORD다. 비유한/범위 밖이면 indefinite의 하위0이다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value < -0x1p63 || value >= 0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 실제 자산 번호에서 같은 TYPE/SHP 항목을 찾는다.
const TypeAsset& Asset(const GameAssets& assets,std::uint32_t type) {
    if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=assets.Types().size()) throw std::out_of_range("커서 위치 자산 TYPE 범위");
    return assets.Types()[type-o::kFirstAssetTypeNumber];
}
}
// 지도는 source flags20에 도달할 때만 필요하므로 생성 시256×256 강제를 하지 않는다.
RawCursorPlacement::RawCursorPlacement(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
    std::span<const std::uint16_t> surfaceMap,CursorDropState& drop,CursorLifecycleState& lifecycle,
    CursorPlacementState& state,CursorPlacementHooks hooks)
    :pool_(pool),types_(types),surfaceMap_(surfaceMap),drop_(drop),lifecycle_(lifecycle),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("커서 위치10.78 판본 계약 오류");
}
// 현재 TYPE 포인터의 의미만 읽는다.
const o::RiftTypeRecord& RawCursorPlacement::Type() const {
    if (lifecycle_.abstractType>=types_.size()) throw std::out_of_range("커서 위치 abstract TYPE 범위");return types_[lifecycle_.abstractType];
}
// SP 부족 판정이 아니다. Form의 연관 지역과 현재 패턴이 겹치는지 검사한다.
std::uint32_t RawCursorPlacement::SourceAllowed() const {
    if (!(drop_.costFlags&0x20)) return 1;
    const auto source=Need(hooks_.source)(CursorPreviewSid(drop_.preview));const auto region=Need(hooks_.region)(source);std::uint32_t allowed{};
    Need(hooks_.walk)({drop_.type,std::bit_cast<std::int32_t>(drop_.argument),std::bit_cast<std::int32_t>(drop_.direction),drop_.x,drop_.y,false},
        [this,region,&allowed](CursorCanonCell cell) {
            const auto x=Truncate(static_cast<double>(cell.x)+static_cast<double>(0.9999f));
            const auto y=Truncate(static_cast<double>(cell.y)+static_cast<double>(0.9999f));
            if (x>=0 && y>=0 && x<256 && y<256) {
                if (surfaceMap_.size()!=65536) throw std::logic_error("커서 연관 지역 표면 지도 누락");
                const auto sid=o::Sid{surfaceMap_[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]};
                if (sid.value && Need(hooks_.region)(sid)==region) { allowed=1;return false; }
            }
            return true;
        });
    return allowed;
}
// 문자열 길이와 표시 범위의 실제 산술을 복원한다. C 출력에서 빠진 세 분기를 포함한다.
void RawCursorPlacement::Query() const {
    drop_.legal=Need(hooks_.describe)(state_.description,drop_.x,drop_.y,drop_.type,drop_.localPlayer);
    state_.descriptionAllowed=drop_.legal;
    const auto length=state_.description.find('\0');const auto textLength=static_cast<std::uint32_t>(length==std::string::npos ? state_.description.size() : length);
    float left=static_cast<float>(std::bit_cast<std::int32_t>(0U-(static_cast<std::uint32_t>(Type().footX)<<4)));
    float top=static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Type().footY)*0xfffffff5U));
    float right{},bottom{};std::size_t index{};state_.hotspotX=state_.hotspotY=0;
    Need(hooks_.walk)({drop_.type,std::bit_cast<std::int32_t>(drop_.argument),std::bit_cast<std::int32_t>(drop_.direction),0,0,false},
        [this,&left,&top,&right,&bottom,&index](CursorCanonCell cell) {
            const auto image=Need(hooks_.image)(lifecycle_.abstractType,cell.frame);
            if (!index) { state_.hotspotX=image.hotspotX;state_.hotspotY=image.hotspotY;state_.imageWidth=image.width;state_.imageHeight=image.height; }
            const float px=static_cast<float>(static_cast<double>(cell.x)*16.0-image.hotspotX);
            const float yProduct=cell.y*11.0f;const double py=static_cast<double>(yProduct)-image.hotspotY;
            if (!(px>left)) left=px;if (!(py>top)) top=static_cast<float>(py);
            const double farX=static_cast<double>(px)+image.width,farY=py+image.height;
            if (!(right>farX)) right=static_cast<float>(farX);if (!(bottom>farY)) bottom=static_cast<float>(farY);
            ++index;return true;
        });
    double finalLeft{};
    if (Type().flags2&4) { finalLeft=static_cast<double>(left)-16.0;top-=11.0f;right+=16.0f;bottom+=11.0f; }
    else {
        finalLeft=static_cast<double>(left)-state_.labelSpacing;
        const auto extent=Sub(state_.imageHeight,state_.hotspotY);
        const auto textExtent=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(state_.labelSpacing)*textLength);
        const double labelTop=static_cast<double>(bottom)-(extent<0 ? 0 : extent)-textExtent;
        if (!(labelTop>top)) top=static_cast<float>(labelTop);
    }
    if ((Type().flags1&0x400) && !(bottom>30.0f)) bottom=30.0f;
    state_.bounds={Truncate(-finalLeft),Truncate(-static_cast<double>(top)),Truncate(static_cast<double>(right)-finalLeft),Truncate(static_cast<double>(bottom)-top)};
    const auto oldGump=lifecycle_.currentGump;drop_.legalGump=0;lifecycle_.currentGump=0;
    lifecycle_.currentGump=Need(hooks_.gumpAt)(drop_.screenX,drop_.screenY);
    if (!lifecycle_.currentGump) {
        const float x=static_cast<float>(std::floor(static_cast<double>(drop_.x)+0.5)),y=static_cast<float>(std::floor(static_cast<double>(drop_.y)+0.5));
        state_.placementAllowed=Need(hooks_.mayPlace)({lifecycle_.abstractType,drop_.argument,x,y,drop_.direction,drop_.localPlayer&255U,1});
        state_.spaceAllowed=state_.obstruction==0;
    } else {
        state_.placementAllowed=0;
        drop_.legalGump=Need(hooks_.accept)(lifecycle_.currentGump,state_.acceptEvent,0,0,0);
        if (drop_.legalGump) {
            state_.placementAllowed=state_.descriptionAllowed=drop_.legal=1;
            if (!Need(hooks_.parent)(drop_.legalGump)) throw std::logic_error("cursorLegalGump->parentSid 원본 진단");
        }
    }
    drop_.legal=drop_.legal && state_.placementAllowed;
    state_.sourceAllowed=SourceAllowed();drop_.legal=drop_.legal && state_.sourceAllowed;state_.placementAllowed=state_.placementAllowed && state_.sourceAllowed;
    if (drop_.cursorGump && (!oldGump)!=(!lifecycle_.currentGump)) Need(hooks_.processEvent)(drop_.cursorGump,1,lifecycle_.currentGump ? 1U : 0U,0);
}
// 원본의 signed 나눗셈과 DWORD 감김으로 세 사각형을 순서대로 계산한다.
void RawCursorPlacement::Prepare() const {
    const auto point=Need(hooks_.project)(drop_.x,drop_.y);const auto& b=state_.bounds;
    const ScreenRect rect{Sub(Sub(point.x,b[0]),2),Sub(Sub(point.y,b[1]),2),Add(Add(Sub(b[2],b[0]),6),point.x),Add(Add(Sub(b[3],b[1]),14),point.y)};
    Need(hooks_.invalidate)(rect,2);
    if (((drop_.costFlags&0x20) && !state_.sourceAllowed) || !state_.descriptionAllowed || (!state_.placementAllowed && state_.placementFlags) || !state_.spaceAllowed) {
        const auto x=Add(Sub(b[2]/2,lifecycle_.warningWidth/2),rect.left),y=Add(Sub(b[3]/2,lifecycle_.warningHeight/2),rect.top);
        Need(hooks_.invalidate)({x,y,Add(Add(lifecycle_.warningWidth,2),x),Add(Add(lifecycle_.warningHeight,2),y)},0);
    }
    if (drop_.costFlags&0x40) {
        const auto raw=pool_.Slot(CursorPreviewSid(lifecycle_.previews[0]));
        const auto cellX=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(raw[22]|static_cast<std::uint16_t>(raw[23])<<8));
        const auto cellY=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(raw[24]|static_cast<std::uint16_t>(raw[25])<<8));
        const auto foot=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Type().footX)<<4);
        const auto x=Sub(Sub(Sub(cellX,foot/2),drop_.cursorWidth/2),state_.ScreenOriginX()),y=Sub(cellY,state_.ScreenOriginY());
        Need(hooks_.invalidate)({x,y,Add(Add(drop_.cursorWidth,2),x),Add(Add(lifecycle_.costHeight,2),y)},0);
    }
}
// 호출자 간 단일 풀 계약을 확인한다.
const o::SidPool& RawCursorPlacement::Pool() const { return pool_; }
// 취득·취소·내려놓기 각각의 판정/준비 호출을 한 객체로 보낸다.
void BindCursorPlacement(CursorLifecycleHooks& lifecycle,CursorDropHooks& drop,RawCursorPlacement& placement) {
    lifecycle.queryCursor=drop.queryCursor=[&placement] { placement.Query(); };lifecycle.prepareCursor=drop.prepareCursor=[&placement] { placement.Prepare(); };
}
// 장치와 독립적으로 실제 Canon 반복자와 SHP 헤더 조회만 공급한다.
void BindCursorPlacementAssets(CursorPlacementHooks& hooks,const GameAssets& assets,const o::PriestPlacementGeometryState& geometry) {
    hooks.walk=[&assets,&geometry](o::CanonTypeQuery query,const std::function<bool(CursorCanonCell)>& visit) {
        const auto& asset=Asset(assets,query.type);const auto frames=asset.definition.FrameTable();
        auto decoder=o::DecodeCanonType(frames,asset.definition.specialFrames.defaultFrame,geometry.patternTypes,query,assets.Edition());
        // false는 현재 칸에서 즉시 반환하며 원본처럼 Advance를 생략한다.
        while (decoder.Valid()) { if (!visit({decoder.Frame(),decoder.X(),decoder.Y()})) return;decoder.Advance(); }
    };
    hooks.image=[&assets](std::uint32_t type,std::int32_t frame) {
        const auto& asset=Asset(assets,type);const auto count=asset.definition.FrameTable().Codes().size();
        if (frame<0 || static_cast<std::uint32_t>(frame)>=count*((assets.TypeTable().Types()[type].flags1&0x440000) ? 2U : 1U)) throw std::logic_error("커서 위치 frameCheck 원본 진단");
        const auto image=assets.Shapes().SquidMetrics(asset.block,static_cast<std::size_t>(frame));
        return CursorCostImage{static_cast<std::uint32_t>(asset.block+1),image.width,image.height,image.hotspotX,image.hotspotY};
    };
}
// 실제 Canon/SHP/배치/dirty 출력만 연결한다. 누락된 GUI/지역은 명시 경계로 유지한다.
void BindCursorPlacementWorld(CursorPlacementHooks& hooks,const o::SidPool& pool,const GameAssets& assets,
    const o::PriestPlacementGeometryState& geometry,o::RawCanonPlacementPipeline& placement,Renderer& renderer) {
    if (&pool!=&placement.Pool() || assets.Edition()!=pool.Edition() || pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("커서 위치 실제 월드 계약 오류");
    BindCursorPlacementAssets(hooks,assets,geometry);
    hooks.mayPlace=[&placement](const o::CanonPlacementQuery& query) { return static_cast<std::uint32_t>(placement.MayPlace(query)); };
    hooks.invalidate=[&renderer](ScreenRect rect,std::uint32_t flags) { renderer.Invalidate(rect,flags); };
}
}
