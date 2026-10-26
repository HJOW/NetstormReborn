// 원본 지형 미리보기의8비트 bitmap과 최초 문자 분류 캐시를 복원한다.
#pragma once
#include "o/SpStore.h"
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace netstorm::o { // 지형 생성/월드 객체와 분리한 원본004c1860 출력이다.
// Native 반환 triple의 포인터를 소유 벡터로 나타낸다. 별도 hotspot이나 팔레트 변환을 넣지 않는다.
struct TerrainBitmapImage {
    std::int32_t width{},height{}; // 원문 signed 출력 폭/높이다.
    std::vector<std::uint8_t> pixels; // 행 우선8비트 원본 색 번호이며 투명 판정은 Draw 부모가 담당한다.
};
// 첫 요청의 녹색 분류 값은 팔레트/Session 교체 뒤에도 같은 Client 수명으로 유지한다.
struct TerrainBitmapState {
    std::uint32_t initialized{}; //005c8bb8의 최초 BSS0/정상 초기화1이다.
    std::array<std::uint8_t,255> classified{}; //005c8ab0의 원문255BYTE clear 범위다. code'x'만 최초 녹색 하위BYTE로 채운다.
    std::uint32_t fringeType{102}; //005411f0의 현재 TYPE이며 지면5411dc와 별개다.
};
// 실제 SHP0을200×200/255 초기 버퍼에 그린 현재 텍스처다.
struct TerrainBitmapTexture {
    std::int32_t width{},height{}; // 원본 Squid 헤더의 폭/높이이며 반복 주기는 각각−1이다.
    std::array<std::uint8_t,40000> pixels; //004a2b10의200 stride 임시 표면과 같다.
};
// 타입/SHP/현재 팔레트 조회만 외부에서 공급한다. 난수와 래스터는 같은 원본 몸체다.
struct TerrainBitmapHooks {
    std::function<std::uint32_t()> green; // 최초 분류 때만005acd20의 현재 색 번호를 읽는다.
    std::function<TerrainBitmapTexture(std::uint32_t)> texture; // 실제 현재 TYPE0/RLE 결과를 빌려온다.
};
// 원문004c1860의 수직 fringe→지면 texture 순서와 로컬 난수 소비를 유지한다.
class RawTerrainBitmap {
public:
    RawTerrainBitmap(TerrainBitmapState& state,const std::uint32_t& islandType,TerrainBitmapHooks hooks); // 저장소/현재 타입은 본체보다 오래 살아야 한다.
    TerrainBitmapImage Render(std::span<const std::uint8_t,65536> codes,std::int32_t width,std::int32_t height,GameRandom& random) const; // 원본 X열 우선 문자를 행 우선 전체 픽셀로 바꾼다.
private:
    TerrainBitmapState& state_;const std::uint32_t& islandType_; // Client 최초 분류와 현재 지면 TYPE의 같은 주소다.
    TerrainBitmapHooks hooks_; // 선택되는 실제 조회 장치만 소유한다.
};
}
