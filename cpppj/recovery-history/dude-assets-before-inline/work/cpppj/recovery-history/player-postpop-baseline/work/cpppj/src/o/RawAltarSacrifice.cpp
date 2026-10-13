// 실제 알터 사건과 다이스 전처리를 복원하고 기존 조회/프레임/Host/공통 삭제를 재사용한다.
#include "o/RawAltarSacrifice.h"
#include "o/RawAssetRegular.h"
#include "o/SquidFactory.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::o {
namespace {
constexpr float kStrugglePayload=0.08F,kFinishedDelay=3.0F,kReleaseOffset=0.5F; // 실제PE 3da3d70a/40400000/3f000000 비트다.
constexpr std::uint32_t kSacrificeDestroy=0x400000,kAltarDestroy=0x48; // 최종 희생 완료와 알터 삭제의 실제 flags다.
constexpr std::array<std::string_view,5> kChains{{"chainRattle23.wav","chainRattle24.wav","chainRattle25.wav","chainRattle26.wav","chainRattle27.wav"}}; // 두 판본의 실제 선택표 첫5개다.
constexpr std::array<std::string_view,3> kStruggles{{"priestStruggleFade01.wav","priestStruggleFade02.wav","priestStruggleFade03.wav"}}; // 원본 난수/1000 선택표다.
// 비정렬 little endian의 필요한 필드 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 인접 필드를 섞지 않고 WORD/DWORD를 조합한다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
// 정상 지도 좌표를 원본 CRT처럼0 방향으로 절삭한다.
int Cell(float value) {
    if (!std::isfinite(value) || static_cast<double>(value)<-2147483648.0 || static_cast<double>(value)>=2147483648.0) throw std::out_of_range("알터 원본 좌표 절삭 범위");
    return static_cast<int>(value);
}
}
// 실제 몸체/같은 풀/필수 효과가 모두 존재해야 첫 원본 쓰기를 허용한다.
RawAltarSacrifice::RawAltarSacrifice(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    const RawSquidNeighbors& footprint,const RawRunePostPop& rune,const RawTempleRitual& temple,const RawCarrierPreDestroy& carrier,
    const RawDaisRegular& dais,const RawFrameAdvance& advance,SquidProcessHost& host,const RawDamageablePreDestroy& damageable,
    DaisRegularState& daisState,AltarSacrificeState& state,AltarSacrificeHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),footprint_(footprint),rune_(rune),temple_(temple),carrier_(carrier),dais_(dais),advance_(advance),host_(host),damageable_(damageable),daisState_(daisState),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&footprint.Pool() || &pool!=&rune.Pool() || &pool!=&temple.Pool() || &pool!=&carrier.Pool() || &pool!=&dais.Pool() || &pool!=&advance.Pool() || &pool!=&host.Pool() || &pool!=&damageable.Pool()
        || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size()
        || !hooks_.setFrame || !hooks_.destroy || !hooks_.reserveShared || !hooks_.addShared || !hooks_.randomInt || !hooks_.randomFloat || !hooks_.playAt || !hooks_.playGlobal || !hooks_.removeAnimation || !hooks_.unpop || !hooks_.repop || !hooks_.create || !hooks_.setOwner || !hooks_.pop || !hooks_.escapeEffect || !hooks_.refreshOwner || !hooks_.damageable)
        throw std::invalid_argument("알터 실제 하위 몸체/같은 풀/필수 효과 연결 누락");
}
// 원본은 free/dead를 isValid 조건에 추가하지 않는다.
bool RawAltarSacrifice::Valid(Sid sid) const { return sid.value>0 && sid.value<pool_.Capacity(); }
// 패치 signed DWORD와 CD BYTE를 구분한다.
std::int32_t RawAltarSacrifice::Frame(Sid sid) const { const auto raw=pool_.Slot(sid);return pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,36)) : raw[34]; }
// 현재 raw타입을 읽어 실제 코드표의 방향을 반환한다.
std::int32_t RawAltarSacrifice::Direction(Sid sid) const { return RawPathAnimation::Direction(frames_[pool_.Slot(sid)[10]],Frame(sid)); }
// 현재 타입의 실제 글자 구간 첫번호다.
std::int32_t RawAltarSacrifice::First(Sid sid,std::uint8_t letter) const { return frames_[pool_.Slot(sid)[10]].Run(letter).first; }
// 표시 setter를 호출하지 않는 원본 해방의 plain 쓰기다.
void RawAltarSacrifice::WriteFrame(Sid sid,std::int32_t value) const {
    auto raw=pool_.AllocatedBytes(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto offset=patch ? 36U : 34U;const auto width=patch ? 4U : 1U;const auto bits=std::bit_cast<std::uint32_t>(value);
    // 정확한 판본 필드 폭만 기록한다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(bits>>(index*8));
}
// 호출 뒤 좌표가 바뀌었으면 다음 위치 효과는 새 값을 읽는다.
std::array<float,2> RawAltarSacrifice::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 실제 최종 가상표를 쓰는 알터만 처리한다.
bool RawAltarSacrifice::Handles(Sid altar) const { const auto raw=pool_.Slot(altar);return raw[10]==daisState_.rune.altarType && Read(raw,0)==(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchAltarVtable : kCdAltarVtable); }
// 기준점typed helper00448bb0/CD00483c50은 추가 owner/dead/free 필터를 적용하지 않는다.
Sid RawAltarSacrifice::DaisAt(Sid sid) const {
    const auto point=Position(sid);const auto type=daisState_.rune.daisType;if (type>=types_.size()) throw std::out_of_range("알터 현재 다이스 타입 범위");const int x=Cell(point[0]),y=Cell(point[1]);RawSquidFinder finder(pool_,hash_,types_);
    // 일반 finder의 현재 버킷과 buried 조건을 재사용한다.
    for (auto current=finder.Begin({x,y,x,y},(types_[type].flags2&6)==0 ? 1U : 0U);current.value;current=finder.Next()) if (pool_.Slot(current)[10]==type) return current;
    return {};
}
// 원본 specialized lookup은 kindWORD/순번0/삭제false로만 호출된다. 공통 Carrier의 같은 body를 사용한다.
Sid RawAltarSacrifice::ContainedPriest(Sid dais) const {
    if (!daisState_.ritual.priestType) throw std::logic_error("알터 specialized kind0은 일반 Carrier wildcard와 다르다");
    return carrier_.FindContained(dais,0,daisState_.ritual.priestType,0,false);
}
// 알터+80은 같은 기준점 다이스의 기존 가상 개수를 반환한다.
std::uint32_t RawAltarSacrifice::Count(Sid altar) const { const auto dais=DaisAt(altar);return Valid(dais) ? temple_.Count(dais) : 0; }
// 첫 프레임/길이는 현재raw타입에서 읽으며 count>=3의 난수 소비도 생략하지 않는다.
void RawAltarSacrifice::EscapeFrame(Sid dais,std::uint32_t count) const {
    auto direction=Direction(dais);const auto type=pool_.Slot(dais)[10];
    if (count<3) { if (!count) direction=14-static_cast<std::int32_t>(temple_.Count(dais));hooks_.setFrame(dais,frames_[type].Run(static_cast<std::uint8_t>(direction+'A')).first+static_cast<std::int32_t>(count),0);return; }
    const auto length=frames_[type].Run(static_cast<std::uint8_t>(direction+'A')).count;static_cast<void>(hooks_.randomInt(3000,length*1000));
}
// 해방의 표시 없는 프레임 쓰기는 일반 Frame.Set으로 대체하지 않는다.
void RawAltarSacrifice::ReleaseFromAltar(Sid altar) const {
    const auto dais=DaisAt(altar);if (pool_.Edition()==OriginalEdition::Patch1078 && daisState_.debug && (Direction(altar)==12 || Direction(altar)==11)) throw std::logic_error("알터 해방 invisible/sacrifice assert");
    if (Valid(dais)) ReleasePriest(dais);hooks_.unpop(altar,0);WriteFrame(altar,First(altar,'P'));hooks_.repop(altar,0);
}
// 두 번 중심 조회와 가상효과 뒤의 현재raw 재읽기를 원본 순서대로 수행한다.
void RawAltarSacrifice::ReleasePriest(Sid dais) const {
    const auto altar=rune_.AltarAt(dais);if (!Valid(altar)) throw std::logic_error("다이스 해방 altar.isValid assert");const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (patch && daisState_.debug && (Direction(altar)==12 || Direction(altar)==11)) throw std::logic_error("다이스 해방 invisible/sacrifice assert");if (!daisState_.ritual.boss) throw std::logic_error("다이스 해방 boss assert");
    const auto captured=ContainedPriest(dais);
    if (captured.value) {
        const auto created=hooks_.create(daisState_.ritual.priestType,0);const auto word=Read(pool_.Slot(captured),12,2);auto raw=pool_.AllocatedBytes(created);raw[12]=static_cast<std::uint8_t>(word);raw[13]=static_cast<std::uint8_t>(word>>8);hooks_.setOwner(created,0);
        const auto y=footprint_.Center(dais)[1];const auto x=footprint_.Center(dais)[0];hooks_.pop(created,static_cast<float>(static_cast<double>(x)+kReleaseOffset),y,0);hooks_.escapeEffect(created,5.0F,0);hooks_.destroy(captured,0);
    }
    hooks_.playGlobal("priestFree.wav",0,0,0,1,0);if (!patch && pool_.Slot(dais)[32]==daisState_.localOwner) hooks_.playGlobal("enemyPriestEsacping.wav",0,0,0,1,0);const auto priest=temple_.FindStrongOwnedPriest(dais);if (Valid(priest)) dais_.StopPriestRitual(priest);
}
// 다음 링크는 원본처럼 현재 후보를 반환하기 전에 저장한다.
void RawAltarSacrifice::FinishOnAltarForms(Sid dais,Sid altar) const {
    Sid current{static_cast<std::uint16_t>(Read(pool_.Slot(altar),6,2))};std::unordered_set<std::uint16_t> visited;
    // 파괴 중에도 원본은 Form을 하나씩 현재 Host에서 읽고 Finish를 매24마다 호출한다.
    while (current.value) {
        if (!Valid(current) || !visited.insert(current.value).second) throw std::logic_error("다이스 알터 프로세스 체인 손상");const auto raw=pool_.Slot(current);const Sid next{static_cast<std::uint16_t>(Read(raw,4,2))};
        if (raw[10]<=69) { if (raw[10]<10 || raw[10]>=host_.ProcessTypeEnd()) throw std::logic_error("다이스 알터 process finder isProcess assert");auto* process=host_.FromForm(current);if (!process) throw std::logic_error("다이스 알터 실제 Kernel 프로세스 누락");const auto* regular=dynamic_cast<const RegularProcess*>(process);if (regular && regular->Event()==24) dais_.FinishSacrifice(dais); }
        current=next;
    }
}
// 다이스 파생 전처리는extra를최초한번만검사하며 마지막 공통 몸체는 항상 호출한다.
void RawAltarSacrifice::DaisPreDestroy(Sid dais,std::uint32_t flags) const {
    const auto raw=pool_.AllocatedBytes(dais);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;if (raw[10]!=daisState_.rune.daisType || Read(raw,0)!=(patch ? kPatchDaisVtable : kCdDaisVtable)) throw std::invalid_argument("다이스 파생 전처리 타입/가상표 오류");
    if (!(raw[patch ? 40 : 35]&9)) {
        if (flags&kSacrificeDestroy) { const auto altar=rune_.AltarAt(dais);if (!Valid(altar)) throw std::logic_error("다이스 삭제의 알터 프로세스 부모 누락");FinishOnAltarForms(dais,altar); }
        const auto priest=temple_.FindStrongOwnedPriest(dais);if (Valid(priest)) dais_.StopPriestRitual(priest);const auto area=footprint_.Footprint(dais);RawSquidFinder finder(pool_,hash_,types_);
        // float 발자국을 원본 CRT로 절삭하고 각현재알터를 실제 파생삭제한다.
        for (auto current=finder.Begin({Cell(area.left),Cell(area.top),Cell(area.right),Cell(area.bottom)},0);current.value;current=finder.Next()) if (pool_.Slot(current)[10]==daisState_.rune.altarType) hooks_.destroy(current,kAltarDestroy);
        // 원본의1..8 현재 플레이어 갱신 순서다.
        for (std::uint32_t owner=1;owner<=kPlayerCount;++owner) hooks_.refreshOwner(owner);
    }
    damageable_.PreDestroy(dais,flags);
}
// 최상위 Regular 분배도 호출 효과 뒤 같은 풀/전역을 다시 읽는다.
float RawAltarSacrifice::Handle(Sid altar,std::uint32_t event,std::uint32_t count,float payload) const {
    if (!Handles(altar)) throw std::invalid_argument("알터 Handle 타입/가상표 오류");auto dais=DaisAt(altar);std::array<float,2> center{};const bool hasDais=Valid(dais);if (hasDais) { const auto point=Position(dais);center={static_cast<float>(static_cast<double>(point[0])-kFinishedDelay),static_cast<float>(static_cast<double>(point[1])-kFinishedDelay)}; }
    if (event==21) {
        if (pool_.Edition()==OriginalEdition::Patch1078) { const auto captured=ContainedPriest(dais);if (daisState_.ritual.boss && Valid(captured)) { const std::uint32_t oldOwner=pool_.Slot(captured)[12]&127,owner=pool_.Slot(dais)[34];if (oldOwner>kPlayerCount || owner>kPlayerCount) throw std::out_of_range("알터 자동 해방 동맹 인덱스");if (daisState_.alliances[oldOwner+owner*(kPlayerCount+1)]) { ReleaseFromAltar(altar);return -1.0F; } } }
        if (Direction(altar)==15) return payload;std::uint32_t nextEvent=20;if (hooks_.randomInt(0,100000)/1000<state_.escapePercent) { nextEvent=22;advance_.Advance(altar,-1,0); }if (hooks_.reserveShared()) hooks_.addShared(altar,nextEvent,kStrugglePayload);
        if (hasDais && Direction(dais)!=10) { const auto chain=hooks_.randomInt(0,5);if (chain<0 || chain>=static_cast<std::int32_t>(kChains.size())) throw std::out_of_range("알터 chain 난수 범위");hooks_.playAt(center[0],center[1],kChains[static_cast<std::size_t>(chain)],0);const auto struggle=hooks_.randomInt(0,3000)/1000;if (struggle<0 || struggle>=static_cast<std::int32_t>(kStruggles.size())) throw std::out_of_range("알터 struggle 난수 범위");hooks_.playAt(center[0],center[1],kStruggles[static_cast<std::size_t>(struggle)],0); }
        return static_cast<float>(static_cast<double>(hooks_.randomFloat(0,2))+state_.struggleSeconds);
    }
    if (event==20 || event==22) { if (Direction(altar)!=14) return 0;const std::int32_t delta=event==20 ? 1 : -1;if (advance_.Advance(altar,delta,0)) { if (delta==-1) advance_.Advance(altar,1,0);return 0; }return payload; }
    if (event==23) return payload;if (event!=24) return hooks_.damageable(altar,event,count,payload);dais=DaisAt(altar);if (!Valid(dais)) throw std::logic_error("알터24 원본의 다이스0 프레임읽기 경계");
    if (Direction(dais)==10) { if (daisState_.ritual.boss) { hooks_.destroy(dais,kSacrificeDestroy);return -1.0F; }return payload; }
    if (!advance_.Advance(altar,1,0)) {
        if (count<3 || hooks_.randomInt(0,100000)/1000<50) { if (Valid(dais)) EscapeFrame(dais,count); }if (hooks_.randomFloat(0,1)<payload) { const auto point=Position(altar);hooks_.playAt(point[0],point[1],"altarBurnCollapse.wav",0); }return payload;
    }
    dais=DaisAt(altar);if (!Valid(dais)) return payload;if (daisState_.ritual.boss) { const auto captured=ContainedPriest(dais);if (captured.value) hooks_.destroy(captured,8); }hooks_.removeAnimation(altar);const auto point=Position(altar);hooks_.playAt(point[0],point[1],"explosion.wav",0);hooks_.setFrame(dais,First(dais,'K'),0);hooks_.setFrame(altar,First(altar,'M'),0);return kFinishedDelay;
}
// 현재풀참조를후속 실제 등록에 제공한다.
const SidPool& RawAltarSacrifice::Pool() const { return pool_; }
// 기존 실제 모듈의 몸체를 재사용해 consumer가 선택하는 동일월드를강제한다.
AltarSacrificeHooks MakeAltarSacrificeHooks(const SidPool& pool,const SquidFrame& frame,SquidProcessHost& host,SquidFactory& factory,const RawAssetRegular& assets,GameRandom& random,AltarSacrificeHooks hooks) {
    if (&pool!=&frame.Pool() || &pool!=&host.Pool() || &pool!=&factory.Pool() || &pool!=&assets.Pool()) throw std::invalid_argument("알터 Frame/Host/Factory/Asset 풀 불일치");
    hooks.setFrame=[&frame](Sid sid,std::int32_t value,std::uint32_t flags) { frame.Set(sid,value,flags); };hooks.addShared=[&host](Sid sid,std::uint32_t event,float payload) { host.AddSharedRegular(sid,event,payload); };hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };hooks.damageable=[&assets](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return assets.DamageableHandle(sid,event,count,payload); };
    hooks.randomInt=[&random](std::int32_t low,std::int32_t high) { const auto span=static_cast<std::uint32_t>(high)-static_cast<std::uint32_t>(low);return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(low)+random.Next(span)); };
    hooks.randomFloat=[&random](float low,float high) { return static_cast<float>(static_cast<double>(random.Next(65536))*(1.0/65536.0)*std::abs(static_cast<double>(high)-low)+low); };return hooks;
}
}
