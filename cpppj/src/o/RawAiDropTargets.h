// AI 건물 배치의 실제 표적 점수와 Gun/장애물 후보 평가를 복원한다.
#pragma once
#include "o/RawAiBuildSearch.h"
#include "o/RawSquidNeighbors.h"

namespace netstorm::o {
// 원본 현재 TYPE 전역을 같은 저장소에서 빌린다.
struct AiDropTargetGlobals {
    const std::uint32_t& altarType; //005412e8의 현재 제단 TYPE다.
    const std::uint32_t& directionalType; //00541290의 현재 윈드 블로커 TYPE다.
};
// 이미 복원된 실제 내용물/TYPE+48/HP/검색/중심 조회만 연결한다.
struct AiDropTargetHooks {
    std::function<Sid(Sid)> captive; //004acb80(mask20,genus200000,0)의 실제 첫 사제 내용물이다.
    std::function<float(Sid)> rate; //004ac550→0049acf0의 현재 TYPE+48 hpPerSec다.
    std::function<std::int32_t(Sid)> maxHitPoints; //004adcb0의 현재 최대HP다.현재HP는 먼저 저장한다.
    std::function<Sid(float,float,std::uint32_t,std::int32_t,std::uint32_t)> gun; //0046b6f0의 실제 Archer/Cannon finder다.
    std::function<std::vector<std::uint32_t>(std::uint32_t,RawPathPoint,std::size_t)> blockers; //00469590의 같은 실제 Gun 수집이며 원본 임시 용량은30이다.
    std::function<RawPathPoint(Sid)> center; //004adea0의 현재 실제 발자국 중심이다.
    std::function<void(std::string_view,std::uint32_t)> assertion; //SID0 운반체에서 원본 진단 후 계속한다.
};
// 같은 본체/물리 풀/현재 TYPE와 실제 검색 하위로만 점수·출력 SID를 계산한다.
class RawAiDropTargets {
public:
    RawAiDropTargets(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,AiDropTargetGlobals globals,AiDropTargetHooks hooks); //같은 PATCH 저장소를 빌리며 생성 중 검색하지 않는다.
    std::int32_t Score(std::uint32_t token,Sid target) const; //00412770 전체 가산점→hpPerSec 합산 ftol→목록/집중→적 소유자 절반이다.
    std::int32_t Gun(std::uint32_t token,std::uint32_t type,RawPathPoint point,std::int32_t direction,Sid& result) const; //004128f0:실제 검색 결과를 먼저 쓰고 비영만 점수화한다.
    std::int32_t Blockers(std::uint32_t token,std::uint32_t type,RawPathPoint point,std::int32_t direction,Sid& result) const; //00412950:모든 점수의 감김 합이며 최고 갱신 시 원본처럼 목록 첫 SID를 쓴다.
    bool Facing(RawPathPoint point,std::int32_t direction,Sid target) const; //0048ece0:flags100000 제외→실제 중심→네 방향 일치다.홀수 방향도 중심은 조회한다.
    const SidPool& Pool() const; //같은 실제 월드 연결 확인용이다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재 살아 있는 실제0xb08 본체만 허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //현재 본체 DWORD를 읽는다.
    const RiftTypeRecord& Type(Sid target) const; //현재 BYTE TYPE를 물리 표 범위 검사 뒤 읽는다.
    Sid Object(std::uint32_t value) const; //목록 DWORD의 물리 범위를 확인하고 WORD SID로 좁힌다.
    bool Contains(std::uint32_t token,std::size_t pointer,Sid target) const; //최초 포인터/count를 순서대로 읽어 첫 일치만 보고한다.
    RawAiBrainMemory& memory_; //다른 AI 행동과 같은 본체/목록 저장소다.
    const SidPool& pool_; //현재 TYPE/HP/소유자/내용물의 유일한 물리 풀이다.
    std::span<const RiftTypeRecord> types_; //현재 flags2/group을 읽는 실제 TYPE 표다.
    AiDropTargetGlobals globals_; //현재 제단/방향 블로커 TYPE 전역의 참조다.
    AiDropTargetHooks hooks_; //선택된 실제 원본 하위다.
};
}
