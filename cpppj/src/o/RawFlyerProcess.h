// 공통 비행 프로세스의 원문 생성/상태 전송/홈 연결과 맨 오 워의 최초 후처리를 복원한다.
#pragma once
#include "o/SquidProcess.h"
#include <array>

namespace netstorm::o {
// 기본 outer0/inner+30과 맨 오 워 추가 필드까지 같은112BYTE로 보존한다.
struct FlyerProcessFields {
    std::array<std::uint8_t,0x70> bytes{}; // 생성자가 건드리지 않는 padding도 caller 입력이다.
    std::array<std::uint8_t,0x18> sunTail{}; // 훨리기그0x88 객체의+70..87이다. 기존112바이트 관측과 별도로 보존한다.
    Sid Form() const; // outer+34의 현재 Form이다.
    Sid Parent() const; // outer+38의 현재 부모다.
    Sid Home() const; // outer+a의 홈 WORD다.
};
// 원본 전역은 현재 값의 참조로 유지한다.
struct FlyerProcessGlobals {
    const bool& server; // 00540bc0의 현재 서버다.
    const double& now; // 0055b4d0의 단일 게임 시각이다.
    const std::uint32_t& processType; // 005410ec의 현재 공통 비행 타입31이다.
};
// 실제 부착/조회와 아직 별도로 복원할 큰 비행 몸체의 명시 경계다.
struct FlyerProcessHooks {
    std::function<void(std::uint32_t)> assertion; // 서버/비행 genus의 원본 assert다.
    std::function<void(FlyerProcessFields&,Sid,std::uint32_t,std::uint32_t)> attach; // 기본 표로 실제 Host에 부착한다.
    std::function<Sid(Sid,std::uint32_t)> findProcess; // 현재 nondead 종속의 첫 타입을 조회한다.
    std::function<FlyerProcessFields&(Sid)> fields; // 현재 Form/PID가 소유한 같은 원문이다.
    std::function<void(FlyerProcessFields&)> frame; // 0045d630/0045c9d0의 별도 실제 몸체다. 누락을 성공으로 버리지 않는다.
    std::function<void(FlyerProcessFields&,std::uint32_t,std::uint32_t,std::uint32_t)> notify; // 0045b760의 서버 표적 해제 몸체다.
    std::function<void(FlyerProcessFields&)> parentDying; // 0045b7a0의 홈 통지 몸체다.
    std::function<void(Sid)> pieces; // 실제4605f0의 현재 비행체 파편 생성이다.
    std::function<void(Sid,std::uint32_t)> destroy; // 종료45bc30 뒤 현재 부모의 가상10 삭제다.
};
// 같은 풀의 실제 공통 프로세스 생성/전송과 홈 갱신만 담당한다.
class RawFlyerProcess {
public:
    RawFlyerProcess(SidPool& pool,std::span<const RiftTypeRecord> types,FlyerProcessGlobals globals,FlyerProcessHooks hooks); // PATCH 자료의 수명/크기를 확인한다.
    void Construct(FlyerProcessFields& fields,Sid parent,Sid home,std::uint32_t speed,float parameter) const; // 0045c340의 쓰기와 inner 부착 순서다.
    static void SpecializeRain(FlyerProcessFields& fields); // 부착이 끝난 뒤0045fac0의 맨 오 워 추가 쓰기다.
    static void SpecializeSun(FlyerProcessFields& fields); // 부착 뒤45ef30의 전용 표/104·112·120 타이머와84 필드만 쓴다.
    void Bind(Sid flyer,Sid home) const; // 0045c1b0의 genus 검사/첫 Form 조회/dirty2/홈 WORD 쓰기다.
    void Frame(FlyerProcessFields& fields) const; // 실제 미복원 Frame은 명시 하위 오류다.
    void Notify(FlyerProcessFields& fields,std::uint32_t message,std::uint32_t info1,std::uint32_t info2) const; // 원본 notify를 명시 소비자로 전달한다.
    void ParentDying(FlyerProcessFields& fields) const; // 원본 홈의 부모 사망 통지를 전달한다.
    void Finish(FlyerProcessFields& fields) const; // 45bc30의 서버 진단→현재 파편→현재 부모 가상 삭제8 순서다.
    static std::uint16_t WriteProcess(FlyerProcessFields& fields,std::span<std::uint8_t> packet,bool includeParent,bool force=false,std::uint8_t protocol=1,std::uint8_t message=0x39); // 0045c310→0045aed0의 WORD/DWORD/double writer다.
    const SidPool& Pool() const; // 다른 Host의 필드와 혼합하지 않도록 검사한다.
private:
    SidPool& pool_; // 같은 자산과 Form을 담는 단일 풀이다.
    std::vector<RiftTypeRecord> types_; // 현재 불변 TYPE의 genus 표다.
    FlyerProcessGlobals globals_; // 현재 장기 전역 참조다.
    FlyerProcessHooks hooks_; // 선택된 실제 큰 몸체 경계다.
};
// 현재 필드의 단일 소유권을 실제 Kernel/Host와 연결한다.
class FlyerProcess final : public SquidProcess {
public:
    FlyerProcess(const RawFlyerProcess& logic,FlyerProcessFields fields); // 생성 이전 caller 바이트를 소유한다.
    void RunFrame() override; // 같은 Kernel의 프레임을 실제 몸체로 전달한다.
    void OnAttached() override; // Host의 현재 Form/부모를 원문 필드에 반영한다.
    void Notify(std::uint32_t message,std::uint32_t info1,std::uint32_t info2) override; // 표적 의존 통지의 실제 명시 경계다.
    void OnParentDying(Sid parent) override; // 현재 프로세스의 홈 통지를 전달한다.
    FlyerProcessFields& Fields(); // 현재 바이트만 수정한다.
    const FlyerProcessFields& Fields() const; // 현재 바이트만 읽는다.
    std::uint16_t WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force=false); // 부착 이전 전송도 실제 Host Form/부모를 기록한다.
private:
    const RawFlyerProcess& logic_; // 이 객체보다 오래 사는 단일 로직이다.
    FlyerProcessFields fields_; // 맨 오 워 확장까지 포함하는 같은 원문 이미지다.
};
// TYPE+e0의 현재 speed 비트를 별도 보충 자료로 공급한다.
struct FlyerPostPopType {
    std::uint32_t speed{}; // 실제 float speed의 IEEE 비트다.
    std::int32_t frames{}; // TYPE+114의 전체 물리 프레임 수다.
    std::uint32_t impactSound{},activeSound{}; // TYPE+74/+6c의 현재 소리 이름표 토큰이다.
};
// 현재 가상 생성/표시/후처리의 실제 큰 경계를 연결한다.
struct FlyerPostPopHooks {
    std::function<bool()> allocateRainProcess; // new70을 먼저 수행한다. 성공 후에만 현재 홈/speed를 읽는다. NULL은 자연 반환이다.
    std::function<void(Sid,Sid,std::uint32_t,float)> createRainProcess; // 확보한 같은 이미지의 기본 생성·부착/맨 오 워 확장이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 그림자의 같은 TYPE/flags2 생성이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 현재 가상+90이다.
    std::function<void(Sid,std::uint32_t)> unpop; // 그림자의 현재 가상+48이다.
    std::function<void(Sid)> initializeUnit; // 004d4e10의 현재1/8초 압축 QA 시각/표면 통지다. __ftol은 난수가 아니다.
    std::function<std::array<float,2>(Sid)> shadowOffset; // 현재 가상+c8의 두float다.
    std::function<void(Sid,std::uint32_t)> damageable; // 같은 실제0044bf80과 공통 장부다.
    std::function<bool()> allocateSunProcess; // 훨리기그 전용new0x88을 먼저 확보한다.
    std::function<void(Sid,Sid,std::uint32_t,float)> createSunProcess; // 같은136바이트의 기본 생성·부착 뒤 전용 쓰기다.
    std::function<void(Sid,std::uint32_t)> repop; // 최초 훨리베이스의frame10 뒤 실제 가상4c 재배치다.
    std::function<bool()> allocateSound; // 첫Pop의new0x28은 서버/클라이언트 모두 요청한다.
    std::function<void(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> createSound; // 확보한 같은47 객체에 부모/74/6c/flags1을 전달한다.
    std::function<bool(Sid,std::uint32_t)> hasRegular; // 더스트 데빌 최초Pop의event0 중복조회다.
    std::function<bool()> allocateWindRegular; // 실제new28 성공뒤에만생성한다.
    std::function<void(Sid,std::uint32_t,float)> createWindRegular; // 실제46에event0/0.25f를flags50으로부착한다.
};
// 실제 맨 오 워144/훨리기그73의 두 전용 PostPop과 공통45e6b0을 등록한다.
class RawFlyerPostPop {
public:
    RawFlyerPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const FlyerPostPopType> details,const bool& server,FlyerPostPopHooks hooks); // PATCH와 보충 표의 동일 번호를 확인한다.
    bool Handles(Sid sid) const; // 실제 두 전용 가상 표에 한정하며 다른 비행체는 별도 복원이 필요하다.
    void PostPop(Sid sid,std::uint32_t flags) const; // 전용45fac0/45ef30→45e6b0→Damageable의 실제 순서다.
    const SidPool& Pool() const; // 공통 PostPop과 같은 풀인지 확인한다.
private:
    SidPool& pool_; // 비행체와 그림자가 공유하는 실제 풀이다.
    std::vector<RiftTypeRecord> types_; // 실제 현재 불변 flags1 표다.
    std::vector<FlyerPostPopType> details_; // 같은 TYPE 번호의 speed/프레임 수다.
    const bool& server_; // 처리 중 변경될 수 있는 현재 server다.
    FlyerPostPopHooks hooks_; // 선택한 실제 큰 몸체 경계다.
};
}
