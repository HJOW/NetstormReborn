// 건설 프로세스(원본 0x38바이트 객체, 프로세스 타입 30)의 시작·생성자·진행·완료·form 삭제 통지를 복원한다.
// 건설 중인 abstract 객체의 HP를 시작~끝 시각의 비율로 올리고, 다 찼을 때 실제 객체로 바꾼다.
// 원본: 00443fe0·00443c40·004428c0·00443e20·00444030 (패치판 10.78) ↔ CD판 004d06a0·004d0720·004d0ba0·004d09e0·004d08e0.
// 이력: 2026-10-11 추가 — 건설 배치 통지가 부르는 건설 시작과 그 뒤의 진행/완료를 잇기 위해.
#pragma once
#include "o/RawConstructionEffects.h"
#include "o/RawConstructionNotice.h"
#include "o/SquidProcess.h"
#include "o/SquidReward.h"
#include <string_view>

namespace netstorm::o {
// 건설 프로세스 한 개가 가진 값이다. 원본 객체의 건설 흐름이 읽고 쓰는 필드만 옮겼다.
// 사용: Start가 만들어 attach 경계에 넘기고, 부착된 뒤에는 프로세스가 소유한다. Run은 start/end만 읽는다.
// 원본: 객체 +8/+0x10/+0x18/+0x20/+0x34. 추가 바이트는 메시지 0x37이 저장/복원하며 진행 몸체는 읽지 않는다.
// 이력: 2026-10-11 추가.
struct ConstructionProcessFields {
    std::uint8_t dirty{0xff}; // +8: 전송할 필드 비트(1 부모, 2 시작, 4 끝, 8 추가 바이트). 생성자는 0xff로 두며 전송 함수가 0으로 되돌린다.
    double start{};           // +0x10: 건설을 시작한 게임 시각이다.
    double end{};             // +0x18: HP가 최대에 이르는 게임 시각이다.
    std::uint32_t sound{};    // +0x34: 건설 진행음(jimBuild.wav)의 소리 항목이다. 1초 이하 건설은 0이다.
    std::uint8_t additional{}; // +0x20: 전송 비트 8이 선택하는 추가 바이트다. 의미를 추정해 변환하지 않고 그대로 보존한다.
};
// 건설 프로세스가 경계 호출 뒤에도 다시 읽는 원본 전역이다. 주소는 패치 / CD 순서다.
// 사용: 월드마다 하나를 두고 RawConstructionProcess에 넘긴다. 프로세스가 살아 있는 동안 유지해야 한다.
// 이력: 2026-10-11 추가.
struct ConstructionProcessState {
    double now{};                        // 0055b4d0 / 005484a8: 게임 시각이다.
    std::uint32_t processType{30};       // 005410e8 / 0051c9d8: 부착할 때 읽는 건설 프로세스 타입 번호(초기값 0x1e)다.
    std::uint32_t soundProcessType{47};  // 0054112c / 0051ca1c: 완료 때 부모에서 지우는 소리 프로세스 타입 번호다.
    std::uint32_t localPlayer{1};        // 00540c70 / 0050f6c8: 부모 소유자 바이트와 DWORD로 비교해 완료 소리를 낼지 정한다.
    bool boss{true};                     // 00540bc4 / 00540a2c: 권한 쪽에서만 건설을 끝내고 abstract를 푼다.
    std::uint32_t pushGenus{0x10444200}; // 005325b4 / 0053fc28: 완성 뒤 발자국의 유닛을 밀어내는 genus 마스크다.
    std::uint32_t daisType{165};         // 005412ec / 0051cbd8: form 삭제 통지가 HP를 채우지 않는 타입이다.
    bool quickBuild{};                   // 0055a49c / 00537eb4: 콘솔 명령 quickbuild(qb). 켜져 있으면 모든 건설이 한 프레임에 끝난다.
    bool quickVortex{};                  // 0055a498 / 00537eb0: 콘솔 명령 quickvortex(qv). genus 0x200(vortex)의 건설만 바로 끝낸다.
    const bool* sharedBoss{}; // 실제 Client boss를 건설 진행/완료 실행기보다 오래 빌린다.
    const bool& Boss() const { return sharedBoss ? *sharedBoss : boss; } // HP·표시·소리 효과 뒤 각각의 완료 관문에서 현재값을 읽는다.
};
// 건설 프로세스 밖의 효과다. attach를 뺀 모든 경계가 필요하며 attach는 실제 host를 주면 비워 둘 수 있다.
// 사용: 검사에서는 호출 사실을 기록하는 함수를, 실제 연결에서는 MakeConstructionProcessHooks가 채운 값을 넘긴다.
//       경계는 같은 풀·전역을 바꿀 수 있으며 복원 코드는 경계 뒤의 현재 값을 다시 읽는다.
// 이력: 2026-10-11 추가.
struct ConstructionProcessHooks {
    // operator_new(0x38)의 성공 여부다. 거짓이면 Start는 아무것도 하지 않는다. 004e4391 / CD 004f1650.
    std::function<bool()> reserve;
    // 부모에 건설 표시 효과를 만든다. 둘째 인자는 지속 시간이 0인 건설(00423840 / CD 004eab60)이면 참, 아니면(00423900 / CD 004eac30) 거짓이다.
    std::function<void(Sid,bool)> effect;
    // 프로세스 부착(fields, 프로세스 타입, 부모, flags 0x10) → 부착된 프로세스가 소유하는 필드. 붙지 않았으면 null이다. 0041bd20 / CD 0048f590.
    std::function<ConstructionProcessFields*(const ConstructionProcessFields&,std::uint32_t,Sid,std::uint32_t)> attach;
    // 소리 이름으로 항목을 찾는다(없으면 0). 004a8ee0 / CD 00437d20.
    std::function<std::uint32_t(std::string_view)> loadSound;
    // 소리 프로세스용 operator_new(0x28)의 성공 여부다. 거짓이어도 소리 항목은 프로세스에 남는다.
    std::function<bool()> reserveSound;
    // 부모에 소리 프로세스를 붙인다(부모, 소리, 대체 소리 0, flags 1 = 부모 위치에서 반복 재생). 004ab240 / CD 00453800.
    std::function<void(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> attachSound;
    // 화면 갱신을 한 번 요청한다. 004da650 / CD 0041eac0.
    std::function<void()> refresh;
    // 객체의 최대 HP다. 004adcb0 / CD 004aed30.
    std::function<std::int32_t(Sid)> maxHitPoints;
    // 부모의 가상 표시 갱신(vtable +0x88)이다. HP를 쓴 직후에 부른다.
    std::function<void(Sid)> updateDisplay;
    // 부모에서 해당 타입의 첫 contained를 지운다(원본 나머지 인자 0, 1). 004b1a90 / CD 004eb310.
    std::function<void(Sid,std::uint32_t)> removeContained;
    // 좌표에서 이름으로 소리를 한 번 낸다(x, y, 이름, 반복 0). 004a9d70 / CD 00438c00.
    std::function<void(float,float,std::string_view,std::uint32_t)> playAt;
    // 타입의 준비 소리 항목(타입 +0x90)이다. 0이면 소리를 내지 않는다. 004ac550 / CD 004abf50이 돌려준 타입에서 읽는다.
    std::function<std::uint32_t(std::uint32_t)> readySound;
    // 소리 항목을 전역으로 한 번 낸다(원본 나머지 인자는 모두 0). 004a9550 / CD 004383f0.
    std::function<void(std::uint32_t)> play;
    // 부모의 가상 Unpop(vtable +0x48, flags 0)과 현재 위치 재등록(vtable +0x4c, flags 9)이다. Unpop은 void를 켜야 한다.
    std::function<void(Sid,std::uint32_t)> unpop;
    // repop: 변경한 대상을 공간에 다시 반영하는 Repop 콜백.
    std::function<void(Sid,std::uint32_t)> repop;
    // 완성된 발자국의 유닛을 밀어낸다(x, y, 타입, 소유자). CD판 원본은 소유자를 넘기지 않는다. 00443800 / CD 004d2450.
    std::function<void(float,float,std::uint32_t,std::uint32_t)> push;
};
// 프로세스 Kill 요청(flags)이다. 호출 뒤에도 프로세스가 남아 있으면 참을 돌려준다. 0041bf10 / CD 0048f7b0.
// 이력: 2026-10-11 추가.
using ConstructionProcessKill=std::function<bool(std::uint32_t)>;

// 건설 프로세스의 원본 몸체다. 프로세스 객체 없이도 부를 수 있게 필드와 부모 SID를 인자로 받는다.
// 사용: 월드마다 하나를 만든다. host를 주면 Start가 실제 ConstructionProcess를 Kernel에 붙이고,
//       host 없이 attach 경계만 주면 호출자가 필드를 보관한다. 풀·타입·상태·host는 이 객체보다 오래 살아야 한다.
// 이력: 2026-10-11 추가.
class RawConstructionProcess {
public:
    // 타입 수가 판본과 다르거나 필수 경계가 비었으면(attach는 host가 없을 때만 필수) std::invalid_argument를 던진다.
    RawConstructionProcess(SidPool& pool,std::span<const RiftTypeRecord> types,ConstructionProcessState& state,
        ConstructionProcessHooks hooks,SquidProcessHost* host=nullptr);
    // 건설을 시작한다. 객체 확보에 실패하면 아무 효과도 없다. duration이 0(또는 -0)이면 10초 건설이 된다.
    // 순서: 확보 → 표시 효과 → 시작/끝 시각(효과 뒤의 현재 시각) → 부착 → 1초보다 긴 건설의 진행음 → 갱신 요청.
    // 판본 차이: 진행음 조건을 패치판은 실수 비교(NaN은 소리 없음), CD판은 비트의 부호 있는 정수 비교(양의 NaN은 소리 있음)로 판단한다.
    // 원본: 00443fe0 → 00443c40 / CD 004d06a0 → 004d0720(기본 시간 helper 004d08a0 포함).
    // 이력: 2026-10-11 추가.
    void Start(Sid parent,float duration) const;
    // 00443dc0 / CD 004d0930→004d0970: 시작/소리를 유지하고 끝만 현재 시각+10초로 바꾼다.
    // 최대 HP 경계를 먼저 실행하며 CD는 진행 HP에서 damage를 뺀 값을 계산한 뒤 기본 시간 helper에 넘긴다.
    // 그 helper의 부모/HP 인자는 사용되지 않는다. end 변경 비트 4만 켜고 다른 비트는 보존한다.
    void ResetEnd(ConstructionProcessFields& fields,Sid parent,std::uint32_t damage) const;
    // 실제 host의 form→Kernel 조회로 건설 프로세스를 찾아 ResetEnd를 실행한다. host/유효한 건설 form이 필요하다.
    // 빈 슬롯·다른 프로세스·오래된 form 연결은 필드 쓰기 전에 예외로 거부한다.
    void ResetEndFromForm(Sid form,std::uint32_t damage) const;
    // 한 프레임의 진행이다. HP = 절삭((현재-시작)/(끝-시작)×최대)를 0~최대로 제한해 부모에 쓰고 표시를 갱신한다.
    // quickBuild(또는 vortex의 quickVortex)이면 바로 최대가 된다. 부모가 abstract가 아니면 Kill하고,
    // HP가 최대이고 권한이 있으면 Kill한다. 첫 Kill이 프로세스를 없앴으면(kill이 거짓) 둘째 Kill은 하지 않는다.
    // 원본은 해제된 객체로 둘째 Kill을 부르지만(해제 뒤 읽기) 그 동작은 옮기지 않았다. fields는 Kill 전에만 읽는다.
    // 원본: 004428c0 / CD 004d0ba0 (vtable +0x18). 패치판 debug assert 구간은 옮기지 않았다.
    // 이력: 2026-10-11 추가.
    void Run(const ConstructionProcessFields& fields,Sid parent,const ConstructionProcessKill& kill) const;
    // 건설을 끝낸다. 진행음 프로세스 삭제 → 갱신 → (로컬 소유·예측 번호가 아닌 객체) 완료음과 타입 준비 소리 →
    // (권한·예측 번호 아님·HP가 최대·abstract) Unpop → abstract 해제 → 재등록(flags 9) → genus가 맞으면 유닛 밀어내기.
    // Unpop 뒤에도 void가 아니면 원본 assert에 해당하므로 std::logic_error를 던진다.
    // 원본: 00443e20 / CD 004d09e0.
    // 이력: 2026-10-11 추가.
    void Complete(Sid parent) const;
    // form이 지워지기 직전의 통지다. Complete를 부른 뒤, 부모가 free/dead가 아니고 dais 타입이 아니면 HP를 최대로 채운다.
    // 원본의 flags 인자는 읽지 않으므로 받지 않는다.
    // 원본: 00444030 / CD 004d08e0 (vtable +0xc).
    // 이력: 2026-10-11 추가.
    void FormDestroyed(Sid parent) const;
    // 어댑터가 같은 풀을 쓰는지 확인할 때 쓴다.
    const SidPool& Pool() const;
private:
    // 부모의 현재 raw 타입 번호를 읽고 타입 표 범위를 확인한다.
    std::uint32_t TypeOf(Sid sid) const;
    // 부모의 현재 타입에서 genus(flags2)를 읽는다. 경계 뒤의 재조회를 위해 값을 보관하지 않는다. 004ac200 / CD 004abae0.
    std::uint32_t Genus(Sid sid) const;
    // 현재 HP를 판본의 폭으로 읽는다(패치 signed DWORD, CD 부호 확장한 WORD).
    std::int32_t HitPoints(Sid sid) const;
    // HP를 판본의 폭으로 쓴다(패치 DWORD, CD 하위 WORD). 주변 필드는 보존한다. 004adca0 / CD 004aed20.
    void SetHitPoints(Sid sid,std::int32_t value) const;
    // 번호가 클라이언트 예측 영역(5 이상, 서버 영역 시작 미만) 밖인지 본다. 완료 효과는 예측 객체에서 내지 않는다.
    bool Confirmed(Sid sid) const;
    // 생성자 몸체다. Start가 확보에 성공한 뒤 부른다.
    void Construct(Sid parent,float duration) const;
    // CD 004d08a0: 끝을 현재 시각+10초로 쓰고 10.0f를 돌려준다. 부모/HP 인자는 원본이 읽지 않는다.
    float DefaultTime(ConstructionProcessFields& fields,Sid parent,std::int32_t hitPoints) const;
    SidPool& pool_;                          // 부모의 HP/abstract 바이트를 쓰는 실제 풀이다.
    std::span<const RiftTypeRecord> types_;  // 부모 현재 타입의 genus를 재조회한다.
    ConstructionProcessState& state_;        // 경계가 바꾼 현재 전역을 공유한다.
    ConstructionProcessHooks hooks_;         // 아직 복원하지 않았거나 다른 계층에 있는 효과다.
    SquidProcessHost* host_;                 // 있으면 attach 경계 대신 실제 프로세스를 붙인다.
};
// Kernel이 소유하는 실제 건설 프로세스다. 매 프레임 진행을 실행하고 form 삭제 통지에서 건설을 끝낸다.
// 사용: 직접 만들지 않는다. host를 준 RawConstructionProcess::Start가 만들어 부모에 붙인다.
// 원본: 객체 0x38바이트의 +0x28 부분(BaseProcess, 가상 표 00506ffc / CD 00506b88).
// 이력: 2026-10-11 추가.
class ConstructionProcess final : public SquidProcess {
public:
    // logic은 이 프로세스보다 오래 살아야 한다. fields는 생성자가 계산한 초기값이다.
    ConstructionProcess(const RawConstructionProcess& logic,const ConstructionProcessFields& fields);
    // 부착된 프로세스의 한 프레임 진행이다. Kill이 이 객체를 없앨 수 있으므로 그 뒤에는 멤버를 읽지 않는다.
    void RunFrame() override;
    // form 삭제 통지다. 부모의 건설 완료 처리와 HP 채움을 실행한다.
    void OnFormDestroy(std::uint32_t flags) override;
    // 현재 필드다. Start는 부착 뒤 소리 항목을 여기에 쓴다.
    ConstructionProcessFields& Fields();
    const ConstructionProcessFields& Fields() const;
private:
    const RawConstructionProcess& logic_; // 원본 몸체다.
    ConstructionProcessFields fields_;    // 원본 +8/+0x10/+0x18/+0x34다.
};
// 최대 HP 조회를 같은 풀의 실제 보상 모듈로, 유닛 밀어내기를 실제 밀어내기 모듈로 잇는다(effects가 null이면 hooks.push 유지).
// 사용: 나머지 경계를 채운 hooks를 넘긴다. reward·effects는 돌려준 hooks보다 오래 살아야 한다. 풀이 다르면 std::invalid_argument를 던진다.
// 이력: 2026-10-11 추가.
ConstructionProcessHooks MakeConstructionProcessHooks(const SidPool& pool,const SquidReward& reward,const RawConstructionEffects* effects,
    ConstructionProcessHooks hooks);
// 건설 배치 통지의 건설 시작 경계를 실제 Start에 잇는다. 둘째 인자는 지속 시간의 단정도 비트다(통지는 0을 넘긴다).
// 사용: process는 돌려준 hooks보다 오래 살아야 한다. 풀이 다르면 std::invalid_argument를 던진다.
// 이력: 2026-10-11 추가.
ConstructionNoticeHooks MakeConstructionProcessNoticeHooks(const SidPool& pool,const RawConstructionProcess& process,ConstructionNoticeHooks hooks);
}
