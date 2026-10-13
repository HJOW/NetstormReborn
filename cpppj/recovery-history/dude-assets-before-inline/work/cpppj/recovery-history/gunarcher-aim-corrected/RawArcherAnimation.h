// SunArcher71(Sun Disc Thrower)의실제회전/각속도발사와sunDisc83 생성을복원한다.
#pragma once
#include "o/RawMissile.h"

namespace netstorm::o {
// 캐논의64바이트객체와구별하는원본SunArcher Fire의0x58바이트이미지다.
struct ArcherAnimationFields {
    // bytes: 원본공통Baseprocess와각위치/각속도/가속도/발사상태를보존한다.
    std::array<std::uint8_t,0x58> bytes{};
    // 실제공통SID와각상태를읽는다.
    Sid Form() const;
    Sid Parent() const;
    Sid GunForm() const;
    Sid Target() const;
    double Time() const;
};
// 원본타입+0x110/+0x114의방향수/방향프레임자료다.미공급0은조회실패다.
struct ArcherTypeData {
    // directions/frameCount: 현재타입의방향수와회전할물리프레임수다.
    std::int32_t directions{},frameCount{};
};
// 같은월드의원본게임시각과mutable SunArcher발사전역이다.
struct ArcherAnimationState {
    // now/delta: 현재게임시각과원본0055b4e0/CD00548430의실제한프레임경과다.
    double now{},delta{};
    // type/displayType: 원본SunArcher71와표시자산sunDisc83이다.
    std::uint32_t type{71},displayType{83};
    // initialSpeed/initialAcceleration: 실제00540990/94(CD00537900/04)의float다.
    float initialSpeed{180},initialAcceleration{180};
    // winding/fireBias/springOffset/resetOffset: 실제발사각의windup회수와앞뒤변형각이다.
    float winding{4},fireBias{30},springOffset{180},resetOffset{180};
    // finishSpeed/finishAngle/finishTolerance: 실제회전종료의각속도/위치/오차조건이다.
    float finishSpeed{180},finishAngle{270},finishTolerance{30};
};
// 실제공통공간/표적/완료/생성경계를재사용하고캐논프레임표를혼용하지않는다.
struct ArcherAnimationHooks {
    // common: 같은월드의중심/공간/사운드/난수/표적배리어코너/Missile생성/완료경계다.
    GunAnimationHooks common;
    // aimDirection: 표적0 회전에서현재자산의원본004ad390/CD004adbc0 방향을읽는다.
    std::function<std::int32_t(Sid)> aimDirection;
    // notifySurface: 총출력WORD bit0 쓰기직후의실제공간통지다.
    std::function<void(Sid)> notifySurface;
};
// 회전방향표와발사각/속도변화를원본float저장순서와판본차이로수행한다.
class RawArcherAnimation {
public:
    // 동일판본풀/현재타입방향자료/전역을연결한다.
    RawArcherAnimation(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const ArcherTypeData> animationTypes,
        ArcherAnimationState& state,ArcherAnimationHooks hooks);
    // 공통ctor접두만초기화한다.실제Host부착직후fire면InitializeFire를수행한다.
    void Initialize(ArcherAnimationFields& fields,bool fire,Sid gunForm,Sid target) const;
    // 실제Firector의부착후초기각/속도/가속도와출력bit0 해제다.
    void InitializeFire(ArcherAnimationFields& fields) const;
    // 실제회전Frame00467940/CD004a26c0이다.
    void Rotate(ArcherAnimationFields& fields) const;
    // 실제Fire Frame00467c80/CD004a2a10이다.현재delta를매프레임읽는다.
    void Fire(ArcherAnimationFields& fields) const;
    // 실제launch의HP표적/배리어코너와speed35/display83/effect0 Missile요청이다.
    bool Launch(ArcherAnimationFields& fields) const;
    // 실제helper의각도선택/초기각/현재Fire프레임/공통각도기하를독립대조에공개한다.
    static std::int32_t NearestDirection(float angle,OriginalEdition edition);
    static float InitialAngle(std::int32_t frame);
    static std::int32_t FireFrame(float angle,bool sprung);
    static double Degrees(std::array<float,2> from,std::array<float,2> to);
    static float DirectionAngle(std::array<float,2> from,std::array<float,2> to,OriginalEdition edition);
    // 실제월드adapter의동일풀/원본타입검사다.
    const SidPool& Pool() const;
    std::uint32_t Type() const;
private:
    // 현재raw피격자/부모의범위와실제frame폭을검사한다.
    std::span<const std::uint8_t> Parent(const ArcherAnimationFields& fields) const;
    std::int32_t Frame(Sid parent) const;
    // 동일공간Unpop→현재frame쓰기→Repop(0x40)을수행한다.
    void SetFrame(Sid parent,std::int32_t frame) const;
    // live표적의현재좌표/중심을갱신하고free/dead이면기존좌표를보존한다.
    bool Track(ArcherAnimationFields& fields) const;
    // 실제GunProcess완료통지와현재Animation flags0x10 Kill이다.
    void Finish(ArcherAnimationFields& fields,std::uint32_t info,std::uint32_t value) const;
    // pool/types/animationTypes: 같은실제저장소와현재타입조회자료다.
    SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const ArcherTypeData> animationTypes_;
    // state/hooks: 매프레임읽는원본전역과실제하위경계다.
    ArcherAnimationState& state_;
    ArcherAnimationHooks hooks_;
};
// 실제Host/Kernel이독립SunArcher 회전/발사객체를소유한다.
class ArcherAnimation final:public SquidProcess {
public:
    // 현재원본이미지와파생종류를소유한다.
    ArcherAnimation(const RawArcherAnimation& logic,ArcherAnimationFields fields,bool fire);
    // 같은Kernel이원본시각/부착필드를현재이미지에연결한다.
    void RunFrame() override;
    void OnAttached() override;
    // 원본생성후각초기화와독립관찰에사용한다.
    ArcherAnimationFields& Fields();
private:
    // logic/fields/fire: 이객체보다오래살아야하는처리기와소유이미지/종류다.
    const RawArcherAnimation& logic_;
    ArcherAnimationFields fields_;
    bool fire_{};
};
// 실제Gun 완료와새파생Animation Kill을같은Host에연결한다.
ArcherAnimationHooks MakeArcherAnimationHostHooks(SquidProcessHost& host,ArcherAnimationHooks hooks);
// 현재GunProcess부모의type71/단계2·5를검사하고원본36·35/flags0x10으로붙인다.
ArcherAnimation* CreateArcherAnimation(SquidProcessHost& host,const RawArcherAnimation& logic,std::uint32_t stage,Sid gunForm,Sid target);
// 원본71생성만이factory에분배하며다른부모는기존startAnimation callback을사용한다.
GunProcessHooks MakeArcherAnimationProcessHooks(SquidProcessHost& host,const RawArcherAnimation& logic,GunProcessHooks hooks);
// 동일월드의sunDisc 생성경계를이미복원한실제Missile factory에연결한다.
ArcherAnimationHooks MakeArcherMissileHooks(SquidProcessHost& host,const RawMissile& missile,ArcherAnimationHooks hooks);
}
