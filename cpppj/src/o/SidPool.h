// Squid.cpp의 SID 풀을 복원한다. 원본 번호에는 이 경로의 세대 비트가 없으며 월드 임시 번호와 별개다.
#pragma once
#include "o/RiftType.h"
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace netstorm::o {
class SquidFactory;
class SquidUnpop;
class SquidForms;
// 암묵적인 런타임 SquidId 변환을 막는 원본 슬롯 번호다.
struct Sid {
    // value: 읽기·쓰기·통지에 전달하는 값.
    std::uint16_t value{};
    // 원본 슬롯 번호끼리만 비교한다.
    bool operator==(const Sid&) const = default;
};
// 판본별 raw 슬롯 크기와 서버/예측 영역의 시작 번호다.
struct SidLayout { /* stride: 원본 객체 슬롯 하나의 바이트 크기. */ /* serverFirst: 서버 슬롯 영역의 첫 번호. */ /* predictableFirst: 예측 슬롯 영역의 첫 번호. */ std::uint32_t stride{},serverFirst{},predictableFirst{}; };
// 원본이 반납 직전에 보존하는 최근 삭제 기록 한 항목이다.
struct SidDeletion { /* sid: 원본 풀에서 객체를 찾는 SID 번호. */ /* type: 타입 표에서 객체 종류를 찾는 번호. */ std::uint32_t sid{},type{}; };

// 원본 SID 영역별 자유 슬롯과 예측 슬롯을 할당·반납하는 저장소이다.
class SidPool {
public:
    // 기존에 확보한 풀의 초기화 경로를 복원한다. CRT malloc의 포인터/실패 처리 자체는 복제하지 않는다.
    SidPool(OriginalEdition edition,std::uint32_t capacity,bool server=true);
    // 004abb10 ↔ CD 004aaad0: capacity>0이면 풀 크기를 다시 확보하고, 0/음수면 기존 풀을 초기화한다.
    // 삭제 기록은 유지한다. 호출자는 기존 프로세스/UI를 먼저 정리해야 하며 양수 재확보 뒤 빌린 span은 무효다.
    void Reset(std::int32_t capacity=0);
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
    // base 생성 시 서버 전용 owner/HP 초기화 여부를 반환한다.
    bool IsServer() const;
    // 타입 표와 같은 판본의 생성자/가상 배치를 사용하도록 판본을 반환한다.
    OriginalEdition Edition() const;
    std::uint32_t Capacity() const;
    std::uint32_t FreeCount() const;
    std::uint32_t PredictableCursor() const;
    Sid FirstFree(bool client) const;
    Sid Tail(bool client) const;
private:
    // Take는 free 슬롯도 받되 풀 목록/카운터를 고치지 않으므로 전용 어댑터만 raw 접근을 허용한다.
    friend class SquidFactory;
    // 지정 내장 Form의 Take는 슬롯 일부만 쓰고 목록/카운터를 유지한다.
    friend class SquidForms;
    // 공간 해제는 raw next와 void만 갱신하고 풀 목록/반납은 별도로 유지한다.
    friend class SquidUnpop;
    // 원본 건물 옆면은 유효 번호이면 free/비자산 후보의 WORD도 갱신한다.
    friend class RawBuildingSpace;
    // 원본 선택 가상+7c는 물리 범위 안 free/dead 슬롯의 extra에도 한 비트를 쓴다.
    friend class RawSquidSelection;
    // 운반 후보 평가는 물리 범위 안 free/dead 후보의 임시 거리 WORD+8도 갱신한다.
    friend class RawCarrierChoice;
    // 원본 short next 필드를 little endian으로 읽거나 쓴다. serverHead는 0번 슬롯+14의 겹친 객체다.
    std::uint16_t ReadNext(std::size_t offset) const;
    void WriteNext(std::size_t offset,std::uint16_t next);
    // 슬롯 주소 산술을 수행하기 전에 번호 범위를 검사한다.
    std::size_t Offset(Sid sid) const;
    // 반납 기록의 20항목 이동을 복원하며 마지막 항목은 버린다.
    void Record(Sid sid,std::uint8_t type);
    // layout_: 좌표 배치 계산 또는 판본별 필드 위치 정보.
    SidLayout layout_;
    // capacity_: 확보한 저장소의 최대 항목 수.
    // freeCount_: 반납 상태인 슬롯의 개수.
    // predictableCursor_: 다음 예측 슬롯 할당을 시작할 번호.
    std::uint32_t capacity_{},freeCount_{},predictableCursor_{};
    // server_: 서버 전용 초기화와 동작을 적용할지.
    bool server_{};
    // bytes_: 필드 접근이나 파일 읽기에 사용하는 원시 바이트.
    std::vector<std::uint8_t> bytes_;
    // tails_: 할당 영역별 자유 슬롯 연결 목록의 꼬리 번호.
    std::array<std::uint16_t,2> tails_{};
    // deletions_: 최근 삭제한 객체의 번호·타입 기록.
    std::array<std::array<SidDeletion,20>,2> deletions_{};
};
}
