// 실제 수집자의 수집/포획/귀환/경로 취소와 필요한 원본 genus 조회·다이스 접근점을 복원한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/RawContainedFinder.h"
#include "o/RawPathRuntime.h"
#include <string_view>

namespace netstorm::o {
//004acb80:현재 내용물 kind WORD의 TYPE genus를 먼저 읽고 전체WORD mask/모든 genus 비트/순번을 검사한다.
Sid FindContainedGenus(const SidPool& pool,const ContainedFinderState& state,std::span<const RiftTypeRecord> types,Sid parent,std::uint32_t mask,std::uint32_t genus,std::uint32_t index);
//004487e0:현재 진단 뒤 다이스 raw좌표-3 또는 같은 월드의 현재 invalid 두 float를 돌려준다.
RawPathPoint DaisApproachPoint(const SidPool& pool,std::span<const RiftTypeRecord> types,const std::uint32_t& checking,const RawPathPoint& invalid,const std::function<void(std::string_view)>& assertion,Sid dais);
// 같은 실제 현재 전역을 참조하고 호출 위치에서 다시 읽는다.
struct AiCarrierActionGlobals {
    const double& now; //0055b4d0의 현재 시각이다.
    const bool& boss; //00540bc4의 현재 권한이며 첫 크리스털 조회 뒤 판정한다.
    const std::uint32_t& checking; //005e4794의 free 진단 활성값이다.
    std::function<std::uint32_t()> invalidGraph; //같은 장부의 현재00540410이다.
};
// 각 분기에 도달한 실제 하위만 호출하며 빈 효과를 성공으로 바꾸지 않는다.
struct AiCarrierActionHooks {
    std::function<std::int32_t(Sid)> maxHitPoints; //prefix004adcb0은 결과가 버려져도 실행한다.
    std::function<Sid(Sid)> prisoner; //004acb80(mask20,genus200000,index0)이다.
    std::function<Sid(Sid)> path; //00411630과 같은 현재TYPE/경로/종속 조회다.
    std::function<RawPathPoint(Sid)> daisPoint; //004487e0의 실제 접근점이다.
    std::function<void(std::uint32_t,Sid,RawPathPoint)> point; //004117e0의 같은 실제 좌표 이동이다.
    std::function<void(std::uint32_t,Sid)> home; //004118a0의 실제 신전 귀환이다.
    std::function<bool(std::uint32_t)> enemyAvailable; //00492280의 현재 참여/포획 상태다.
    std::function<bool(Sid)> vulnerable; //00411ca0의 현재HP/최대HP 비교다.
    std::function<bool(std::uint32_t)> flyingGetter; //00410980의 같은384 목록 비행 여부다.
    std::function<std::uint32_t(Sid)> graph; //SID0 아닌00410260→00462bc0 실제 그래프다.
    std::function<std::int32_t(Sid)> crystals; //004acc60(mask20,현재크리스털TYPE)의 실제 총수다.
    std::function<Sid(Sid)> firstCrystal; //004ac9f0(mask20,현재크리스털TYPE,index0)이다.
    std::function<void(Sid)> dropRequest; //00427320→00427070(actor,content0)의 실제 요청이다.
    std::function<void(Sid)> destroy; //첫 크리스털 Form의 실제 가상+10(flags0)이다.
    std::function<void(std::uint32_t,Sid,Sid)> target; //004116d0의 같은 실제 대상 이동이다.
    std::function<void(std::uint32_t,Sid)> remember; //004109d0의 같은384 등록/로그다.이동 실패여도 호출한다.
    std::function<void(std::uint32_t,Sid)> collect; //00411900의 실제 수집 또는 귀환 판단이다.
    std::function<std::array<std::uint32_t,2>(Sid)> targets; //00426150의 실제 현재 경로 TargetA/B다.
    std::function<void(std::uint32_t)> reclassify; //00411350의 같은330/348 재분류다.
    std::function<void(Sid)> stop; //00411680의 현재 경로 가상 삭제다.
    std::function<void(std::string_view,std::uint32_t)> assertion; //Ai.cpp의 복귀 가능한 원본 진단과 줄 번호다.
};
// 원본00412130 전체를 같은 본체/목록/현재 월드에서 수행한다.자동 프레임 순서는 별도다.
class RawAiCarrierActions {
public:
    RawAiCarrierActions(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,AiCarrierActionGlobals globals,AiCarrierActionHooks hooks); //같은 PATCH188 저장소와 실제 하위를 빌린다.
    void Act(std::uint32_t token) const; //포획 사제 귀환을 우선하고 한 수집자의 수집/포획/현재 경로를 판단한다.
    const SidPool& Pool() const; //같은 실제 월드 연결을 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //실제 살아 있는0xb08 본체만 허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //현재 본체 DWORD다.
    void Field(std::uint32_t token,std::size_t at,std::uint32_t value) const; //해당 DWORD만 쓴다.
    std::int32_t Count(std::uint32_t token,std::size_t list) const; //현재 목록+8의 signed count다.
    std::uint32_t Item(std::uint32_t token,std::size_t list,std::int32_t index) const; //현재 포인터의 실제 DWORD를 읽는다.
    bool Contains(std::uint32_t token,std::size_t list,std::uint32_t value) const; //현재 active 목록의 첫 일치다.
    void Append(std::uint32_t token,std::size_t list,std::uint32_t value) const; //용량 부족이면 포인터를 읽지 않는다.
    void Remove(std::uint32_t token,std::size_t list,std::uint32_t value) const; //모든 같은 SID를 안정 제거한다.
    bool Valid(std::uint32_t value) const; //원본0040ea40은 물리1..capacity-1만 검사한다.
    Sid Object(std::uint32_t value) const; //물리 범위 검사 뒤 WORD SID로 바꾼다.0은 물리 슬롯이다.
    std::uint32_t Genus(Sid actor) const; //현재TYPE의 순수flags2다.
    std::uint32_t Graph(Sid actor) const; //00410260:0이면 현재 무효 그래프,그 외 실제 getter다.
    bool Ready(std::uint32_t token,std::size_t at) const; //원본 now→due/C0 검사와 같은 ordered now>=due다.
    void Deliver(std::uint32_t token,Sid actor) const; //현재 다이스2a4가 있으면 좌표 접근,없으면 신전 복귀다.
    bool MayCapture(std::uint32_t token,Sid actor) const; //능력/용량/물리 적 사제/참여/HP/기한/비행·그래프의 원본 단락 순서다.
    void Capture(std::uint32_t token,Sid actor) const; //크리스털 비우기/실제 적 이동/포획 운반 등록이다.
    RawAiBrainMemory& memory_; //다른 AI 모듈과 같은 본체/목록 소유자다.
    const SidPool& pool_; //현재 free/TYPE/목표 상태의 유일한 실제 풀이다.
    std::span<const RiftTypeRecord> types_; //같은 현재188TYPE다.
    AiCarrierActionGlobals globals_; //현재 시각/권한/진단/무효 그래프의 참조다.
    AiCarrierActionHooks hooks_; //선택된 실제 하위 효과다.
};
}
