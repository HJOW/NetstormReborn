// 타입/가상 표별 몸체를 구별하고 외부 효과 뒤의 현재 raw 상태를 다시 읽는다.
#include "o/RawAssetPostPop.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제 후처리 몸체의 분류다. 숫자는 호스트 분배용이며 원본 타입 번호와 다르다.
constexpr std::uint32_t kGenerator=1,kGunAngle=2,kGunFirst=3,kGunSun=4,kGunArcher=5,kGunDecoration=6,
    kResidence=7,kBlocker=8,kGeyser=9,kVortexRain=10,kVortexWind=11,kVortexThunder=12,kSunAviary=13,kSunFence=14,kRainAviary=15;
// 독립 모듈의 기존 상태에서 신전 네 필드를 원본 저장 순서로 선택하는 멤버 주소다.
constexpr std::array<std::uint32_t AssetPostPopPlayer::*,4> kTempleFields{
    &AssetPostPopPlayer::initialTemple,&AssetPostPopPlayer::initialTempleCopy,
    &AssetPostPopPlayer::newTemple,&AssetPostPopPlayer::newTempleCopy};
// 복원한 PE 가상 표와 +0x20의 몸체 부류를 대응한다.
struct PostPopTable { /* vtable: 원본 객체의 가상 함수 표 주소. */ /* kind: 자료나 동작의 종류를 구별하는 값. */ std::uint32_t vtable,kind; };
// 패치10.78의 실제 +0x20 주소마다 분배한다. 캐논과 빈 패치타입186도 같은 VortexWind 몸체다.
constexpr std::array<PostPopTable,19> kPatchTables{{
    {0x501ea0,kGenerator},{0x50b6e8,kGunAngle},{0x50b7c0,kGunFirst},{0x50b9b8,kGunSun},{0x50bad0,kGunSun},
    {0x50b888,kGunArcher},{0x50b570,kGunDecoration},{0x50f540,kResidence},
    {0x50e940,kBlocker},{0x50ea08,kBlocker},{0x50ead0,kBlocker},{0x50da88,kGeyser},
    {0x5196f0,kVortexRain},{0x519860,kVortexWind},{0x5197a8,kVortexWind},{0x519918,kVortexThunder},{0x509930,kSunAviary},{0x509470,kSunFence},{0x5099e8,kRainAviary}}};
// CD10.72는 캐논 부류의 두 가상 표와 레지던스/제네레이터 위치가 다르다.
constexpr std::array<PostPopTable,15> kCdTables{{
    {0x5022f8,kGenerator},{0x5011e8,kGunAngle},{0x5012b0,kGunFirst},{0x501120,kGunSun},{0x501378,kGunSun},
    {0x5014f8,kGunArcher},{0x501440,kGunDecoration},{0x501b90,kResidence},
    {0x506168,kBlocker},{0x506230,kBlocker},{0x5062f8,kBlocker},{0x500760,kGeyser},
    {0x501d28,kVortexRain},{0x501de0,kVortexWind},{0x501e98,kVortexThunder}}};
