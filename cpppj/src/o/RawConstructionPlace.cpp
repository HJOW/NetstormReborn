// 건설 배치의 비용 판정·조각별 쓰기 순서와 패치판 비용 부족 처리/환불/취소 통지를 원본 순서대로 유지한다.
#include "o/RawConstructionPlace.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 두 판본 공통 raw 필드: 타입·상태 바이트, +0xc 단어, HP(패치 DWORD / CD WORD)의 위치다.
constexpr std::size_t kType=10,kState=11,kWord=12,kHp=26;
// 판본별 abstract 비트 바이트와 프레임 필드(패치 DWORD / CD BYTE)의 위치다.
constexpr std::size_t kPatchExtra=40,kCdExtra=35,kPatchFrame=36,kCdFrame=34;
// 상태 바이트의 void 비트와 일반 객체의 첫 번호다. 번호 0~4는 풀의 예약 슬롯이다.
constexpr std::uint8_t kVoid=4;
constexpr std::uint16_t kFirstSid=5;
// 배치할 수 없는 서버 플레이어 번호(GAME_SERVER_ID)다.
constexpr std::uint32_t kServerPlayer=9;
// Player +0x54의 비용 부족 표시 비트와 noIsland 시작 칸에 쓰는 단어다.
constexpr std::uint32_t kShortfallFlag=0x400;
constexpr std::uint16_t kOriginWord=0x5d4;
// Pop 플래그: 확정 배치는 1 | 0x40, abstract 인자가 0이 아닌 배치는 2다.
constexpr std::uint32_t kPopPlaced=0x41,kPopAbstract=2;
// 다리 품질 인자와 그 프레임 플래그, 단어의 수명 비트(bit 3~6), 끝 칸 판정의 경계 글자다.
constexpr std::uint32_t kQualityCracked=1,kQualityHard=3,kCrackedFlag=0x20,kHardFlag=0x40;
constexpr std::uint16_t kLifeMask=0x78;
constexpr char kLastPlankSide='K';
// 0040e800의 좌표 상한(DAT_00531928 = 256)이다. 0 초과 256 미만만 유효하다.
constexpr float kMapLimit=256.0F;
// 비용 부족 안내의 서식 key와 고정 문구다(패치 005071a0, 00507174, 00507188).
constexpr std::string_view kClientCheated="ClientCheated(%s)",kServerCheated="ServerCheated(%s)",kServerThinks="ServerThinksYouCheated";
// 원본 __ftol: 0방향으로 절삭한 64비트 정수의 하위 32비트다. NaN·무한대·64비트 범위 밖은 x87 정수 불확정값(0x8000000000000000)이라 하위 DWORD가 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 정렬되지 않은 little endian 필드를 폭만큼 쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 낮은 바이트부터 쓴다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 0040e800: 두 좌표가 모두 0 초과 지도 크기 미만인지 본다. NaN은 유효하지 않다.
bool ValidPosition(float x,float y) { return 0.0F<x && x<kMapLimit && 0.0F<y && y<kMapLimit; }
}
// 풀 일치와 필수 경계는 첫 효과 전에 검사한다.
RawConstructionPlace::RawConstructionPlace(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const PriestPlainCanonType> frames,
    ConstructionPlaceState& state,const SquidReward& reward,ConstructionPlaceHooks hooks)
    :pool_(pool),types_(types),frames_(frames),state_(state),reward_(reward),hooks_(std::move(hooks)) {
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;
    if (&pool!=&reward.Pool() || !hooks_.take || !hooks_.notifySurface || !hooks_.setOwner || !hooks_.pop || !hooks_.sendMoney ||
        (patch && (!hooks_.storedSp || !hooks_.addLocalSp || !hooks_.addPlayerSp || !hooks_.format || !hooks_.tellOthers ||
                   !hooks_.tellPlayer || !hooks_.tell || !hooks_.sendReject)))
        throw std::invalid_argument("건설 배치 풀/필수 효과 연결 오류");
}
// 패치판에만 있는 몸체의 진입 검사다. CD판 풀에서 부르면 원본에 없는 동작이므로 거부한다.
void RawConstructionPlace::RequirePatch() const {
    if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::logic_error("패치판에만 있는 건설 비용 처리다");
}
// 타입 번호를 표 크기와 타입 바이트 범위로 검사한 뒤 레코드를 돌려준다. 주소 산술 전에 부른다.
const RiftTypeRecord& RawConstructionPlace::Type(std::uint32_t type) const {
    if (type>=types_.size() || type>=frames_.size() || type>0xff) throw std::out_of_range("건설 배치 타입 번호 오류");
    return types_[type];
}
// 조각 수를 먼저 세어 개수 불일치를 효과 전에 거부한 뒤 원본 순서(비용 판정 → 조각 순회)로 실행한다.
void RawConstructionPlace::Place(const ConstructionPlaceRequest& request) {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    Type(request.type);
    if (request.player==kServerPlayer) throw std::logic_error("건설 배치 플레이어가 서버 번호다(playerId != GAME_SERVER_ID assert)");
    const auto& meta=frames_[request.type];
    auto decoder=DecodeCanonType(meta.frames,meta.defaultFrame,state_.patternTypes,{request.type,std::bit_cast<int>(request.argument),
        std::bit_cast<int>(request.direction),request.x,request.y,false},pool_.Edition());
    std::size_t pieces=0;
    // 사본으로 유효 칸을 끝까지 세어 본다. 원본 반복자는 건드리지 않는다.
    for (auto probe=decoder;probe.Valid();probe.Advance()) ++pieces;
    if (pieces!=request.sids.size()) throw std::invalid_argument("건설 배치 SID 개수가 조각 수와 다르다(i == sidCount assert)");
    if (patch) ChargeForPlacement(request);
    const std::size_t extraOffset=patch ? kPatchExtra : kCdExtra;
    const std::uint32_t popFlags=request.abstract ? kPopAbstract : kPopPlaced;
    const std::uint32_t serverFirst=pool_.Layout().serverFirst;
    const auto codes=meta.frames.Codes();
    std::size_t index=0;
    // 유효 칸마다 조각 하나를 놓는다. 한 조각의 쓰기 순서는 abstract 비트 → HP → 프레임 → 단어 → (다리 품질) → 소유자 → Pop이다.
    for (;decoder.Valid();decoder.Advance()) {
        Sid sid{request.sids[index++]};
        if (!state_.server && (sid.value<kFirstSid || sid.value>=serverFirst)) sid=hooks_.take(request.type,sid);
        auto raw=pool_.AllocatedBytes(sid);
        if (!(raw[kState]&kVoid)) throw std::logic_error("건설 배치 조각이 void가 아니다(isVoid assert)");
        raw[extraOffset]=static_cast<std::uint8_t>((raw[extraOffset]&0xfe)|(request.abstract&1));
        // HP 유무는 방금 쓴 abstract 비트를 읽는다(dais 부류). abstract 인자가 0이 아니면 HP 0으로 시작한다.
        if (reward_.HasHitPoints(sid)) {
            const auto hp=request.abstract ? 0U : std::bit_cast<std::uint32_t>(reward_.MaxHitPoints(sid));
            Put(raw,kHp,hp,patch ? 4 : 2);
        }
        const int frame=decoder.Frame();
        Put(raw,patch ? kPatchFrame : kCdFrame,std::bit_cast<std::uint32_t>(frame),patch ? 4 : 1);
        // noIsland의 시작 좌표 칸만 단어 0x5d4를 받는다. 패치판은 순서 있는 같음, CD판은 같거나 비교 불가일 때다.
        const float x=decoder.X(),y=decoder.Y();
        const bool origin=request.type==state_.noIslandType &&
            (patch ? (x==request.x && y==request.y) : (!(x<request.x) && !(x>request.x) && !(y<request.y) && !(y>request.y)));
        SetWord(sid,origin ? kOriginWord : static_cast<std::uint16_t>(request.argument));
        if (request.type==state_.bridgeType) {
            if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("다리 조각 프레임 범위 오류");
            std::uint32_t qualityFlags=0;std::int32_t life=0;
            if (request.quality==kQualityCracked) { qualityFlags=kCrackedFlag;life=state_.bridgeLife-1; }
            else if (request.quality==kQualityHard) qualityFlags=kHardFlag;
            // 방향 글자가 K보다 뒤(끝 칸 L~O 등)이면 품질과 무관하게 수명이 하나 준다. 글자는 signed char로 비교한다.
            const auto code=codes[static_cast<std::size_t>(frame)];
            if (static_cast<std::int8_t>(code.side)>kLastPlankSide) life=state_.bridgeLife-1;
            const int quality=meta.frames.FindFlags(code.side,code.variant,
                static_cast<std::int32_t>(static_cast<std::int8_t>(code.flags))|std::bit_cast<std::int32_t>(qualityFlags));
            if (quality<0) throw std::logic_error("다리 품질 프레임이 타입에 없다(Rifttype.cpp assert)");
            Put(raw,patch ? kPatchFrame : kCdFrame,std::bit_cast<std::uint32_t>(quality),patch ? 4 : 1);
            // 단어의 bit 3~6만 수명으로 바꾼다.
            const auto word=static_cast<std::uint16_t>(raw[kWord]|(raw[kWord+1]<<8));
            const auto mixed=static_cast<std::uint16_t>(word^((static_cast<std::uint16_t>(std::bit_cast<std::uint32_t>(life)<<3)^word)&kLifeMask));
            Put(raw,kWord,mixed,2);
            hooks_.notifySurface(sid);
        }
        hooks_.setOwner(sid,request.player);
        hooks_.pop(sid,x,y,popFlags);
    }
}
// 로컬 플레이어·차단 전역·범위 밖 번호·비용 0·nugget은 판정하지 않는다. 잔액 비교는 x87과 같이 NaN을 "모자람"으로 본다.
void RawConstructionPlace::ChargeForPlacement(const ConstructionPlaceRequest& request) {
    const auto player=std::bit_cast<std::int32_t>(request.player);
    if (request.player==state_.localPlayer || state_.chargeBlocked || player<=0 || player>=static_cast<std::int32_t>(kServerPlayer)) return;
    const float cost=types_[request.type].cost;
    if (cost==0.0F || request.type==state_.nuggetType) return;
    if (!(Money(request.player)>=cost)) ChargeShortfall(request.type,request.player,request.x,request.y,request.argument,request.direction);
    else AddMoney(request.player,-cost);
}
// 00442b50: 표시 비트를 먼저 읽어 두고 켠 뒤 권한 여부로 갈린다. 권한 쪽은 SP를 빼지 않고 환불/취소 통지를 보낸다.
void RawConstructionPlace::ChargeShortfall(std::uint32_t type,std::uint32_t player,float x,float y,std::uint32_t argument,std::uint32_t direction) {
    RequirePatch();
    const float cost=Type(type).cost;
    if (player>static_cast<std::uint32_t>(kPlayerCount)) throw std::out_of_range("건설 비용 처리 플레이어 번호 오류");
    auto& flags=state_.playerFlags[player];
    const bool reported=(flags&kShortfallFlag)!=0;
    flags|=kShortfallFlag;
    if (state_.authority) {
        // 권한 쪽: 처음이면 다른 플레이어에게 알리고, 당사자에게 알린 뒤 좌표가 유효하면 환불과 취소 통지를 보낸다.
        if (!reported) hooks_.tellOthers(player,hooks_.format(kClientCheated,state_.playerNames[player]));
        hooks_.tellPlayer(player,kServerThinks);
        if (ValidPosition(x,y)) Reject(type,player,x,y,argument,direction);
        return;
    }
    // 비권한 쪽: 잔액과 무관하게 차감하고 처음이면 로컬 안내를 띄운다.
    AddMoney(player,-cost);
    if (!reported) hooks_.tell(hooks_.format(kServerCheated,state_.playerNames[player]));
}
// 00442a40 / CD 004d1bc0: 비용을 정수로 절삭해 돌려주고 양수이면 SP 통지를 보낸다. CD판은 통지만 한다.
void RawConstructionPlace::Refund(std::uint32_t type,std::uint32_t player) {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto& record=Type(type);
    if (patch && type==state_.nuggetType) return;
    const std::int32_t amount=Ftol(static_cast<double>(record.cost));
    // 패치판만 SP를 돌려준다. 전투 표시 중의 로컬 플레이어는 가산을 건너뛴다.
    if (patch && !(state_.battleShown && player==state_.localPlayer)) AddMoney(player,static_cast<float>(amount));
    if (amount>0) hooks_.sendMoney(player,{static_cast<std::uint8_t>(player),static_cast<std::uint8_t>(player),1,amount});
}
// 00442af0: 환불 뒤 좌표·타입·두 인자의 하위 바이트를 실은 취소 통지를 보낸다.
void RawConstructionPlace::Reject(std::uint32_t type,std::uint32_t player,float x,float y,std::uint32_t argument,std::uint32_t direction) {
    RequirePatch();
    Refund(type,player);
    hooks_.sendReject(player,{x,y,static_cast<std::uint8_t>(type),static_cast<std::uint8_t>(argument),static_cast<std::uint8_t>(direction)});
}
// 0040ef30: 전투 표시 중의 로컬 플레이어는 번호 검사 없이, 그 밖에는 1~8일 때만 저장소를 읽는다.
float RawConstructionPlace::Money(std::uint32_t player) const {
    RequirePatch();
    if (state_.battleShown && player==state_.localPlayer) return hooks_.storedSp(state_.localPlayer);
    const auto number=std::bit_cast<std::int32_t>(player);
    return number>0 && number<static_cast<std::int32_t>(kServerPlayer) ? hooks_.storedSp(player) : 0.0F;
}
// 0041dff0: 로컬 플레이어이면 로컬 가산, 아니면 플레이어 가산으로 넘긴다.
void RawConstructionPlace::AddMoney(std::uint32_t player,float delta) {
    RequirePatch();
    if (player==state_.localPlayer) hooks_.addLocalSp(delta);
    else hooks_.addPlayerSp(player,delta);
}
// 004ac220 / CD 004abb00: 단어를 쓴 뒤 객체의 타입 바이트가 다리 타입이면 표면 알림을 부른다.
void RawConstructionPlace::SetWord(Sid sid,std::uint16_t word) {
    auto raw=pool_.AllocatedBytes(sid);
    Put(raw,kWord,word,2);
    if (static_cast<std::uint32_t>(raw[kType])==state_.bridgeType) hooks_.notifySurface(sid);
}
// 어댑터와 호출자가 같은 풀을 쓰는지 확인할 수 있게 실제 풀을 돌려준다.
const SidPool& RawConstructionPlace::Pool() const { return pool_; }
// 같은 풀의 실제 수신/소유자/SP 모듈만 바꿔 끼운다. Pop과 통지 경계는 호출자가 준 것을 유지한다.
ConstructionPlaceHooks MakeConstructionPlaceHooks(const SidPool& pool,SquidFactory& factory,SquidOwner& owner,SquidReward& reward,
    ScrambledSpStore* store,const ConstructionPlaceState& state,ConstructionPlaceHooks hooks) {
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;
    if (&pool!=&factory.Pool() || &pool!=&owner.Pool() || &pool!=&reward.Pool() || (patch && !store))
        throw std::invalid_argument("건설 배치/factory/소유자/보상 SID 풀 또는 SP 저장소 연결 오류");
    hooks.take=[&factory](std::uint32_t type,Sid sid) { return factory.Take(type,sid); };
    hooks.setOwner=[&owner](Sid sid,std::uint32_t player) { owner.Set(sid,player); };
    if (patch) {
        hooks.storedSp=[store](std::uint32_t player) { return std::bit_cast<float>(store->Get(player)); };
        hooks.addLocalSp=[&reward,&state](float delta) { reward.AddSp(state.localPlayer,delta); };
        hooks.addPlayerSp=[&reward](std::uint32_t player,float delta) { reward.AddSp(player,delta); };
    }
    return hooks;
}
}
