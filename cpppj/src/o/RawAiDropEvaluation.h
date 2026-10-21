// AI의 부족 좌표 점수/시설별 평가 분배와 실제 배치 후보·에너지 승인을 복원한다.
#pragma once
#include "o/RawAiDropTargets.h"
#include <string>

namespace netstorm::o {
// 현재 원본 마나 전역을 빌려 매 평가 위치에서 다시 읽는다.
struct AiDropEvaluationGlobals {
    const float& manaRadius; //0052f488의현재공급반경이다.제곱반경과구분한다.
    const std::uint32_t& probeType; //00541174의현재공급문자열조회TYPE다.초기값71이다.
};
// 같은 실제 마나·배치 검사·부족 좌표와 원본 로그 효과다.
struct AiDropEvaluationHooks {
    std::function<void(std::string&,float,float,std::uint32_t,std::uint32_t,std::int32_t)> collect; //004730c0:초기빈문자열/현재TYPE·owner/theme-1이다.
    std::function<bool(const CanonPlacementQuery&)> mayPlace; //0049b510전체:argument0/flags0/ownerBYTE/mode1이다.
    std::function<std::uint32_t(std::string&,float,float,std::uint32_t,std::uint32_t)> energy; //004734d0의실제DWORD승인이다.bool로축소하지않는다.
    std::function<bool(std::uint32_t,float,float,std::uint32_t)> dry; //0040f1c0의같은실제부족좌표추가다.
    std::function<void(std::uint32_t,std::uint32_t)> unsupported; //004105b0 종류29의현재TYPE이름진단이며b00비영때만호출한다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> dryLog; //004105b0의현재Dry Spot종류/절삭좌표이며실제추가뒤만호출한다.
};
// 별도 점수·목록 사본 없이 같은 실제본체와 표적 평가기를 사용한다.
class RawAiDropEvaluation {
public:
    RawAiDropEvaluation(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,const RawAiDropTargets& targets,AiDropEvaluationGlobals globals,AiDropEvaluationHooks hooks); //같은PATCH풀/본체/현재TYPE를빌린다.
    float DryDistance(std::uint32_t token,RawPathPoint point,std::int32_t& index) const; //00412640:0거리제외/1000-Manhattan최대의첫기록이며index미선택은보존한다.
    std::int32_t SupplyScore(std::uint32_t token,RawPathPoint point) const; //004126c0:거리>1000-2반경→현재공급C문자열길이→저장float절삭/(길이+1)이다.
    std::int32_t Evaluate(std::uint32_t token,std::uint32_t type,RawPathPoint point,std::int32_t direction,Sid& target) const; //00412ab0의flags800/4000000/group3/0·1/6/진단순서를보존한다.
    bool Consider(std::uint32_t token,std::uint32_t type,RawPathPoint point,std::int32_t direction,RawPathPoint& bestPoint,std::int32_t& bestScore,Sid& target) const; //00412b90:배치→평가→양수동률허용→실제에너지→Y/X/점수기록또는부족좌표추가다.
    const SidPool& Pool() const; //같은실제월드연결확인용이다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재살아있는실제0xb08본체만허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //하위효과뒤현재DWORD를읽는다.
    const RiftTypeRecord& Type(std::uint32_t type) const; //현재전체TYPE범위를검사한다.
    RawAiBrainMemory& memory_; //같은본체/부족좌표backing저장소다.
    const SidPool& pool_; //같은실제월드정체성이다.
    std::span<const RiftTypeRecord> types_; //현재시설flags2/group과진단TYPE를읽는다.
    const RawAiDropTargets& targets_; //같은실제Gun/지상/블로커점수다.
    AiDropEvaluationGlobals globals_; //현재반경/조회TYPE의참조다.
    AiDropEvaluationHooks hooks_; //선택된실제하위와로그장치다.
};
}
