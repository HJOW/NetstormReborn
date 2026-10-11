// 건설 프로세스의 원본 읽기/쓰기 순서와 경계 뒤의 현재 자료 재조회를 보존한다.
// 이력: 2026-10-11 추가.
#include "o/RawConstructionProcess.h"
#include <bit>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// raw 타입·상태·좌표·HP 위치와 판본별 소유자/extra(abstract 비트) 위치다.
constexpr std::size_t kType=10,kState=11,kX=14,kY=18,kHp=26,kPatchOwner=34,kCdOwner=32,kPatchExtra=40,kCdExtra=35;
// 상태 비트 free·dead·void와 extra의 abstract 비트다.
constexpr std::uint8_t kFree=1,kDead=2,kVoid=4,kAbstract=1;
// 지속 시간 0의 기본 건설 시간(00501450 / CD 00506940·00506948)과 진행음을 붙이는 경계(005012d0, CD는 비트 0x3f800000)다.
constexpr double kDefaultSeconds=10.0;
constexpr float kSoundSeconds=1.0F;
// 부착 flags(client 번호 form), 소리 프로세스 flags(부모 위치 반복 재생), 완성 뒤 재등록 flags다.
constexpr std::uint32_t kAttachFlags=0x10,kSoundFlags=1,kRepopFlags=9;
// quickVortex가 적용되는 genus(vortex)다.
constexpr std::uint32_t kVortexGenus=0x200;
// 예측 영역의 첫 번호다. 이보다 작은 번호는 예약 슬롯이다.
constexpr std::uint32_t kFirstSid=5;
// 원본 소리 이름이다(005071e8·005071f8 / CD 0053fcf4·0053fd04).
constexpr std::string_view kBuildSound="jimBuild.wav",kDoneSound="buildDone.wav";
// 정렬되지 않은 raw 값을 little endian으로 읽는다. width는 1~4다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    // 지정 폭의 바이트만 낮은 자리부터 합친다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return result;
}
// 원본 __ftol의 하위 signed DWORD다. 비유한 값과 64비트 범위 밖은 하위 DWORD가 0이다. 004e49c0 / CD 004f161c.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
// 미연결 효과는 첫 확보/쓰기 전에 거부한다. host가 있으면 attach 경계는 쓰지 않으므로 비어 있어도 된다.
RawConstructionProcess::RawConstructionProcess(SidPool& pool,std::span<const RiftTypeRecord> types,ConstructionProcessState& state,
    ConstructionProcessHooks hooks,SquidProcessHost* host)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)),host_(host) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || (host && &host->Pool()!=&pool) ||
        !hooks_.reserve || !hooks_.effect || (!hooks_.attach && !host) || !hooks_.loadSound || !hooks_.reserveSound || !hooks_.attachSound ||
        !hooks_.refresh || !hooks_.maxHitPoints || !hooks_.updateDisplay || !hooks_.removeContained || !hooks_.playAt ||
        !hooks_.readySound || !hooks_.play || !hooks_.unpop || !hooks_.repop || !hooks_.push)
        throw std::invalid_argument("건설 프로세스 자료/효과 연결 오류");
}
// 현재 raw 타입이 타입 표 안인지 확인한다.
std::uint32_t RawConstructionProcess::TypeOf(Sid sid) const {
    const std::uint32_t type=pool_.Slot(sid)[kType];
    if (type>=types_.size()) throw std::out_of_range("건설 프로세스 부모 타입 오류");
    return type;
}
// 경계 호출 뒤의 현재 타입으로 genus를 다시 읽는다.
std::uint32_t RawConstructionProcess::Genus(Sid sid) const { return types_[TypeOf(sid)].flags2; }
// 패치판은 DWORD 전체, CD판은 WORD를 부호 확장해 읽는다.
std::int32_t RawConstructionProcess::HitPoints(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    if (pool_.Edition()==OriginalEdition::Patch1078) return std::bit_cast<std::int32_t>(Read(raw,kHp));
    return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(raw,kHp,2)));
}
// 판본의 폭만큼만 쓴다. CD판은 값의 하위 WORD만 남는다.
void RawConstructionProcess::SetHitPoints(Sid sid,std::int32_t value) const {
    auto raw=pool_.AllocatedBytes(sid);const std::size_t width=pool_.Edition()==OriginalEdition::Patch1078 ? 4 : 2;
    // 낮은 바이트부터 판본의 HP 폭까지만 쓴다.
    for (std::size_t i=0;i<width;++i) raw[kHp+i]=static_cast<std::uint8_t>(std::bit_cast<std::uint32_t>(value)>>(8*i));
}
// 원본 조건 `번호 < 5 || 번호 >= 15000(CD 6000)`이다.
bool RawConstructionProcess::Confirmed(Sid sid) const { return sid.value<kFirstSid || sid.value>=pool_.Layout().serverFirst; }
// new 실패는 생성자를 부르지 않고 끝난다.
void RawConstructionProcess::Start(Sid parent,float duration) const {
    if (!hooks_.reserve()) return;
    Construct(parent,duration);
}
// 최대 HP 조회 뒤의 현재 시각을 사용한다. 시작·소리 필드나 부모 HP를 덮어쓰지 않는다.
void RawConstructionProcess::ResetEnd(ConstructionProcessFields& fields,Sid parent,std::uint32_t damage) const {
    const auto maximum=hooks_.maxHitPoints(parent);
    auto candidate=maximum;
    if (pool_.Edition()==OriginalEdition::Cd1072) {
        const auto progress=Ftol(((state_.now-fields.start)/(fields.end-fields.start))*static_cast<double>(maximum));
        // x86 sub의 DWORD 감김 뒤 signed 비교를 보존한다. 결과는 기본 시간 helper의 미사용 인자다.
        candidate=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(progress)-damage);
        if (candidate<=1) candidate=1;
    }
    static_cast<void>(DefaultTime(fields,parent,candidate));
}
// 원본의 null 프로세스 접근 대신 유효한 실제 건설 프로세스 연결을 요구한다.
void RawConstructionProcess::ResetEndFromForm(Sid form,std::uint32_t damage) const {
    if (!host_) throw std::logic_error("건설 끝 시각 변경에 프로세스 host가 필요합니다");
    auto* process=dynamic_cast<ConstructionProcess*>(host_->FromForm(form));
    if (!process || process->Form()!=form) throw std::invalid_argument("건설 끝 시각 변경의 form 연결 오류");
    ResetEnd(process->Fields(),process->Parent(),damage);
}
// CD helper가 받는 부모/HP는 무시하고 현재 전역 시각을 더한다. 패치의 직접 쓰기도 같은 결과다.
float RawConstructionProcess::DefaultTime(ConstructionProcessFields& fields,Sid,std::int32_t) const {
    fields.end=state_.now+kDefaultSeconds;fields.dirty|=4;return static_cast<float>(kDefaultSeconds);
}
// 표시 효과 뒤의 현재 시각으로 시작/끝을 정하고, 부착 뒤에 진행음을 붙인다.
void RawConstructionProcess::Construct(Sid parent,float duration) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    ConstructionProcessFields fields;
    // 패치판의 실수 비교와 CD판의 `비트 & 0x7fffffff` 검사는 0/-0에서만 참이다(NaN은 거짓).
    const bool instant=duration==0.0F;
    hooks_.effect(parent,instant);
    fields.start=state_.now;fields.dirty|=2;
    if (instant) { fields.end=state_.now+kDefaultSeconds;fields.dirty|=4;duration=static_cast<float>(kDefaultSeconds); }
    fields.end=static_cast<double>(duration)+state_.now;fields.dirty|=4;
    ConstructionProcessFields* attached=nullptr;
    if (host_) {
        // 패치판에서 부모가 free/dead이면 붙지 않는다. 원본은 그 객체를 해제하지 않고 남은 쓰기를 계속한다.
        auto* process=static_cast<ConstructionProcess*>(host_->Attach(std::make_unique<ConstructionProcess>(*this,fields),state_.processType,parent,kAttachFlags));
        if (process) attached=&process->Fields();
    } else attached=hooks_.attach(fields,state_.processType,parent,kAttachFlags);
    auto& live=attached ? *attached : fields;
    // 패치판은 fcomp(비교 불가는 소리 없음), CD판은 지속 시간 비트의 signed 비교다.
    const bool audible=patch ? duration>kSoundSeconds : std::bit_cast<std::int32_t>(duration)>std::bit_cast<std::int32_t>(kSoundSeconds);
    if (audible) {
        live.sound=hooks_.loadSound(kBuildSound);
        // 확보 경계 뒤의 현재 소리 필드를 다시 읽어 넘긴다.
        if (hooks_.reserveSound()) hooks_.attachSound(parent,live.sound,0,kSoundFlags);
    } else live.sound=0;
    hooks_.refresh();
}
// 최대 HP 경계 뒤의 현재 시각으로 비율을 계산하고, 표시 경계 뒤의 abstract 비트와 Kill 뒤의 권한을 다시 읽는다.
void RawConstructionProcess::Run(const ConstructionProcessFields& fields,Sid parent,const ConstructionProcessKill& kill) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto maximum=hooks_.maxHitPoints(parent);
    // x87 순서: (현재-시작)/(끝-시작)에 정수 최대 HP를 곱한 뒤 0방향으로 절삭한다.
    auto value=Ftol(((state_.now-fields.start)/(fields.end-fields.start))*static_cast<double>(maximum));
    if (state_.quickBuild || (state_.quickVortex && (Genus(parent)&kVortexGenus))) value=maximum;
    // 두 판본 모두 min(값, 최대)이 음수(CD는 0 이하)면 0, 아니면 그 min이다.
    if (value>maximum) value=maximum;
    if (value<0) value=0;
    SetHitPoints(parent,value);
    hooks_.updateDisplay(parent);
    if (!(pool_.Slot(parent)[patch ? kPatchExtra : kCdExtra]&kAbstract) && !kill(0)) return;
    if (value==maximum && state_.boss) static_cast<void>(kill(0));
}
// 각 경계 뒤에 부모의 현재 HP·소유자·좌표·타입과 전역을 다시 읽는다. 최대 HP와 HP는 처음 읽은 값을 끝까지 쓴다.
void RawConstructionProcess::Complete(Sid parent) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const std::size_t owner=patch ? kPatchOwner : kCdOwner,extra=patch ? kPatchExtra : kCdExtra;
    const auto maximum=hooks_.maxHitPoints(parent);const auto hitPoints=HitPoints(parent);
    hooks_.removeContained(parent,state_.soundProcessType);
    hooks_.refresh();
    if (pool_.Slot(parent)[owner]==state_.localPlayer && Confirmed(parent)) {
        const auto raw=pool_.Slot(parent);
        hooks_.playAt(std::bit_cast<float>(Read(raw,kX)),std::bit_cast<float>(Read(raw,kY)),kDoneSound,0);
        if (const auto ready=hooks_.readySound(TypeOf(parent))) hooks_.play(ready);
    }
    if (!state_.boss || !Confirmed(parent) || hitPoints!=maximum || !(pool_.Slot(parent)[extra]&kAbstract)) return;
    hooks_.unpop(parent,0);
    // 0042b050 / CD 인라인: void가 아니면 원본 assert다. abstract 비트만 끄고 다른 비트는 보존한다.
    if (!(pool_.Slot(parent)[kState]&kVoid)) throw std::logic_error("건설 완료의 abstract 해제는 void 상태여야 합니다");
    pool_.AllocatedBytes(parent)[extra]&=static_cast<std::uint8_t>(~kAbstract);
    hooks_.repop(parent,kRepopFlags);
    if (!(state_.pushGenus&Genus(parent))) return;
    const auto raw=pool_.Slot(parent);
    hooks_.push(std::bit_cast<float>(Read(raw,kX)),std::bit_cast<float>(Read(raw,kY)),raw[kType],raw[owner]);
}
// 완료 처리 뒤의 현재 상태/타입으로 HP 채움 여부를 정하고 최대 HP를 다시 묻는다.
void RawConstructionProcess::FormDestroyed(Sid parent) const {
    Complete(parent);
    const auto raw=pool_.Slot(parent);
    if ((raw[kState]&(kFree|kDead)) || raw[kType]==state_.daisType) return;
    SetHitPoints(parent,hooks_.maxHitPoints(parent));
}
// 동일 풀 연결 검사용 참조다.
const SidPool& RawConstructionProcess::Pool() const { return pool_; }

