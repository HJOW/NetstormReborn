// Factory의 실제 event3 예측 그림·완료와 종속 그림 정리를 같은 월드에서 처리한다.
#pragma once
#include "o/SquidProcess.h"
#include <string_view>

namespace netstorm::o {
// 원본 각 호출 지점에서 다시 읽는 전역의 참조다.
struct FactoryUpgradeState {
    const std::uint32_t& outpost; //00541250:완료를 생략하는 아웃포스트 TYPE다.
    const std::uint32_t& dependType; //00541074:현재 DependFinder 필터다.
    const std::uint32_t& localPlayer; //00540c70:완료 안내 소리의 현재 로컬 소유자다.
    const bool& checking; //005e4794:dead 부모 finder 진단 선택이다.
    const bool& challenge; //00594fc8:dead 부모 finder 진단을 생략한다.
    const bool& boss; //00540bc4:dead 부모 finder 진단을 생략한다.
};
// 실제 가상 메서드·건설·효과·AI 저장소를 연결하며 선택한 하위만 요구한다.
struct FactoryUpgradeHooks {
    std::function<std::uint32_t(Sid)> level; //가상80의 현재 레벨이다.
    std::function<void(Sid,std::uint32_t)> setLevel; //가상84의 Level 쓰기와 표시 갱신이다.
    std::function<std::int32_t(Sid)> maxHp; //004adcb0의 현재 최대 HP다.
    std::function<void(Sid)> adjustFrame; //00453780의 상태 글자 프레임 보정이다.
    std::function<void(Sid)> display; //가상88의 실제 표시 갱신이다.
    std::function<void(float,float,std::string_view)> soundAt; //004a9d70의 위치 소리(loop0)다.
    std::function<void(std::string_view)> sound; //004a9cb0의 전역 소리(0,0,0,1,0)다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> flare; //0045a6c0의 실제 개수/사건 효과다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //004af530의 같은 타입/flags2 생성이다.
    std::function<void(Sid,std::uint32_t)> owner; //가상74의 현재 소유자 지정이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; //가상90의 실제 공간 등록이다.
    std::function<void(Sid,float)> construction; //00443fe0의 같은30/17/47 건설 진행이다.
    std::function<void(Sid,Sid)> depend; //004b1510의 실제 watched/watcher 쌍이다.
    std::function<Sid(Sid)> watcher; //004ae510의 상대 Form을 통한 현재 감시자 SID다.
    std::function<void(Sid,std::uint32_t)> destroy; //예측 그림의 가상10(flags0)이다.
    std::function<bool(Sid)> aiPresent; //0040f100의 현재 소유자와 Player+b0 검사다.
    std::function<void(std::uint32_t,Sid)> notifyAi; //00413b70/00411180의 현재 AI 생산 목록/지갑 통지다.
    std::function<void(std::uint32_t,std::string_view)> log; //원본 종류3/4의 진행 로그다.
    std::function<void(std::uint32_t)> assertion; //Factory/Squid.cpp의 원본 진단 줄 번호다.
};
// 별도 복사 그림/예약 목록 없이 같은 raw 풀과 Host를 사용한다.
class RawFactoryUpgrade {
public:
    RawFactoryUpgrade(SidPool& pool,SquidProcessHost& host,FactoryUpgradeState state,FactoryUpgradeHooks hooks); //같은 PATCH 풀/Host 계약만 확인한다.
    std::uint32_t NextLevel(Sid parent) const; //00453750:상한3과 상한 미만의 두 번째 가상 조회다.
    void Begin(Sid parent,float payload) const; //004549c0 count0:다음레벨 abstract client 그림→건설→감시쌍이다.
    void Complete(Sid parent) const; //004545e0:레벨/HP/그림/소리/플레어→모든4/3예약→예측그림 삭제다.
    float Finish(Sid parent) const; //004549c0 count양수:로그→완료→현재 AI 통지→-1이다.
    bool HasPicture(Sid parent) const; //004537f0:같은TYPE/extra8/client범위의 첫 감시자를 찾는다.
    bool IsUpgrading(Sid parent) const; //00453c90:그림→event4→event3의 단락 조회다.
    const SidPool& Pool() const; //같은 월드 조립을 확인한다.
private:
    bool Picture(Sid parent,Sid candidate) const; //타입/extra8/번호 범위만 원본 순서로 검사한다.
    Sid First(Sid parent) const; //004b22d0의 진단 뒤 현재 head를 읽는다.
    Sid Next(Sid& next,std::size_t& steps) const; //004b1b80/004b1bd0의 저장next/현재TYPE 필터다.
    SidPool& pool_; //실제 Factory/예측 그림/종속 Form의 같은 저장소다.
    SquidProcessHost& host_; //실제 SharedRegular/Regular 검색·삭제다.
    FactoryUpgradeState state_; //원본 전역의 현재 참조다.
    FactoryUpgradeHooks hooks_; //선택된 실제 가상·장치·AI 경계다.
};
}
