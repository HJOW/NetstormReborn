// Client 수명의 현재 Player/커서/검색/미션 전역과 짧은 실제 Runtime 대여를 구별한다.
#pragma once
#include "client/RawCursorDrop.h"
#include "o/RawCampaignWorldLoad.h"
#include "o/RawContainedFinder.h"
#include "o/RawEnergySupply.h"
#include <memory>

namespace netstorm::client {
class ClientSubscriberGlobals;class RawCampaignSession; // 실제 전역 소유자와 현재 Runtime을 복제하지 않는다.
class ClientGameplayGlobals {
private:
    std::shared_ptr<ClientSubscriberGlobals> subscribers_; // world의 두 DWORD 참조보다 오래 유지한다.
public:
    explicit ClientGameplayGlobals(std::shared_ptr<ClientSubscriberGlobals> subscribers={}); // 최초 전역값을 한 번 만들며 독립 Session만 자체 subscriber 값을 소유한다.
    ClientGameplayGlobals(const ClientGameplayGlobals&)=delete;ClientGameplayGlobals& operator=(const ClientGameplayGlobals&)=delete; // 살아 있는 전역 주소를 복제하지 않는다.
    class Use {
    public:
        ~Use(); // 일반 대여는 기존 무효화를 유지하고 준비 상태 대여는 Runtime 연결만 반납한다.
        Use(const Use&)=delete;Use& operator=(const Use&)=delete; // 단일 Runtime 대여를 복제하지 않는다.
        void Publish(RawCampaignSession& session); // 완성한 실제 Session만 현재 실행 대상으로 연결한다.
        void Withdraw(RawCampaignSession& session); // 모듈 소멸이 시작되기 전에 실행 대상만 해제한다.
    private:
        friend class ClientGameplayGlobals;
        explicit Use(ClientGameplayGlobals& globals,bool prepared=false); // 중복 대여를 먼저 거부하며 명시 준비 대여는 게임 값을 초기화하지 않는다.
        ClientGameplayGlobals& globals_; // Client의 실제 고정 주소다.
        bool prepared_; // 실제 Native 부모의 준비 값을 대여 종료에서도 암묵 정리하지 않는 수명 정책이다.
    };
    std::unique_ptr<Use> Acquire(); // 기존 독립 Session 생성 의미를 같은 전역 주소에서 수행한다. NativeState 전체 순서와 구별한다.
    std::unique_ptr<Use> AcquirePrepared(); // 명시 Native 정리/prefix의 현재 값을 그대로 빌린다. 준비 완료 판정은 조립 호출자의 책임이다.
    RawCampaignSession& RequireSession() const; // 선택된 게임 효과가 실제 Runtime을 요구할 때만 호출한다.
    bool Borrowed() const; // 현재 대여 여부의 읽기 전용 관측이다.
    const std::shared_ptr<ClientSubscriberGlobals>& Subscribers() const; // 다른 Client의 참조를 잘못 합치지 않는다.
    o::PlayerWorldState player; //595344/540c70/540cac 등 실제 현재 Player 조회 전역이다.
    CursorDropState cursor{.lockDepth=1}; //5325c8 초기 잠금1과 실제 현재 조각/마우스/플래그다.
    o::ContainedFinderState finder; // 실제 현재 권한/TYPE6/진단을 모든 생산 소비자가 공유한다.
    o::CampaignWorldGlobals world; // 실제 두 Subscriber와 editor/missionPost를 같은 주소에서 보관한다.
    o::EnergySupplyState energy; // 공급 모듈과 메뉴가 같은5cab30을 빌린다. userCheat은 Runtime 교체로 지우지 않는다.
    std::uint32_t specialAuxiliary{}; // 원문55a484이며 설정 해석에서55a480과 함께0으로 쓴다. 별도 기능 해석은 후속이다.
private:
    RawCampaignSession* current_{};bool borrowed_{}; // 게임 값의 대체가 아닌 현재 C++ Runtime 대여다.
};
}
