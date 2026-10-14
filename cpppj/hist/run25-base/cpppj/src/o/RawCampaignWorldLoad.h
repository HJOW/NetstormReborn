// 실제 미션 새login의 저장 월드 부모를 generic Fort 메뉴 부모와 별도 몸체로 연결한다.
#pragma once
#include "o/RawFortWorldLoad.h"
#include "o/RawPlayerWorld.h"
#include <string_view>

namespace netstorm::o {
// 실제 부모 prologue와 선택한 mission 후처리가 읽는 현재 전역이다.
struct CampaignWorldGlobals {
    std::uint32_t mySubscriber{},loginSubscriber{}; // PATCH540c74/54265c·CD50f6cc/511c7c의 현재 두값이다.
    std::uint32_t editor{}; // PATCH5c85a4/CD518904: 현재 Player의 residence/island DWORD를0으로 쓴다.
    bool missionPost{true}; // PATCH594fa4/CD52e170: 후처리923e0/40daf0의 두큰분기를 모두 생략하는 실제 미션 전역이다.
};
// 이미 완료한 동일 월드 하위 몸체와 명시 번역/config/spec/UI 경계를 공급한다.
struct CampaignWorldLoadHooks {
    std::function<std::span<const std::uint8_t>()> translateYou; // 원본 번역키"You"의 NUL 포함 실제 결과 BYTE다.
    std::function<void(std::string_view,std::span<const std::uint8_t>)> setConfig; // fort.mySubHandle과 NUL 포함 현재handle을 원본config 경계로 전달한다.
    std::function<void()> refreshFortSpecification; // PATCH460730/CD4ca4e0의 현재 사양 재조회 경계다.
    std::function<void(RawIslandBuilderAssertion)> builderAssertion; // 실제 PlaceAll/Add/Bounds/CanonDecoder 진단을 보존한다.
    std::function<void(const RawIslandBuilderState&,std::int32_t)> generate; // 같은지도/3204BYTE/두난수의 현재 정상 지형 생성이다.
    std::function<const FortTemplate&(std::uint32_t)> currentTemplate; // Objects 직전의 현재Template owner를 다시 선택한다.
    std::function<void(const FortTemplate&,std::uint32_t)> objects; // 기존 실제 ObjectLoad(현재owner,0,20)를 같은 Factory/Pool로 실행한다.
    std::function<void(bool)> rebuildGraph; // 실제 Graph.Rebuild(false)를 정확히 한 번 요청한다.
    std::function<void()> resetCamera; // PATCH4340d0/CD488f50의 미복원 카메라/UI 경계다.
};
// PATCH4b5300/CD4322e0은 Money/Badges/Technology를 호출하지 않는 실제 미션 설치 부모다.
class RawCampaignWorldLoad {
public:
    // 모든 현재 자료는 부모보다 오래 살고 동일 Pool·지도·현재 Player9 저장소를 사용해야 한다.
    RawCampaignWorldLoad(ChunkMap& world,RawIslandRuntime& islands,std::span<const RiftTypeFrames> frames,
        std::uint32_t& puzzlePiece,std::int32_t& noRegion,const RawFortWorldLoad& territory,RawPlayerWorld& player,
        CampaignWorldGlobals& globals,std::span<std::uint8_t> templateHeaders,std::span<std::uint8_t> currentHandle,CampaignWorldLoadHooks hooks);
    // 실제 prologue→Player→Builder/Territory/PlaceAll→Generate→Objects→Graph0→mission finish→Camera→현재이름 순서다.
    void Load() const;
    // 실제923e0/CD40daf0의 mission1 자연 반환만 복원한다. 다른 큰 분기는 명시 거부한다.
    void FinishMissionPriests() const;
    // 실제 소비자의 동일 Pool 연결을 확인한다.
    const SidPool& Pool() const;
private:
    // NUL 포함 현재 좁은문자 입력을 실제 저장소에 복사하며 물리메모리 밖 실행을 거부한다.
    static void CopyName(std::span<std::uint8_t> target,std::span<const std::uint8_t> source);
    // 현재 battleOwner의 실제 Player9 기록을 직접 빌린다.
    std::span<std::uint8_t> CurrentPlayer() const;
    ChunkMap& world_; // 실제builder attach의 원본 ChunkMap이며 사본을 만들지 않는다.
    RawIslandRuntime& islands_; // 실제 current count/Exists/Owner/Bounds의3204BYTE 저장소다.
    std::span<const RiftTypeFrames> frames_; // 기존PlaceAll decoder가 읽는 같은 TYPE 프레임이다.
    std::uint32_t& puzzlePiece_; // 현재 원본 puzzlePiece 타입 전역이다.
    std::int32_t& noRegion_; // Generate 직전 재조회하는 현재 지역없음 전역이다.
    const RawFortWorldLoad& territory_; // 기존 실제120BYTE getter/CD depth 진단 몸체를 읽기 재사용한다.
    RawPlayerWorld& player_; // 실제152 wrapper/Post와 현재9기록/두owner를 직접 공유한다.
    CampaignWorldGlobals& globals_; // 매 단계의 현재subscriber/editor/missionPost를 참조한다.
    std::span<std::uint8_t> templates_; // 현재9×20BYTE Template header이며+8 DWORD를 직접 쓴다.
    std::span<std::uint8_t> handle_; // 현재mySubHandle의 실제 NUL 저장소다.
    CampaignWorldLoadHooks hooks_; // 미복원 번역/config/spec/UI를 기본성공으로 덮지 않는다.
};
}
