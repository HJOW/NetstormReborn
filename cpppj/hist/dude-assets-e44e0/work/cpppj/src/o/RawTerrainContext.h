// 실제 초기 지형 constructor의 지도·문자 대응·청크/칸 bounds와 도장 앞뒤 문자 변환을 보관한다.
#pragma once
#include "o/RawTerrainStamp.h"
#include "o/ChunkMap.h"

namespace netstorm::o {
// constructor가 읽는 원본 mutable 타입/모드 전역이다.
struct TerrainContextGlobals {
    // islandType/edgeType: x/=/X 문자의 초기 지면 타입 번호다.
    std::uint32_t islandType{97},edgeType{98};
    // noRegion: 초기 지역BYTE 배열을 채우는 현재 지역 없음 값이다.
    std::int32_t noRegion{127};
    // battle: fringe 제외flags0x10을 켜는 현재 전투 화면 상태다.
    bool battle{};
};
// 패치 bounds 저장 함수의 두 assertion 원인이다. CD는 같은 초과에서 보고 없이 청크 값만 줄인다.
enum class TerrainContextAssertion {
    // RightChunkLimit: 입력 right가 최대 청크16보다 크다.
    RightChunkLimit,
    // BottomChunkLimit: 현재 저장한 bottom이 최대 청크16보다 크다.
    BottomChunkLimit
};
// 지도/대응표는 이 객체가 소유하며 Map/가변 span의 수명은 이 객체와 같다.
class RawTerrainContext {
public:
    // 패치004c0390/CD0049cdd0의 실제 기본 배열·특수 mapping/alias와 bounds를 만든다.
    RawTerrainContext(OriginalEdition edition,std::uint32_t owner,std::int32_t region,ChunkRect bounds,
        const TerrainContextGlobals& globals,std::function<void(TerrainContextAssertion)> diagnostic);
    RawTerrainContext(const RawTerrainContext&)=delete;
    RawTerrainContext& operator=(const RawTerrainContext&)=delete;
    // 배열을 이동하면 빌린 Map span이 틀려지므로 이 초기 context는 이동하지 않는다.
    RawTerrainContext(RawTerrainContext&&)=delete;
    RawTerrainContext& operator=(RawTerrainContext&&)=delete;
    // 칸 bounds를 먼저16배 저장한 뒤 패치 진단/CD 조용한 청크 right/bottom 제한을 적용한다.
    void SetBounds(ChunkRect bounds);
    // 004c09a0/CD0049d890: 여덟 이웃의 bounds·alias·지역 중 하나라도 다르면 현재 code를 edge로 바꾼다.
    void MarkEdges(std::int32_t code,std::uint8_t edge);
    // 004c0ad0/CD0049d9a0: 도장 뒤 현재 =/o/X 문자를 x로 바꾸며 지역BYTE는 보존한다.
    void NormalizeAfterStamp();
    // 초기 지면 성장/초기 stamp가 동일한 배열/context를 사용하게 한다.
    TerrainStampMap& Map();
    // 원본 성장 몸체가 쓰는 X열 우선 문자/지역 배열이다. 렌더 행 자료는 별도로 전치한다.
    std::span<std::uint8_t> Codes();
    std::span<std::uint8_t> Regions();
    // 원본 초기화/편집 helper가 같은 문자 대응DWORD를 갱신할 때 사용한다.
    std::span<std::int32_t> Aliases();
    // 칸 bounds와 별도로 보존하는 제한 후 청크 범위다.
    ChunkRect Chunks() const;
    // 원본 context의 생성 요청 소유자/대상 지역이다. 후속 wrapper는 현재 값을 읽어야 한다.
    std::uint32_t Owner() const;
    std::int32_t Region() const;
private:
    // 손상 범위의 배열 밖 읽기를 정상 지도 접근으로 바꾸지 않는다.
    static std::size_t Index(std::int32_t x,std::int32_t y);
    // 원본 signed char mapping 주소에서 정상 ASCII 영역 밖은 명시적으로 거부한다.
    static std::size_t Character(std::uint8_t value);
    // edition_: 필드 폭과 패치/CD assertion 차이를 선택한다.
    OriginalEdition edition_;
    // owner_/region_: 생성 요청의 현재 플레이어·대상 지역이다.
    std::uint32_t owner_;
    std::int32_t region_;
    // chunks_: 제한 후 청크 범위이며 map_.bounds의 칸 범위와 합치지 않는다.
    ChunkRect chunks_{};
    // codes_/regions_: 원본 X열 우선 두65,536BYTE 지도다.
    std::array<std::uint8_t,65536> codes_{},regions_{};
    // mapping_/aliases_: 원본 문자별256DWORD의 현재 대응표다.
    std::array<std::int32_t,256> mapping_{},aliases_{};
    // map_: 위 실제 저장소를 초기 RawTerrainStamp와 공유하는 context다.
    TerrainStampMap map_;
    // diagnostic_: 패치가 bounds 오류를 보고한 뒤 계속 진행할 때 호출하는 필수 경계다.
    std::function<void(TerrainContextAssertion)> diagnostic_;
};
}
