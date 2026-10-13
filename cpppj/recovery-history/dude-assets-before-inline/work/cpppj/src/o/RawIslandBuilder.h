// 실제 영역 패턴을 현재 ChunkMap과3204BYTE 섬 목록에 원본 순서로 놓는다.
#pragma once
#include "o/RawIslandRuntime.h"
#include "o/ChunkMap.h"
#include "o/CanonDecoder.h"
#include <functional>

namespace netstorm::o {
// 실제 builder의 현재 청크 bounds와 원래 배치 소유자 필드다.
struct RawIslandBuilderState {
    ChunkRect bounds{DefaultChunkRect()}; // 최초 SetMap의(1,1)~(15,15), 실제 Place가 다시 읽는 청크 경계다.
    std::uint32_t owner{1}; // SetMap이 지정한 기본 owner이며 Runtime.Add에만 사용한다.
};
// 원본 helper의 보고 후 계속 진행을 보존하는 진단 종류다.
enum class RawIslandBuilderAssertion {
    InvalidBounds, // Place의 builder cRect 네 필드 중 하나가−1이다.
    EqualRange, // PlaceAll의 start==end이며 실제 순회는 없을 수 있다.
    OddDirection, // CD CanonDecoder constructor의 원본(rot&1)==0 진단이다.
    MissingFrame, // 원본 assert FindNumber가 해당 방향·변형·번호를 찾지 못한 뒤 -1을 반환한다.
    ChunkRange, // patch getter가XY 범위의combined 진단을 보고한뒤clamp한다. CD는 무진단이다.
    BoundsMissingIsland, // world Bounds helper의 실제 Exists 진단이다.
    BoundsNotFound // CD world Bounds가 일치 청크를 찾지 못한 found 진단이다.
};
// 같은 world ChunkMap 주소일 때만 실제 Runtime.Add/Bounds를 실행한다. 임시지도는 전달 지역을 유지한다.
class RawIslandBuilder {
public:
    // 원본 owner/bounds/프레임 전역과 target/world 지도는 이 객체보다 오래 살아야 한다.
    // null 지도는 호스트 참조로 표현하지 않으며 손상 포인터 메모리 실행은 지원하지 않는다.
    RawIslandBuilder(ChunkMap& target,const ChunkMap& world,RawIslandRuntime& islands,
        std::span<const RiftTypeFrames> frames,std::uint32_t& puzzlePieceType,RawIslandBuilderState& state,
        std::function<void(RawIslandBuilderAssertion)> diagnostic);
    // 0046dba0/CD00490750: record6BYTE·위치nibble·현재유효패턴·ordinal·같은32바이트 Bounds 기록이다.
    std::uint32_t Place(std::span<const std::uint8_t,6> record,std::uint32_t territory,bool ignorePosition=false) const;
    // 0046dd70/CD00490980: flag bit0/bit1을 현재 읽으며[start,end) 순서로 Place를 호출한다.
    // 호스트 자료는 실제20개 Territory의120BYTE를 포함해야 한다.
    void PlaceAll(std::span<const std::uint8_t> records,std::int32_t start=0,std::int32_t end=20) const;
    // 포인터 동일성만 판단하며 같은 크기의 지도 사본을 world로 해석하지 않는다.
    bool InWorld() const;
    // 같은 pool의 실제 Runtime 소비자임을 월드 바인딩에서 확인한다.
    const SidPool& Pool() const;
private:
    // 실제 ChunkMap getter의 범위 진단을 보고한 뒤 기존 clamp 몸체로 같은 저장소를 얻는다.
    ChunkEntry& ChunkAt(std::int32_t x,std::int32_t y) const;
    // target_/world_: 원본 this+0의 배치 지도와 실제 world 전역 지도다.
    ChunkMap& target_;
    const ChunkMap& world_;
    // islands_: count/Exists/current owner/원래 owner/Bounds를 실제3204BYTE에 기록한다.
    RawIslandRuntime& islands_;
    // frames_/puzzlePieceType_: 실제 decoder가 현재 읽는 타입 프레임 표와 타입 전역이다.
    std::span<const RiftTypeFrames> frames_;
    std::uint32_t& puzzlePieceType_;
    // state_: 현재 청크 범위/owner를 같은 builder 수명으로 보관한다.
    RawIslandBuilderState& state_;
    // diagnostic_: 원본 진단을 요청하며 빈 효과로 성공을 대신하지 않는다.
    std::function<void(RawIslandBuilderAssertion)> diagnostic_;
};
}