// 정렬되지 않은 raw DWORD/WORD를 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    // 필요한 바이트만 현재 슬롯에서 조합한다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return result;
}
// 원본 WORD/DWORD/byte의 쓰기 폭을 유지한다.
void Write(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 지정한 필드 폭 바깥의 payload를 보존한다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 현재 float 좌표의 비트 패턴을 읽는다.
float Position(std::span<const std::uint8_t> raw,std::size_t offset) { return std::bit_cast<float>(Read(raw,offset)); }
// 원본 전체 목록은 capacity가 남아 있는 경우만 덧붙인다.
void Add(SquidPostPopList& list,Sid sid,bool unique) {
    const auto items=list.Items();
    if (list.count<list.entries.size() && (!unique || std::ranges::find(items,sid.value)==items.end()))
        list.entries[list.count++]=sid.value;
}
// CRT ftol의 유한 signed64 범위에서 low DWORD를 반환한다.
std::uint32_t Truncate(float value) {
    const auto wide=static_cast<double>(value);
    if (!std::isfinite(wide) || wide<-9223372036854775808.0 || wide>=9223372036854775808.0)
        throw std::out_of_range("자산 postPop CRT 정수 범위 오류");
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(wide));
}
}
// 같은 풀의 직접 Damageable와 공통 장부를 사용해야 깊이를 한 번만 감소시킬 수 있다.
RawAssetPostPop::RawAssetPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPop& base,
    const RawCarrierPostPop& damageable,AssetPostPopState& state,AssetPostPopHooks hooks)
    :pool_(pool),types_(types.begin(),types.end()),base_(base),damageable_(damageable),state_(state),hooks_(std::move(hooks)) {
    if (&base.Pool()!=&pool_ || &damageable.Pool()!=&pool_ || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U))
        throw std::invalid_argument("자산 postPop 풀/판본/타입 표 불일치");
}
// 전체 물리기록 크기를 확인한 뒤 같은9 Player 저장소와 현재 소유자 전역을 빌린다.
void RawAssetPostPop::BindPlayerRecords(std::span<std::uint8_t> records,const std::uint32_t* currentLocalOwner) {
    const auto stride=pool_.Edition()==OriginalEdition::Patch1078 ? 0xb4U : 0xacU; // 실제 Player 한 기록의 BYTE 크기다.
    if (records.size()!=(kPlayerCount+1)*stride) throw std::invalid_argument("자산 postPop 실제9 Player 기록 크기 오류");
    playerRecords_=records;localOwner_=currentLocalOwner;
}
// 네 필드는 PATCH+7c/+80/+84/+88 및 CD+74/+78/+7c/+80의 원본DWORD다.
std::size_t RawAssetPostPop::PlayerTempleOffset(std::uint32_t owner,std::size_t field) const {
    if (owner>kPlayerCount || field>=kTempleFields.size()) throw std::out_of_range("자산 postPop Player 신전 필드 범위 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078; // 현재 모듈의 원본 판본이다.
    return owner*(patch ? 0xb4U : 0xacU)+(patch ? 0x7cU : 0x74U)+field*4;
}
// 외부 음향/가상 효과 뒤에 변경된 Player DWORD도 매번 현재 원문에서 읽는다.
std::uint32_t RawAssetPostPop::PlayerTemple(std::uint32_t owner,std::size_t field) const {
    const auto offset=PlayerTempleOffset(owner,field); // 현재 owner/field의 물리 저장 주소다.
    return playerRecords_.empty() ? state_.players[owner].*kTempleFields[field] : Read(playerRecords_,offset);
}
// 공유기록 모드에서는 이전 별도 상태 사본을 갱신하지 않아 쓰기 원천을 하나로 유지한다.
void RawAssetPostPop::SetPlayerTemple(std::uint32_t owner,std::size_t field,std::uint32_t sid) const {
    const auto offset=PlayerTempleOffset(owner,field); // 원본이 쓰는 정확DWORD 위치다.
    if (playerRecords_.empty()) state_.players[owner].*kTempleFields[field]=sid;
    else Write(playerRecords_,offset,sid,4);
}
// callback에서 owner를 바꾼 경우에도 다음 비교는 변경된 같은 전역을 사용한다.
std::uint32_t RawAssetPostPop::LocalOwner() const { return localOwner_ ? *localOwner_ : state_.localOwner; }
// 주소는 기록값으로만 비교한다. 호스트 포인터로 호출하지 않는다.
std::uint32_t RawAssetPostPop::Kind(Sid sid) const {
    const auto values=pool_.Edition()==OriginalEdition::Patch1078 ? std::span<const PostPopTable>(kPatchTables) : std::span<const PostPopTable>(kCdTables);
    const auto address=Read(pool_.Slot(sid),0);
    // 같은 주소의 실제 몸체 부류를 반환한다.
    for (const auto& item:values) if (item.vtable==address) return item.kind;
    return 0;
}
// 지원 여부는 실제 표와 연결 인스턴스로 결정한다. 효과 누락은 별도의 Validate가 거부한다.
bool RawAssetPostPop::Handles(Sid sid) const { return Kind(sid)!=0; }
// 외부 효과가 필요한 현재 입력 분기를 쓰기 전에 확인한다. callback의 후속 상태 변화도 같은 필수 경계를 사용한다.
void RawAssetPostPop::Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop) const {
    const auto kind=Kind(sid);const auto raw=pool_.Slot(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (!kind || raw[10]>=types_.size()) throw std::invalid_argument("자산 postPop 가상 표/타입 미복원");
    base_.ValidateBase(sid,flags,pop);
    if (kind==kSunAviary) { if (!hooks_.sunAviaryPostPop) throw std::logic_error("썬 아비어리 실제 전체 후처리 미연결");return; } // 일반 Factory/신전 몸체로 분배하지 않는다.
    if (kind==kRainAviary) { if (!hooks_.rainAviaryPostPop) throw std::logic_error("맨 오 워 풀 실제 전체 후처리 미연결");return; } // 휘리베이스의 상태 기계로 대체하지 않는다.
    if (kind==kSunFence) { if (!hooks_.sunFencePostPop) throw std::logic_error("썬 펜스 실제 전체 후처리 미연결");return; } // 다른 Damageable의 공통 몸체만 호출하지 않는다.
    if (kind==kGenerator) return;
    if (kind==kBlocker) {
        if (state_.inBattle && (flags&1) && !state_.loadingDepth && !hooks_.startBlocker)
            throw std::invalid_argument("배리케이드 최초 프로세스 경계 누락");
        return;
    }
    if (kind>=kGunAngle && kind<=kGunArcher) {
        if (!hooks_.createGunProcess || (kind==kGunAngle && (!hooks_.gunFrame || !hooks_.setGraph)) ||
            (kind==kGunArcher && !hooks_.removeAnimation) || (patch && state_.checkingForms && !hooks_.checkForms))
            throw std::invalid_argument("Gun postPop 효과 누락");
        if (patch && state_.inBattle && !(raw[40]&1) && (flags&1) &&
            ((kind==kGunSun && !state_.loadingDepth) || (kind==kGunArcher && !state_.fortMode)) &&
            ((flags&0x50) || (sid.value>=5 && sid.value<pool_.Layout().serverFirst)))
            throw std::logic_error("Gun postPop 원본 서버/로딩 assert 경로");
        return;
    }
    if (!(flags&1) && kind<kVortexRain) return;
    if (!hooks_.reserveRegular || !hooks_.addRegular) throw std::invalid_argument("자산 postPop Regular 경계 누락");
    if (kind==kResidence && (!hooks_.surface || !hooks_.setOwner || !hooks_.notifySurface))
        throw std::invalid_argument("레지던스 지면/소유자/연결 경계 누락");
    if (kind==kGeyser && (!hooks_.geyserRegistration || !hooks_.atPosition || !hooks_.unpop || !hooks_.repop))
        throw std::invalid_argument("가이저 등록/공간 경계 누락");
    if (kind>=kVortexRain && (!hooks_.typeDword || !hooks_.playImpactSound || !hooks_.setOwner || !hooks_.islandNumber ||
        !hooks_.islandExists || !hooks_.registerTemple || !hooks_.regionNumber || !hooks_.destroy || !hooks_.hasContent ||
        !hooks_.createSlot || !hooks_.refreshProduction || !hooks_.clearPlacement || (!patch && !hooks_.islandTemple)))
        throw std::invalid_argument("Vortex 전체 postPop 효과 누락");
    static_cast<void>(state_.residences.Items());static_cast<void>(state_.temples.Items());static_cast<void>(state_.geysers.Items());
}
// 각각의 확보가 실패해도 뒤의 다음 확보/공통 후처리를 계속한다.
void RawAssetPostPop::Regular(Sid sid,std::uint32_t event,float delay) const {
    if (hooks_.reserveRegular()) hooks_.addRegular(sid,event,delay);
}
// 실제 외부 효과 뒤 현재 extra/타입/HP를 읽어 원본 분기 순서를 유지한다.
void RawAssetPostPop::Gun(Sid sid,std::uint32_t flags,std::uint32_t kind) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto extra=patch ? 40U : 35U,word=patch ? 36U : 34U;
    if (kind==kGunAngle && (flags&7)) {
        const auto angle=patch ? std::bit_cast<std::int32_t>(Read(pool_.Slot(sid),word)) : pool_.Slot(sid)[word];
        const auto graph=hooks_.gunFrame(angle);hooks_.setGraph(sid,graph);
    }
    const bool branch=kind==kGunAngle ? (flags&7)!=0 : (flags&1)!=0;
    if (branch && (kind!=kGunSun || !(pool_.Slot(sid)[extra]&1))) {
        if (state_.inBattle && !(pool_.Slot(sid)[extra]&1)) {
            if (kind==kGunArcher) hooks_.removeAnimation(sid);
            hooks_.createGunProcess(sid);
            if (patch && state_.checkingForms) hooks_.checkForms(sid);
        }
        if (kind==kGunSun && state_.loadingDepth && (types_.at(pool_.Slot(sid)[10]).flags1&0x1000))
            Write(pool_.AllocatedBytes(sid),word,Read(pool_.Slot(sid),12,2),patch ? 4U : 1U);
    }
    if (kind==kGunSun && (types_.at(pool_.Slot(sid)[10]).flags1&0x1000))
        Write(pool_.AllocatedBytes(sid),12,Read(pool_.Slot(sid),word,patch ? 4U : 1U),2);
    damageable_.DamageablePostPop(sid,flags);
}
// Residence의 extra 차단은 없다. patch event3와 +42의 잔여 프레임은 CD에 없는 처리다.
void RawAssetPostPop::Residence(Sid sid,std::uint32_t flags) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (flags&1) {
        const auto raw=pool_.Slot(sid);const auto surface=hooks_.surface(Position(raw,14),Position(raw,18));
        if (surface.value) { const auto owner=pool_.Slot(surface)[patch ? 34 : 32];if (owner) hooks_.setOwner(sid,owner); }
        Add(state_.residences,sid,false);Regular(sid,0,0);Regular(sid,1,0);
        if (patch) Regular(sid,3,0);
        pool_.AllocatedBytes(sid)[12]&=0xf0;hooks_.notifySurface(sid);
        if (patch) {
            auto value=std::bit_cast<std::int32_t>(Read(pool_.Slot(sid),36));
            if (state_.residenceAnimation) {
                value=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>((value/2)*2)+(pool_.Slot(sid)[12]&15));
                const auto group=value<6 ? 0 : value<12 ? 1 : value<18 ? 2 : 3;
                value=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)-static_cast<std::uint32_t>(group*6));
            }
            Write(pool_.AllocatedBytes(sid),42,std::bit_cast<std::uint32_t>(value),4);
        }
    }
    base_.PostPopBase(sid,flags);
}
// 가이저의 비용이 실제 결정량 WORD가 된다. 주변 iterator는 원본 순서로 현재 raw 타입을 다시 읽는다.
void RawAssetPostPop::Geyser(Sid sid,std::uint32_t flags) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if ((flags&1) && !(pool_.Slot(sid)[patch ? 40 : 35]&9)) {
        Add(state_.geysers,sid,false);hooks_.geyserRegistration(sid,1);
        Write(pool_.AllocatedBytes(sid),12,Truncate(types_.at(pool_.Slot(sid)[10]).cost),2);Regular(sid,0,0.1f);
        const auto raw=pool_.Slot(sid);const auto neighbors=hooks_.atPosition(Position(raw,14),Position(raw,18));
        // 현재 지점에 걸린 섬 받침/종유석만 중립 프레임으로 다시 등록한다.
        for (const auto neighbor:neighbors) {
            const auto type=pool_.Slot(neighbor)[10];
            if (type==state_.islandType || type==state_.stalagType) {
                hooks_.unpop(neighbor,0);Write(pool_.AllocatedBytes(neighbor),patch ? 36U : 34U,8,patch ? 4U : 1U);hooks_.repop(neighbor,0x40);
            }
        }
    }
    damageable_.DamageablePostPop(sid,flags);
}
// 신전 공통몸체는 Damageable를 먼저 호출하여 공통 깊이를 줄인다. 그 뒤의 상태 변경도 원본 순서다.
void RawAssetPostPop::VortexBase(Sid sid,std::uint32_t flags) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto ownerOffset=patch ? 34U : 32U,extraOffset=patch ? 40U : 35U;
    damageable_.DamageablePostPop(sid,flags);
    if (flags&2) {
        const auto owner=pool_.Slot(sid)[ownerOffset];if (!owner || owner>kPlayerCount) throw std::logic_error("Vortex postPop 새 신전 소유자 assert");
        SetPlayerTemple(owner,2,sid.value);SetPlayerTemple(owner,3,sid.value);
    }
    if (!(flags&1)) return;
    ++state_.templeCount;
    if (!(pool_.Slot(sid)[extraOffset]&8)) {
        hooks_.playImpactSound(hooks_.typeDword(sid,0x74));Write(pool_.AllocatedBytes(sid),12,0,2);
        const auto owner=pool_.Slot(sid)[ownerOffset];
        if (owner && owner<=kPlayerCount && !PlayerTemple(owner,0) && !(pool_.Slot(sid)[extraOffset]&1)) {
            SetPlayerTemple(owner,0,sid.value);SetPlayerTemple(owner,1,sid.value);
        }
    }
    if (!(pool_.Slot(sid)[extraOffset]&9)) {
        Add(state_.temples,sid,true);Add(state_.residences,sid,true);
        if (state_.selected && pool_.Slot(sid)[ownerOffset]==LocalOwner()) {
            const auto form=pool_.Slot(Sid{static_cast<std::uint16_t>(state_.selected)});const auto type=Read(form,18,2);
            if (type<types_.size() && (types_[type].flags2&0x200)) hooks_.clearPlacement();
        }
        Write(pool_.AllocatedBytes(sid),patch ? 36U : 34U,hooks_.typeDword(sid,0x130),patch ? 4U : 1U);
        const auto raw=pool_.Slot(sid);const auto island=hooks_.islandNumber(Position(raw,14),Position(raw,18));
        if (!hooks_.islandExists(island)) throw std::logic_error("Vortex postPop islandList.exists assert");
        if (!patch && hooks_.islandTemple(island).value) throw std::logic_error("CD Vortex postPop 기존 신전 assert");
        hooks_.registerTemple(island,sid);hooks_.setOwner(sid,pool_.Slot(sid)[ownerOffset]);
        if (state_.boss) {
            auto current=2U;
            // 최초 번호는 해제 슬롯을 건너뛰며 첫 할당 자산/Form을 그대로 반환한다.
            while (current<pool_.Capacity() && (pool_.Slot(Sid{static_cast<std::uint16_t>(current)})[11]&1)) ++current;
            // 원본 iterator처럼 해제 슬롯을 건너뛰고 다음 번호를 삭제 호출 전에 미리 확보한다.
            while (current<pool_.Capacity()) {
                const auto other=Sid{static_cast<std::uint16_t>(current)};auto next=current+1;
                // 가상 삭제의 외부 효과가 다음 슬롯을 바꾸어도 원본 iterator의 현재 번호를 유지한다.
                while (next<pool_.Capacity() && (pool_.Slot(Sid{static_cast<std::uint16_t>(next)})[11]&1)) ++next;
                const auto otherRaw=pool_.Slot(other);const auto type=otherRaw[10];
                if (!(otherRaw[extraOffset]&8) && type<types_.size() && (types_[type].flags2&0x200) && other!=sid) {
                    auto x=Position(otherRaw,14),y=Position(otherRaw,18);
                    if (!patch) {
                        x=static_cast<float>(std::bit_cast<std::int32_t>(Truncate(x))/16);
                        y=static_cast<float>(std::bit_cast<std::int32_t>(Truncate(y))/16);
                    }
                    if (hooks_.regionNumber(x,y)==island) hooks_.destroy(other,0x100000);
                }
                current=next;
            }
        }
    }
    if (!state_.loadingDepth && !(pool_.Slot(sid)[extraOffset]&9)) {
        if (!hooks_.hasContent(sid,state_.slotType)) hooks_.createSlot(sid,state_.slotType);
        hooks_.refreshProduction();
    }
}
// 공통몸체가 현재 raw 값을 바꾼 다음 원소별 초기 추가 extra와 Regular 예약을 수행한다.
void RawAssetPostPop::Vortex(Sid sid,std::uint32_t flags,std::uint32_t kind) const {
    VortexBase(sid,flags);
    if (flags&1) {
        const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
        if (kind!=kVortexWind) Write(pool_.AllocatedBytes(sid),patch ? 36U : 34U,hooks_.typeDword(sid,0x134),patch ? 4U : 1U);
        Regular(sid,1,kind==kVortexThunder ? 0.1f : 0.08f);
    }
}
// 가상 표마다 복원한 몸체만 호출한다. 각 몸체는 공통 장부를 내부에서 정확히 한 번 호출한다.
void RawAssetPostPop::PostPop(Sid sid,std::uint32_t flags) const {
    Validate(sid,flags);const auto kind=Kind(sid);
    if (kind==kSunAviary) hooks_.sunAviaryPostPop(sid,flags);
    else if (kind==kRainAviary) hooks_.rainAviaryPostPop(sid,flags);
    else if (kind==kSunFence) hooks_.sunFencePostPop(sid,flags);
    else if (kind==kGenerator) damageable_.DamageablePostPop(sid,flags);
    else if (kind>=kGunAngle && kind<=kGunArcher) Gun(sid,flags,kind);
    else if (kind==kGunDecoration) { if (flags&1) Regular(sid,0,0.05f);base_.PostPopBase(sid,flags); }
    else if (kind==kResidence) Residence(sid,flags);
    else if (kind==kBlocker) { if (state_.inBattle && (flags&1) && !state_.loadingDepth) hooks_.startBlocker(sid);damageable_.DamageablePostPop(sid,flags); }
    else if (kind==kGeyser) Geyser(sid,flags);
    else Vortex(sid,flags,kind);
}
// 일반 Pop 분배와 같은 raw 저장소인지 검사하도록 반환한다.
const SidPool& RawAssetPostPop::Pool() const { return pool_; }
}
