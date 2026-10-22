// AI 다리의 단계별 탐색·조각 소비·연결점 진행을 원본 본체에 적용한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/RawContainedFinder.h"
#include "o/RawPathRuntime.h"
#include <string_view>

namespace netstorm::o {
// 원본 전역의 현재 값을 각 효과 위치에서 다시 읽는다.
struct AiBridgePathGlobals {
    const double& now; //0055b4d0의 현재 게임 시각이다.
    const std::int32_t& mapLimit; //00531928의 좌표 상한이며 float로 변환해 검사한다.
    const std::uint32_t& bridgeType; //005411a0의 실제 다리 TYPE다.
    const std::uint32_t& logKind; //0052f3cc가29이면 디버그 표식도 배치한다.
    const std::uint32_t& startMarker; //0054121c의 시작점 표식 TYPE다.
    const std::uint32_t& endMarker; //00541234의 연결점 표식 TYPE다.
    const std::uint32_t& targetMarker; //0054122c의 목표점 표식 TYPE다.
};
// 탐색의 하위 알고리즘과 기존 실제 배치·기술부모·수명 경계를 연결한다.
struct AiBridgePathHooks {
    std::function<void(std::uint32_t,RawPathPoint,RawPathPoint)> initialize; //0040fde0:같은5f0 탐색 상태를 초기화한다.
    std::function<bool(std::uint32_t)> prepare; //0040fe90:현재 후보 준비의 원래 완료값이다.
    std::function<bool(std::uint32_t)> search; //004144b0:현재 다리 후보 탐색의 원래 완료값이다.
    std::function<RawPathPoint(std::uint32_t,RawPathPoint)> start; //00410b10:현재 그래프의 다리 시작점 탐색이다.
    std::function<void(std::uint32_t,std::uint32_t,RawPathPoint,std::uint32_t,std::uint32_t)> deploy; //004108f0:일반 다리 배치와 통지다.
    std::function<void(std::uint32_t,std::uint32_t,RawPathPoint)> marker; //004106e0:argument0/방향0의 표식 배치다.
    std::function<Sid(std::uint32_t)> technology; //0048fc90:현재 owner의 기술 부모다.
    std::function<void(std::uint32_t,Sid)> refresh; //0040fcb0:같은 실제 다리 재고를 다시 읽고 보충한다.
    std::function<void(Sid,std::uint32_t)> destroy; //첫 일치 조각의 가상+10(flags0) 삭제다.
    std::function<void(std::uint32_t,std::uint32_t,std::string_view)> log; //004105b0의 현재 종류/본체 출력 관문 뒤 실제 로그다.
};
//0040fd30/004167a0/004173b0/004174c0을 같은 본체와 내용물 풀에서 수행한다.
class RawAiBridgePath {
public:
    RawAiBridgePath(RawAiBrainMemory& memory,const SidPool& pool,const ContainedFinderState& finder,AiBridgePathGlobals globals,AiBridgePathHooks hooks); //PATCH 풀과 현재 원본 저장소를 빌린다.
    bool Initialize(std::uint32_t token,RawPathPoint source,RawPathPoint target) const; //0040fde0:실제 CRT 절삭·나선 시작·미정 결과·마지막 후보 번호만 초기화하고 현재 단계/주변 바이트를 보존한다.
    void Consume(std::uint32_t token,Sid parent,std::uint32_t pattern) const; //사용 가능하고 패턴 WORD가 전체 DWORD와 같은 첫 조각만 삭제한다.
    bool Step(std::uint32_t token,RawPathPoint& source,RawPathPoint target) const; //한 호출에서 한 탐색 단계만 처리하며 상태5에서만 true와 결과를 반환한다.
    RawPathPoint Extend(std::uint32_t token,RawPathPoint source,RawPathPoint target) const; //완료된 연결점의 배치→소비→재고 갱신을 원래 순서로 실행한다.
    bool Advance(std::uint32_t token,RawPathPoint target) const; //3d0 연결점을 처음 찾거나 연장하고 signed timeout을 처리한다.
    const SidPool& Pool() const; //실제 월드 조립 시 동일한 풀을 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재 살아 있는0xb08 본체만 조회한다.
    std::uint32_t Field(std::uint32_t token,std::size_t offset) const; //비정렬 DWORD를 현재 본체에서 읽는다.
    void Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const; //주변 필드를 보존하고 지정 DWORD만 쓴다.
    RawPathPoint Point(std::uint32_t token,std::size_t offset) const; //두 float의 원문 비트를 읽는다.
    void Point(std::uint32_t token,std::size_t offset,RawPathPoint value) const; //두 float의 원문 비트를 쓴다.
    bool Valid(RawPathPoint point) const; //0040e800의 두 축 strict0..상한/NaN 거부다.
    RawAiBrainMemory& memory_; //다른 AI 모듈과 같은 본체·목록 소유자다.
    const SidPool& pool_; //같은 현재 내용물 슬롯 풀이다.
    const ContainedFinderState& finder_; //현재 권한·내용물 TYPE를 공유한다.
    AiBridgePathGlobals globals_; //현재 시각/TYPE/로그 전역의 참조다.
    AiBridgePathHooks hooks_; //도달한 실제 하위 효과만 호출한다.
};
}
