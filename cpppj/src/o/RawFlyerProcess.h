// 공통 비행 프로세스의 원문 생성/상태 전송/홈 연결과 맨 오 워의 최초 후처리를 복원한다.
#pragma once
#include "o/SquidProcess.h"
#include <array>

namespace netstorm::o {
// 기본 outer0/inner+30과 맨 오 워 추가 필드까지 같은112BYTE로 보존한다.
struct FlyerProcessFields {
    std::array<std::uint8_t,0x70> bytes{}; // 생성자가 건드리지 않는 padding도 caller 입력이다.
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
};
// 같은 풀의 실제 공통 프로세스 생성/전송과 홈 갱신만 담당한다.
class RawFlyerProcess {
public:
    RawFlyerProcess(SidPool& pool,std::span<const RiftTypeRecord> types,FlyerProcessGlobals globals,FlyerProcessHooks hooks); // PATCH 자료의 수명/크기를 확인한다.
    void Construct(FlyerProcessFields& fields,Sid parent,Sid home,std::uint32_t speed,float parameter) const; // 0045c340의 쓰기와 inner 부착 순서다.
    static void SpecializeRain(FlyerProcessFields& fields); // 부착이 끝난 뒤0045fac0의 맨 오 워 추가 쓰기다.
    void Bind(Sid flyer,Sid home) const; // 0045c1b0의 genus 검사/첫 Form 조회/dirty2/홈 WORD 쓰기다.
    void Frame(FlyerProcessFields& fields) const; // 실제 미복원 Frame은 명시 하위 오류다.
    void Notify(FlyerProcessFields& fields,std::uint32_t message,std::uint32_t info1,std::uint32_t info2) const; // 원본 notify를 명시 소비자로 전달한다.
    void ParentDying(FlyerProcessFields& fields) const; // 원본 홈의 부모 사망 통지를 전달한다.
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
};
// TYPE144/0050a388의 실제 파생 PostPop만 등록한다.
class RawFlyerPostPop {
public:
    RawFlyerPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const FlyerPostPopType> details,const bool& server,FlyerPostPopHooks hooks); // PATCH와 보충 표의 동일 번호를 확인한다.
    bool Handles(Sid sid) const; // 주소 표를 실제 완성한 맨 오 워 몸체로 한정한다.
    void PostPop(Sid sid,std::uint32_t flags) const; // 0045fac0→0045e6b0→Damageable의 실제 순서다.
    const SidPool& Pool() const; // 공통 PostPop과 같은 풀인지 확인한다.
private:
    SidPool& pool_; // 비행체와 그림자가 공유하는 실제 풀이다.
    std::vector<RiftTypeRecord> types_; // 실제 현재 불변 flags1 표다.
    std::vector<FlyerPostPopType> details_; // 같은 TYPE 번호의 speed/프레임 수다.
    const bool& server_; // 처리 중 변경될 수 있는 현재 server다.
    FlyerPostPopHooks hooks_; // 선택한 실제 큰 몸체 경계다.
};
}
