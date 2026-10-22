// 실제 AI 다리 탐색의 나선24칸과 연결점 후보6개를 같은 현재 월드에서 준비한다.
#pragma once
#include "o/RawAiBrain.h"

namespace netstorm::o {
// 원본 getter가 읽는 현재 TYPE 전역이며 번호를 별도 사본으로 고정하지 않는다.
struct AiBridgeCandidateTypes {
    const std::uint32_t& bridge; //005411a0의 전체 DWORD로 다리 여부를 비교한다.
    const std::uint32_t& fencemark; //005411c0의 낮은 BYTE는 graph getter의 예외 타입이다.
    const std::uint32_t& windArcher; //00541288의 낮은 BYTE는 graph getter의 두 번째 예외 타입이다.
};
//0040fe90의 현재 상태/실제 hash0/spot/프레임 코드만 읽고 후보 상태를 갱신한다.
class RawAiBridgeCandidates {
public:
    RawAiBridgeCandidates(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,std::span<const std::uint16_t> surfaces,std::span<const std::uint8_t> spots,AiBridgeCandidateTypes globals); //같은 PATCH188/65536 지도와 원본 본체를 빌린다.
    bool Prepare(std::uint32_t token) const; //한 호출24칸 후 false, 경계/나선24/단계8부터후보3개 조건에서 true이며 최대6개만 저장한다.
    const SidPool& Pool() const; //실제 Session 조립 시 다른 월드가 섞이지 않게 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재 살아 있는0xb08 본체만 조회한다.
    std::uint32_t Field(std::uint32_t token,std::uint32_t offset) const; //현재 비정렬 DWORD를 읽는다.
    void Field(std::uint32_t token,std::uint32_t offset,std::uint32_t value) const; //지정 네 BYTE만 쓰며 물리 본체 밖을 거부한다.
    std::uint8_t Spot(std::int32_t x,std::int32_t y) const; //원본 flat 주소의 행 경계 통과를 보존하며65536BYTE 밖은 거부한다.
    std::uint32_t Graph(Sid sid) const; //실제4ad390의 타입 조건과 raw graph BYTE를 읽는다.
    std::uint32_t Connections(Sid sid) const; //실제4ad680의 현재 코드와 기존A~P 원본 연결표를 사용한다.
    RawAiBrainMemory& memory_; //같은 실제 Brain/목록 소유 메모리다.
    const SidPool& pool_; //실제50BYTE surface 슬롯을 읽는다.
    std::span<const RiftTypeRecord> types_; //현재 graph getter의 surface flag 표다.
    std::span<const RiftTypeFrames> frames_; //현재 raw프레임의 방향 코드를 읽는 실제 표다.
    std::span<const std::uint16_t> surfaces_; //같은 hash0의65536 WORD다.
    std::span<const std::uint8_t> spots_; //같은 현재 점유65536 BYTE다.
    AiBridgeCandidateTypes globals_; //원본 전역의 현재 참조다.
};
}