// 생성자가 계산한 필드를 복사해 보관한다.
ConstructionProcess::ConstructionProcess(const RawConstructionProcess& logic,const ConstructionProcessFields& fields):logic_(logic),fields_(fields) {}
// Kill 뒤에는 미리 읽어 둔 host·번호·주소만으로 등록 여부를 묻는다. 부착 전에는 Kernel에 없으므로 실행되지 않는다.
void ConstructionProcess::RunFrame() {
    auto* host=Host();if (!host) return;
    const auto pid=Pid();const SquidProcess* self=this;
    logic_.Run(fields_,Parent(),[host,pid,self,this](std::uint32_t flags) { host->Kill(*this,flags);return host->IsRegistered(pid,self); });
}
// 원본 flags 인자는 읽지 않는다.
void ConstructionProcess::OnFormDestroy(std::uint32_t) { logic_.FormDestroyed(Parent()); }
ConstructionProcessFields& ConstructionProcess::Fields() { return fields_; }
const ConstructionProcessFields& ConstructionProcess::Fields() const { return fields_; }

// 보상 모듈의 최대 HP와 밀어내기 모듈을 같은 풀에서만 연결한다. 밀어내기의 반환값(적대 거부)은 원본처럼 버린다.
ConstructionProcessHooks MakeConstructionProcessHooks(const SidPool& pool,const SquidReward& reward,const RawConstructionEffects* effects,
    ConstructionProcessHooks hooks) {
    if (&pool!=&reward.Pool() || (effects && &pool!=&effects->Pool())) throw std::invalid_argument("건설 프로세스 어댑터의 풀 연결 오류");
    hooks.maxHitPoints=[&reward](Sid sid) { return reward.MaxHitPoints(sid); };
    if (effects) hooks.push=[effects](float x,float y,std::uint32_t type,std::uint32_t owner) { static_cast<void>(effects->Displace(x,y,type,owner)); };
    return hooks;
}
// 통지의 둘째 인자를 단정도 비트 그대로 지속 시간으로 넘긴다.
ConstructionNoticeHooks MakeConstructionProcessNoticeHooks(const SidPool& pool,const RawConstructionProcess& process,ConstructionNoticeHooks hooks) {
    if (&pool!=&process.Pool()) throw std::invalid_argument("통지 건설 시작 어댑터의 풀 연결 오류");
    hooks.start=[&process](Sid sid,std::uint32_t duration) { process.Start(sid,std::bit_cast<float>(duration)); };
    return hooks;
}
}
