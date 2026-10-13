// GunProcess의 원본 0x38 필드·상태 기계·표적 종속·패킷을 실제 ProcessForm에 연결한다.
#pragma once
#include "o/SquidProcess.h"
#include "o/Player.h"
#include <array>

namespace netstorm::o {
// 원본 new(0x38)의 미초기화 padding도 독립 대조에서 보존하도록 raw 필드를 소유한다.
struct GunProcessFields {
    // bytes: 필드 접근이나 파일 읽기에 사용하는 원시 바이트.
    std::array<std::uint8_t,0x38> bytes{};
    // 실제 +8 dirty·+0x10 다음 시각·+0x18 WORD 표적·+0x1c 단계·+0x20 group을 읽는다.
    std::uint8_t Dirty() const;
    double Time() const;
    Sid Target() const;
    std::uint32_t Stage() const;
    std::int32_t Group() const;
    // +0x2c/+0x30은 내부 Baseprocess의 Form/Parent DWORD 기록이다.
    Sid Form() const;
    Sid Parent() const;
};
// 원본 전역을 실행 월드가 같은 시점에 공급한다. 동맹 표는 0~8 두 소유자 번호로 접근한다.
struct GunProcessState {
    // now: 현재 판정에 사용하는 시각(초).
    double now{};
    // boss: boss 여부.
    // server: 서버 전용 초기화와 동작을 적용할지.
    // fortMode: Fort 모드 여부.
    // fixedMode: 고정 모드 여부.
    // disabled: 비활성 출력 색 또는 동작 차단 상태.
    // friendlyFire: 아군 발사 여부.
    // checking: 검사 여부.
    bool boss{true},server{true},fortMode{},fixedMode{},disabled{},friendlyFire{},checking{};
    // processType: 프로세스 타입.
    // windArcherType: 윈드 아처 자산을 타입 표에서 찾는 번호.
    // thunderCannonType: 썬더 캐논 자산을 타입 표에서 찾는 번호.
    // rainCannonType: 레인 캐논 자산을 타입 표에서 찾는 번호.
    std::uint32_t processType{34},windArcherType{140},thunderCannonType{131},rainCannonType{129};
    // allies: 동맹.
    std::array<std::array<bool,kPlayerCount+1>,kPlayerCount+1> allies{};
};
// 명시 외부 경계다. 실제 원본 몸체가 아직 없으면 호출자가 예외를 유지하며 noop로 성공시키지 않는다.
struct GunProcessHooks {
    std::function<void(Sid,Sid)> removeDependency; // 004ae780/CD00434ae0: 현재 gun Form이 표적에 만든 감시 종속을 제거한다.
    std::function<void(Sid,GunProcessFields&)> addDependency; // 004b15f0/CD00434aa0: 현재 표적과 inner Process의 감시 쌍을 만든다.
    std::function<void(GunProcessFields&)> repopForm; // 0041c010/CD0048f8d0: 현재 Form Unpop→Attach(flags0)/전송이다.
    std::function<Sid(Sid)> animationForm; // 00469070/CD004349e0: 부모 체인의 현재 fire/rotate AnimationForm 첫 일치다.
    std::function<void(Sid,std::uint32_t)> killAnimation; // 현재 PID로 AnimationProcess Kill(0x40)한다.
    std::function<void(Sid)> refreshRange; // 004d5000/CD004433e0: 현재 부모의 사거리 표시 갱신이다.
    std::function<std::uint32_t(Sid,std::uint8_t)> targetingMode; // 004d4d00/CD00443110: 현재 부모 Gun/소유자 공격 모드를 읽는다.
    std::function<Sid(float,float,std::uint32_t,std::uint32_t,std::uint8_t)> findTarget; // 실제 Gun 표적 검색 0046b6f0/CD00435e10이다.
    std::function<double(float,float)> randomBetween; // 실제 00455930/CD0048cc50의 x87 난수 구간이다.
    std::function<void(std::uint32_t,Sid,Sid)> startAnimation; // 004689c0/CD004a18f0(stage,gunForm,target) 전체 생성 경계다.
    std::function<void(Sid)> validateIslandOwners; // 패치 debug의 실제 부모 발자국 noIsland 소유자 진단이다.
    std::function<void(Sid)> checkForms; // 발사 전/뒤의 현재 부모 Form 체인 진단이다.
    std::function<bool(Sid)> parentBlocked; // 0041dee0/CD의 parent extra0x20: 발사 전 현재 차단을 검사한다.
    std::function<std::array<std::uint32_t,8>(Sid)> shotContext; // 0045ae60/CD의 현재 위치/중심/타입/소유자/frame 8DWORD다.
    std::function<bool(const std::array<std::uint32_t,8>&,Sid)> shoot; // 0046b070/CD00434c30의 각도 판정·실제 투사체 생성이다.
    std::function<double(Sid)> firingInterval; // 현재 타입 +0x4c float 발사 간격을 읽는다.
    std::function<void(GunProcessFields&,Sid)> attach; // 실제 inner Attach(processType,parent,0,flags0)와 Kernel/전송이다.
};
// 원본 GunProcess 몸체를 raw 필드와 같은 풀의 자산 상태로 실행한다.
class RawGunProcess {
public:
    // pool/types/state/hook 대상은 객체보다 오래 살아야 한다. 타입 표는 불변 자산으로 복사한다.
    RawGunProcess(SidPool& pool,std::span<const RiftTypeRecord> types,GunProcessState& state,GunProcessHooks hooks);
    // 0046a1a0/CD00434090: 지정한 raw 버퍼의 ctor 쓰기와 flags0 부착을 실행한다.
    void Construct(GunProcessFields& fields,Sid parent) const;
    // 실제 Kernel factory가 flags0 Attach 전에 같은 생성자 필드를 준비한다. 단독 생성 완료가 아니다.
    void Initialize(GunProcessFields& fields,Sid parent) const;
    // 0046b830/CD004342f0: 상태0~10의 시간·권한·현재 표적·애니메이션 흐름을 실행한다.
    void Frame(GunProcessFields& fields) const;
    // 0046a290/CD00434720: 표적 감시 제거·WORD0·선택적 상태1·현재 Form 재전송을 처리한다.
    void ClearTarget(GunProcessFields& fields,bool restart) const;
    // 0046a960/CD00434290: 감시 교체·상태2·현재 애니메이션 Kill·사거리/전송 갱신이다.
    void SetTarget(GunProcessFields& fields,Sid target) const;
    // inner OnAttached 004690f0/CD00434cb0: 상태3→2,6→5만 되돌린다.
    static void Attached(GunProcessFields& fields);
    // inner notify 0046a340/CD00434900: animation 완료와 표적 삭제 사건을 처리한다.
    void Notify(GunProcessFields& fields,std::uint32_t event,std::uint32_t info,std::uint32_t value) const;
    // outer 가상 발사 0046bd20/CD00434790: 유효 표적의 실제 shot 경계와 재시작·현재 시각 cooldown이다.
    void Fire(GunProcessFields& fields) const;
    // outer observer 0046a4b0/CD00434a70: 부모 변경·dirty8·상태3/6의 현재 AnimationProcess를 중지한다.
    void ReceiveUpdate(GunProcessFields& fields,std::uint8_t dirty,std::uint32_t parentChanged) const;
    // 00468ed0/CD00433db0: 헤더·선택 dirty 필드·부모·길이·dirty0을 원본 순서로 기록한다.
    static std::uint16_t WriteData(GunProcessFields& fields,std::span<std::uint8_t> packet,
        std::uint32_t form,std::uint32_t parent,bool force,std::uint8_t protocol,std::uint8_t message);
    // 0046a090/CD00436d90: includeParent 또는 force 때 현재 inner 부모 DWORD를 쓴다.
    static std::uint16_t WriteProcess(GunProcessFields& fields,std::span<std::uint8_t> packet,
        bool includeParent,bool force,std::uint8_t protocol,std::uint8_t message);
    // 외부 Kernel 어댑터가 같은 월드인지 확인한다.
    const SidPool& Pool() const;
private:
    // 현재 Parent raw/group과 필드 읽기를 검증한다. free/dead 표적은 여기서 거부하지 않는다.
    std::span<const std::uint8_t> Parent(const GunProcessFields& fields) const;
    // 현재 AnimationForm을 반복 조회하는 원본 호출 순서를 보존하며 진단한다.
    void CheckNoAnimation(GunProcessFields& fields) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    // state_: 원본 전역을 실행 월드가 같은 시점에 공급한다. 동맹 표는 0~8 두 소유자 번호로 접근한다 (GunProcessState).
    GunProcessState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (GunProcessHooks).
    GunProcessHooks hooks_;
};
// 실제 Kernel의 inner Baseprocess를 C++ 수명/가상 메서드로 표현한다.
class GunProcess final : public SquidProcess {
public:
    // 원본 ctor 필드 초기화와 flags0 Attach는 별도 factory가 완료한다.
    GunProcess(const RawGunProcess& logic,GunProcessFields fields);
    // 실제 Kernel 한 프레임을 원본 상태 기계로 넘긴다.
    void RunFrame() override;
    // 부착 후 현재 Form/Parent를 기록하고 원본 Attached를 실행한다.
    void OnAttached() override;
    // 현재 상태 packet 및 외부 notify에 사용할 같은 raw 필드다.
    GunProcessFields& Fields();
    const GunProcessFields& Fields() const;
    // inner 가상 notify를 같은 raw 상태 기계에 전달한다.
    void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value);
    // AttachForm은 OnAttached 전 writer를 부르므로 현재 base Form/Parent를 먼저 동기화한다.
    std::uint16_t WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force=false,std::uint8_t protocol=1,std::uint8_t message=0x3a);
private:
    // logic_: 원본 GunProcess 몸체를 raw 필드와 같은 풀의 자산 상태로 실행한다 (RawGunProcess).
    const RawGunProcess& logic_;
    // fields_: 원본 프로세스의 직접 읽기·쓰기 필드 이미지.
    GunProcessFields fields_;
};
// 원본 flags0를 유지한 실제 Kernel factory다. 필요한 서버 전송을 host가 연결하지 않았으면 기존 예외를 유지한다.
GunProcess* AttachGunProcess(SquidProcessHost& host,const RawGunProcess& logic,GunProcessFields fields,Sid parent,std::uint32_t type=34);
// RawAssetPostPop.createGunProcess에 사용할 생성자+실제 Kernel/전송 factory다. 0x38 padding은 호스트에서0으로 확보한다.
GunProcess* CreateGunProcess(SquidProcessHost& host,const RawGunProcess& logic,Sid parent,std::uint32_t type=34);
}
