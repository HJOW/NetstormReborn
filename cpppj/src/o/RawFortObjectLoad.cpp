// 원본 저장 필드의 좁은 쓰기·현재 전역/타입 재조회·재귀와 청크 순서를 보존한다.
#include "o/RawFortObjectLoad.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 raw 타입/WORD/HP와 판본별 프레임 위치다.
constexpr std::size_t kType=10,kWord=12,kHp=26,kPatchFrame=36,kCdFrame=34;
// 특수 프레임/contained·dais 부류와 내용물 flags 변환 비트다.
constexpr std::uint32_t kTerrainFrame=0x200000,kRandomFrame=0x100000,kContainer=0x20000,kDais=0x400000,
    kContentLocal=0x10,kContentFiltered=0x100;
// 원본 P/P 고정 요청의 방향/변형 문자 및 후보 최대 개수다.
constexpr std::uint8_t kLetterP='P',kLetterA='A';
constexpr std::size_t kCandidates=256;
// little endian 좁은 쓰기는 인접 payload를 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 실제 지정 폭의 바이트만 기록한다.
    for (std::size_t byte=0;byte<width;++byte) raw[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// signed 원본 좌표의 DWORD 감김 덧셈이다.
std::int32_t Add(std::int32_t origin,std::uint32_t value) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(origin)+value); }
// 현재 분기가 요구한 저장 필드가 없으면 임의0을 대신 쓰지 않고 거부한다.
template<class T> T Required(const std::optional<T>& value) {
    if (!value) throw std::logic_error("Fort 현재 타입이 요구한 저장 필드 누락");return *value;
}
}
// 원본 계산 전에 판본·같은 자료 크기와 모든 실제 효과 경계를 검사한다.
RawFortObjectLoad::RawFortObjectLoad(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const RiftTypeFrames> frames,GameRandom& random,FortObjectLoadState& state,FortObjectLoadHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),random_(random),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size() ||
        !hooks_.create || !hooks_.notifySurface || !hooks_.setState || !hooks_.setOwner || !hooks_.hasHitPoints ||
        !hooks_.maxHitPoints || !hooks_.virtualCount || !hooks_.pop || !hooks_.destroy || !hooks_.createContent ||
        !hooks_.chunkRegion || !hooks_.territoryAllowed || !hooks_.loadingDepthChanged || !hooks_.finishOwner)
        throw std::invalid_argument("Fort 로더의 판본/자료/실제 효과 연결 오류");
}
// callback이 바꾼 현재 타입을 읽으며 객체 할당 여부도 확인한다.
const RiftTypeRecord& RawFortObjectLoad::Type(Sid sid) const {
    const auto raw=pool_.AllocatedBytes(sid);if (raw[kType]>=types_.size()) throw std::out_of_range("Fort 현재 타입 범위 오류");
    return types_[raw[kType]];
}
// 초기 저장/특수 프레임은 표시 갱신 없이 원본 필드에 직접 쓴다.
void RawFortObjectLoad::Frame(Sid sid,std::uint32_t value) const {
    Put(pool_.AllocatedBytes(sid),pool_.Edition()==OriginalEdition::Patch1078 ? kPatchFrame : kCdFrame,value,
        pool_.Edition()==OriginalEdition::Patch1078 ? 4U : 1U);
}
// QA 낮은 BYTE/QB WORD 뒤 현재 다리 전역을 비교한다.
void RawFortObjectLoad::Word(Sid sid,std::uint32_t value) const {
    Put(pool_.AllocatedBytes(sid),kWord,value,2);if (pool_.Slot(sid)[kType]==state_.bridgeType) hooks_.notifySurface(sid);
}
// Fort 좌표는 이미 signed 정수에서 float로 저장했다. 같은 칸 일반 finder의 첫 island genus를 선택한다.
Sid RawFortObjectLoad::Terrain(float x,float y) const {
    const auto ix=static_cast<std::int32_t>(x),iy=static_cast<std::int32_t>(y);RawSquidFinder finder(pool_,hash_,types_);
    // finder 자체의 void/footprint/후행 링크 규칙을 사용하며 이 helper에서 추가 상태 필터를 넣지 않는다.
    for (auto sid=finder.Begin({ix,iy,ix,iy});sid.value;sid=finder.Next()) if (Type(sid).flags2&TypeFlag2::kIsland) return sid;
    return {};
}
// P/P fallback 변환은 실제 두 PE의 방향 mask에서 P=0이므로 다시 P/P다. 그 뒤 P/A, P/임의 변형을 찾는다.
std::int32_t RawFortObjectLoad::RandomFrame(Sid sid) const {
    const auto type=pool_.Slot(sid)[kType];if (type>=frames_.size()) throw std::out_of_range("Fort 랜덤 프레임 타입 오류");
    const auto codes=frames_[type].Codes();std::array<std::int32_t,kCandidates> candidates{};std::size_t count=0;
    // 실제 고정 wildcard 요청의 P/P→변환P/P→P/A→P/아무 변형 순서를 유지한다.
    for (int pass=0;pass<4 && !count;++pass) {
        // 최대256 후보만 저장하고 나머지 코드는 계속 순회한다.
        for (std::size_t index=0;index<codes.size();++index) {
            const auto& code=codes[index];
            if (code.side==kLetterP && (pass==3 || code.variant==(pass==2 ? kLetterA : kLetterP)) && count<kCandidates)
                candidates[count++]=static_cast<std::int32_t>(index);
        }
    }
    return count ? candidates[random_.Next(static_cast<std::uint32_t>(count))] : -1;
}
// 저장 자료의 선택과 실제 객체 부수 효과를 원본 순서로 실행한다.
Sid RawFortObjectLoad::Object(const FortObject& object,std::int32_t originX,std::int32_t originY,std::uint32_t defaultOwner) const {
    std::uint32_t owner=defaultOwner;
    const auto type=object.storedType>=0xf6 ? state_.bridgeType : static_cast<std::uint32_t>(object.type);
    if (object.storedType>=0xf6) owner=255U-object.storedType;
    if (!type || type==state_.islandType) return {};
    if (type>=types_.size()) throw std::out_of_range("Fort 저장 타입 범위 오류");
    const float x=static_cast<float>(Add(originX,static_cast<std::uint32_t>(object.CellX()))),
        y=static_cast<float>(Add(originY,static_cast<std::uint32_t>(object.CellY())));
    const auto sid=hooks_.create(type,0);Type(sid);
    if (Type(sid).flags1&TypeFlag1::kSaveFrame) Frame(sid,Required(object.frame));
    if (Type(sid).flags1&TypeFlag1::kSaveQA) Word(sid,(static_cast<std::uint32_t>(pool_.Slot(sid)[kWord+1])<<8)|Required(object.quantityByte));
    if (Type(sid).flags1&TypeFlag1::kSaveQB) Word(sid,static_cast<std::uint32_t>(Required(object.quantityWord)));
    if (Type(sid).flags2&TypeFlag2::kBridge) Frame(sid,Required(object.bridgeFrame));
    if (state_.version==0) hooks_.setState(sid,Required(object.legacyState)); // 원본 legacy state 판정은 OR 명령 때문에 항상 참이다.
    if (std::bit_cast<std::int32_t>(state_.version)>1 && (Type(sid).flags2&TypeFlag2::kFactory)) hooks_.setState(sid,Required(object.factoryState));
    if (std::bit_cast<std::int32_t>(state_.version)>0) {
        if (Type(sid).flags2&state_.ownerSavedMask) {
            owner=Required(object.owner);if (!owner || owner>8) owner=1;
            if (!state_.special && !state_.mission) owner=defaultOwner;
        }
    } else owner=defaultOwner;
    if (Type(sid).flags2&state_.ownerClearedMask) owner=0;
    const auto currentType=pool_.Slot(sid)[kType];
    if (currentType==state_.islandType || currentType==state_.noIslandType) owner=0;
    if (currentType==state_.largeIslandType) throw std::logic_error("Fort isleBig 로드 원본 assert");
    hooks_.setOwner(sid,owner);
    if (hooks_.hasHitPoints(sid)) {
        const auto hp=hooks_.maxHitPoints(sid);
        // Fort는 사제의 가상 HP 전환을 부르지 않고 실제 공통004adca0/CD004aed20에 직접 저장한다.
        Put(pool_.AllocatedBytes(sid),kHp,static_cast<std::uint32_t>(hp),pool_.Edition()==OriginalEdition::Patch1078 ? 4U : 2U);
    }
    if (Type(sid).flags1&kTerrainFrame) {
        if (const auto ground=Terrain(x,y);ground.value) {
            const auto raw=pool_.Slot(ground);std::uint32_t frame=raw[kCdFrame];
            if (pool_.Edition()==OriginalEdition::Patch1078) {
                frame=0;for (std::size_t byte=0;byte<4;++byte) frame|=static_cast<std::uint32_t>(raw[kPatchFrame+byte])<<(8*byte); // 판본 DWORD 전체를 읽는다.
                if (frame==0xffffffffU) frame=0;
            }
            Frame(sid,frame); // CD의255는 zero-extend되어 -1이 아니므로 그대로 보존한다.
        }
    }
    if (Type(sid).flags1&kRandomFrame) { const auto frame=RandomFrame(sid);Frame(sid,frame==-1 ? 0U : static_cast<std::uint32_t>(frame)); }
    hooks_.pop(sid,x,y,9);
    if (Type(sid).flags1&kContainer) Contents(sid,Required(object.contents),9);
    if (state_.fort && !state_.fortDestroyBlocked) {
        const auto genus=Type(sid).flags2;
        if (!(genus&state_.fortKeepMask) ||
            (!state_.special && !state_.mission && (Type(sid).flags2&kDais) && hooks_.virtualCount(sid)==1)) hooks_.destroy(sid,0);
    }
    return sid; // Fort의 삭제 분기에서는 반환 SID가 이미 해제됐을 수 있다. 원본 body의 반환 의미를 새로 만들지 않는다.
}
// 실제 부모 타입/내용물 타입의 현재 listFlags를 조회해 저장 데이터가 요구한 효과를 적용한다.
bool RawFortObjectLoad::Contents(Sid parent,const FortContentList& contents,std::uint32_t flags) const {
    // 한 목록 안에서 Local 플래그가 한 번 켜지면 뒤 형제에도 유지된다.
    for (const auto& item:contents.items) {
        if (!item.type) return false;if (item.type<0 || static_cast<std::size_t>(item.type)>=types_.size()) throw std::out_of_range("Fort 내용물 타입 범위 오류");
        const auto savedFlags=types_[static_cast<std::size_t>(item.type)].flags1;
        auto listFlags=types_[static_cast<std::size_t>(item.type)].contentListFlags;
        if (!listFlags) listFlags=Type(parent).containerListFlags;
        if (!listFlags) {
            if (!item.listFlagsStored) throw std::logic_error("Fort 현재 내용물 부모가 요구한 listFlags 저장 BYTE 누락");listFlags=item.listFlags;
        }
        if (listFlags&kContentLocal) flags=(flags&~8U)|16U;
        const auto child=hooks_.createContent(parent,static_cast<std::uint32_t>(item.type),static_cast<std::uint32_t>(item.quantity),listFlags&~kContentFiltered,flags,0);
        if (savedFlags&kContainer) {
            FortContentList nested{item.contents,item.contentsComplete};if (!Contents(child,nested,flags)) return false;
        }
    }
    return contents.complete;
}
// 판본의 실제 청크16배 좌표를 구하고 파일 객체 순서를 유지한다.
void RawFortObjectLoad::Chunk(const FortChunk& chunk,std::int32_t x,std::int32_t y,std::uint32_t defaultOwner) const {
    const auto originX=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(x)<<4),originY=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(y)<<4);
    // 청크 내 저장 순서대로 즉시 월드를 바꾼다.
    for (const auto& object:chunk.objects) Object(object,originX,originY,defaultOwner);
}
// Terr 선택은 현재 지역 정책 반환값을 사용하며 Chaff에서도 실제 청크 지도를 읽는다.
void RawFortObjectLoad::Section(const FortChunkSection& section,std::uint32_t owner,std::int32_t territory) const {
    state_.version=section.version;if (!section.present) return;state_.scope=territory<0 ? 2U : 1U;std::size_t index=0;
    // 원본은 y를 바깥 순회하며 각 반복에서 현재 지도 크기를 읽는다.
    for (std::int32_t y=0;y<state_.chunksY;++y) {
        // x가 안쪽이며 정책 callback이 바꾼 후행 지도/범위도 다음 비교에서 반영한다.
        for (std::int32_t x=0;x<state_.chunksX;++x) {
            const auto region=hooks_.chunkRegion(x,y);if (territory>=0 && !hooks_.territoryAllowed(region,owner,territory)) continue;
            if (index>=section.chunks.size()) throw std::out_of_range("Fort 선택 청크 저장 자료 소진");Chunk(section.chunks[index++],x,y,owner);
        }
    }
}
// 상위 depth/판본/Terr 순서와 후처리까지 실제 월드 경계에 전달한다.
void RawFortObjectLoad::Load(const FortTemplate& fort,std::int32_t templateSlot,std::int32_t start,std::int32_t end) const {
    if (start==end || start<0 || end>20) throw std::invalid_argument("Fort Terr 범위 오류");
    if (pool_.Edition()==OriginalEdition::Cd1072 && (templateSlot<1 || templateSlot>8)) throw std::out_of_range("CD Fort Template 소유자 원본 assert");
    const auto owner=templateSlot>=1 && templateSlot<=8 ? static_cast<std::uint32_t>(templateSlot) : 0U;
    ++state_.loadingDepth;hooks_.loadingDepthChanged(state_.loadingDepth);
    // 타입 변환은 FortTemplate 파서가 소유한 레코드/변환표를 이 로딩 호출 내내 사용한다.
    try {
        // Terr 번호는 start부터 end 직전까지 증가하며 섹션별 현재 버전을 사용한다.
        for (auto territory=start;territory<end;++territory) Section(fort.territories[static_cast<std::size_t>(territory)],owner,territory);
        if (state_.fort || state_.chaffMode) Section(fort.chaff,owner,-1);
        hooks_.finishOwner(owner);
    } catch (...) {
        --state_.loadingDepth;hooks_.loadingDepthChanged(state_.loadingDepth);throw; // 호스트 예외에서 깊이를 복구하되 앞선 월드 효과는 되돌리지 않는다.
    }
    --state_.loadingDepth;hooks_.loadingDepthChanged(state_.loadingDepth);
}
// 같은 월드 확인용 풀 참조다.
const SidPool& RawFortObjectLoad::Pool() const { return pool_; }
// 이미 복원한 실제 생성/HP/내용물 모듈을 같은 풀에서만 사용한다.
FortObjectLoadHooks MakeFortObjectLoadHooks(const SidPool& pool,SquidFactory& factory,const SquidReward& hp,
    const RawContainedCreate& contained,FortObjectLoadHooks hooks) {
    if (&pool!=&factory.Pool() || &pool!=&hp.Pool() || &pool!=&contained.Pool()) throw std::invalid_argument("Fort 생성/HP/내용물 SID 풀 불일치");
    hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };
    hooks.hasHitPoints=[&hp](Sid sid) { return hp.HasHitPoints(sid); };hooks.maxHitPoints=[&hp](Sid sid) { return hp.MaxHitPoints(sid); };
    hooks.createContent=[&contained](Sid parent,std::uint32_t type,std::uint32_t quantity,std::uint32_t listFlags,std::uint32_t flags,std::uint32_t quality) {
        return contained.Create(parent,type,quantity,listFlags,flags,quality);
    };
    return hooks;
}
}
