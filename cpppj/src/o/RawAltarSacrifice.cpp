// 실제 제단 사건과 다이스 전처리를 복원하고 기존 조회/프레임/Host/공통 삭제를 재사용한다.
#include "o/RawAltarSacrifice.h"
#include "o/RawDaisDeletion.h" // 실제 전처리와 제단24 순회를 장기 Native 삭제와 공유한다.
#include "o/RawSquidWord.h"
#include "o/RawAltarReleaseParent.h" // 패치의 실제 진단 복귀/저장 검색 결과를 공유한다.
#include "o/RawAssetRegular.h"
#include "o/SquidFactory.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::o {
namespace {
template<class F> F AltarNeed(const F& function) { if (!function) throw std::logic_error("제단의 선택된 실제 장치 미연결");return function; } // 선택한 callable을 복사해 실제 호출 중 연결 교체에도 수명을 유지한다.
std::span<std::uint8_t> AltarRaw(SidPool& pool,Sid sid) { if (pool.Edition()!=OriginalEdition::Patch1078) return pool.AllocatedBytes(sid);const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // Unpop/해방 콜백 뒤 PATCH plain frame은 같은 물리 슬롯을 쓰며 CD 계약은 유지한다.
constexpr float kReleaseOffset=0.5F; // 해방 위치의 실제3f000000 비트이며 정기 전용 상수는 공용 분기 본문에 둔다.
// 비정렬 little endian의 필요한 필드 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 인접 필드를 섞지 않고 WORD/DWORD를 조합한다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
// 정상 지도 좌표를 원본 CRT처럼0 방향으로 절삭한다.
int Cell(float value) {
    if (!std::isfinite(value) || static_cast<double>(value)<-2147483648.0 || static_cast<double>(value)>=2147483648.0) throw std::out_of_range("제단 원본 좌표 절삭 범위");
    return static_cast<int>(value);
}
}
// 같은 풀/표 물리를 먼저 확인하며 PATCH 효과는 실제 도달할 때 요구한다.
RawAltarSacrifice::RawAltarSacrifice(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    const RawSquidNeighbors& footprint,const RawRunePostPop& rune,const RawTempleRitual& temple,const RawCarrierPreDestroy& carrier,
    const RawDaisRegular& dais,const RawFrameAdvance& advance,SquidProcessHost& host,const RawDamageablePreDestroy& damageable,
    DaisRegularState& daisState,AltarSacrificeState& state,AltarSacrificeHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),footprint_(footprint),rune_(rune),temple_(temple),carrier_(carrier),dais_(dais),advance_(advance),host_(host),damageable_(damageable),daisState_(daisState),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&footprint.Pool() || &pool!=&rune.Pool() || &pool!=&temple.Pool() || &pool!=&carrier.Pool() || &pool!=&dais.Pool() || &pool!=&advance.Pool() || &pool!=&host.Pool() || &pool!=&damageable.Pool()
        || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size()
        || (pool.Edition()!=OriginalEdition::Patch1078 && (!hooks_.setFrame || !hooks_.destroy || !hooks_.reserveShared || !hooks_.addShared || !hooks_.randomInt || !hooks_.randomFloat || !hooks_.playAt || !hooks_.playGlobal || !hooks_.removeAnimation || !hooks_.unpop || !hooks_.repop || !hooks_.create || !hooks_.setOwner || !hooks_.pop || !hooks_.escapeEffect || !hooks_.refreshOwner || !hooks_.damageable)))
        throw std::invalid_argument("제단 실제 하위 몸체/같은 풀/필수 효과 연결 누락");
}
// 실제 월드의 권한 getter를 우선하고 독립 호출자의 기존 상태도 유지한다.
bool RawAltarSacrifice::Boss() const { return hooks_.boss ? AltarNeed(hooks_.boss)() : daisState_.ritual.Boss(); }
// 원본은 free/dead를 isValid 조건에 추가하지 않는다.
bool RawAltarSacrifice::Valid(Sid sid) const { return sid.value>0 && sid.value<pool_.Capacity(); }
// 패치 signed DWORD와 CD BYTE를 구분한다.
std::int32_t RawAltarSacrifice::Frame(Sid sid) const { const auto raw=pool_.Slot(sid);return pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,36)) : raw[34]; }
// 현재 raw타입을 읽어 실제 코드표의 방향을 반환한다.
std::int32_t RawAltarSacrifice::Direction(Sid sid) const { const auto type=pool_.Slot(sid)[10];if (type>=frames_.size()) throw std::out_of_range("제단 현재 방향 TYPE 물리 범위");return RawPathAnimation::Direction(frames_[type],Frame(sid)); }
// 현재 타입의 실제 글자 구간 첫번호다.
std::int32_t RawAltarSacrifice::First(Sid sid,std::uint8_t letter) const { const auto type=pool_.Slot(sid)[10];if (type>=frames_.size()) throw std::out_of_range("제단 현재 frame TYPE 물리 범위");return frames_[type].Run(letter).first; }
// 표시 setter를 호출하지 않는 원본 해방의 plain 쓰기다.
void RawAltarSacrifice::WriteFrame(Sid sid,std::int32_t value) const {
    auto raw=AltarRaw(pool_,sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto offset=patch ? 36U : 34U;const auto width=patch ? 4U : 1U;const auto bits=std::bit_cast<std::uint32_t>(value);
    // 정확한 판본 필드 폭만 기록한다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(bits>>(index*8));
}
// 호출 뒤 좌표가 바뀌었으면 다음 위치 효과는 새 값을 읽는다.
std::array<float,2> RawAltarSacrifice::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 실제 최종 가상표를 쓰는 제단만 처리한다.
bool RawAltarSacrifice::Handles(Sid altar) const { const auto raw=pool_.Slot(altar);return pool_.Edition()==OriginalEdition::Patch1078 ? Read(raw,0)==kPatchAltarVtable : raw[10]==daisState_.rune.AltarType() && Read(raw,0)==kCdAltarVtable; }
// 기준점typed helper00448bb0/CD00483c50은 추가 owner/dead/free 필터를 적용하지 않는다.
Sid RawAltarSacrifice::DaisAt(Sid sid) const {
    const auto point=Position(sid);const auto type=daisState_.rune.DaisType();if (type>=types_.size()) throw std::out_of_range("제단 현재 다이스 타입 범위");const int x=Cell(point[0]),y=Cell(point[1]);RawSquidFinder finder(pool_,hash_,types_);
    // 일반 finder의 현재 버킷과 buried 조건을 재사용한다.
    for (auto current=finder.Begin({x,y,x,y},(types_[type].flags2&6)==0 ? 1U : 0U);current.value;current=finder.Next()) if (pool_.Slot(current)[10]==type) return current;
    return {};
}
// 원본 specialized lookup은 kindWORD/순번0/삭제false로만 호출된다. 공통 Carrier의 같은 body를 사용한다.
Sid RawAltarSacrifice::ContainedPriest(Sid dais) const {
    if (!daisState_.ritual.PriestType()) throw std::logic_error("제단 specialized kind0은 일반 Carrier wildcard와 다르다");
    return carrier_.FindContained(dais,0,daisState_.ritual.PriestType(),0,false);
}
// 지정된 대상의 현재 가상80만 조회하며 typed 검색이나 대상을 바꾸는 효과를 추가하지 않는다.
std::uint32_t RawAltarSacrifice::VirtualCount(Sid dais) const {
    if (hooks_.count) return AltarNeed(hooks_.count)(dais);
    if (Read(pool_.Slot(dais),0)!=(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchDaisVtable : kCdDaisVtable))
        throw std::logic_error("제단 Count의 현재 가상80 분배 장치 미연결");
    return temple_.Count(dais); // 기존 독립 fixture가 명시한 실제 Dais 가상 몸체만 유지하며 다른 VT를 Temple로 바꾸지 않는다.
}
// 제단+80은 typed 조회 한 번의 결과를 고정하고 대상의 현재 가상80으로 위임한다.
std::uint32_t RawAltarSacrifice::Count(Sid altar) const { const auto dais=DaisAt(altar);return Valid(dais) ? VirtualCount(dais) : 0; }
// 첫 프레임/길이는 현재raw타입에서 읽으며 count>=3의 난수 소비도 생략하지 않는다.
void RawAltarSacrifice::EscapeFrame(Sid dais,std::uint32_t count) const {
    auto direction=Direction(dais);const auto type=pool_.Slot(dais)[10];
    if (count<3) {
        if (!count) {
            direction=std::bit_cast<std::int32_t>(14U-VirtualCount(dais)); // 원문 DWORD 뺄셈의 감김을 보존하며 가상 반환값을 signed 뺄셈 UB로 만들지 않는다.
            if (direction<0 || direction>15) throw std::out_of_range("다이스 이탈의 가상 count 첫 프레임 표 범위 오류"); // 원문의16DWORD 방향 표 밖을 BYTE 절단으로 유효 글자에 재매핑하지 않는다.
        }
        AltarNeed(hooks_.setFrame)(dais,frames_[type].Run(static_cast<std::uint8_t>(direction+'A')).first+static_cast<std::int32_t>(count),0);return;
    }
    const auto length=frames_[type].Run(static_cast<std::uint8_t>(direction+'A')).count;static_cast<void>(AltarNeed(hooks_.randomInt)(3000,length*1000));
}
// 제단/다이스의 선택된 원문 진단만 요구하며 진단 전 효과를 되돌리지 않는다.
void RawAltarSacrifice::AssertRelease(std::string_view expression,std::uint32_t line) const { if (!hooks_.assertion) throw std::logic_error("제단/다이스 해방 원문 진단 장치 미연결");AltarNeed(hooks_.assertion)(expression,".\\Dais.cpp",line); }
// 해방의 표시 없는 프레임 쓰기는 일반 Frame.Set으로 대체하지 않는다.
void RawAltarSacrifice::ReleaseFromAltar(Sid altar) const {
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        AltarReleaseParentHooks parent; // 같은 실제 조회/해방/공간/프레임 몸체만 빌린다.
        parent.findDais=[this](Sid sid) { return DaisAt(sid); };parent.direction=[this](Sid sid) { return Direction(sid); };parent.assertion=hooks_.assertion;
        parent.valid=[this](Sid sid) { return Valid(sid); };parent.release=[this](Sid sid) { ReleasePriest(sid); };
        parent.unpop=hooks_.unpop;parent.repop=hooks_.repop;parent.plainP=[this](Sid sid) { WriteFrame(sid,First(sid,'P')); };
        RunAltarReleaseParent(altar,daisState_.debug,parent);return;
    }
    const auto dais=DaisAt(altar);if (Valid(dais)) ReleasePriest(dais);AltarNeed(hooks_.unpop)(altar,0);WriteFrame(altar,First(altar,'P'));AltarNeed(hooks_.repop)(altar,0);
}
// 두 번 중심 조회와 가상효과 뒤의 현재raw 재읽기를 원본 순서대로 수행한다.
void RawAltarSacrifice::ReleasePriest(Sid dais) const {
    const auto altar=rune_.AltarAt(dais);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (!Valid(altar)) { if (patch) AssertRelease("altar.isValid()",0x3cb);else throw std::logic_error("다이스 해방 altar.isValid assert"); }
    if (patch && daisState_.debug && (Direction(altar)==12 || Direction(altar)==11)) AssertRelease("!altar.isInvisibleFrame() && !altar.isSacrifice()",0x3cc);
    if (!Boss()) { if (patch) AssertRelease("boss",0x3cd);else throw std::logic_error("다이스 해방 boss assert"); } // 원문은 진단이 돌아오면 기존 내용물/음향 조회를 계속한다.
    const auto captured=ContainedPriest(dais);
    if (captured.value) {
        const auto created=AltarNeed(hooks_.create)(daisState_.ritual.PriestType(),0);const auto word=Read(pool_.Slot(captured),12,2);SetRawSquidWord(pool_,created,static_cast<std::uint16_t>(word),daisState_.BridgeType(),hooks_.notifySurface);AltarNeed(hooks_.setOwner)(created,0);
        const auto y=footprint_.Center(dais)[1];const auto x=footprint_.Center(dais)[0];AltarNeed(hooks_.pop)(created,static_cast<float>(static_cast<double>(x)+kReleaseOffset),y,0);AltarNeed(hooks_.escapeEffect)(created,5.0F,0);AltarNeed(hooks_.destroy)(captured,0);
    }
    AltarNeed(hooks_.playGlobal)("priestFree.wav",0,0,0,1,0);if (!patch && pool_.Slot(dais)[32]==daisState_.localOwner) AltarNeed(hooks_.playGlobal)("enemyPriestEsacping.wav",0,0,0,1,0);const auto priest=temple_.FindStrongOwnedPriest(dais);if (Valid(priest)) dais_.StopPriestRitual(priest);
}
// 다음 링크는 원본처럼 현재 후보를 반환하기 전에 저장한다.
void RawAltarSacrifice::FinishOnAltarForms(Sid dais,Sid altar) const {
    RawDaisDeletion::FinishForms(pool_,host_,dais,altar,daisState_.clock.RegularType(),daisState_.clock.SharedRegularType(),[this](Sid sid) { dais_.FinishSacrifice(sid); });
}
// 다이스 파생 전처리는extra를최초한번만검사하며 마지막 공통 몸체는 항상 호출한다.
void RawAltarSacrifice::DaisPreDestroy(Sid dais,std::uint32_t flags) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto raw=patch ? pool_.Slot(dais) : pool_.AllocatedBytes(dais);if ((!patch && raw[10]!=daisState_.rune.DaisType()) || Read(raw,0)!=(patch ? kPatchDaisVtable : kCdDaisVtable)) throw std::invalid_argument("다이스 파생 전처리 타입/가상표 오류");
    DaisDeletionHooks hooks;hooks.findAltar=[this](Sid sid) { return rune_.AltarAt(sid); };hooks.findPriest=[this](Sid sid) { return temple_.FindStrongOwnedPriest(sid); };
    hooks.finishForms=[this](Sid sid,Sid altar) { if (!Valid(altar)) throw std::logic_error("다이스 삭제의 제단 프로세스 부모 누락");FinishOnAltarForms(sid,altar); }; // 독립 typed의 기존 손상 입력 계약은 유지한다.
    hooks.stopPriest=[this](Sid sid) { dais_.StopPriestRitual(sid); };hooks.destroy=hooks_.destroy;hooks.refreshOwner=hooks_.refreshOwner;hooks.pre=[this](Sid sid,std::uint32_t value) { damageable_.PreDestroy(sid,value); };
    RawDaisDeletion(pool_,hash_,types_,footprint_,daisState_.rune.AltarType(),daisState_.ritual.RuneType(),std::move(hooks)).DaisPre(dais,flags);
}
// 최상위 Regular 분배도 호출 효과 뒤 같은 풀/전역을 다시 읽는다.
float RawAltarSacrifice::Handle(Sid altar,std::uint32_t event,std::uint32_t count,float payload) const {
    if (!Handles(altar)) throw std::invalid_argument("제단 Handle 타입/가상표 오류");AltarRegularHooks current;
    current.dais=[this](auto sid) { return DaisAt(sid); };current.priest=[this](auto sid) { return ContainedPriest(sid); };current.boss=[this] { return Boss(); };current.alliance=[this](auto original,auto owner) { return daisState_.alliances[original+owner*(kPlayerCount+1)]; };
    current.release=[this](auto sid) { ReleaseFromAltar(sid); };current.escapeFrame=[this](auto sid,auto number) { EscapeFrame(sid,number); };current.advance=[this](auto sid,auto delta,auto flags) { return advance_.Advance(sid,delta,flags); };
    current.frame=[this](auto sid,auto value,auto flags) { AltarNeed(hooks_.setFrame)(sid,value,flags); };current.destroy=[this](auto sid,auto flags) { AltarNeed(hooks_.destroy)(sid,flags); };current.reserveShared=[this] { return AltarNeed(hooks_.reserveShared)(); };current.addShared=[this](auto sid,auto code,auto value) { AltarNeed(hooks_.addShared)(sid,code,value); };
    current.randomInt=[this](auto low,auto high) { return AltarNeed(hooks_.randomInt)(low,high); };current.randomFloat=[this](auto low,auto high) { return AltarNeed(hooks_.randomFloat)(low,high); };current.sound=[this](auto x,auto y,auto name,auto flags) { AltarNeed(hooks_.playAt)(x,y,name,flags); };
    current.removeAnimation=[this](auto sid) { AltarNeed(hooks_.removeAnimation)(sid); };current.damageable=[this](auto sid,auto code,auto number,auto value) { return AltarNeed(hooks_.damageable)(sid,code,number,value); };
    return RunAltarRegular(pool_,frames_,altar,event,count,payload,state_,current);
}
// 현재풀참조를후속 실제 등록에 제공한다.
const SidPool& RawAltarSacrifice::Pool() const { return pool_; }
// 기존 실제 모듈의 몸체를 재사용해 consumer가 선택하는 동일월드를강제한다.
AltarSacrificeHooks MakeAltarSacrificeHooks(const SidPool& pool,const SquidFrame& frame,SquidProcessHost& host,SquidFactory& factory,const RawAssetRegular& assets,GameRandom& random,AltarSacrificeHooks hooks) {
    if (&pool!=&frame.Pool() || &pool!=&host.Pool() || &pool!=&factory.Pool() || &pool!=&assets.Pool()) throw std::invalid_argument("제단 Frame/Host/Factory/Asset 풀 불일치");
    hooks.setFrame=[&frame](Sid sid,std::int32_t value,std::uint32_t flags) { frame.Set(sid,value,flags); };hooks.addShared=[&host](Sid sid,std::uint32_t event,float payload) { host.AddSharedRegular(sid,event,payload); };hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };hooks.damageable=[&assets](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return assets.DamageableHandle(sid,event,count,payload); };
    hooks.randomInt=[&random](std::int32_t low,std::int32_t high) { const auto span=static_cast<std::uint32_t>(high)-static_cast<std::uint32_t>(low);return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(low)+random.Next(span)); };
    hooks.randomFloat=[&random](float low,float high) { return static_cast<float>(static_cast<double>(random.Next(65536))*(1.0/65536.0)*std::abs(static_cast<double>(high)-low)+low); };return hooks;
}
}
