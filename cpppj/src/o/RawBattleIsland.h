// 일반 전투 참가자의 현재 Territory 한 개를 실제 월드에 배치하는 원문 부모다.
#pragma once
#include "o/RawFortWorldLoad.h"

namespace netstorm::o { // 파일 수신/State 전환은 소유하지 않고 같은 섬/지형/객체 부모만 호출한다.
struct BattleIslandHooks {
    std::function<void(std::uint32_t)> assertion; // owner0의 Islandbuilder.cpp:0x10f 진단이며 복귀하면 계속한다.
    std::function<void(std::uint32_t,std::span<std::uint8_t,120>)> territory; //004bd660의 현재 Template Territory 읽기다.
    std::function<ChunkRect(std::uint32_t,std::span<const std::uint8_t,6>)> placement; //00432050의 현재 Player별 실제 배치 범위다.
    std::function<void(std::uint32_t,std::uint32_t)> region; // 현재 Player+78에 배치된 섬 번호를 쓴다.
    std::function<void(const RawIslandBuilderState&,std::int32_t)> generate; //0046da70의 현재 bounds/owner/섬 지형 생성이다.
    std::function<bool()> boss; // 지형 생성 뒤 현재00540bc4를 다시 읽는다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> objects; // 권한이 있을 때만 현재 Template의 선택 Terr 범위를 읽어 실제 객체를 만든다.
    std::function<void(bool)> graph; // 마지막00463110(0)의 실제 전체 Graph 재구성이다.
    std::function<void(RawIslandBuilderAssertion)> builderAssertion; // 기존 RawIslandBuilder가 선택한 진단이다.
    const CanonDecoderDiagnostics* decoderDiagnostics{}; // 같은 실제 Client Canon 진단 수명을 빌린다.
};
// PATCH0046d9b0→0046da60→0046e2c0의 생성/선택/현재 필드 쓰기 순서를 복원한다.
class RawBattleIsland {
public:
    RawBattleIsland(const SidPool& pool,ChunkMap& world,RawIslandRuntime& islands,std::span<const RiftTypeFrames> frames,std::uint32_t& puzzlePiece,BattleIslandHooks hooks); // 동일 Pool/월드/타입/장치는 Run이 끝날 때까지 살아 있어야 한다.
    void Build(std::uint32_t owner) const; // 첫 flag4 영역 또는0번을 배치하고 권한별 객체 적재/Graph0을 수행한다. 실패 이전 효과를 되돌리지 않는다.
private:
    const SidPool& pool_; // 기존 런타임과 같은 판본/물리 Pool이다.
    ChunkMap& world_; // builder의 SetWorld가 선택하는 실제 전역 청크 지도다.
    RawIslandRuntime& islands_; // 같은 현재 섬 목록의 Add/Bounds를 사용한다.
    std::span<const RiftTypeFrames> frames_; // 현재 puzzlePiece가 조회하는 실제 타입 프레임 표다.
    std::uint32_t& puzzlePiece_; // 패턴 배치마다 다시 읽는 같은 Client 전역이다.
    BattleIslandHooks hooks_; // 현재 Template/Player/지형/객체/Graph의 실제 동기 장치다.
};
}
