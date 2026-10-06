// Squid.cpp의 SID 풀을 복원한다. 원본 번호에는 이 경로의 세대 비트가 없으며 월드 임시 번호와 별개다.
#pragma once
#include "o/RiftType.h"
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace netstorm::o {
// 암묵적인 런타임 SquidId 변환을 막는 원본 슬롯 번호다.
struct Sid {
    std::uint16_t value{};
    // 원본 슬롯 번호끼리만 비교한다.
    bool operator==(const Sid&) const = default;
};
// 판본별 raw 슬롯 크기와 서버/예측 영역의 시작 번호다.
struct SidLayout { std::uint32_t stride{},serverFirst{},predictableFirst{}; };
// 원본이 반납 직전에 보존하는 최근 삭제 기록 한 항목이다.
struct SidDeletion { std::uint32_t sid{},type{}; };

class SidPool {
public:
    // 기존에 확보한 풀의 초기화 경로를 복원한다. CRT malloc의 포인터/실패 처리 자체는 복제하지 않는다.
    SidPool(OriginalEdition edition,std::uint32_t capacity,bool server=true);
    // 004abb10 ↔ CD 004aaad0의 기존 풀 초기화다. 삭제 기록은 별도 계층이므로 유지한다.
    void Reset();
    // 004af1d0 ↔ CD 004aae40. bit 1=예측, bit 2=클라이언트이며 예측이 우선한다.
    Sid Allocate(std::uint32_t flags=0);
    // 004abf60 ↔ CD 004ab250. void 객체만 반납하며 raw 슬롯을 지운 뒤 이전 타입 번호를 남긴다.
    void Release(Sid sid);
    // 004abe00 ↔ CD 004aad30. free 서버 슬롯을 번호순으로 다시 잇고 기존 freeCount에 더한다.
    void RebuildServer();
    // 파생 생성자의 후속 연결 지점이다. 이미 할당한 슬롯에만 raw 필드를 쓸 수 있다.
    std::span<std::uint8_t> AllocatedBytes(Sid sid);
    // 슬롯·풀·삭제 기록을 읽기 전용으로 제공한다. 32비트 원본 포인터를 호스트 포인터로 해석하지 않는다.
    std::span<const std::uint8_t> Slot(Sid sid) const;
    std::span<const std::uint8_t> Bytes() const;
    std::span<const SidDeletion> Deletions(bool client) const;
    // 검증과 후속 어댑터에 필요한 판본별 경계·카운터·머리/꼬리를 반환한다.
    SidLayout Layout() const;
    std::uint32_t Capacity() const;
    std::uint32_t FreeCount() const;
    std::uint32_t PredictableCursor() const;
    Sid FirstFree(bool client) const;
    Sid Tail(bool client) const;
private:
    // 원본 short next 필드를 little endian으로 읽거나 쓴다. serverHead는 0번 슬롯+14의 겹친 객체다.
    std::uint16_t ReadNext(std::size_t offset) const;
    void WriteNext(std::size_t offset,std::uint16_t next);
    // 슬롯 주소 산술을 수행하기 전에 번호 범위를 검사한다.
    std::size_t Offset(Sid sid) const;
    // 반납 기록의 20항목 이동을 복원하며 마지막 항목은 버린다.
    void Record(Sid sid,std::uint8_t type);
    SidLayout layout_;
    std::uint32_t capacity_{},freeCount_{},predictableCursor_{};
    bool server_{};
    std::vector<std::uint8_t> bytes_;
    std::array<std::uint16_t,2> tails_{};
    std::array<std::array<SidDeletion,20>,2> deletions_{};
};
}
