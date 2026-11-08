// Client의 실제 건설 상태와 그 raw 참조가 빌리는 장기 owner를 함께 유지한다.
#pragma once
#include "client/RawCampaignConstructionBindings.h"

namespace netstorm::client { // Session 상태를 복제하지 않고 같은 Client 저장소에 결합한다.
class ClientGameplayGlobals; // 현재 Player/카메라/미션 전역의 실제 owner다.
struct ClientBuildReplayGlobals; // 현재 재생 DWORD와 객체를 같은 수명으로 보관한다.
class ClientConstructionGlobals {
public:
    ClientConstructionGlobals()=default; // 새 Client의 건설 상태를 한 번 구성하며 실제 결합/게임 초기화는 따로 수행한다.
    ClientConstructionGlobals(const ClientConstructionGlobals&)=delete;ClientConstructionGlobals& operator=(const ClientConstructionGlobals&)=delete; // 같은 실제 전역 상태를 복제하지 않는다.
    ClientConstructionGlobals(ClientConstructionGlobals&&)=delete;ClientConstructionGlobals& operator=(ClientConstructionGlobals&&)=delete; // 현재 Session이 빌린 상태 주소를 이동하지 않는다.
    // 실제 소유자를 결합만 하며 World/Player/SP/난수/게임 상태를 초기화하지 않는다.
    void Bind(std::shared_ptr<RawSurfaceStorage> storage,std::shared_ptr<ClientGameplayGlobals> gameplay,std::shared_ptr<ClientBuildReplayGlobals> replay);
    bool Matches(const std::shared_ptr<RawSurfaceStorage>& storage,const std::shared_ptr<ClientGameplayGlobals>& gameplay,const std::shared_ptr<ClientBuildReplayGlobals>& replay) const; // 다른 Client/전역을 섞기 전에 정체성만 확인한다.
    RawCampaignConstructionState& State(); // 같은 현재 건설 상태의 주소를 Session 교체 뒤에도 유지한다.
    const RawCampaignConstructionState& State() const; // 값 사본 없이 같은 장기 상태를 조회한다.
private:
    std::shared_ptr<RawSurfaceStorage> storage_; // 상태의 TYPE/모드/Canon raw 참조보다 오래 유지한다.
    std::shared_ptr<ClientGameplayGlobals> gameplay_; // Player/카메라/미션 raw 참조의 실제 owner다.
    std::shared_ptr<ClientBuildReplayGlobals> replay_; // 선택적 재생 raw DWORD 참조의 수명을 보증한다.
    RawCampaignConstructionState state_; // 참조를 빌린 앞의 세 owner보다 먼저 반납한다. 전체 건설 body의 소유권과 구별한다.
};
}
