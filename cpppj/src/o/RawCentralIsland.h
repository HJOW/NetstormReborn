// 원문0046e140의 일반 중앙섬 생성·중심 기록·거주 구역·군도 분리 부모다.
#pragma once
#include "o/RawIslandBuilder.h"
#include <array> // 실제 중앙 X/Y의 두 float 저장소 형식이다.

namespace netstorm::o { // Template 특수 배치와 metadata/State는 외곽 부모가 선택한다.
struct CentralIslandGlobals {
    const bool& battle;const bool& boss; // 현재 전투 모드와 권한을 원문 위치에서 각각 읽는다.
    const std::uint32_t& terrainEnabled;const std::uint32_t& archipelago; //00540a2c/005c8bb0의 현재 두 DWORD다.
    std::array<float,2>& position; //00568afc/00568b00의 같은 장기 중심 X/Y다.
    const std::int32_t& unassignedTerritory; //00540a30의 현재 invalidRegion127이며 owner0과 구별한다.
};
struct CentralIslandHooks {
    std::function<void(std::string_view,std::uint32_t)> assertion; // 원문 ea/eb/ec/ed 진단 뒤 복귀하면 현재 상태로 계속한다.
    std::function<ChunkRect(std::span<const std::uint8_t,6>)> placement; //00431ff0의 원점6/6과 현재 패턴 크기다.
    std::function<void(const RawIslandBuilderState&,std::int32_t)> generate; // 같은 실제0046da70의 지형/간헐천/섬 후처리다.
    std::function<void(std::int32_t)> residence; // 권한 및 terrainEnabled0일 때만 실제0046df80을 실행한다.
    std::function<void()> partition; // 현재 archipelago 비영일 때 실제0046f7d0의 지역 분리다.
    std::function<void(bool)> graph; // 마지막00463110(0)의 실제 Graph 전체 재구성이다.
    std::function<void(RawIslandBuilderAssertion)> builderAssertion; // 기존 Place의 같은 진단 장치다.
    const CanonDecoderDiagnostics* decoderDiagnostics{}; // Client의 현재 Canon 진단 수명을 빌린다.
};
// 실제 builder/목록/프레임과 현재 전역의 수명을 새 사본 없이 조립한다.
class RawCentralIsland {
public:
    RawCentralIsland(const SidPool& pool,ChunkMap& world,RawIslandRuntime& islands,std::span<const RiftTypeFrames> frames,std::uint32_t& puzzlePiece,CentralIslandGlobals globals,CentralIslandHooks hooks); // 모든 자료는 Build의 동기 호출보다 오래 살아야 한다.
    void Build(std::span<const std::uint8_t,6> record) const; // 값으로 전달된 실제 Zone6BYTE로 일반 중앙섬을 만들고 앞선 부분 효과를 보존한다.
private:
    const SidPool& pool_;ChunkMap& world_;RawIslandRuntime& islands_; // 동일 Pool/지도/원본100개 섬 목록이다.
    std::span<const RiftTypeFrames> frames_;std::uint32_t& puzzlePiece_; // 현재 타입 프레임 표와 실제 패턴 TYPE 전역이다.
    CentralIslandGlobals globals_;CentralIslandHooks hooks_; // 현재 전역 참조와 선택한 실제 부모 장치다.
};
}
