// Dais 의식의 각 하위 효과 뒤 현재 프레임/owner/player 전역을 다시 읽는다.
#include "o/RawDaisRegular.h"
#include "o/RawSquidWord.h"
#include "o/RawAssetRegular.h"
#include "o/RawPathAnimation.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
template<class F> F DaisNeed(const F& function) { if (!function) throw std::logic_error("다이스의 선택된 실제 장치 미연결");return function; } // 선택한 callable을 복사해 실제 호출 중 연결 교체에도 수명을 유지한다.
std::span<std::uint8_t> DaisRaw(SidPool& pool,Sid sid) { if (pool.Edition()!=OriginalEdition::Patch1078) return pool.AllocatedBytes(sid);const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 의식의 plain 쓰기는 PATCH에서 도달한 현재 물리 슬롯을 사용하며 CD 계약은 유지한다.
constexpr std::array<std::int32_t,4> kUpgradeFirst{{3,64,120,0}}; // 원본 DAT532624/CD52e2b8[count]의 다음 첫 프레임이다.
constexpr float kInitialDelay=0.01F,kCollapseDelay=0.1F,kSacrificeDelay=0.11999999731779099F; //305/306/altar24의 실제 float 비트다.
constexpr double kThunderDelay=2.799999952316284; // 원본 double DAT507918/CD504048의 지연이다.
constexpr std::uint32_t kSoundFlags=0xc,kConsumedFlags=0x400048; // 지연 전역소리의flags와 포획 내용물의 실제 가상삭제flags다.
// 비정렬 현재 raw 필드를 원본 폭만큼 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // WORD/BYTE의 옆 필드를 읽거나 바꾸지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 현재 float 좌표/SP를 원본 __ftol의 정상 signed32 범위에서 절삭한다.
std::int32_t Integer(float value) {
    if (!std::isfinite(value) || static_cast<double>(value)<-2147483648.0 || static_cast<double>(value)>=2147483648.0) throw std::out_of_range("Dais 정상 절삭 범위");
    return static_cast<std::int32_t>(value);
}
}
// 실제 Player 기록은 장기 주소를 빌리며 판본별 정확한9기록 크기를 요구한다.
void DaisRegularState::BindPlayerRecords(OriginalEdition edition,std::span<std::uint8_t> records) {
    if (records.size()!=9U*(edition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU)) throw std::invalid_argument("다이스 실제 Player 기록 크기 오류");playerRecords=records;playerEdition=edition;
}
// 독립 입력과 실제 세션 기록을 같은 현재 조회 규칙으로 읽는다.
std::uint8_t DaisRegularState::GrantFlag(std::uint32_t owner) const {
    if (owner>kPlayerCount) throw std::out_of_range("다이스 grant Player 범위");return playerRecords.empty() ? grantFlags[owner] : playerRecords[owner*(playerEdition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU)+(playerEdition==OriginalEdition::Patch1078 ? 0x71U : 0x69U)];
}
std::uint8_t& DaisRegularState::GrantFlagSlot(std::uint32_t owner) { if (owner>kPlayerCount) throw std::out_of_range("다이스 grant 쓰기 Player 범위");return playerRecords.empty() ? grantFlags[owner] : playerRecords[owner*(playerEdition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU)+(playerEdition==OriginalEdition::Patch1078 ? 0x71U : 0x69U)]; } // PATCH Count 앞의 실제 Player 쓰기 주소를 저장할 때만 사용한다.
// 실제 BYTE 하나만 쓰며 독립 검사의 배열 동작은 그대로 유지한다.
void DaisRegularState::SetGrantFlag(std::uint32_t owner,std::uint8_t value) {
    if (owner>kPlayerCount) throw std::out_of_range("다이스 grant Player 범위");if (playerRecords.empty()) grantFlags[owner]=value;else playerRecords[owner*(playerEdition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU)+(playerEdition==OriginalEdition::Patch1078 ? 0x71U : 0x69U)]=value;
}
// 실제 결과 DWORD의 네 바이트만 빌리며 반환 주소는 다음 BindPlayerRecords로 교체하지 않는다.
std::span<std::uint8_t> DaisRegularState::PlayerResultSlot(std::uint32_t owner) {
    if (owner>kPlayerCount) throw std::out_of_range("다이스 결과 Player 범위");
    if (playerRecords.empty()) return {reinterpret_cast<std::uint8_t*>(&playerResults[owner]),sizeof(std::uint32_t)}; // unsigned BYTE로 독립 DWORD의 객체 표현을 읽고 쓴다.
    const auto at=owner*(playerEdition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU)+(playerEdition==OriginalEdition::Patch1078 ? 0xa0U : 0x98U);return playerRecords.subspan(at,4); // 실제 기록의 정확한 DWORD 위치다.
}
// 현재 기록으로 쓰는 기존 API이며 Native CD의 Count 복귀 뒤 저장소 재조회를 유지한다.
void DaisRegularState::SetPlayerResult(std::uint32_t owner,std::uint32_t value) {
    auto destination=PlayerResultSlot(owner);for (std::size_t index=0;index<4;++index) destination[index]=static_cast<std::uint8_t>(value>>(index*8)); // 원본 little endian DWORD의 네 바이트만 쓴다.
}
// 같은 풀/TYPE 물리를 확인하고 PATCH 효과는 선택한 원문 위치에서 요구한다.
RawDaisRegular::RawDaisRegular(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    const RawTempleRitual& ritual,const RawRunePostPop& rune,const RawCarrierPreDestroy& carrier,SquidProcessHost& host,
    const RawContainedLookup& lookup,RawPathStart& path,DaisRegularState& state,DaisRegularHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),ritual_(ritual),rune_(rune),carrier_(carrier),host_(host),lookup_(lookup),path_(path),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&ritual.Pool() || &pool!=&rune.Pool() || &pool!=&carrier.Pool() || &pool!=&host.Pool() || &pool!=&lookup.Pool() || &pool!=&path.Pool()
        || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || types.size()!=frames.size()
        || (pool.Edition()!=OriginalEdition::Patch1078 && (!hooks_.setFrame || !hooks_.destroy || !hooks_.reserveShared || !hooks_.addShared || !hooks_.playAt || !hooks_.reserveSound
        || !hooks_.loadSound || !hooks_.attachSound || !hooks_.wallSeconds || !hooks_.setSoundStart || !hooks_.stopVoice || !hooks_.processType
        || !hooks_.pathMetadata || !hooks_.damageable || !hooks_.addLocalSp || !hooks_.normalizeForeignSp || !hooks_.grantForeignType
        || !hooks_.localSpecial || !hooks_.localType || !hooks_.refreshOwner || !hooks_.refreshLocal || !hooks_.priestExists || !hooks_.multiplayerStatus || !hooks_.chat)))
        throw std::invalid_argument("Dais 실제 몸체/풀/필수 효과 연결 누락");
}
// 선택된 진단만 실제 장치를 요구하며 정상 복귀 뒤 현재 상태 조회를 계속한다.
void RawDaisRegular::Assert(std::string_view expression,std::string_view file,std::uint32_t line) const {
    if (!hooks_.assertion) throw std::logic_error("Dais의 선택된 실제 원문 진단 장치 미연결");DaisNeed(hooks_.assertion)(expression,file,line);
}
bool RawDaisRegular::Boss() const { return hooks_.boss ? DaisNeed(hooks_.boss)() : state_.ritual.Boss(); } // 실제 Client의 현재 권한과 기존 독립 입력을 구별한다.
std::int32_t RawDaisRegular::Alliance(std::uint32_t row,std::uint32_t column) const { // 원문 방향별9×9 행/열을 바꾸지 않는다.
    Player(row);Player(column);return hooks_.alliance ? DaisNeed(hooks_.alliance)(row,column) : state_.alliances[row*(kPlayerCount+1)+column];
}
std::uint32_t RawDaisRegular::Count(Sid dais) const { return hooks_.count ? DaisNeed(hooks_.count)(dais) : ritual_.Count(dais); } // 원문 자기 가상80 지점만 이 경계에 위임한다.
std::uint32_t RawDaisRegular::PriestType() const { return hooks_.priestType ? DaisNeed(hooks_.priestType)() : state_.ritual.PriestType(); } // 실제 Session의 현재 전역과 기존 독립 상태를 구별한다.
// 원본 프레임의 판본별 폭과 부호를 유지한다.
std::int32_t RawDaisRegular::CurrentFrame(Sid sid) const {
    const auto raw=pool_.Slot(sid);return pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,36)) : raw[34];
}
// 실제 현재 좌표/owner를 하위효과 뒤 다시 읽는다.
TempleRitualPoint RawDaisRegular::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
std::uint32_t RawDaisRegular::Owner(Sid sid) const { return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]; }
// side별 첫 프레임은 현재 raw 타입을 사용한다.
std::int32_t RawDaisRegular::First(Sid sid,std::uint8_t letter) const { const auto type=pool_.Slot(sid)[10];if (type>=frames_.size()) throw std::out_of_range("다이스 현재 frame TYPE 물리 범위");return frames_[type].Run(letter).first; }
// 실제 Rune/Dais 생성자의 가상표만 받아 일반 모형의 base vtable로 대체하지 않는다.
bool RawDaisRegular::Handles(Sid sid) const {
    const auto raw=pool_.Slot(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto vtable=Read(raw,0);
    if (patch) return vtable==kPatchRuneVtable || vtable==kPatchDaisVtable;
    return (raw[10]==state_.rune.RuneType() && vtable==(patch ? kPatchRuneVtable : kCdRuneVtable)) || (raw[10]==state_.rune.DaisType() && vtable==(patch ? kPatchDaisVtable : kCdDaisVtable));
}
// Dais wrapper의allowDead1과CanBridge의0을 기존 공통 선택몸체에 전달한다.
Sid RawDaisRegular::Contained(Sid dais,std::uint32_t mask,std::uint32_t kind,bool allowDead) const { return carrier_.FindContained(dais,mask,kind,0,allowDead); }
// 포획한 내용물의kind는 현재+18 WORD다.
std::uint32_t RawDaisRegular::CapturedWord(Sid dais) const { const auto captured=Contained(dais,4,0);return captured.value ? Read(pool_.Slot(captured),18,2) : 0; }
// 중립0과 플레이어1..8 배열만 접근한다. 손상 원본 범위 밖 읽기는 실행하지 않는다.
void RawDaisRegular::Player(std::uint32_t owner) const { if (owner>kPlayerCount) throw std::out_of_range("Dais 현재 플레이어 범위"); }
// altar 이탈이 결과를 버리는 Dais typed lookup도 동일한 일반 finder에서 수행한다.
Sid RawDaisRegular::FindDaisAt(Sid sid) const {
    const auto point=Position(sid);const auto type=state_.rune.DaisType();if (type>=types_.size()) throw std::out_of_range("Dais 현재 타입 범위");const auto x=Integer(point.x),y=Integer(point.y);RawSquidFinder finder(pool_,hash_,types_);
    // buried 제외/버킷순서는기존일반몸체이며추가free/dead/owner필터를넣지않는다.
    for (auto current=finder.Begin({x,y,x,y},(types_[type].flags2&6)==0 ? 1U : 0U);current.value;current=finder.Next()) if (pool_.Slot(current)[10]==type) return current;
    return {};
}
// 실제 Regular 분배의 반환 지연을 원본 float 비트대로 유지한다.
float RawDaisRegular::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (!Handles(sid)) throw std::invalid_argument("Dais/Rune Handle 타입/가상표 오류");
    if (pool_.Edition()==OriginalEdition::Patch1078 ? Read(pool_.Slot(sid),0)==kPatchRuneVtable : pool_.Slot(sid)[10]==state_.rune.RuneType()) {
        if (event!=0x304) return payload;const auto dais=rune_.FindOwnedDais(sid);if (dais.value && dais.value<pool_.Capacity()) Progress(dais);DaisNeed(hooks_.destroy)(sid,0x48);return -1.0F;
    }
    if (event==0x305) { const auto frame=CurrentFrame(sid);const auto next=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(frame)+1);const auto nextStage=ritual_.Stage(sid,next),stage=ritual_.Stage(sid,frame);DaisNeed(hooks_.setFrame)(sid,next,0);return stage==nextStage ? kCollapseDelay : 0; }
    if (event==0x306) return SacrificeAdvance(sid);
    if (event==0x307 && count) { const auto priest=ritual_.FindStrongOwnedPriest(sid);ritual_.Ritual(sid,priest,1);return 0; }
    return DaisNeed(hooks_.damageable)(sid,event,count,payload);
}
// 현재K..O에서만604한개를삭제하고현재타입E첫프레임으로복귀한다.
void RawDaisRegular::StopPriestRitual(Sid priest) const {
    StopPriestRitualIn(pool_,frames_,host_,priest,hooks_.setFrame);
}
// 현재 방향 조회→첫604 실제 삭제→다시 현재 TYPE의 E 프레임 조회/지정 순서를 보존한다.
void RawDaisRegular::StopPriestRitualIn(const SidPool& pool,std::span<const RiftTypeFrames> frames,SquidProcessHost& host,Sid priest,
    const std::function<void(Sid,std::int32_t,std::uint32_t)>& setFrame) {
    const auto raw=pool.Slot(priest);const auto frame=pool.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,36)):std::int32_t(raw[34]);
    const auto direction=RawPathAnimation::Direction(frames[raw[10]],frame);if (direction<=9 || direction>=15) return;
    if (auto* process=host.FindEvent(priest,604)) host.Kill(*process,0);DaisNeed(setFrame)(priest,frames[pool.Slot(priest)[10]].Run('E').first,0);
}
// new/Shared효과뒤현재단계와좌표를다시읽는다.
void RawDaisRegular::Collapse(Sid dais) const {
    if (DaisNeed(hooks_.reserveShared)()) DaisNeed(hooks_.addShared)(dais,0x305,kInitialDelay);const auto point=ritual_.StagePoint(dais,ritual_.Stage(dais));DaisNeed(hooks_.playAt)(point.x,point.y,"altarBurnCollapse.wav",0);
}
// 실제 코드의extra2/altarM/0또는45/Shared306순서다.
void RawDaisRegular::StartSacrifice(Sid dais) const {
    auto raw=DaisRaw(pool_,dais);raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]|=2;const auto altar=rune_.AltarAt(dais);if (altar.value && altar.value<pool_.Capacity()) DaisNeed(hooks_.setFrame)(altar,First(altar,'M'),0);
    const auto count=Count(dais);if (count==1) DaisNeed(hooks_.setFrame)(dais,0,0);else if (count==2) DaisNeed(hooks_.setFrame)(dais,45,0);if (DaisNeed(hooks_.reserveShared)()) DaisNeed(hooks_.addShared)(dais,0x306,kInitialDelay);
}
// 전역169코드구간을사용하며현재Dais타입의범용한번감김Advance로대신하지않는다.
float RawDaisRegular::SacrificeAdvance(Sid dais) const {
    const auto current=CurrentFrame(dais);const auto type=state_.sacrificeType;if (type>=frames_.size() || current<0 || static_cast<std::size_t>(current)>=frames_[type].Codes().size()) throw std::out_of_range("Dais 희생 코드 범위");
    const auto run=frames_[type].Run(frames_[type].Codes()[static_cast<std::size_t>(current)].side);auto next=run.first;
    if (run.first<=current && static_cast<std::int64_t>(current)<static_cast<std::int64_t>(run.first)+run.count) { if (run.count<=0) throw std::logic_error("Dais 희생 구간 길이 오류");next=run.first+(current-run.first+1)%run.count; }
    if (current<=next) { DaisNeed(hooks_.setFrame)(dais,next,0);return kSacrificeDelay; }
    DaisRaw(pool_,dais)[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&=static_cast<std::uint8_t>(~2U);FinishSacrifice(dais);return 0;
}
// 지연 시각은실제부착성공뒤에만읽으며두음향의new실패는독립이다.
void RawDaisRegular::DelaySound(Sid dais,std::string_view name,double delay) const {
    if (!DaisNeed(hooks_.reserveSound)()) return;const auto sound=DaisNeed(hooks_.loadSound)(name,0,kSoundFlags);const auto process=DaisNeed(hooks_.attachSound)(dais,sound,0,kSoundFlags);if (process) DaisNeed(hooks_.setSoundStart)(process,DaisNeed(hooks_.wallSeconds)()+delay);
}
// 최초stage를유지하되하위효과뒤사제/권한/포획조건은현재raw에서다시읽는다.
void RawDaisRegular::Progress(Sid dais) const {
    const auto stage=ritual_.Stage(dais);const auto priest=ritual_.FindStrongOwnedPriest(dais);if (priest.value && priest.value<pool_.Capacity()) StopPriestRitual(priest);
    if (stage<4) { if (Boss()) BridgeCommand(dais,{},0);Collapse(dais);return; }
    const auto point=Position(dais);DaisNeed(hooks_.playAt)(point.x,point.y,"itIsDone2.wav",0);const bool upgrade=Contained(dais,4,pool_.Slot(dais)[10]).value!=0;
    DelaySound(dais,upgrade ? "priestSacrifice4.wav" : "priestSacrifice2.wav",upgrade ? 1.5 : 4.0);DelaySound(dais,"thunderCrack.wav",kThunderDelay);
    if (upgrade) { StartSacrifice(dais);return; }if (CapturedWord(dais) || state_.rune.SuppressDuration()) { const auto altar=rune_.AltarAt(dais);if (altar.value && altar.value<pool_.Capacity()) AltarEscape(altar); }
}
// 이탈전에원본의Dais조회도수행하고현재altarL프레임및24예약을연결한다.
void RawDaisRegular::AltarEscape(Sid altar) const {
    static_cast<void>(FindDaisAt(altar));DaisNeed(hooks_.setFrame)(altar,First(altar,'L'),0);if (DaisNeed(hooks_.reserveShared)()) DaisNeed(hooks_.addShared)(altar,24,kSacrificeDelay);
}
// 내용물 첫 후보/next는 genus 진단 전에 저장하고 복귀 뒤 현재 TYPE/owner/후보 조건/동맹을 읽는다.
bool RawDaisRegular::CanBridge(Sid dais,Sid priest) const {
    if (ritual_.Stage(dais)==4 && ritual_.HasAbstractRune(dais)) return false;const auto altar=rune_.AltarAt(dais);if (!altar.value || altar.value>=pool_.Capacity()) return false;
    const auto direction=[this,altar] { // 두 원문 직접 호출은 각각 현재 TYPE/프레임의 signed 코드 BYTE를 읽는다.
        const auto type=pool_.Slot(altar)[10];if (type>=frames_.size()) throw std::out_of_range("Dais 자격의 현재 제단 TYPE 범위");return RawPathAnimation::Direction(frames_[type],CurrentFrame(altar));
    };
    if (direction()==12 || direction()==11) return false;
    RawContainedFinder finder(pool_,carrier_.FinderState());auto captured=finder.Begin(dais,false);
    const auto genus=[this,priest] { // 진단이 고친 현재 사제 TYPE의 genus를 같은 실제 표에서 다시 읽는다.
        const auto type=pool_.Slot(priest)[10];if (type>=types_.size()) throw std::out_of_range("Dais 자격의 현재 사제 TYPE 범위");return types_[type].flags2;
    };
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (patch && state_.debug && !(genus()&0x200000)) Assert("priest.getGenus() & gPRIEST",".\\Dais.cpp",0x140);
    if (!(genus()&0x200000) || Owner(priest)!=Owner(dais)) return false;
    while (captured.value) { // 진단 전 저장한 첫 후보/next에서 계속하며 현재 mask/kind와 현재 사제 종류를 비교한다.
        const auto raw=pool_.Slot(captured);
        if ((raw[20]&0x20U) && Read(raw,18,2)==PriestType()) {
            if (!patch) return true;const auto originalOwner=raw[12]&127U;return Alliance(Owner(altar),originalOwner)==0;
        }
        captured=finder.Next();
    }
    return false;
}
// 현재첫Path form을같은mapper/종속/Host에서조회한다.
std::int32_t RawDaisRegular::MoveAwayDepth(Sid priest) const {
    const auto type=DaisNeed(hooks_.processType)(pool_.Slot(priest)[10]);const auto form=lookup_.Find(priest,type);if (!form.value) return 0;const auto* process=dynamic_cast<const RawPathProcess*>(host_.FromForm(form));if (!process) throw std::logic_error("Dais 현재 Path form 실제 프로세스 누락");
    const auto pid=Read(pool_.Slot(form),18); // Host.FromForm과 같은 현재 Form+0x12 DWORD 번호를 저장해 진단 중 경로 삭제를 확인한다.
    const auto current=[this,process,pid] { if (!host_.IsRegistered(pid,process)) throw std::logic_error("Dais 이동 깊이 진단 중 실제 경로 수명 종료");return process->Runtime().MoveAwayDepth(); }; // 같은 저장한 경로의 현재 DWORD를 다시 읽는다.
    if (pool_.Edition()==OriginalEdition::Patch1078 && state_.debug) {
        const auto saved=current(); // 두 진단의 비교는 최초 depth를 쓰고 반환만 현재 depth를 다시 읽는다.
        if (saved<0) Assert("0 <= p->moveAwayDepth","\\Ns\\O\\Carrier.cpp",0x87b);
        if (state_.debug && saved>3) Assert("p->moveAwayDepth <= MAX_MOVE_AWAY_DEPTH","\\Ns\\O\\Carrier.cpp",0x87c);
    }
    return current();
}
// 실제명령꼬리와마지막꼭짓점의위치를원본과같은순서로지정한다.
void RawDaisRegular::BridgeCommand(Sid dais,Sid priest,std::uint32_t currentStage) const {
    if (!priest.value) priest=ritual_.FindStrongOwnedPriest(dais);if (!priest.value || !CanBridge(dais,priest) || ritual_.Stage(dais)>=5) return;
    if (!priest.value || priest.value>=pool_.Capacity()) return;const auto stage=ritual_.Stage(dais)+(currentStage ? 0U : 1U);if (ritual_.Stage(dais)>=5) return;if (stage>4) throw std::logic_error("Dais 손상 목표5의 원본 미초기화 좌표 경계");
    const auto target=ritual_.StagePoint(dais,stage),point=Position(priest);if (Integer(point.x)==Integer(target.x) && Integer(point.y)==Integer(target.y)) { ritual_.Ritual(dais,priest,0);return; }
    if (!Boss()) return;const auto depth=MoveAwayDepth(priest);const auto next=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(depth)+1);if (next>=4) return;
    auto* process=path_.Start({priest,{target.x,target.y},1,0,dais.value,stage,0});if (!process) { if (pool_.Edition()==OriginalEdition::Patch1078 && state_.debug) Assert("p",".\\Dais.cpp",0x18e);return; }
    process->Runtime().ReplacePoint(process->Runtime().Count()-1,{target.x,target.y});const auto meta=DaisNeed(hooks_.pathMetadata)(priest);process->Prepare(0,meta.speed,meta.turningSpeed);process->Runtime().SetMoveAwayDepth(next);
}
// patch의비로컬획득과CD의생략분기를추가추정없이보존한다.
void RawDaisRegular::GrantCapturedType(std::uint32_t type,std::uint32_t owner) const {
    Player(owner);const bool local=owner==state_.localOwner;state_.SetGrantFlag(owner,0);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (type==state_.NuggetType()) { if (local) { DaisNeed(hooks_.addLocalSp)(5000);DaisNeed(hooks_.chat)({DaisChat::Kind::Stormpower}); }else if (patch) DaisNeed(hooks_.normalizeForeignSp)(owner);return; }
    if (type==state_.noIslandType) { if (local) DaisNeed(hooks_.localSpecial)(0);return; }
    if (local) { DaisNeed(hooks_.localType)(type);state_.selectedType=type; }else if (patch) DaisNeed(hooks_.grantForeignType)(owner,type);
}
// 실제cue/altar/현재count·owner/포획삭제/획득/UI/사제존재순서를유지한다.
void RawDaisRegular::FinishSacrifice(Sid dais) const {
    DaisNeed(hooks_.stopVoice)(state_.voiceCue,0,0);const auto altar=rune_.AltarAt(dais);if (!altar.value) { if (pool_.Edition()==OriginalEdition::Patch1078) Assert("altarSid",".\\Dais.cpp",0x2ba);else throw std::logic_error("Dais altarSid assert"); }DaisNeed(hooks_.setFrame)(altar,First(altar,'P'),0);const auto originalOwner=Read(pool_.Slot(altar),12,2);
    if (!state_.rune.SuppressDuration() && Contained(dais,4,pool_.Slot(dais)[10]).value) {
        const auto count=Count(dais);if (count>=kUpgradeFirst.size()) throw std::out_of_range("Dais 다음 첫 프레임 범위");DaisNeed(hooks_.setFrame)(dais,kUpgradeFirst[count],0);const auto owner=Owner(dais);Player(owner);
        if (pool_.Edition()==OriginalEdition::Patch1078) { auto destination=state_.PlayerResultSlot(owner);const auto currentCount=Count(dais);for (std::size_t index=0;index<4;++index) destination[index]=static_cast<std::uint8_t>(currentCount>>(index*8)); } // PATCH는 Count 앞에서 Player 주소 전체를 저장한다.
        else { const auto currentCount=Count(dais);state_.SetPlayerResult(owner,currentCount); } // CD는 owner만 저장하고 복귀 뒤 현재 기록에 쓴다.
        auto captured=Contained(dais,0,pool_.Slot(dais)[10]);if (captured.value && captured.value<pool_.Capacity()) DaisNeed(hooks_.destroy)(captured,kConsumedFlags);captured=Contained(dais,0,PriestType());
        if (captured.value && captured.value<pool_.Capacity()) { DaisNeed(hooks_.destroy)(captured,kConsumedFlags);SetRawSquidWord(pool_,altar,0,state_.BridgeType(),hooks_.notifySurface); }
        if (Owner(dais)==state_.localOwner) DaisNeed(hooks_.chat)({DaisChat::Kind::Upgrade});
    } else { const auto type=CapturedWord(dais);if (!state_.rune.SuppressDuration()) GrantCapturedType(type,Owner(dais));Player(Owner(dais));state_.SetPlayerResult(Owner(dais),type); }
    // 원본은항상플레이어1..8을순서대로갱신한다.
    for (std::uint32_t owner=1;owner<=kPlayerCount;++owner) DaisNeed(hooks_.refreshOwner)(owner);
    if (!state_.rune.SuppressDuration()) {
        if (Owner(dais)==state_.localOwner) DaisNeed(hooks_.refreshLocal)();if (originalOwner==state_.localOwner) DaisNeed(hooks_.chat)({DaisChat::Kind::LocalPriestDead,originalOwner,Owner(dais),DaisNeed(hooks_.multiplayerStatus)()});else if (Owner(dais)!=state_.localOwner) DaisNeed(hooks_.chat)({DaisChat::Kind::OtherPriestDead,originalOwner,Owner(dais),0});
        if (!state_.rune.SuppressDuration()) return;
    }
    if (Owner(dais)==state_.localOwner) {
        DaisNeed(hooks_.addLocalSp)(5000);bool deadOpponent=false;Player(state_.localOwner);
        // 현재 각 플레이어의 사제 존재와 단방향 동맹을 효과 뒤 현재 전역으로 읽는다.
        for (std::uint32_t owner=1;owner<=kPlayerCount;++owner) if (!DaisNeed(hooks_.priestExists)(owner) && !Alliance(state_.localOwner,owner)) deadOpponent=true;
        if (deadOpponent) DaisNeed(hooks_.chat)({DaisChat::Kind::Stormpower});
    }
}
// 다른월드의소비자등록을거부할때사용한다.
const SidPool& RawDaisRegular::Pool() const { return pool_; }
// 기존실제Frame/Host/Damageable/패치SP와CD정확localsetter를공유한다.
DaisRegularHooks MakeDaisRegularHooks(const SidPool& pool,const SquidFrame& frame,SquidProcessHost& host,const RawAssetRegular& assets,
    SquidReward& reward,ScrambledSpStore* spStore,DaisRegularState& state,std::function<void()> refreshSp,DaisRegularHooks hooks) {
    if (&pool!=&frame.Pool() || &pool!=&host.Pool() || &pool!=&assets.Pool() || &pool!=&reward.Pool() || !refreshSp || (pool.Edition()==OriginalEdition::Patch1078 && !spStore)) throw std::invalid_argument("Dais 실제Frame/Host/SP 풀 연결 누락");
    hooks.setFrame=[&frame](Sid sid,std::int32_t value,std::uint32_t flags) { frame.Set(sid,value,flags); };
    hooks.addShared=[&host](Sid sid,std::uint32_t event,float payload) { host.AddSharedRegular(sid,event,payload); };
    hooks.damageable=[&assets](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return assets.DamageableHandle(sid,event,count,payload); };
    hooks.addLocalSp=[&pool,&reward,&state,refreshSp=std::move(refreshSp)](float delta) { if (pool.Edition()==OriginalEdition::Patch1078) reward.AddSp(state.localOwner,delta);else { state.sp.cdLocalSp=static_cast<float>(static_cast<double>(state.sp.cdLocalSp)+static_cast<double>(delta));if (state.sp.fixedSpB) state.sp.cdLocalSp=100000.0F;refreshSp(); } };
    // 비로컬 SP는 호출자가 실제 getter→5000 가산→정책 setter로 연결한다. 현재 known/Editor/UI를 모르는 직접 저장 대체는 설치하지 않는다.
    return hooks;
}
}
