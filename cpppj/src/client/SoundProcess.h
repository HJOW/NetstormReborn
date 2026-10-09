// Soundprocess.cpp(원본 .\Soundprocess.cpp / CD soundProcess.cpp): 객체에 붙어 소리를 내는 프로세스(soundProcessType, 타입 47)를 복원한다.
// 실제 소리 장치의 재생/정지는 SoundProcessHooks로 받는다(Sound.cpp의 재생 계층은 아직 옮기지 않았다).
#pragma once
#include "client/Sound.h"
#include "o/RawPriestShield.h"
#include "o/SquidProcess.h"
#include <array>

namespace netstorm::client {
// 소리 프로세스의 타입 번호 초기값이다. 부착할 때는 SoundProcessState::processType의 현재 값을 읽는다.
// 원본: 전역 soundProcessType(DAT_0054112c / CD 0051ca1c)의 초기값 0x2f.
// 이력: 2026-10-10 추가.
inline constexpr std::uint32_t kSoundProcessType=47;
// 소리 프로세스 flags(원본 객체 +0x18)의 비트다. 낮은 네 비트만 쓰인다.
//   kSoundPlayAt       : 부모 객체의 현재 좌표를 넘겨 재생한다(없으면 좌표를 넘기지 않는 전역 재생).
//   kSoundUseAlternate : 부모의 상태 단어(raw +0xc)가 0이 아니면 대체 소리를 쓴다.
//   kSoundPlayOnce     : 한 번 재생을 요청한 뒤 프로세스를 끝낸다(없으면 반복 재생하고 프로세스가 살아 있는 동안 유지한다).
//   kSoundPlayPriority : 값 8을 재생 함수에 그대로 넘긴다. 전역 재생에서는 다섯째 인자, 위치 한 번 재생에서는 넷째 인자다.
//                        이름은 원본 assert 문자열의 playPriority이며 재생 함수 안에서의 뜻은 재생 계층을 옮길 때 확정한다.
// 사용: SoundProcessSystem::Add의 flags로 조합해 준다. 보호막은 kSoundPlayAt만 쓴다.
// 원본: 004ab070 / CD 00453900의 `flags & 1/2/4/8`, assert 문자열 "playOnce || playAt"·"playOnce || !playPriority".
// 이력: 2026-10-10 추가.
inline constexpr std::uint32_t kSoundPlayAt=1,kSoundUseAlternate=2,kSoundPlayOnce=4,kSoundPlayPriority=8;

// 소리 프로세스가 읽고 쓰는 원본 전역이다. 값은 호출자(클라이언트)가 소유하고 프레임마다 갱신한다.
// 사용: 한 월드에 하나를 만들어 SoundProcessSystem에 넘긴다. 프로세스가 살아 있는 동안 유지해야 한다.
// 이력: 2026-10-10 추가.
struct SoundProcessState {
    // 프레임 카운터의 하위 32비트다(DAT_0055b4a8 / CD 0050f248). 시각 고정 때마다 1씩 늘어난다.
    std::uint32_t frame{};
    // 부착할 때 쓰는 프로세스 타입 번호다(DAT_0054112c / CD 0051ca1c).
    std::uint32_t processType{kSoundProcessType};
    // 패치판 debug 전역(DAT_005e4794)이다. 켜져 있으면 flags 조합을 검사해 assert를 보고한다. CD판은 읽지 않는다.
    bool debug{};
    // 타입마다 마지막으로 소리를 낸 프레임 번호다. 같은 프레임에 같은 타입의 소리가 여러 번 시작되지 않게 막는다.
    // 원본은 타입 레코드 안의 필드(패치 +0x1f0 / CD +0x1d0)이며 다른 소리 요청 함수도 함께 쓴다. 색인은 타입 번호다.
    std::array<std::uint32_t,256> typeFrames{};
};

// 소리 프로세스의 외부 효과다. report를 뺀 여섯 개는 모두 연결해야 한다.
// 사용: 소리 장치 계층(또는 검사용 기록 함수)을 넣어 SoundProcessSystem에 넘긴다. 인자와 반환값은 원본 함수 그대로다.
// 이력: 2026-10-10 추가.
struct SoundProcessHooks {
    // 정지와 무관하게 흐르는 벽시계(초)다. 00460d70 / CD 004011b0.
    std::function<double()> wallSeconds;
    // 위치 반복 재생 (x, y, 재생 중인 항목, 소리) → 재생 중인 항목(없으면 0). 004a9860 / CD 00438800.
    std::function<SoundHandle(float,float,SoundHandle,SoundHandle)> playLoopAt;
    // 위치 한 번 재생 (x, y, 소리, flags & 8, 0). 반환값은 쓰지 않는다. 004a9c30 / CD 00438ae0.
    std::function<SoundHandle(float,float,SoundHandle,std::uint32_t,std::uint32_t)> playOnceAt;
    // 전역 재생 (소리, 한 번 재생이면 0·아니면 1, 0, 0, flags & 8, 0) → 재생 중인 항목(없으면 0). 004a9550 / CD 004383f0.
    // 둘째 인자는 반복 재생 여부로 보인다(추정 — 재생 함수 몸체는 아직 대조하지 않았다).
    std::function<SoundHandle(SoundHandle,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> play;
    // 재생 중인 항목을 멈춘다. 004a97e0 / CD 004386f0.
    std::function<void(SoundHandle)> stop;
    // 항목이 아직 재생 중이면 0이 아닌 값이다. 004a9810 / CD 004387a0.
    std::function<std::uint32_t(SoundHandle)> isPlaying;
    // 원본 assert 보고다. 없으면 보고를 건너뛴다. 보고 뒤에도 실행은 계속된다.
    SoundAssertReport report;
};

class SoundProcessSystem;
// 부모 객체에 붙어 매 프레임 소리 재생을 요청하는 프로세스다.
// 사용: 직접 만들지 않고 SoundProcessSystem::Add로 붙인다. Kernel이 매 프레임 RunFrame을 부르고,
//       부모가 지워지거나 Kill되면 form 삭제 통지(OnFormDestroy)에서 반복 소리를 멈춘다.
// 원본: 객체 0x28바이트 — +4 form, +8 부모, +0xc 재생 중인 소리, +0x10 소리, +0x14 대체 소리, +0x18 flags, +0x20 시작 시각.
//       가상 표 00511ff4 / CD 00502020.
// 이력: 2026-10-10 추가.
class SoundProcess final : public o::SquidProcess {
public:
    // 필드 초기값만 정한다. 재생 중인 소리는 없고 시작 시각은 0.0(바로 시작)이다.
    // 원본: 004ab240 / CD 00453800의 필드 쓰기 부분. 부착(BaseProcess 생성)은 SoundProcessSystem::Add가 한다.
    // 이력: 2026-10-10 추가.
    SoundProcess(SoundProcessSystem& system,SoundHandle sound,SoundHandle alternate,std::uint32_t flags);
    // 한 프레임의 처리다. 벽시계가 시작 시각에 이르지 않았으면 아무것도 하지 않는다. 그 뒤 flags에 따라
    //   위치+반복 : 재생 중이거나 이번 프레임에 이 타입의 소리가 아직 없으면 위치 반복 재생을 요청하고 결과를 기억한다.
    //   위치+한 번: 이번 프레임에 이 타입의 소리가 아직 없으면 위치 한 번 재생을 요청한다. 어느 쪽이든 프로세스를 끝낸다.
    //   전역      : 재생 중인 소리가 없고 이번 프레임에 이 타입의 소리가 아직 없으면 전역 재생을 요청한다. 한 번 재생이면 프로세스를 끝낸다.
    // 재생을 요청했으면(반복은 재생 중인 항목이 있을 때만) 부모 타입의 마지막 소리 프레임을 현재 프레임으로 적는다.
    // 판본 차이: 벽시계나 시작 시각이 NaN이면 패치판은 실행하지 않고 CD판은 실행한다. 패치판만 debug일 때 flags 조합을 보고한다.
    // 원본: 004ab070 / CD 00453900 (vtable +0x18).
    // 이력: 2026-10-10 추가.
    void RunFrame() override;
    // form이 지워지기 직전의 통지다. 반복 소리가 재생 중이면 멈추고, 멈춘 뒤에도 재생 중이면
    // "!isSoundPlaying( currentSound )"(줄 21)를 보고한다. 한 번 재생 프로세스는 소리를 멈추지 않는다. flags 인자는 읽지 않는다.
    // 원본: 004ab030 / CD 004538b0 (vtable +0xc).
    // 이력: 2026-10-10 추가.
    void OnFormDestroy(std::uint32_t flags) override;
    // 원본 +0xc. 재생 함수가 돌려준, 지금 재생 중인 항목이다(없으면 0).
    SoundHandle Current() const;
    // 재생 중인 항목을 직접 정한다. 저장한 상태를 되살리거나 검사에서 상태를 구성할 때만 쓴다.
    void SetCurrent(SoundHandle current);
    // 원본 +0x10. 평소에 재생하는 소리 항목이다.
    SoundHandle Sound() const;
    // 원본 +0x14. kSoundUseAlternate이고 부모의 상태 단어가 0이 아닐 때 대신 재생하는 소리 항목이다.
    SoundHandle Alternate() const;
    // 원본 +0x18. kSoundPlayAt 등의 비트 조합이다. 생성 뒤에는 바뀌지 않는다.
    std::uint32_t Flags() const;
    // 원본 +0x20. 벽시계(초)가 이 값 이상이 되어야 재생을 시작한다. 생성 직후에는 0.0이다.
    double StartTime() const;
    // 시작 시각을 정한다. 원본 호출자는 생성 직후 "현재 벽시계 + 지연"을 +0x20에 적어 지연 재생을 만든다(예: 0044a0be~0044a0e4).
    void SetStartTime(double time);
private:
    // 부모의 현재 타입에 현재 프레임 번호를 적는다. 재생 함수가 돌아온 뒤의 값을 다시 읽는다.
    void MarkTypeFrame();
    // 보고 함수가 연결돼 있으면 원본 assert 보고를 전달한다.
    void Report(std::string_view expression,int line) const;
    SoundProcessSystem& system_;
    SoundHandle current_{},sound_{},alternate_{};
    std::uint32_t flags_{};
    double startTime_{};
};

// 소리 프로세스들이 함께 쓰는 전역과 외부 효과를 묶고, 프로세스를 만들어 부모 객체에 붙인다.
// 사용: 월드마다 하나를 만든다. host·state·효과 대상은 이 객체보다, 이 객체는 붙인 프로세스(Kernel)보다 오래 살아야 한다.
//       필수 효과가 빠져 있으면 std::invalid_argument를 던진다.
// 이력: 2026-10-10 추가.
class SoundProcessSystem {
public:
    // host는 프로세스를 붙일 풀/Kernel, state는 프레임·타입 전역, hooks는 소리 장치 효과다. report 외의 효과가 비어 있으면 거부한다.
    SoundProcessSystem(o::SquidProcessHost& host,SoundProcessState& state,SoundProcessHooks hooks);
    // 소리 프로세스를 만들어 현재 soundProcessType으로 parent에 붙인다(부착 flags 0x10: form을 client 번호로 확보).
    // 돌려준 포인터는 Kernel이 소유한다. 패치판에서 parent가 free/dead이면 붙이지 않고 null을 돌려준다(SquidProcessHost::Attach 규칙).
    // 원본: 호출자의 new(0x28) 뒤 생성자 004ab240 / CD 00453800 → BaseProcess 부착 0041bd20 / CD 0048f590 (타입, 부모, 0, 0x10).
    //       new 실패(생성 생략)는 호출자가 구분한다.
    // 이력: 2026-10-10 추가.
    SoundProcess* Add(o::Sid parent,SoundHandle sound,SoundHandle alternate,std::uint32_t flags);
    // 다른 월드의 풀과 섞이지 않았는지 확인할 때 쓴다.
    const o::SidPool& Pool() const;
private:
    friend class SoundProcess;
    o::SquidProcessHost& host_;
    SoundProcessState& state_;
    SoundProcessHooks hooks_;
};

// 사제 보호막 생성 wrapper(RawPriestShield)의 소리 조회와 소리 프로세스 요청을 실제 이름 표와 소리 프로세스로 바꾼다.
// 사용: 다른 효과를 채운 hooks를 넘기면 loadSound·attachSound만 교체해 돌려준다. reserveSound(new 성공 여부)는 호출자가 준다.
//       pool은 system이 붙는 풀과 같아야 한다(아니면 std::invalid_argument). sounds·system은 돌려준 hooks보다 오래 살아야 한다.
// 원본: 00493d30 / CD 0040bfb0의 `004a8ee0("priestForceField.wav")` → `004ab240(보호막, 소리, 0, 1)`.
// 이력: 2026-10-10 추가.
o::PriestShieldHooks MakePriestShieldSoundHooks(const o::SidPool& pool,SoundList& sounds,SoundProcessSystem& system,o::PriestShieldHooks hooks);
}
