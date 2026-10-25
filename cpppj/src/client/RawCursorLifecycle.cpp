// 원본 커서 취득/취소의 가상 호출 순서·BYTE/WORD 폭·현재 전역 재조회를 보존한다.
#include "client/RawCursorLifecycle.h"
#include "client/GameAssets.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// PE005325cc의 다섯 배치 거부 문구이며 번역 후 실제 일반 글자 측정기로 넘긴다.
constexpr std::array<std::string_view,5> kWarnings{"Priest can't get here.","Must be placed over constructing artifact",
    "Cannot place here","Energy not satisfied","Only on Controlled Island"};
// 도달한 경계가 빠졌을 때 원본 동작을 생략하지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("실제 커서 수명 경계 미연결");return function; }
// little endian WORD는 비정렬 호스트 읽기에 의존하지 않는다.
std::uint32_t Word(std::span<const std::uint8_t> raw,std::size_t offset) { return raw[offset]|(static_cast<std::uint32_t>(raw[offset+1])<<8); }
// 실제 CRT _ftol의 하위 DWORD이며 비유한/범위 밖 값의 integer-indefinite 하위 DWORD는0이다.
std::int32_t Truncate(float value) {
    if (!std::isfinite(value) || std::fabs(static_cast<double>(value))>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 현재 번호를 실제 .type 저장소의 동일 항목에 연결하며 unsigned 뺄셈 전 범위를 확인한다.
const TypeAsset& Asset(const GameAssets& assets,std::uint32_t type) {
    if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=assets.Types().size()) throw std::out_of_range("커서 현재 자산 TYPE 범위 오류");
    return assets.Types()[type-o::kFirstAssetTypeNumber];
}
}
// 큰DWORD를하위WORD로접어다른유효객체로만들지않는다.
o::Sid CursorPreviewSid(std::uint32_t number) { if (number>65535U) throw std::out_of_range("커서원문DWORD의호스트SID범위");return o::Sid{static_cast<std::uint16_t>(number)}; }
// 다른 판본의 raw 필드에10.78의 오프셋을 적용하지 않는다.
RawCursorLifecycle::RawCursorLifecycle(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,CursorDropState& drop,
    CursorLifecycleState& state,CursorLifecycleHooks hooks):pool_(pool),types_(types),drop_(drop),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("커서 수명10.78 Pool/TYPE 계약 오류");
}
// 취소의 조기 반환은 이 조회를 수행하지 않는다.
std::span<const std::uint8_t> RawCursorLifecycle::Raw(o::Sid sid) const {
    if (sid.value<5 || sid.value>=pool_.Capacity()) throw std::out_of_range("커서 수명 현재 SID 범위 오류");return pool_.Slot(sid);
}
// 원본이 저장한 TYPE 포인터는 선택 타입 DWORD와 별개이며 콜백 뒤 현재 값을 읽는다.
const o::RiftTypeRecord& RawCursorLifecycle::Type() const {
    if (state_.abstractType>=types_.size()) throw std::out_of_range("커서 수명 현재 TYPE 범위 오류");return types_[state_.abstractType];
}
// 준비가 flags/목록/개수를 바꿀 수 있으므로 원본처럼 준비 뒤 조건과 반복문을 평가한다.
void RawCursorLifecycle::Activate(std::uint32_t force) const {
    Need(hooks_.prepareCursor)();
    if ((drop_.costFlags&0x200) || force) {
        // 가상 삭제 후 다음 반복에서도 현재 signed 개수와 다음 칸을 읽는다.
        for (std::int32_t index=0;index<state_.previewCount;++index) {
            if (index>=static_cast<std::int32_t>(state_.previews.size())) throw std::out_of_range("커서 임시 목록 원본30칸 범위 오류");
            const auto sid=CursorPreviewSid(state_.previews[static_cast<std::size_t>(index)]);Raw(sid);Need(hooks_.destroy)(sid,0);
        }
        state_.previewCount=0;state_.previews.fill({});
    }
}
// 현재 preview를 캡처한 후 콜백이 전역 preview를 바꿔도 raw 반환 대상은 유지한다.
void RawCursorLifecycle::Cancel() const {
    if (!drop_.lockDepth && drop_.preview.value) {
        Need(hooks_.highlight)(0);
        if (!drop_.lockDepth) { Activate(0);Need(hooks_.selectCursor)(1); }
        ++drop_.lockDepth;const auto preview=drop_.preview;const auto raw=Raw(preview);
        if (!(raw[11]&1)) {
            auto bytes=pool_.AllocatedBytes(preview);bytes[20]=static_cast<std::uint8_t>(drop_.inputFlags);bytes[21]=static_cast<std::uint8_t>(drop_.inputFlags>>8);
            if (drop_.inputFlags&0x80) {
                const auto source=Need(hooks_.source)(preview);const auto sourceRaw=Raw(source);
                if (!(sourceRaw[11]&4)) Need(hooks_.detach)(source,0);Need(hooks_.unpop)(source,0);
            }
            if (drop_.costFlags&0x100) Need(hooks_.destroy)(preview,0);
            else {
                const auto source=Need(hooks_.source)(preview);Raw(source);Need(hooks_.sourceNotice)(source,drop_.preview,4);
                if (drop_.costFlags&0x80) {
                    Need(hooks_.sound)("teleporterActive.wav");
                    if (state_.teleportBlocked) throw std::logic_error("커서 취소 teleporter 원본 false 진단");
                    Need(hooks_.addLocalSp)(-Type().cost);
                }
            }
        }
    }
    Activate(1);
    if (drop_.cursorGump) { Need(hooks_.removeProcess)(drop_.cursorGump,0);drop_.cursorGump=0; }
    state_.previews.fill({});drop_.preview={};drop_.lockDepth=1;state_.previewCount=0;
}
// 팝업 정리부터 현재 마우스 꼬리 호출까지 원본 순서다. 미리보기 생성/판정의 계산은 별도 몸체다.
void RawCursorLifecycle::Take(o::Sid preview,std::uint32_t flags) const {
    Cancel();if (drop_.cursorGump) throw std::logic_error("cursorManaProcess == NULL 원본 진단");Need(hooks_.closePopups)();
    drop_.preview=preview;auto raw=Raw(preview);drop_.type=Word(raw,18);state_.abstractType=Word(raw,18);Type();
    drop_.argument=Word(raw,12);if (!drop_.argument) drop_.argument=Need(hooks_.defaultArgument)(state_.abstractType);
    drop_.direction=0;state_.directionArgument=0;drop_.inputFlags=Word(raw,20);
    auto writable=pool_.AllocatedBytes(preview);writable[21]|=1;
    const auto source=Need(hooks_.source)(preview);Raw(source);Need(hooks_.sourceNotice)(source,drop_.preview,4);
    drop_.costFlags=flags;if (Type().flags2&4) drop_.costFlags=flags|0x200;
    drop_.lockDepth=0;Need(hooks_.createPreviews)();Need(hooks_.selectCursor)(drop_.legalGump ? 3 : 2);
    drop_.legalGump=0;state_.currentGump=0;
    if (Type().flags2&0xa00) {
        drop_.cursorGump=Need(hooks_.createMana)(CursorPreviewSid(state_.previews[0]),0);
        if (state_.currentGump && drop_.cursorGump) Need(hooks_.processEvent)(drop_.cursorGump,1,1,0);
    }
    const auto typeNumber=Need(hooks_.typeNumber)(state_.abstractType);
    if (Need(hooks_.hasRange)(typeNumber)) drop_.cursorGump=Need(hooks_.createRange)(CursorPreviewSid(state_.previews[0]),0);
    Need(hooks_.queryCursor)();state_.warningContext[2]=state_.warningFont;state_.warningWidth=state_.warningHeight=0;
    // 각 문구의 현재 측정 결과를 개별 저장하고 signed 최대값을 갱신한다.
    for (std::size_t index=0;index<kWarnings.size();++index) {
        const auto text=Need(hooks_.translate)(kWarnings[index]);const auto size=Need(hooks_.measure)(text,state_.warningContext);
        state_.warningWidths[index]=size[0];state_.warningHeights[index]=size[1];
        if (state_.warningWidth<=state_.warningWidths[index]) state_.warningWidth=state_.warningWidths[index];
        if (state_.warningHeight<=state_.warningHeights[index]) state_.warningHeight=state_.warningHeights[index];
    }
    state_.costContext[2]=state_.costFont;state_.costText=std::to_string(Truncate(Type().cost));
    const auto size=Need(hooks_.measure)(state_.costText,state_.costContext);state_.costTextWidth=size[0];state_.costHeight=size[1];
    const auto image=Need(hooks_.costImage)(state_.costType,3);state_.costImage=image.token;state_.costHotspotX=image.hotspotX;state_.costHotspotY=image.hotspotY;
    drop_.cursorWidth=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(image.width)+static_cast<std::uint32_t>(state_.costTextWidth));
    if (state_.costHeight<=image.height) state_.costHeight=image.height;state_.imageHeight=image.height;
    const auto mouse=Need(hooks_.mouse)();Need(hooks_.updatePosition)(mouse.x,mouse.y);
}
// 어댑터마다 상태 사본을 생성하지 않는다.
const o::SidPool& RawCursorLifecycle::Pool() const { return pool_; }
// 이미 복원된 두 호출자의 경계만 같은 실제 수명으로 대체한다.
void BindCursorLifecycle(PriestMenuHooks& priest,CursorDropHooks& drop,RawCursorLifecycle& lifecycle) {
    priest.takePreview=[&lifecycle](o::Sid sid,std::uint32_t flags) { lifecycle.Take(sid,flags); };
    drop.activatePreview=[&lifecycle](std::uint32_t force) { lifecycle.Activate(force); };drop.invalidGui=[&lifecycle] { lifecycle.Cancel(); };
}
// 원본의 네 group 또는 flags2의0x100만 검사하며 현재 TYPE 저장소를 매번 읽는다.
bool HasCursorRange(std::span<const o::RiftTypeRecord> types,std::uint32_t typeNumber) {
    if (typeNumber>=types.size()) throw std::out_of_range("커서 사정거리 현재 TYPE 범위 오류");
    const auto& type=types[typeNumber];return type.group==0 || type.group==1 || (type.flags2&0x100)!=0 || type.group==6 || type.group==3;
}
// 실제 TYPE 번호와 specialFrame·같은 SHP 추가 헤더를 구별한다. 별도 타입/프레임 사본을 만들지 않는다.
void BindCursorLifecycleAssets(CursorLifecycleHooks& hooks,const GameAssets& assets) {
    if (assets.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("커서 자산10.78 판본 계약 오류");
    hooks.defaultArgument=[&assets](std::uint32_t type) { return static_cast<std::uint32_t>(Asset(assets,type).definition.specialFrames.defaultFrame); };
    hooks.typeNumber=[&assets](std::uint32_t type) {
        if (type>=assets.TypeTable().Types().size()) throw std::out_of_range("커서 현재 TYPE 번호 범위 오류");return type;
    };
    hooks.hasRange=[&assets](std::uint32_t type) { return HasCursorRange(assets.TypeTable().Types(),type); };
    hooks.costImage=[&assets](std::uint32_t type,std::uint32_t frame) {
        const auto& asset=Asset(assets,type);const auto count=asset.definition.FrameTable().Codes().size();const auto flags=assets.TypeTable().Types()[type].flags1;
        if (frame>=count*((flags&0x440000) ? 2U : 1U)) throw std::logic_error("커서 비용 그림 frameCheck 원본 진단");
        const auto metrics=assets.Shapes().SquidMetrics(asset.block,frame);
        return CursorCostImage{static_cast<std::uint32_t>(asset.block+1),metrics.width,metrics.height,metrics.hotspotX,metrics.hotspotY};
    };
}
// 기존 글자 출력기를 쓰는 호출자는 자산 조회와 실제 측정을 함께 연결한다.
void BindCursorLifecycleAssets(CursorLifecycleHooks& hooks,const GameAssets& assets,const GumpText& text) {
    BindCursorLifecycleAssets(hooks,assets);
    hooks.measure=[&text](std::string_view value,const std::array<std::uint32_t,8>& context) {
        const std::string terminated(value);std::int32_t width{},height{};text.Measure(std::span(terminated.c_str(),terminated.size()+1),DecodeGumpTextContext(context),width,height,-1);return std::array{width,height};
    };
}
}
