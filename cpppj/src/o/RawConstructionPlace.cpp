// 건설 배치의 비용 판정·조각별 쓰기 순서와 패치판 비용 부족 처리/환불/취소 통지를 원본 순서대로 유지한다.
#include "o/RawConstructionPlace.h"
#include <algorithm>
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
        std::bit_cast<int>(request.direction),request.x,request.y,false},pool_.Edition(),state_.decoderDiagnostics);
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
        if (!state_.Server() && (sid.value<kFirstSid || sid.value>=serverFirst)) sid=hooks_.take(request.type,sid);
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
// 실제 wire 배치는 예약/free 슬롯도 원문처럼 접근하며 풀 바깥의 호스트 메모리만 거부한다.
std::span<std::uint8_t> RawConstructionPlace::ActualBytes(Sid sid) const { return std::span(pool_.bytes_).subspan(pool_.Offset(sid),pool_.Layout().stride); }
// 진단 장치가 선택한 현재 바이트/전역을 바꿔도 정상 복귀 뒤 같은 배치 순서를 계속한다.
void RawConstructionPlace::AssertActual(std::string_view expression,std::string_view file,std::uint32_t line) const {
    if (!hooks_.assertion) throw std::logic_error("실제 건설 배치의 Client 진단 장치 미연결");hooks_.assertion(expression,file,line);
}
// 고정 scalar/count와 lazy SID pointer의 실제 실행 경로이며 typed Place의 선행 전수 검사는 사용하지 않는다.
void RawConstructionPlace::ActualPlace(const ConstructionPlaceRequest& request,std::uint32_t declaredCount,const ConstructionSidReader& sidReader) {
    RequirePatch();if (!sidReader) throw std::invalid_argument("실제 건설 배치의 현재 SID reader 없음");
    if (request.type>=types_.size()) throw std::out_of_range("실제 건설 배치 TYPE 물리 범위");
    const auto& selectedType=types_[request.type]; //49a840은 player 진단/decoder보다 먼저 고른 같은 TYPE 레코드 주소다.
    if (request.player==kServerPlayer) AssertActual("playerId != GAME_SERVER_ID","\\Ns\\O\\Construction.cpp",0x273);
    if (request.type>=frames_.size()) throw std::out_of_range("실제 건설 배치 frame TYPE 물리 범위");
    const auto& meta=frames_[request.type];const auto patternTypes=state_.CurrentPatternTypes(); //진단 복귀 뒤 현재 패턴 번호로 생성하되 반복자는 한 번만 만든다.
    auto decoder=DecodeCanonType(meta.frames,meta.defaultFrame,patternTypes,{request.type,std::bit_cast<int>(request.argument),std::bit_cast<int>(request.direction),request.x,request.y,false},pool_.Edition(),state_.decoderDiagnostics);
    const auto player=std::bit_cast<std::int32_t>(request.player); //원문 비용 범위는 signed DWORD다.
    if (request.player!=state_.LocalPlayer() && !(state_.chargeBlockedRead ? state_.chargeBlockedRead() : state_.chargeBlocked) && player>0 && player<static_cast<std::int32_t>(kServerPlayer) && selectedType.cost!=0.0F && request.type!=state_.NuggetType()) {
        const auto money=Money(request.player); //SP 조회 뒤 비용은 진입 때 선택한 같은 TYPE에서 다시 읽는다.
        if (!(money>=selectedType.cost)) ChargeShortfall(request.type,request.player,request.x,request.y,request.argument,request.direction);
        else AddMoney(request.player,-selectedType.cost);
    }
    const auto serverFirst=pool_.Layout().serverFirst;const auto popFlags=request.abstract ? kPopAbstract:kPopPlaced; //원문 별도 local1c의1/2에 확정 배치40h를 합친 값이다.
    std::uint32_t index{}; //패킷의 선언 개수와 별도로 decoder가 실제 소비한 SID 수다.
    for (;decoder.Valid();decoder.Advance()) { //개수 선검사 없이 비용/앞 조각의 부분 효과 뒤 다음 현재 SID WORD를 읽는다.
        Sid sid{sidReader(index++)};
        if (!state_.Server() && (sid.value<kFirstSid || sid.value>=serverFirst)) sid=hooks_.take(request.type,sid);
        auto raw=ActualBytes(sid);if (!(raw[kState]&kVoid)) { AssertActual("isVoid()","c:\\ns\\o\\squid.h",0x282);raw=ActualBytes(sid); }
        raw[kPatchExtra]=static_cast<std::uint8_t>((raw[kPatchExtra]&0xfe)|(request.abstract&1U));
        if (reward_.HasHitPoints(sid)) { const auto hp=request.abstract ? 0U:std::bit_cast<std::uint32_t>(reward_.MaxHitPoints(sid));Put(ActualBytes(sid),kHp,hp,4); }
        const auto frame=decoder.Frame();Put(ActualBytes(sid),kPatchFrame,std::bit_cast<std::uint32_t>(frame),4); //decoder가 보유한 현재 frame을 쓴다.
        const float x=decoder.X(),y=decoder.Y();const bool origin=request.type==state_.NoIslandType() && x==request.x && y==request.y; //원문의 ordered 좌표 같음이다.
        Put(ActualBytes(sid),kWord,origin ? kOriginWord:static_cast<std::uint16_t>(request.argument),2);
        if (ActualBytes(sid)[kType]==state_.BridgeType()) hooks_.notifySurface(sid); //4ac220은 방금 쓴 조각의 현재 TYPE와 현재 다리 번호를 비교한다.
        if (request.type==state_.BridgeType()) {
            std::uint32_t qualityFlags{};std::int32_t life{}; //원문의 품질 flag DWORD와 실제 수명 char를 따로 보유한다.
            if (request.quality==kQualityCracked) { qualityFlags=kCrackedFlag;life=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(std::bit_cast<std::uint32_t>(state_.BridgeLife())-1U)); }
            else if (request.quality==kQualityHard) qualityFlags=kHardFlag;
            const auto bridge=state_.BridgeType();if (bridge>=frames_.size()) throw std::out_of_range("실제 건설 품질의 현재 다리 TYPE 물리 범위");
            const auto& bridgeFrames=frames_[bridge].frames;const auto codes=bridgeFrames.Codes(); //표면 통지 뒤 현재 다리 TYPE의 코드 표를 선택한다.
            if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("실제 건설 품질의 코드 물리 범위");
            const auto code=codes[static_cast<std::size_t>(frame)];if (static_cast<std::int8_t>(code.side)>kLastPlankSide) life=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(std::bit_cast<std::uint32_t>(state_.BridgeLife())-1U));
            const auto quality=bridgeFrames.FindFlags(code.side,code.variant,static_cast<std::int32_t>(static_cast<std::int8_t>(code.flags))|std::bit_cast<std::int32_t>(qualityFlags)); //같은 current code/품질로 원문49aa30을 찾는다.
            if (quality<0) AssertActual("0","\\Ns\\O\\Rifttype.cpp",0x16a); //원문 진단 복귀 뒤 반환값-1도 실제 frame에 쓴다.
            raw=ActualBytes(sid);Put(raw,kPatchFrame,std::bit_cast<std::uint32_t>(quality),4);
            const auto word=static_cast<std::uint16_t>(raw[kWord]|(std::uint32_t(raw[kWord+1])<<8));const auto mixed=static_cast<std::uint16_t>(word^((static_cast<std::uint16_t>(std::bit_cast<std::uint32_t>(life)<<3)^word)&kLifeMask)); //수명 비트3..6만 바꾼다.
            Put(raw,kWord,mixed,2);hooks_.notifySurface(sid);
        }
        hooks_.setOwner(sid,request.player);hooks_.pop(sid,x,y,popFlags);
    }
    if (index!=declaredCount) AssertActual("i == sidCount","\\Ns\\O\\Construction.cpp",0x2f2); //모든 decoder 칸의 부분 효과가 끝난 뒤 진단한다.
}
// 로컬 플레이어·차단 전역·범위 밖 번호·비용 0·nugget은 판정하지 않는다. 잔액 비교는 x87과 같이 NaN을 "모자람"으로 본다.
void RawConstructionPlace::ChargeForPlacement(const ConstructionPlaceRequest& request) {
    const auto player=std::bit_cast<std::int32_t>(request.player);
    if (request.player==state_.localPlayer || (state_.chargeBlockedRead ? state_.chargeBlockedRead() : state_.chargeBlocked) || player<=0 || player>=static_cast<std::int32_t>(kServerPlayer)) return; // 같은 원문의 short-circuit 순서에서만 현재 전역을 조회한다.
    const float cost=types_[request.type].cost;
    if (cost==0.0F || request.type==state_.NuggetType()) return;
    if (!(Money(request.player)>=cost)) ChargeShortfall(request.type,request.player,request.x,request.y,request.argument,request.direction);
    else AddMoney(request.player,-cost);
}
// 00442b50: 표시 비트를 먼저 읽어 두고 켠 뒤 권한 여부로 갈린다. 권한 쪽은 SP를 빼지 않고 환불/취소 통지를 보낸다.
void RawConstructionPlace::ChargeShortfall(std::uint32_t type,std::uint32_t player,float x,float y,std::uint32_t argument,std::uint32_t direction) {
    RequirePatch();
    const float cost=Type(type).cost;
    if (player>static_cast<std::uint32_t>(kPlayerCount)) throw std::out_of_range("건설 비용 처리 플레이어 번호 오류");
    const auto offset=static_cast<std::size_t>(player)*0xb4; // 현재 Player의 원본 stride다.
    auto flags=state_.playerFlags[player]; // 값형 원문 fixture의 입력이며 실제 연결에서는 현재 raw로 교체한다.
    if (!players_.empty()) { flags=0;
        // 현재 비정렬 DWORD를 little endian으로 읽는다.
        for (std::size_t i=0;i<4;++i) flags|=static_cast<std::uint32_t>(players_[offset+0x54+i])<<(8*i);
    }
    const bool reported=(flags&kShortfallFlag)!=0;flags|=kShortfallFlag;
    if (players_.empty()) state_.playerFlags[player]=flags;
    else {
        // 표시 비트 외의 현재 Player 비트를 모두 보존한다.
        for (std::size_t i=0;i<4;++i) players_[offset+0x54+i]=static_cast<std::uint8_t>(flags>>(8*i));
    }
    const auto name=[this,offset,player] { // 안내를 선택하는 현재 위치에서만 실제 원본 이름을 읽는다.
        if (players_.empty()) return state_.playerNames[player];
        const auto begin=players_.begin()+offset+0xc,end=players_.begin()+offset+0x2c; // 원본 Player 이름은+c부터subscriber+2c 직전의32바이트다.
        const auto terminator=std::find(begin,end,0); // 정상 Player 이름의 종료를 인접 필드까지 넘겨 찾지 않는다.
        if (terminator==end) throw std::out_of_range("건설 비용 안내 Player 이름의 NUL 누락");return std::string(begin,terminator);
    };
    if (state_.Authority()) {
        // 권한 쪽: 처음이면 다른 플레이어에게 알리고, 당사자에게 알린 뒤 좌표가 유효하면 환불과 취소 통지를 보낸다.
        if (!reported) hooks_.tellOthers(player,hooks_.format(kClientCheated,name()));
        hooks_.tellPlayer(player,kServerThinks);
        if (ValidPosition(x,y)) Reject(type,player,x,y,argument,direction);
        return;
    }
    // 비권한 쪽: 잔액과 무관하게 차감하고 처음이면 로컬 안내를 띄운다.
    AddMoney(player,-cost);
    if (!reported) hooks_.tell(hooks_.format(kServerCheated,name()));
}
// 00442a40 / CD 004d1bc0: 비용을 정수로 절삭해 돌려주고 양수이면 SP 통지를 보낸다. CD판은 통지만 한다.
void RawConstructionPlace::Refund(std::uint32_t type,std::uint32_t player) {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto& record=Type(type);
    if (patch && type==state_.NuggetType()) return;
    const std::int32_t amount=Ftol(static_cast<double>(record.cost));
    // 패치판만 SP를 돌려준다. 전투 표시 중의 로컬 플레이어는 가산을 건너뛴다.
    if (patch && !(state_.BattleShown() && player==state_.LocalPlayer())) AddMoney(player,static_cast<float>(amount));
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
    if (state_.BattleShown() && player==state_.LocalPlayer()) return hooks_.storedSp(state_.LocalPlayer());
    const auto number=std::bit_cast<std::int32_t>(player);
    return number>0 && number<static_cast<std::int32_t>(kServerPlayer) ? hooks_.storedSp(player) : 0.0F;
}
// 0041dff0: 로컬 플레이어이면 로컬 가산, 아니면 플레이어 가산으로 넘긴다.
void RawConstructionPlace::AddMoney(std::uint32_t player,float delta) {
    RequirePatch();
    if (player==state_.LocalPlayer()) hooks_.addLocalSp(delta);
    else hooks_.addPlayerSp(player,delta);
}
// 004ac220 / CD 004abb00: 단어를 쓴 뒤 객체의 타입 바이트가 다리 타입이면 표면 알림을 부른다.
void RawConstructionPlace::SetWord(Sid sid,std::uint16_t word) {
    auto raw=pool_.AllocatedBytes(sid);
    Put(raw,kWord,word,2);
    if (static_cast<std::uint32_t>(raw[kType])==state_.BridgeType()) hooks_.notifySurface(sid);
}
// 실제 Player 기록은 Session보다 오래 살아야 하며 PATCH의9개 구조체 크기를 요구한다.
void RawConstructionPlace::BindPlayerRecords(std::span<std::uint8_t> players) {
    RequirePatch();if (players.size()!=9*0xb4) throw std::invalid_argument("건설 배치의 실제 Player 기록 크기 오류");players_=players;
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
        hooks.addLocalSp=[&reward,&state](float delta) { reward.AddSp(state.LocalPlayer(),delta); }; //실제 공유 로컬 번호는 가산 선택 뒤에도 같은 소비 위치에서 읽는다.
        hooks.addPlayerSp=[&reward](std::uint32_t player,float delta) { reward.AddSp(player,delta); };
    }
    return hooks;
}
}
