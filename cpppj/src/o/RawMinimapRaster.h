// 원본004808d0의 지면/해시 순서와 팔레트 BYTE 쓰기를 같은 월드에 적용한다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidHash.h"

namespace netstorm::o {
// 10.78 미니맵 셀 그림이 매 호출에서 읽는 실제 전역이다.
struct MinimapRasterState {
    std::uint32_t isleType{97},noIslandType{157}; // 지면 WORD에서 색을 표시하는 두 실제 TYPE 번호다.
    std::uint32_t battleIslandType{109},playerIslandType{108}; // Challenge 모드의 두 2×2 표식 TYPE이다.
    std::uint32_t challenge{},suppressColors{},frame{}; // 현재 모드/색 억제 및 억제 시 색으로 쓰는 프레임 DWORD다.
    std::uint32_t black{},neutralIsle{},neutralNoIsland{},challengeGreen{}; // 원본 검색으로 얻은 검정/두 중립 지면/Challenge 초록이다.
    std::array<std::uint32_t,8> owners{}; // 소유자1..8의 미니맵 전용 색이며 일반 Player 색 표와 다르다.
};
// bitmap은 RawMetadisplayDirtyState의 같은128×128 저장소를 매 호출에 빌린다.
class RawMinimapRaster {
public:
    // 실제 Pool/Hash/TYPE와 전역은 처리기보다 오래 살아야 한다. 현재 복원 판본은10.78이다.
    RawMinimapRaster(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,const MinimapRasterState& state);
    //004808d0: 2×2 초기화→지면4×4→해시2/1/0을 원본의 X/Y 순서로 덮는다.
    void DrawCell(std::span<std::uint8_t> bitmap,std::int32_t cellX,std::int32_t cellY) const;
private:
    // 소유자 색은 원본의1..8 switch/default를 유지하며 억제 전역을 매번 조회한다.
    std::uint8_t OwnerColor(std::uint8_t owner,std::uint32_t fallback) const;
    // 좌표를 float→signed64 하위DWORD로 절삭하며 bitmap 밖 원본 메모리 손상은 거부한다.
    static std::int32_t Coordinate(std::span<const std::uint8_t> raw,std::size_t at);
    // 단일 픽셀은 BYTE만 쓰고 현재 bitmap의 실제 폭128을 사용한다.
    static void Pixel(std::span<std::uint8_t> bitmap,std::int32_t x,std::int32_t y,std::uint8_t color);
    // 현재 체인 next를 각 효과 뒤에 읽으며 원본의 특수 모드/표면 플래그를 따른다.
    void Chain(std::span<std::uint8_t> bitmap,std::uint16_t head,bool challengeMarks) const;
    const SidPool& pool_; // 실제50BYTE 슬롯의 현재 type/next/owner/좌표다.
    const SquidHash& hash_; // 지면 WORD와 세 단계 버킷은 동일 월드의 공간 저장소다.
    std::span<const RiftTypeRecord> types_; //004ac200이 반환하는 현재TYPE flags2다.
    const MinimapRasterState& state_; // 복사하지 않는 현재 색/모드/프레임 전역이다.
};
}
