// 신전51h 수신과 섬 상승 완료의 Territory/표면/객체 정리를 원문 순서로 실행한다.
#pragma once
#include "o/RawVortexAscend.h"
#include "o/RawIslandRuntime.h"
#include "o/ChunkMap.h"
#include <string_view>

namespace netstorm::o { // 같은 현재 섬/풀/지도와 실제 저장·진단 장치를 빌리는 원문 부모다.
struct VortexDepartureHooks { // 선택된 효과만 소비하며 누락을 정상 완료로 대신하지 않는다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // Vortex/Chunkmap의 실제 단언이며 정상 복귀 뒤 계속한다.
    std::function<void(Sid,std::uint16_t)> state; //004ac220의 상태WORD/특정TYPE 표면 통지를 실행한다.
    std::function<void(std::uint32_t,std::span<std::uint8_t,120>)> readTerritory; //004bd660은 부모가0으로 비운120BYTE를 읽는다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t,120>)> writeTerritory; //004bd600의 같은 슬롯/manager 저장이다.
    std::function<void()> refreshProduction; //004324e0이 현재 같은 섬 기록을 바꾼 뒤 호출한다.
    std::function<void()> snapshot; //004bfac0의 두 독립 현재모드/현재owner 저장이다.
    std::function<void(Sid,std::uint32_t)> owner; // 현재 신전의 가상+74를 owner0으로 호출한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 현재 파생 가상+10의 실제 삭제다.
    std::function<std::int32_t(float,float)> islandNumber; //0046f530의 현재 hash0/표면 지역WORD다. 직접 청크 조회와 구별한다.
    std::function<std::uint32_t(std::uint32_t)> ownedIslands; //0046f430의 현재owner 섬 수다.
    std::function<void()> requestEdit; // 마지막 자기 섬이 사라지면004b3560 전체 부모를 호출한다.
    std::function<void(std::string_view)> log; //29번 원문 로그의 실제 출력이다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> sendExcept; // 서버가 시작 패킷을 최초 송신자 외의 연결로 중계한다.
    std::function<void(Sid)> regular; // 실제 새 Regular event3/count0을 같은 Kernel에 붙인다.
    std::function<void(std::uint32_t)> warnAscend; //00484480→global.subject→WarnAscend를 실행한다.
    std::function<void(std::string_view)> sound; // 원문 전역 음향의 여섯 인자를 실제 장치에 전달한다.
};
class RawVortexDeparture { // PATCH004dc050/004dbce0와 두 표면 정리/추가 후보 판정을 함께 복원한다.
public:
    RawVortexDeparture(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        ChunkMap& chunks,RawIslandRuntime& islands,const RawVortexAscend& ascend,
        const std::uint32_t& localOwner,const bool& server,std::uint32_t& revision,VortexDepartureHooks hooks); // 모든 현재 입력은 같은 World 수명이다.
    void Receive(std::int32_t sender,std::span<const std::uint8_t> packet) const; //51h/8BYTE를 해석하고 시작·완료를 구별한다.
    void Remove(Sid temple,std::int32_t island) const; // 자신의 Territory와 무관한 원격 섬도 실제 공간/자산을 정리한다.
private:
    std::uint32_t Genus(Sid sid) const; // 매 소비 위치의 현재 TYPE.flags2를 읽는다.
    std::array<float,2> Position(Sid sid) const; // 같은 부모의 비정렬 float 두 개를 값으로 읽는다.
    std::int32_t IslandAt(float x,float y) const; // 원문 _ftol/16의 signed 청크 번호로 현재 지도를 읽는다.
    bool AdditionalCandidate(Sid sid,std::int32_t island) const; //004db750의 y-4/현재 원점/보호 genus 분기다.
    void Surfaces(std::int32_t island,bool neighbors) const; // 현재 표면을 찾고 네 이웃 또는 받침 자체를 원문처럼 삭제한다.
    SidPool& pool_;const SquidHash& hash_; // 같은 실제 슬롯과0단계 표면/일반 finder 지도다.
    std::span<const RiftTypeRecord> types_; // 재평가마다 현재 TYPE를 읽는 고정 물리다.
    ChunkMap& chunks_;RawIslandRuntime& islands_;const RawVortexAscend& ascend_; // 같은 현재 섬 기록/기존 후보 몸체다.
    const std::uint32_t& localOwner_;const bool& server_;std::uint32_t& revision_; // 효과 뒤 재조회하는 실제 원문 전역이다.
    VortexDepartureHooks hooks_; // 동기 원문 하위 장치를 소유하며 등록표나 Session을 별도로 소유하지 않는다.
};
}
