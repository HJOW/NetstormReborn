// 원본 Territory snapshot과 현재 섬 검색/빈 지역 seed를 같은 월드에 연결한다.
#pragma once
#include "o/ChunkMap.h"
#include "o/RawMemStream.h"

namespace netstorm::o {
// 저장 부모가 실제 선택하는 두 진단 경계다. 복귀 뒤 현재 필드를 다시 읽는다.
enum class FortTerritorySaveAssertion { MissingManager,WriteCapacity };
// 기존 자료 읽기와 현재 월드 검색만 외부에 맡기며 저장 분기/순서는 본체가 수행한다.
struct FortTerritorySaveHooks {
    std::function<void(std::int32_t,std::span<std::uint8_t,120>)> load; //004bd660: 기존 Territory를 로컬120BYTE에 먼저 읽는다.
    std::function<std::uint32_t()> special; //005c85a4는 getter가 끝난 뒤 한 번 읽어 일반/Editor를 고른다.
    std::function<std::int32_t(std::int32_t,std::int32_t)> island; //0046f380: clamp한 owner와 지역0..19를 조회한다.
    std::function<std::int32_t(std::int32_t)> anyIsland; //0046f3f0: Editor의 현재 지역을 owner 조건 없이 조회한다.
    std::function<std::int32_t()> noRegion; // 각 검색 결과 뒤 현재0050c038을 다시 읽는다.
    std::function<std::array<std::uint8_t,6>(std::int32_t)> record; //00432430: 같은 현재 ChunkMap의 첫 일치 record를 값으로 받는다.
    std::function<void(std::span<std::uint8_t,6>,std::uint32_t)> seed; //004b37c0: 기존 기록 일부만 누락 순번/공유 난수로 바꾼다.
    std::function<std::uint32_t(std::int32_t)> manager; // 같은 고정 Template 슬롯의 현재 manager다.
    std::function<void(std::uint32_t)> create; // 새 Territory를 만들고 반환 stream은 사용하지 않는다.
    std::function<std::uint32_t(std::uint32_t)> current; // 생성 뒤 다시 읽은 manager+8의 현재 stream을 고정한다.
    std::function<void(FortTerritorySaveAssertion)> assertion; // 원본 Template521/MemStream36행 경계다.
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; // 고정 stream의 실제16BYTE/버퍼를 동기적으로 빌린다.
};
//004bf6d0: 기존 기록→현재 지도/빈 seed→현재 stream 재조회→120BYTE 쓰기 순서다.
void SnapshotFortTerritory(std::int32_t slot,const FortTerritorySaveHooks& hooks);
//0046f380: 일반은 originalOwner와 currentOwner, Fort는 currentOwner0 또는 요청owner를 허용한다.
std::int32_t FindFortTerritoryIsland(std::span<const std::uint8_t> islands,bool fort,std::int32_t owner,std::int32_t region);
//0046f3f0: 존재하는 첫 signed region BYTE 일치 항목을 반환한다.
std::int32_t FindAnyFortTerritoryIsland(std::span<const std::uint8_t> islands,std::int32_t region);
//00432430: y 바깥/x 안쪽 순서로 같은 섬의 첫 record6BYTE를 복사하고 없으면0을 돌려준다.
std::array<std::uint8_t,6> FortTerritoryIslandRecord(const ChunkMap& chunks,std::int32_t island);
//004b37c0: 첫 두 BYTE를 먼저 바꾸고 Next(254)가 복귀한 뒤 byte3만 쓴다.
void SeedFortTerritoryRecord(std::span<std::uint8_t,6> record,std::uint32_t index,const std::function<std::uint32_t(std::uint32_t)>& random);
}
