// 소리 프로세스의 실행·종료 순서를 원본 기계어의 읽기/호출 순서대로 복원한다.
#include "client/SoundProcess.h"
#include <bit>
#include <memory>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 부모 raw 객체에서 읽는 필드 위치(두 판본 공통): 타입, 상태 단어, 좌표다.
// 원본: 004ab070 / CD 00453900의 `[esi+0xa]`(타입 조회 004ac550 / CD 004abf50), `[esi+0xc]`, `[esi+0xe]`, `[esi+0x12]`.
// 이력: 2026-10-10 추가.
constexpr std::size_t kType=10,kWord=12,kX=14,kY=18;
// 부착 flags다. form을 client 번호로 확보한다.
// 원본: 004ab283 / CD 00453855의 `push 0x10`.
// 이력: 2026-10-10 추가.
constexpr std::uint32_t kAttachFlags=0x10;
// 패치판 debug에서 보고하는 조건식과 줄 번호, 그리고 두 판본 공통의 정지 확인 보고다.
// 원본: 004ab133("playOnce || !playPriority", 0x35), 004ab15c("playOnce || playAt", 0x36),
//       004ab056 / CD 004538d9("!isSoundPlaying( currentSound )", 0x15).
// 이력: 2026-10-10 추가.
constexpr std::string_view kPriorityExpression="playOnce || !playPriority",kPlayAtExpression="playOnce || playAt",
    kStoppedExpression="!isSoundPlaying( currentSound )";
constexpr int kPriorityLine=0x35,kPlayAtLine=0x36,kStoppedLine=0x15;
// 비정렬 raw 좌표를 산술 변환 없이 읽는다. NaN과 부호 있는 0도 비트 그대로 재생 함수에 넘어간다.
// 이력: 2026-10-10 추가.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 낮은 바이트부터 네 바이트를 조합한다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(bits);
}
}

// 원본 생성자는 +0xc(재생 중인 소리)를 0, +0x20(시작 시각)을 0.0으로 둔다.
SoundProcess::SoundProcess(SoundProcessSystem& system,SoundHandle sound,SoundHandle alternate,std::uint32_t flags)
    :system_(system),sound_(sound),alternate_(alternate),flags_(flags) {}

// 재생 함수가 부모를 바꿀 수 있으므로 타입과 프레임 번호를 호출 뒤에 다시 읽는다(원본도 타입 조회를 다시 한다).
void SoundProcess::MarkTypeFrame() {
    auto& state=system_.state_;
    state.typeFrames[system_.host_.Pool().Slot(Parent())[kType]]=state.frame;
}

// 원본 assert 보고는 돌아온 뒤 실행을 계속한다.
void SoundProcess::Report(std::string_view expression,int line) const {
    if (system_.hooks_.report) system_.hooks_.report(expression,line);
}

// 읽는 순서: 소리 선택(부모 상태 단어) → 벽시계 → flags → 타입의 마지막 소리 프레임 → (패치 debug 보고) → 좌표 → 재생.
void SoundProcess::RunFrame() {
    auto* host=Host();
    // 부착하지 않은 프로세스는 Kernel에 없으므로 실행되지 않는다.
    if (!host) return;
    auto& state=system_.state_;
    const auto& hooks=system_.hooks_;
    const bool patch=host->Pool().Edition()==o::OriginalEdition::Patch1078;
    // [원본] 패치판 debug는 여기서 isRobust(1)(0041be80: form/부모 SID의 풀 범위 검사, 항상 참)을 부른다.
    //        SquidProcessHost::Attach가 두 SID를 보장하므로 따로 검사하지 않는다.
    const auto raw=host->Pool().Slot(Parent());
    SoundHandle sound=sound_;
    if ((flags_&kSoundUseAlternate) && (raw[kWord]|raw[kWord+1])) sound=alternate_;
    const double now=hooks.wallSeconds();
    // 패치판은 "현재 < 시작"이거나 비교할 수 없으면(NaN) 돌아가고, CD판은 "시작 > 현재"일 때만 돌아간다.
    if (patch ? !(now>=startTime_) : startTime_>now) return;
    const bool playAt=(flags_&kSoundPlayAt)!=0,playOnce=(flags_&kSoundPlayOnce)!=0;
    const std::uint32_t priority=flags_&kSoundPlayPriority;
    const bool played=state.typeFrames[raw[kType]]==state.frame;
    if (patch && state.debug) {
        if (!playOnce && priority) Report(kPriorityExpression,kPriorityLine);
        // 원본은 첫 보고 뒤 debug 전역을 다시 읽는다.
        if (state.debug && !playOnce && !playAt) Report(kPlayAtExpression,kPlayAtLine);
    }
    if (playAt) {
        const float x=Coordinate(raw,kX),y=Coordinate(raw,kY);
        if (!playOnce) {
            // 반복 재생: 재생 중이면 위치/음량 갱신을 위해 매 프레임 부르고, 아니면 이 타입이 이번 프레임에 조용할 때만 시작한다.
            if (current_!=0 || !played) current_=hooks.playLoopAt(x,y,current_,sound);
            if (current_!=0) MarkTypeFrame();
            return;
        }
        // 한 번 재생: 반환값을 기억하지 않는다. 이미 이 타입이 소리를 냈으면 재생 없이 끝낸다.
        if (!played) { hooks.playOnceAt(x,y,sound,priority,0);MarkTypeFrame(); }
    } else if (current_==0 && !played) {
        current_=hooks.play(sound,playOnce ? 0U : 1U,0,0,priority,0);
        MarkTypeFrame();
    }
    // Kill은 form을 지우고 그 안에서 이 객체가 소멸한다. 이 뒤로는 멤버를 건드리지 않는다.
    if (playOnce) host->Kill(*this,0);
}

