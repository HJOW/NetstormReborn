// 원본의 생성/재사용 인덱스와 연결 객체 꼬리 추가 순서를 유지한다.
#include "client/RawCursorPreviews.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 도달한 실제 효과의 누락을 성공 기본값으로 대체하지 않는다.
template<class T> const T& Need(const T& effect) { if (!effect) throw std::logic_error("커서 미리보기 의존 동작 누락");return effect; }
// 비정렬 little endian 필드를 원래 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result{};
    // 각 바이트를 낮은 자리부터 합친다.
    for (std::size_t index=0;index<width;++index) result|=static_cast<std::uint32_t>(raw[offset+index])<<(8*index);return result;
}
// 옆 필드를 유지하며 원래 폭의 하위 비트만 쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 원본 WORD/DWORD 쓰기의 바이트 범위를 보존한다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 실제 _ftol signed64 절삭의 하위WORD다. 비유한/범위 밖은 indefinite 하위0이다.
std::uint16_t FtolWord(double value) {
    if (!std::isfinite(value) || value < -0x1p63 || value>=0x1p63) return 0;return static_cast<std::uint16_t>(static_cast<std::int64_t>(value));
}
}
// 첫 미리보기 유무와 실제 개수는 Create 호출 시점에 읽는다.
RawCursorPreviews::RawCursorPreviews(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,CursorDropState& drop,
    CursorLifecycleState& lifecycle,const o::BridgeConnectState& surfaces,CursorPreviewHooks hooks)
    :pool_(pool),types_(types),drop_(drop),lifecycle_(lifecycle),surfaces_(surfaces),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("커서 미리보기10.78 판본 계약 오류");
}
// 생성 도중 외부 효과가 바꾼 현재 TYPE도 다시 조회한다.
const o::RiftTypeRecord& RawCursorPreviews::Type() const {
    if (lifecycle_.abstractType>=types_.size()) throw std::out_of_range("커서 미리보기 abstract TYPE 범위");return types_[lifecycle_.abstractType];
}
// 원본의 signed 목록 인덱스를 호스트 배열 범위에서만 허용한다.
o::Sid RawCursorPreviews::At(std::int32_t index) const {
    if (index<0 || static_cast<std::size_t>(index)>=lifecycle_.previews.size()) throw std::out_of_range("커서 미리보기 목록 인덱스");return lifecycle_.previews[static_cast<std::size_t>(index)];
}
// 원본은 호출 뒤 개수를 읽어 append한다. 별도 생성 번호 목록을 만들지 않는다.
void RawCursorPreviews::Append(o::Sid sid) const {
    At(lifecycle_.previewCount);lifecycle_.previews[static_cast<std::size_t>(lifecycle_.previewCount)]=sid;++lifecycle_.previewCount;
}
// 화면 캐시는 지도 칸의 정수 좌표와 다르다. 음수도 원래 +0.5 뒤0쪽 절삭을 유지한다.
void RawCursorPreviews::Position(o::Sid sid,float x,float y) const {
    auto raw=pool_.AllocatedBytes(sid);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));
    Put(raw,22,FtolWord(static_cast<double>(x)*16.0+0.5),2);Put(raw,24,FtolWord(static_cast<double>(y)*11.0+0.5),2);
}
// 실물 Pop 없이 raw 위치/프레임과 임시 SID만 갱신한다.
void RawCursorPreviews::Create() const {
    Need(hooks_.queryCursor)();const bool create=!lifecycle_.previews[0].value;std::int32_t ordinal{};
    Need(hooks_.walk)({drop_.type,std::bit_cast<std::int32_t>(drop_.argument),std::bit_cast<std::int32_t>(drop_.direction),drop_.x,drop_.y,false},
        [this,create,&ordinal](CursorCanonCell cell) {
            auto sid=At(ordinal++);
            if (create) { sid=Need(hooks_.create)(drop_.type,2);Append(sid); }
            auto raw=pool_.AllocatedBytes(sid);raw[40]|=8;Put(raw,36,std::bit_cast<std::uint32_t>(cell.frame));
            Need(hooks_.setWord)(sid,drop_.argument);Need(hooks_.setOwner)(sid,drop_.localPlayer);
            if (Need(hooks_.hasHitPoints)(sid)) Need(hooks_.setHitPoints)(sid,0);
            Position(sid,cell.x,cell.y);
            if (Type().flags2&4) {
                const auto quality=pool_.Slot(drop_.preview)[30];const std::int32_t qualityFlags=quality==1 ? 0x20 : quality==3 ? 0x40 : 0;
                const auto& frames=Need(hooks_.frames)(surfaces_.bridgeType);
                if (cell.frame<0 || static_cast<std::size_t>(cell.frame)>=frames.Codes().size()) throw std::out_of_range("커서 다리 코드 프레임 범위");
                const auto code=frames.Codes()[static_cast<std::size_t>(cell.frame)];
                const auto frame=frames.FindFlags(code.side,code.variant,static_cast<std::int32_t>(std::bit_cast<std::int8_t>(code.flags))|qualityFlags);
                raw=pool_.AllocatedBytes(sid);Put(raw,36,std::bit_cast<std::uint32_t>(frame));
                const float oldX=std::bit_cast<float>(Read(raw,14)),oldY=std::bit_cast<float>(Read(raw,18));
                const float x=static_cast<float>(std::floor(static_cast<double>(oldX)+0.5)),y=static_cast<float>(std::floor(static_cast<double>(oldY)+0.5));
                const float deltaX=x-oldX,deltaY=y-oldY;Position(sid,x,y);
                if (lifecycle_.previewCount<0 || lifecycle_.previewCount>30) throw std::out_of_range("커서 연결 객체 꼬리 범위");
                const auto count=Need(hooks_.connect)(sid,std::span(lifecycle_.previews).subspan(static_cast<std::size_t>(lifecycle_.previewCount)));
                // 연결 객체는 임시 정수 맞춤으로 생긴 차이만큼 되돌린다. 목록 개수는 매회 현재 값을 읽는다.
                for (std::int32_t index=0;index<count;++index) {
                    const auto connector=At(lifecycle_.previewCount+index);const auto data=pool_.Slot(connector);
                    const float px=std::bit_cast<float>(Read(data,14))-deltaX,py=std::bit_cast<float>(Read(data,18))-deltaY;Position(connector,px,py);
                }
                Position(sid,oldX,oldY);
                if (count>6) throw std::logic_error("sidCount <= NUM_CONNECTORS 원본 진단");
                lifecycle_.previewCount=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(lifecycle_.previewCount)+static_cast<std::uint32_t>(count));
            }
            if (lifecycle_.previewCount>=30) throw std::logic_error("cursorNumSids < MAX_CURSOR_SIDS 원본 진단");return true;
        });
    if (Type().flags1&0x400) {
        // 원본은 실제 Canon 칸 수로 재사용 슬롯을 고른다. 연결 객체 수를 인덱스에 더하지 않는다.
        for (std::uint32_t part=0;part<2;++part) {
            auto sid=At(ordinal++);
            if (create) { sid=Need(hooks_.create)(part ? surfaces_.stalagType : surfaces_.islandType,2);Append(sid); }
            auto raw=pool_.AllocatedBytes(sid);raw[40]|=8;Put(raw,36,8);Position(sid,drop_.x,drop_.y);
        }
    }
    Need(hooks_.prepareCursor)();
}
// 실제 생성 연결이 동일 풀을 쓰는지 확인한다.
const o::SidPool& RawCursorPreviews::Pool() const { return pool_; }
// Take의 createPreviews 경계만 바꾼다. 다른 가상 효과를 대체하지 않는다.
void BindCursorPreviews(CursorLifecycleHooks& lifecycle,RawCursorPreviews& previews) {
    lifecycle.createPreviews=[&previews] { previews.Create(); };
}
// 실제 Query/Prepare와 이미 연결된 Canon 순회를 공유한다.
void BindCursorPreviewPlacement(CursorPreviewHooks& hooks,const o::SidPool& pool,RawCursorPlacement& placement,const CursorPlacementHooks& placementHooks) {
    if (&pool!=&placement.Pool() || !placementHooks.walk) throw std::invalid_argument("커서 미리보기/위치 단일 풀 또는 Canon 연결 오류");
    hooks.queryCursor=[&placement] { placement.Query(); };hooks.prepareCursor=[&placement] { placement.Prepare(); };hooks.walk=placementHooks.walk;
}
// 실제 Factory/공통 HP/연결 생성과 현재 bridge TYPE 표를 사용한다.
void BindCursorPreviewWorld(CursorPreviewHooks& hooks,o::SidPool& pool,o::SquidFactory& factory,
    const o::SquidReward& reward,o::RawBridgeConnect& connect,std::span<const o::PriestPlainCanonType> frames,
    const o::BridgeConnectState& surfaces,std::function<void(o::Sid)> notifySurface) {
    if (&pool!=&factory.Pool() || &pool!=&reward.Pool() || &pool!=&connect.Pool() || pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("커서 미리보기 실제 월드 단일 풀 계약 오류");
    hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };
    hooks.setWord=[&pool,&surfaces,notifySurface=std::move(notifySurface)](o::Sid sid,std::uint32_t word) {
        auto raw=pool.AllocatedBytes(sid);Put(raw,12,word,2);
        if (raw[10]==surfaces.bridgeType) Need(notifySurface)(sid);
    };
    hooks.hasHitPoints=[&reward](o::Sid sid) { return reward.HasHitPoints(sid); };
    hooks.setHitPoints=[&pool](o::Sid sid,std::int32_t value) { o::SetRawSquidHitPoints(pool,sid,value); };
    hooks.frames=[frames](std::uint32_t type)->const o::RiftTypeFrames& { if (type>=frames.size()) throw std::out_of_range("커서 실제 bridge 프레임 표 범위");return frames[type].frames; };
    hooks.connect=[&connect](o::Sid sid,std::span<o::Sid> output) {
        std::int32_t count{};
        connect.ConnectPreview(sid,[output,&count](o::Sid born) {
            if (static_cast<std::size_t>(count)>=output.size()) throw std::out_of_range("커서 실제 연결 객체 꼬리 넘침");output[static_cast<std::size_t>(count++)]=born;
        });
        return count;
    };
}
}
