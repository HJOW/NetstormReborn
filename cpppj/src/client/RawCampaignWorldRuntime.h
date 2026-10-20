// 실제 캠페인 부모와 일반 지형·저장 객체를 동일 raw 월드에 설치한다.
#pragma once
#include "client/RawCampaignOwnership.h"
#include "client/RawNormalTerrainWorldBinding.h"
#include "o/RawCampaignWorldLoad.h"
#include <memory>

namespace netstorm::client {
// 상위 로그인/Template 계층이 소유하는 현재 저장소를 그대로 빌린다.
struct RawCampaignInstallData {
    std::span<const o::FortTemplate> templates; // 현재9 Template의 실제 값형 해석 자료다.
    std::span<o::FortTerritoryStream> territories; // 같은9 Template의 Territory cursor/120BYTE 저장소다.
    std::span<std::uint8_t> headers; // 실제9×20BYTE header이며 부모가 현재 owner의+8에 쓴다.
    std::span<std::uint8_t> handle; // 실제 mySubHandle의 NUL 포함 저장소다.
    std::function<const o::FortTemplate&(std::uint32_t)> templateAt; // 실제 Client는 현재 manager를 조회해 빌린 자료도 같은 객체로 반환한다.
    std::function<o::FortTerritoryStream&(std::uint32_t)> territoryAt; // 같은 manager의 현재 지형 cursor다. 기존 독립 입력은 위 span을 유지한다.
};
// Core 밖의 실제 UI/번역/가상 상태/진단 경계만 공급한다. gameplay 효과는 아래 실제 모듈을 사용한다.
struct RawCampaignInstallEffects {
    std::function<std::span<const std::uint8_t>()> translateYou; // 현재 번역키 You의 실제 NUL 포함 바이트다.
    std::function<void(std::string_view,std::span<const std::uint8_t>)> setConfig; // 현재 fort.mySubHandle을 저장한다.
    std::function<void()> refreshWindowTitle,resetCamera; // 원본 창 제목 갱신과 카메라 재설정의 실제 소비자다.
    std::function<void(o::RawIslandBuilderAssertion)> builderAssertion; // PlaceAll의 실제 진단이다.
    std::function<void(o::FortWorldLoadAssertion)> territoryAssertion; // 현재 Territory의 누락/범위 진단이다.
    std::function<void(o::Sid)> notifySurface; // 저장 QA 지정 뒤 다리의 실제 표면 통지다.
    std::function<void(o::Sid,std::uint32_t)> setState; // Factory 상태의 실제 가상+84 분배기다.
    std::function<std::int32_t(o::Sid)> virtualCount; // 실제 가상+80이며 편집 분기에서도 기본1로 바꾸지 않는다.
};
// Player/사제/Asset PostPop을 먼저 같은 Core에 등록한 뒤 첫 객체 생성 전에 구성한다.
// 이 연결은 전투 프레임/Ready/경제 스크립트 부모를 대신하지 않는다.
class RawCampaignWorldRuntime {
public:
    // 모든 자료와 실제 모듈은 이 연결보다 오래 살아야 한다. 다른 Pool/Player 기록은 거부한다.
    RawCampaignWorldRuntime(RawSurfaceWorld& world,o::ChunkMap& chunks,o::RawIslandRuntime& islands,
        o::RawPlayerWorld& player,RawNormalTerrainWorldBinding& normal,o::GameRandom& random,const o::SquidReward& hp,
        RawCampaignOwnership& ownership,o::RawFortFinishOwner& finish,const o::RawContainedCreate& contained,
        o::FortObjectLoadState& objects,o::CampaignWorldGlobals& globals,std::uint32_t& puzzlePiece,
        std::int32_t& noRegion,RawCampaignInstallData data,RawCampaignInstallEffects effects);
    // 내부 훅의 고정 주소를 유지하며 복사/이동하지 않는다.
    ~RawCampaignWorldRuntime();
    RawCampaignWorldRuntime(const RawCampaignWorldRuntime&)=delete;
    RawCampaignWorldRuntime& operator=(const RawCampaignWorldRuntime&)=delete;
    // BeginLoad→실제 캠페인 부모→Graph(false) 이후 종료를 한 번 실행한다. 실패를 로드 완료로 바꾸지 않는다.
    void Load();
    // 검사/후속 미션 부모가 같은 Pool과 저장객체 몸체를 확인할 때 사용한다.
    const o::SidPool& Pool() const;
    const o::RawFortObjectLoad& Objects() const;
private:
    struct Impl; // 동일 Core와 실제 세 로더 몸체를 고정 주소에 소유한다.
    std::unique_ptr<Impl> impl_; // 공급한 상태와 객체는 이 구현보다 오래 살아야 한다.
};
}