// 한 번 재생 소리는 프로세스가 끝나도 끝까지 들리게 둔다.
void SoundProcess::OnFormDestroy(std::uint32_t) {
    if (current_==0 || (flags_&kSoundPlayOnce)) return;
    const auto& hooks=system_.hooks_;
    hooks.stop(current_);
    if (hooks.isPlaying(current_)!=0) Report(kStoppedExpression,kStoppedLine);
}

// 아래 접근자는 원본 객체 필드를 그대로 읽고 쓴다.
SoundHandle SoundProcess::Current() const { return current_; }
void SoundProcess::SetCurrent(SoundHandle current) { current_=current; }
SoundHandle SoundProcess::Sound() const { return sound_; }
SoundHandle SoundProcess::Alternate() const { return alternate_; }
std::uint32_t SoundProcess::Flags() const { return flags_; }
double SoundProcess::StartTime() const { return startTime_; }
void SoundProcess::SetStartTime(double time) { startTime_=time; }

// 실행 중에 빠진 효과를 만나지 않도록 구성 단계에서 확인한다.
SoundProcessSystem::SoundProcessSystem(o::SquidProcessHost& host,SoundProcessState& state,SoundProcessHooks hooks)
    :host_(host),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.wallSeconds || !hooks_.playLoopAt || !hooks_.playOnceAt || !hooks_.play || !hooks_.stop || !hooks_.isPlaying)
        throw std::invalid_argument("소리 프로세스 필수 효과 누락");
}

// 타입 번호는 부착하는 순간의 전역 값을 쓴다. 번호가 프로세스 타입 범위 밖이면 Attach가 거부한다.
SoundProcess* SoundProcessSystem::Add(o::Sid parent,SoundHandle sound,SoundHandle alternate,std::uint32_t flags) {
    auto process=std::make_unique<SoundProcess>(*this,sound,alternate,flags);
    return static_cast<SoundProcess*>(host_.Attach(std::move(process),state_.processType,parent,kAttachFlags));
}

// 프로세스 host가 쓰는 풀이다.
const o::SidPool& SoundProcessSystem::Pool() const { return host_.Pool(); }

// 조회한 항목 값을 그대로 소리 프로세스의 소리로 넘긴다. 프로세스는 사제가 아니라 새 보호막에 붙는다.
o::PriestShieldHooks MakePriestShieldSoundHooks(const o::SidPool& pool,SoundList& sounds,SoundProcessSystem& system,o::PriestShieldHooks hooks) {
    if (&pool!=&system.Pool()) throw std::invalid_argument("보호막/소리 프로세스 SID 풀이 다릅니다");
    hooks.loadSound=[&sounds](std::string_view name) { return sounds.Lookup(name); };
    hooks.attachSound=[&system](o::Sid sid,std::uint32_t sound,std::uint32_t alternate,std::uint32_t flags) {
        static_cast<void>(system.Add(sid,sound,alternate,flags));
    };
    return hooks;
}
}
