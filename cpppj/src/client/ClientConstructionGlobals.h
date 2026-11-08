// Client의 실제 건설 상태와 그 raw 참조가 빌리는 장기 owner를 함께 유지한다.
#pragma once
#include "client/RawCampaignConstructionBindings.h"

namespace netstorm::client { // Session 상태를 복제하지 않고 같은 Client 저장소에 결합한다.
class ClientGameplayGlobals; // 현재 Player/카메라/미션 전역의 실제 owner다.
struct ClientBuildReplayGlobals; // 현재 재생 DWORD와 객체를 같은 수명으로 보관한다.
class ClientSpGlobals; //초기화/난수 소비 없이 같은 실제 SP 저장소를 보유한다.
class ClientPlayerRecords; //현재 Player base를 원문 선택 위치에서만 읽는 실제 할당 owner다.
// 현재 Session의 큰 효과만 짧게 빌리며 장기 body는 이 객체를 강하게 소유하지 않는다.
struct ClientConstructionRuntimeBindings {
    RawCampaignConstructionEffects effects; //실제 현재 설정/표시/비용 안내/진단의 콜백이다.
    RawCampaignConstructionGameplay gameplay; //실제 현재 Builder/사제/배치/건설 모듈의 콜백이다.
    std::function<std::shared_ptr<void>()> pin; //공개된 전체 Runtime을 선택 호출 동안만 보유한다. 생성/직접 로드는 호출자의 소유권을 따른다.
};
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
    void BindRuntime(const std::shared_ptr<ClientConstructionRuntimeBindings>& runtime); //실제 현재 대여만 약하게 공개하고 중복된 살아 있는 대여는 거부한다.
    RawCampaignConstructionBindings& Body(std::shared_ptr<ClientSpGlobals> sp,std::shared_ptr<ClientPlayerRecords> players,std::shared_ptr<std::array<std::uint8_t,87>> options); //같은 장기 물리/현재 Player getter로 본체를 한 번 구성하며 World를 만들지 않는다.
    RawCampaignConstructionBindings& Body(); //이미 구성한 장기 본체를 빌리며 Session 반납 뒤에도 주소/상태를 보존한다.
private:
    std::shared_ptr<RawSurfaceStorage> storage_; // 상태의 TYPE/모드/Canon raw 참조보다 오래 유지한다.
    std::shared_ptr<ClientGameplayGlobals> gameplay_; // Player/카메라/미션 raw 참조의 실제 owner다.
    std::shared_ptr<ClientBuildReplayGlobals> replay_; // 선택적 재생 raw DWORD 참조의 수명을 보증한다.
    std::shared_ptr<ClientSpGlobals> sp_; //본체의 같은 실제 분산 SP 주소를 마지막 호출 뒤까지 보유한다.
    std::shared_ptr<ClientPlayerRecords> players_; //본체는 span을 고정하지 않고 이 owner의 현재base를 선택 위치에서 읽는다.
    std::shared_ptr<std::array<std::uint8_t,87>> options_; //같은87BYTE의 mapSize 주소를 본체보다 오래 유지한다.
    std::weak_ptr<const ClientConstructionRuntimeBindings> runtime_; //큰 Session 효과를 반납 뒤 보유하지 않으며 새 실제 대여로 교체할 수 있다.
    RawCampaignConstructionState state_; //현재 값과 raw 참조는 본체보다 오래 유지하고 앞의 실제 backing보다 먼저 반납한다.
    std::unique_ptr<RawCampaignConstructionBindings> body_; //전체 건설/Bounds/최근 목록 본체를 Client 수명에 보유하고 State/backing보다 먼저 반납한다.
};
}
