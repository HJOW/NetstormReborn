// 골렘의 귀환·간헐천 목록 수집과 실제 경로 시각에 따른 후보 평가를 복원한다.
#pragma once
#include "o/RawBoardTarget.h"
#include "o/RawCarrierCandidateSort.h"
#include "o/SquidPostPop.h"

namespace netstorm::o {
// 후보 평가가 읽는 현재 전역과 원본 임시 metric DWORD다.
struct CarrierChoiceState {
    bool comparePaths{true}; //5316ac: AI/비행체가 아니면 거친 점수보다 나은 후보의 경로를 평가한다.
    std::array<std::uint32_t,4> cardinalTypes{137,156,135,187}; //488fb0이 현재 타입 getter 네 개와 비교한다.
    std::uint32_t baselineBits{0x461c4000},currentMetricBits{0x461c4000}; //540bf8/540bfc: 마지막에는 float 변환 없이 DWORD를 복사한다.
};
// 기존 발자국/BFS/유효성 몸체와 관측용 주소 재배치만 외부 경계로 둔다.
struct CarrierChoiceHooks {
    std::function<std::array<float,2>(Sid)> movement; // 현재 TYPE+e0/e4의 speed/turningSpeed를 처음에 한 번 읽는다.
    std::function<SquidFootprint(Sid)> footprint; // 현재4ade70이며 호출 전 보관한 좌표를 임의 재조회하지 않는다.
    std::function<void(RawPathRuntime&,Sid,RawPathPoint,RawPathPoint,std::uint32_t)> search; // 같은 실제488af0/BFS이며 임시 Path에만 결과를 쓴다.
    std::function<bool(Sid,Sid)> validReturn,validGeyser; // 기존4274b0/427580의 현재 소유권/Graph/상태 검사다.
    std::function<std::uint32_t(Sid)> graphFor; //429420의 반환값을 쓰지 않는462bc0 호출도 생략하지 않는다.
    std::function<void(std::uint32_t)> assertion; // 실제 Carrier.cpp ace/b03 진단이며 복귀 뒤 현재 값을 다시 읽는다.
    std::function<std::uint32_t(std::span<const std::uint32_t>,const void*)> relocate; // 독립 원문의 배열/풀 주소 재배치다. 게임 경로에서는 비워 둔다.
};
// 참조하는 현재 Player9/두 목록/시계/검사 전역은 이 객체보다 오래 살아야 한다.
class RawCarrierChoice {
public:
    RawCarrierChoice(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> players,
        SquidPostPopList& returns,SquidPostPopList& geysers,const double& now,const bool& checking,
        const RawBoardTarget& board,CarrierChoiceState& state,CarrierChoiceHooks hooks); // PATCH의 물리 폭과 참조 계약을 확인한다.
    Sid Evaluate(SquidPostPopList& candidates,Sid anchor,Sid mover) const; //4280a0: WORD 거리→원본 정렬→필요한 임시 경로→엄격한 최소값이다.
    Sid ChooseReturn(Sid parent) const; //428ea0: 현재Player+7c를 먼저 넣고 별도 outposts 목록을 수집한다.
    Sid ChooseGeyser(Sid parent) const; //429420: 먼저 귀환점을 고른 뒤 현재 geysers 목록을 그 점에 대해 평가한다.
    const SidPool& Pool() const; // 다른 월드의 후보·경로 연결을 확인한다.
private:
    Sid Number(std::uint32_t number) const; // DWORD 후보를 물리 범위 검사 뒤 SID로 바꾼다.
    RawPathPoint Position(Sid sid) const; // 두 좌표 float를 원본 읽기 시점에 보관한다.
    const RiftTypeRecord& Type(Sid sid) const; // 효과 뒤의 현재 raw 타입을 다시 읽는다.
    bool Controlled(Sid sid) const; //40f100: owner0은 거짓이며 그 외는 현재Player+b0이다.
    void Metric(Sid sid,std::uint16_t value) const; // free/dead 필터 없이 원본 임시 WORD+8만 쓴다.
    static std::int32_t Count(const SquidPostPopList& list); // 원본 signed count를 보존한다.
    SidPool& pool_; // 부모·후보·임시 거리의 같은 물리 저장소다.
    std::span<const RiftTypeRecord> types_; // 현재 genus/surface 플래그를 읽는다.
    std::span<const std::uint8_t> players_; // 현재9개 Player의 귀환점/AI 포인터를 읽는다.
    SquidPostPopList& returns_; //55a70c/710/714의 현재 별도 신전·아웃포스트 목록이다.
    SquidPostPopList& geysers_; //5950dc/e0/e4의 현재 간헐천 목록이다.
    const double& now_; // 실제55b4d0 QWORD이며 탐색 뒤 현재 시각을 사용한다.
    const bool& checking_; // 실제5e4794 진단 전역이다.
    const RawBoardTarget& board_; // 복원한 일반 접근점41d530을 재사용한다.
    CarrierChoiceState& state_; // 현재 탐색 설정/타입/임시 metric의 공유 전역이다.
    CarrierChoiceHooks hooks_; // 실제 큰 하위와 관측용 주소 변환이다.
};
}
