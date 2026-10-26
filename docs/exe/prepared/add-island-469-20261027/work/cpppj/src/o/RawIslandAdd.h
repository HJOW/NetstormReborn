// 편집기의 실제 한 섬 배치를 지형·population·Graph·Territory 저장까지 조립한다.
#pragma once
#include "o/RawIslandPlacement.h"
#include "o/RawIslandPopulation.h"

namespace netstorm::o { // UI drag 수명과 실제 월드 변경 부모를 구별한다.
struct IslandAddHooks {
    std::function<void()> fortAssertion; //0046e0d0의Fort모드진단뒤본문을계속한다.
    std::function<void(const RawIslandBuilderState&,std::int32_t)> generate; //같은현재Builder/지역으로실제0046da70/정상지형본문을실행한다.
    std::function<void(bool)> graph; //00463110의실제전체Graph재구성이며false를그대로전달한다.
    std::function<void(std::uint32_t)> territory; //Builder의현재owner를사용한004bf6d0실제전체저장이다.
};
class RawIslandAdd {
public:
    RawIslandAdd(ChunkMap& world,RawIslandRuntime& islands,std::span<const RiftTypeFrames> frames,std::uint32_t& puzzlePieceType,RawIslandBuilderState& builderState,const bool& fort,RawIslandPopulation& population,std::function<void(RawIslandBuilderAssertion)> assertion,IslandAddHooks hooks); //실제단일World/Pool/builder참조만빌린다.
    std::uint32_t Add(std::span<const std::uint8_t,6> record,std::uint32_t territory) const; //0046e0d0의원문인자를직접받고성공/실패부분효과를그대로보존한다.
    bool TryDrop(const ChunkMap& preview,std::span<std::uint8_t,6> record) const; //0047fb90의CanPlace→flag&fd→실제SetMap의bounds/owner1→현재count→Add까지실행한다.좌표와drag수명은UI가소유한다.
private:
    ChunkMap& world_;RawIslandRuntime& islands_;std::span<const RiftTypeFrames> frames_;std::uint32_t& piece_;RawIslandBuilderState& state_;const bool& fort_;RawIslandPopulation& population_; //실제현재지도·지역·TYPE·owner의같은주소다.
    std::function<void(RawIslandBuilderAssertion)> assertion_;IslandAddHooks hooks_; //기존완료Builder진단과실제큰하위장치를연결한다.
};
}
