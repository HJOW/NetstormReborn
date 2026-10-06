// Graph.cpp의 정수 표면 연결·flood·병합과 기존 그래프 표 관리 경로다.
#pragma once
#include "o/SquidFinder.h"
#include <map>

namespace netstorm::o {
// 원본 그래프 레코드의 6바이트 WORD 세 개다. 세 번째 WORD는 이 경로에서 변경하지 않는다.
struct GraphRecord { std::int16_t surfaces{},inUse{},reserved{}; };
// 실제 raw 슬롯의 그래프 byte와 state를 표면 스냅샷 번호에 연결한 입력이다.
struct GraphMembership { std::uint16_t id{}; std::uint8_t graph{254},state{}; };
class Graph {
public:
    // 원본 정상 그래프 수·무효 번호·전체 표 폭·flood 스택 크기다.
    static constexpr std::size_t kCount=251,kTableSize=255,kFloodSize=4096;
    static constexpr std::uint8_t kInvalid=254;
    // 표면 탐색기 수명은 Graph보다 길어야 한다. 기존 표를 생략하면 빈 표와 원본 sentinel을 만든다.
    Graph(const SurfaceFinder& surfaces,std::span<const GraphMembership> members,std::span<const GraphRecord> records={});
    // 00463330 ↔ CD 0045c390. 첫 미사용 번호를 확보한다. 소진 복구는 쓰기 전에 거부한다.
    std::uint8_t Allocate();
    // 00462e50 ↔ CD 0045c460. 254는 그대로 두고 유효 레코드의 앞 두 WORD만 지운다.
    void Free(std::uint8_t graph);
    // 00462a50 ↔ CD 0045b530. 활성/양수일 때 감소하고 0이 되면 미사용으로 전환한다.
    void Remove(std::uint8_t graph);
    // 00462f10 ↔ CD 0045bb90. 원본 LIFO 순서·중복 push·WORD 감김·변경 수를 유지한다.
    std::uint32_t Flood(std::uint16_t id,std::uint8_t graph);
    // 004633c0 ↔ CD 0045b7d0. 가장 큰 이웃 그래프에 붙이고 나머지를 flood로 병합한다.
    void Add(std::uint16_t id);
    // 레코드/전체 스택/번호는 읽기 전용으로 제공한다. 스택의 미사용 흔적도 보존한다.
    std::span<const GraphRecord> Records() const;
    // 원본 LIFO 스택의 사용 후 흔적도 검사할 수 있게 전체를 읽는다.
    std::span<const std::uint32_t> FloodStack() const;
    // 표면 객체의 현재 그래프 번호를 읽는다.
    std::uint8_t Number(std::uint16_t id) const;
    // 그래프 연산이 보존한 raw 상태 입력을 읽는다.
    std::uint8_t State(std::uint16_t id) const;
    // 앞 251개 레코드에서 inUse가 0이 아닌 개수를 센다.
    std::size_t InUse() const;
private:
    // 호스트 메모리 산술 대신 입력 번호를 검증한다.
    static void CheckNumber(std::uint8_t graph,bool allowInvalid);
    // 손상된 연결·과도한 스택 입력이 있으면 복사본을 버리기 위한 내부 처리다.
    std::uint32_t FloodImpl(std::uint16_t id,std::uint8_t graph);
    // 원본 연결 목록·최대 크기 선택·병합의 내부 처리다.
    void AddImpl(std::uint16_t id);
    const SurfaceFinder& surfaces_;
    std::array<GraphRecord,kTableSize> records_{};
    std::array<std::uint32_t,kFloodSize> stack_{};
    std::map<std::uint16_t,GraphMembership> members_;
};
}
