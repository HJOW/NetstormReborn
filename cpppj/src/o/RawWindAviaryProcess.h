// 데빌메이커의 실제80BYTE 생산 상태를 현재 Kernel과 같은 풀에 연결한다.
#pragma once
#include "o/RawWindAviaryState.h"
#include "o/SquidProcess.h"
#include <optional>

namespace netstorm::o {
// 처리 도중 바뀔 수 있는 원본 전역은 같은 수명의 참조로 읽는다.
struct WindAviaryGlobals {
    const bool& fort; const bool& friendlyFire; const bool& server; // 편집/친군 사격/서버의 실제 관문이다.
    const bool& boss; const std::uint32_t& editor; const bool& checking; // 같은 세션의 현재 생성 권한/편집 차단/개발자 진단이다.
    const double& now; // 모든 생산 단계가 사용하는 현재 게임 시각이다.
    const std::uint32_t& processType; const std::uint32_t& flyerType; const std::uint32_t& aviaryType; // 59/138/139의 현재 타입이다.
};
// 원문에서 큰 효과를 호출한 위치만 분리하고 실제 런타임 소비자를 요구한다.
struct WindAviaryHooks {
    std::function<Sid(Sid,std::uint32_t)> findProcess; // 현재 부모의 첫 살아 있는 타입59 Form이다.
    std::function<std::optional<WindAviaryProcessFields>()> allocate; // 실제 new50의 미정 바이트 또는 NULL이다.
    std::function<void(std::optional<WindAviaryProcessFields>,Sid,std::uint32_t)> attach; // flags8로 같은 Kernel에 부착한다.
    std::function<void(Sid,std::uint32_t)> damageable; // 전용 후처리 뒤 공통 Damageable이다.
    std::function<void(std::uint32_t)> assertion; // 원본 Flyer.cpp의 해당 행 진단이다.
    std::function<void(Sid,std::int32_t)> setFrame; // 실제 현재 슬롯의004acee0(frame,0)이다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> checkFrame; // checking일 때만 실행하는 현재 프레임 진단이다.
    std::function<Sid(std::uint32_t)> create; // 실제004af530(type,0)이다.
    std::function<void(Sid,std::uint32_t)> setOwner; // 새 더스트 데빌의 가상74다.
    std::function<float(Sid,std::int32_t)> height; // 현재 SHP 헤더의+14 높이다.
    std::function<float(Sid)> footY; // 현재 TYPE+1d8의 signed 발자국 높이다.
    std::function<void(Sid,float,float)> pop; // 새 플라이어의 실제 가상90(flags0)이다.
    std::function<void(Sid,Sid)> createFlyerProcess; // new68 성공 시45e1c0의 생성·부착과 전용 표 쓰기다.
    std::function<void(Sid,Sid)> bindFlyer; // 0045c1b0의 현재 홈 WORD 연결이다.
    std::function<std::array<float,2>(Sid,std::int32_t,std::int32_t)> circlePosition; // 특별 부모의004adea0/0041e020과 프레임13 높이를 적용한다.
    std::function<void(WindAviaryProcessFields&)> finish; // 상태13의 실제0041bf10(flags0)이다.
};
// 생성자/패킷은 검증된 State 모듈을 공유하며 Frame은45f0a0 그대로 실행한다.
class RawWindAviaryProcess {
public:
    RawWindAviaryProcess(SidPool& pool,WindAviaryGlobals globals,WindAviaryHooks hooks); // 같은 PATCH 풀과 현재 전역을 빌린다.
    void PostPop(Sid parent,std::uint32_t flags) const; // 0045edf0의 관문/할당/부착/공통 순서다.
    void Frame(WindAviaryProcessFields& fields) const; // 일반 생산과 특별 부모의 상태1..13이다.
    void FlyerDied(WindAviaryProcessFields& fields,const std::function<void(WindAviaryProcessFields&)>& refresh) const; // 0045b680의 상태2와 Form 갱신이다.
private:
    SidPool& pool_; WindAviaryGlobals globals_; WindAviaryHooks hooks_; // 원문·전역·효과의 단일 수명이다.
};
// 원본 inner+38의 실제 Kernel 소유권과80BYTE를 한 객체에 둔다.
class WindAviaryProcess final : public SquidProcess {
public:
    WindAviaryProcess(const RawWindAviaryProcess& logic,WindAviaryProcessFields fields); // 초기화한80BYTE를 소유한다.
    void RunFrame() override; // 같은 Kernel 프레임을 원문 상태 기계로 전달한다.
    void OnAttached() override; // 현재 Host의 Form/부모를 inner에 기록한다.
    void Notify(std::uint32_t,std::uint32_t,std::uint32_t) override; // 원본 ret12의 빈 통지다.
    WindAviaryProcessFields& Fields(); // 수신/현재 상태를 사본 없이 제공한다.
    std::uint16_t WritePacket(std::span<std::uint8_t> packet,bool parentChanged); // 같은 Host의 실제0x74 writer다.
private:
    const RawWindAviaryProcess& logic_; WindAviaryProcessFields fields_; // 처리기 참조와 유일한80BYTE 소유권이다.
};
}
