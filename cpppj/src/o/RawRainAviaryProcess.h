// PATCH 맨 오 워 풀의 실제 최초 후처리/0x30 생성자/프레임/상태 패킷을 같은 Kernel에 연결한다.
#pragma once
#include "o/SquidProcess.h"
#include <array>
#include <optional>
#include <string_view>

namespace netstorm::o {
// 생성자가 쓰지 않는 padding과 비서버 상태도 caller 입력으로 보존한다.
struct RainAviaryProcessFields {
    std::array<std::uint8_t,0x30> bytes{}; // 실제 outer0/inner+20의48BYTE 이미지다.
    std::uint8_t Dirty() const; // outer+8의 원본 변경 비트다.
    double Time() const; // outer+10의 다음 프레임 시각이다.
    std::uint32_t Stage() const; // outer+18의 AP 상태다.
    Sid Form() const; // inner+4, 즉 outer+24의 현재 Form이다.
    Sid Parent() const; // inner+8, 즉 outer+28의 현재 부모다.
};
// 모든 참조는 처리기보다 오래 살아야 하며 선택 효과 뒤 현재 값을 다시 읽는다.
struct RainAviaryProcessGlobals {
    const bool& fort; // 00594fb8의 현재 편집 모드다.
    const bool& friendlyFire; // 0055a490의 현재 shootMyFriends이며 fort 프로세스 허용에 쓰인다.
    const bool& server; // 00540bc0이며 boss와 구별한다.
    const std::uint32_t& editor; // 005c85a4이며 비제로는 Frame을 생략한다.
    const bool& checking; // 005e4794의 개발자 프레임 진단이다.
    const double& now; // 0055b4d0의 같은 게임 시각이다.
    const std::uint32_t& processType; // 00541160의 현재 타입60다.
    const std::uint32_t& flyerType; // 00541298의 현재 rainFlyer144이다.
};
// 원문이 실제로 선택한 큰 효과만 필수로 요구한다. 비어 있는 성공 소비자는 만들지 않는다.
struct RainAviaryProcessHooks {
    std::function<Sid(Sid,std::uint32_t)> findProcess; // 004b1a90(parent,type,0,0)의 첫 nondead Form이다.
    std::function<std::optional<RainAviaryProcessFields>(std::size_t)> allocate; // new0x30의 caller 바이트 또는 NULL 입력이다.
    std::function<void(std::optional<RainAviaryProcessFields>,Sid,std::uint32_t,std::uint32_t)> attach; // inner+20을 실제0041bd20(type,parent,0,8)에 부착한다.
    std::function<void(Sid,std::uint32_t)> damageable; // 같은 실제0044bf80→공통 PostPop이다.
    std::function<void(std::uint32_t)> assertion; // 중복257/부모 머리a4/상태820의 실제 원본 진단이다.
    std::function<void(Sid,std::int32_t)> setFrame; // 같은 풀의004acee0(frame,0)이다.
    std::function<void(float,float,std::string_view)> sound; // 004a9d70의 현재 좌표 manOWarBuild.wav 요청이다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> checkFrame; // 개발자 frameCheck와 진단의 외부 경계다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 실제004af530(type,0)이다.
    std::function<void(Sid,std::uint32_t)> setOwner; // 새 플라이어의 가상+74다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 새 플라이어의 가상+90이다.
    std::function<void(Sid,Sid)> bindFlyer; // 0045c1b0의 현재 BaseFlyerProcess 연결이다.
};
// 부모 계산을 원본 raw 저장소에서 수행하며 서로 다른 월드의 사본을 만들지 않는다.
class RawRainAviaryProcess {
public:
    RawRainAviaryProcess(SidPool& pool,RainAviaryProcessGlobals globals,RainAviaryProcessHooks hooks); // PATCH와 현재 전역 수명을 확인한다.
    void Initialize(RainAviaryProcessFields& fields) const; // 0045d810의 실제 생성자 쓰기만 수행한다.
    void PostPop(Sid parent,std::uint32_t flags) const; // 0045ed10의 중복/할당/생성/부착/Damageable 순서다.
    void Frame(RainAviaryProcessFields& fields) const; // inner0045dde0의 실제 상태1~10을 실행한다.
    static std::uint16_t WriteProcess(RainAviaryProcessFields& fields,std::span<std::uint8_t> packet,bool includeParent,bool force=false,std::uint8_t protocol=1,std::uint8_t message=0x73); // 0045d870→0045b2a0의 변경 비트와 길이를 기록한다.
    const SidPool& Pool() const; // Host와 동일 풀인지 확인한다.
private:
    SidPool& pool_; // 실제 아비어리/플라이어와 QA를 공유하는 단일 풀이다.
    RainAviaryProcessGlobals globals_; // 값 사본이 아닌 현재 장기 전역 참조다.
    RainAviaryProcessHooks hooks_; // 선택된 큰 효과의 실제 소비자다.
};
// 원본 inner Baseprocess를 실제 C++ Kernel 수명과 연결한다.
class RainAviaryProcess final : public SquidProcess {
public:
    RainAviaryProcess(const RawRainAviaryProcess& logic,RainAviaryProcessFields fields); // 이미 초기화한 원문 필드 소유권을 받는다.
    void RunFrame() override; // Kernel의 같은 프레임을 복원한 상태 기계에 전달한다.
    void OnAttached() override; // 원본 Host가 기록한 Form/부모를 같은 원문 필드에 반영한다.
    void Notify(std::uint32_t,std::uint32_t,std::uint32_t) override; // 실제 inner+1c는 ret12의 빈 몸체다.
    RainAviaryProcessFields& Fields(); // 원문 writer/검사가 같은 이미지를 조회한다.
    const RainAviaryProcessFields& Fields() const; // 상수 소비자도 사본을 만들지 않는다.
    std::uint16_t WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force=false); // OnAttached 전에도 현재 Host의 Form/부모를 쓴다.
private:
    const RawRainAviaryProcess& logic_; // 같은 Pool/현재 시각/효과를 사용하는 장기 처리기다.
    RainAviaryProcessFields fields_; // 실제48BYTE의 유일한 소유 이미지다.
};
}
