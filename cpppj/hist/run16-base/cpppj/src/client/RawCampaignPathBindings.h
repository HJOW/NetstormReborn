// 실제 캠페인의 사제 명령을 같은 raw 월드/BFS/ProcessHost/Kernel에 연결한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "client/RawPriestWorldBindings.h"
#include "o/RawPathStart.h"
#include "o/RawPathSearch.h"
#include "o/RawPathData.h"
#include "o/RawPathCleanup.h"
#include "o/RawPriestHalt.h"
#include "o/RawPriestArrival.h"

namespace netstorm::client {
// 실제 자산 조회와 아직 별도로 연결해야 하는 큰 하위 효과의 명시 경계다.
struct RawCampaignPathEffects {
    std::function<o::RawPathStartType(o::Sid)> metadata; // 현재 raw 부모 타입의 실제 speed/turningSpeed/defaultFrame을 조회한다.
    std::function<std::uint32_t(o::Sid)> soundId; // 현재 타입의 stepSound를 실제 자산 저장소에서 조회한다.
    std::function<void(o::Sid,float,float,std::uint32_t)> stepSound; // 원본 step 요청의 현재 부모/x/y/소리 번호를 전달한다.
    std::function<void(o::RawPathPoint,std::uint32_t,std::uint32_t)> startBlocked; // PathStart 실패가 요청한 실제 위치/type/owner 겹침 정리다.
    std::function<void(o::Sid,o::RawPathPoint,std::uint32_t,std::uint32_t)> arrivalBlocked; // Path 도착의 실제 parent/point/genus/event 겹침 정리다.
    std::function<void(o::RawPathRuntime&,o::Sid)> retarget; // 실제 운반 대상 사망/변경 후 재탐색 몸체다. 미복원 선택은 caller가 예외로 드러낸다.
    std::function<std::uint64_t()> observationStamp; // CD 관찰 시계이며 원본 게임 시각과 별개다. 미연결이면 기존 Process의0 입력을 유지한다.
};
// 경로 계산과 프레임 실행을 새 이동 모형으로 대체하지 않는 같은 월드 연결부다.
// 참조하는 월드/사제/도착/cleanup/상태/effects는 이 객체 및 부착한 Path보다 오래 살아야 한다.
class RawCampaignPathBindings {
public:
    // 첫 명령 전에 실제 모듈/시계/판본/같은 풀 계약을 확인하며 Path 소유권은 기존 Host에 맡긴다.
    RawCampaignPathBindings(RawSurfaceWorld& world,RawWorldCore core,const RawPriestWorldBindings& priests,
        const o::RawPriestHalt& halt,const o::RawPriestArrival& arrival,const o::RawPathCleanup& cleanup,
        const o::RawContainedLookup& lookup,o::RawPathStartState& startState,o::RawPathProcessState& processState,
        o::RawPathSearchState& searchState,RawCampaignPathEffects effects);
    // 부착 경로 callback이 이 객체를 참조한다. 외부에서 ClearProcesses 뒤 이 객체를 해제한다.
    ~RawCampaignPathBindings();
    RawCampaignPathBindings(const RawCampaignPathBindings&)=delete;
    RawCampaignPathBindings& operator=(const RawCampaignPathBindings&)=delete;
    // 실제158/사제VT 명령만 기존 Start→Init→BFS→Prepare→Host54→부모 통지로 실행한다.
    // 원본 실패 후 재귀가 성공해도 바깥 반환은nullptr일 수 있으므로 현재 경로는 다시 조회한다.
    o::RawPathProcess* StartPriest(o::RawPathStartRequest request);
    // 현재 종속 체인의 실제54 Form을 조회하고 같은 Host의 기존 Path 객체를 반환한다.
    o::RawPathProcess* CurrentPath(o::Sid parent) const;
    // CarrierArrival 등 순환 연결에서 같은 실제 현재 Path 저장소를 빌린다.
    o::RawPathRuntime* CurrentRuntime(o::Sid parent) const;
    // 이미 있는 경로의 두 target DWORD만 기존 Runtime API로 쓴다. 경로가 없으면false다.
    bool SetTargets(o::Sid parent,std::uint32_t first,std::uint32_t second) const;
    // 실제 운반 재탐색 callback을 현재 Path에 호출한다. 새 직선 경로를 계산하지 않는다.
    bool RetargetCurrent(o::Sid parent) const;
    // 서버 Form의 실제 raw 경로 직렬화와 같은 버퍼 전송을 만든다. 다른 프로세스 송신은 caller가 공급한다.
    o::ProcessFormTransmission Transmission(o::RawPathDataHeader& header,std::span<std::uint8_t> buffer,
        std::function<void(std::int32_t)> transport,o::ProcessFormTransmission other={}) const;
    // 외부 도착/프로토콜 모듈이 다른 월드 경로를 연결하지 않게 실제 풀을 제공한다.
    const o::SidPool& Pool() const;
private:
    // 실제 현재 parent의 raw 타입/가상 표와 살아 있는 자산 상태를 검사한다.
    void ValidatePriest(o::Sid parent) const;
    // Process hook은 공간/부모/cleanup 실제 모듈을 호출하고 탐색만 기존 BFS에 연결한다.
    std::unique_ptr<o::RawPathProcess> Create(bool priest);
    RawSurfaceWorld& world_; // 실제 가상 Pop/Unpop/Repop과 유일한 Kernel을 소유한 월드다.
    RawWorldCore core_; // 실제 pool/hash/spot/type/frame/SHP/Host를 빌리는 참조 묶음이다.
    const RawPriestWorldBindings& priests_; // 기존 mutable 이동 프로세스 선택 몸체를 공유한다.
    const o::RawPriestHalt& halt_; // 실제 +38 부모의 지면/HP/Carrier 확인 몸체다.
    const o::RawPriestArrival& arrival_; // 실제 +3c 사제/Carrier 도착 몸체다.
    const o::RawPathCleanup& cleanup_; // 기존 Path 공간 접두 뒤 예약/의식/운반 정리 몸체다.
    const o::RawContainedLookup& lookup_; // 현재54 Form을 실제 종속 체인에서 찾는다.
    o::RawPathStartState& startState_; // root가 소유한 mutable 타입/권한/시각/스냅/재시도 상태를 직접 공유한다.
    o::RawPathProcessState& processState_; // 같은 ProcessHost 시계와 실제 사제 경로 상태다.
    RawCampaignPathEffects effects_; // 현재 metadata/소리/겹침/큰 재탐색의 명시 연결이다.
    o::RawPathSearch search_; // 같은 hash0/spot/type/frame에서 원본 BFS를 실행한다.
    std::unique_ptr<o::RawPathStart> start_; // 생성 callback이 이 객체를 참조하는 실제 명령 부모다.
};
}
