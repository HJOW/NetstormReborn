// 섬 배치 후의 거주 구역·초기 객체 생성과 record 후처리를 같은 실제 월드에 연결한다.
#pragma once
#include "o/RawIslandBuilder.h"
#include "o/RawFortFinishOwner.h"
#include "o/RawFortTerritorySave.h"
#include <array>

namespace netstorm::o { // 10.78 Islandbuilder/Addrandom의 실제 부모를 World UI와 구별한다.
enum class IslandPopulationAssertion { BoundsMissing,RecordMissing,ResidenceAuthority,PopulationAuthority,MissingPlacement }; //원문 c2/139/c3/d1/118 보고 뒤 현재 상태로 계속한다.
struct IslandPopulationGlobals {
    const bool& boss;const bool& fort; //원문00540bc4/00594fb8의 현재 장기 주소다.
    const std::uint32_t& residence;const std::uint32_t& primary;const std::uint32_t& fallback;const std::uint32_t& replacement; //5411bc/541174/54126c/5412d0의 현재 TYPE다.
};
struct IslandPopulationHooks {
    std::function<void(IslandPopulationAssertion)> assertion; //같은 실제 보고 장치이며 callback이 바꾼 현재 권한도 다시 읽는다.
    std::function<double()> wallSeconds; //Fort 모드004bcab0의 실제 현재 시각이다.
    std::function<std::array<float,2>(Sid)> position; //004adea0의 현재 TYPE 발자국/2를 뺀 중심을 삭제 전에 얻는다. 저장 raw 좌표와 구별한다.
    std::function<void(Sid,std::uint32_t)> destroy; //가상+10의 전체 삭제이며 위치 snapshot 뒤에 실행한다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //004af530의 실제 생성/초기화다.
    std::function<std::int32_t(std::uint32_t)> defaultFrame,typeHitPoints; //현재 TYPE+118과0049fce0이며 HP 유무 진단과 구별한다.
    std::function<bool(Sid)> hasHitPoints; //004adc60의 현재 실제 객체/TYPE 판정이다.
    std::function<void(Sid,std::uint32_t)> setOwner; //가상+74이며 실제 지역의 현재 owner를 전달한다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; //가상+90의 실제 공간·표시·postPop이다.
    std::function<void()> refreshProduction; //004324e0의 모든 record 쓰기 뒤0047d890이다.
};
class RawIslandPopulation {
public:
    RawIslandPopulation(SidPool& pool,ChunkMap& target,ChunkMap& world,const RawIslandRuntime& islands,const RawFortFinishOwner& randomPlacement,IslandPopulationGlobals globals,IslandPopulationHooks hooks); //모든 자료가 같은 실제 Pool이며 더 오래 살아야 한다.
    void Residence(std::int32_t region) const; //0046df80→현재 world record seed→0040e130→기존 실제0040ded0이다. 전달 record 인자는 원문에서 읽지 않는다.
    void Populate(std::int32_t region) const; //0046e000의 사각형/record snapshot→Residence→교체 객체→bit0x800/모든 청크/생산 통지다.
    void ReplaceInitial(const FortRandomRectangle& area,std::int32_t region) const; //0040e2b0의 중앙4×4 두 종류 최대10회/선택 객체 교체다.
    const SidPool& Pool() const; //상위 Builder/World와 같은 실제 저장소만 조립한다.
private:
    FortRandomRectangle Bounds(std::int32_t region) const; //현재 target의 실제 bounds와 원문16배 DWORD 감김이다.
    void ResidenceIn(const FortRandomRectangle& area,std::int32_t region,std::uint32_t quantity) const; //0040e130은 실제 world record의BYTE3×103 로컬 난수를 쓴다.
    void WriteRecord(std::int32_t region,std::span<const std::uint8_t,6> record) const; //004324e0의 y/x 전체 순회와 마지막 생산 통지를 실행한다.
    Sid Candidate(std::uint32_t value) const; //원문 반환값을 조용히 성공 SID로 바꾸지 않고 실제 WORD/Pool 물리 경계만 확인한다.
    SidPool& pool_;ChunkMap& target_;ChunkMap& world_;const RawIslandRuntime& islands_;const RawFortFinishOwner& placement_; //실제 현재 메모리와 기존 완료 단일 배치 몸체다.
    IslandPopulationGlobals globals_;IslandPopulationHooks hooks_; //현재 전역 참조와 선택된 실제 장치만 빌린다.
};
}
