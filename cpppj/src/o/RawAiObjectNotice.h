// 원본004166e0/00414de0의 같은 Player→AI 생성·삭제 목록 갱신이다.
#pragma once
#include "o/RawAiBrain.h"

namespace netstorm::o {
// 원본의 세 로그 요청을 실제 표시 장치에 전달하는 종류다.
enum class AiObjectLogKind { Built,MadeGeyserIneligible,CannotExcludeGeyser }; // 건설/간헐천 제외 성공/제외 후보 없음의 원문 분기다.
// 게임 계산을 끝낸 뒤 원본 로그 포맷에 필요한 현재 값을 전달한다.
struct AiObjectLog {
    AiObjectLogKind kind{}; // 출력할 원문 종류다.
    std::uint32_t object{},type{}; // 생성/삭제 자산 SID 또는 제외한 간헐천 번호와 현재 타입이다.
    std::int32_t x{},y{}; // Built 로그의 원본 signed 좌표 절삭이다.
};
// 같은 경로/발 기준점/로그 장치를 선택된 위치에서만 조회한다.
struct AiObjectNoticeHooks {
    std::function<std::array<std::uint32_t,2>(Sid)> carrierTargets; //00426150의 현재 이동 프로세스 두 대상이며 없으면0/0이다.
    std::function<double(Sid,std::uint32_t)> distanceSquared; //00411120의 두 현재 발 중심 간 거리 제곱이다.
    std::function<void(std::uint32_t,const AiObjectLog&)> log; //004105b0의 형식화/표시 경계다. 게임 타이머는 본문이 갱신한다.
};
// 실제0xb08/17목록과 Player9를 공유하며 부착 bool 사본을 만들지 않는다.
class RawAiObjectNotice {
public:
    RawAiObjectNotice(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const std::uint8_t> players,const double& now,const std::uint32_t& outpostType,
        std::function<bool()> disabled,AiObjectNoticeHooks hooks); //PATCH188/현재00594fc8 및00541250을 빌린다.
    void Notice(std::uint32_t owner,Sid sid,bool added) const; //004166e0의 현재 Player+b0 비영일 때만 본문을 호출한다.
    void Update(std::uint32_t token,Sid sid,bool added) const; //00414de0의 타입 수·목록·목표·타이머를 원본 순서로 갱신한다.
    const SidPool& Pool() const; // 공통 Pop/삭제와 같은 풀인지 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; // 실제 크기의 살아 있는 본체만 빌린다.
    std::int32_t Count(std::uint32_t token,std::size_t offset) const; // 현재 목록의 signed count다.
    std::uint32_t Item(std::uint32_t token,std::size_t offset,std::int32_t index) const; // 현재 포인터/물리 범위를 읽는다.
    bool Contains(std::uint32_t token,std::size_t offset,std::uint32_t value) const; // 첫 중복에서 자연 반환한다.
    void Append(std::uint32_t token,std::size_t offset,std::uint32_t value) const; // signed 용량이 남을 때만 추가한다. 중복을 제거하지 않는다.
    void AppendUnique(std::uint32_t token,std::size_t offset,std::uint32_t value) const; // full이면 검색도 생략하는00414450이다.
    void Remove(std::uint32_t token,std::size_t offset,std::uint32_t value) const; //0040ea00의 모든 중복 압축과 inactive 꼬리 보존이다.
    void ExcludeGeyser(std::uint32_t token,std::uint32_t geyser) const; //00410f80의 능력bit3/후보 목록/로그 순서다.
    void Built(std::uint32_t token,Sid sid) const; //00410620의 두 QWORD 기한과 원본 로그 인수를 준비한다.
    RawAiBrainMemory& memory_; // 생성자/피해/전략과 같은 실제 본체·목록 소유자다.
    const SidPool& pool_; // 현재 SID 타입/좌표를 읽는 실제 풀이다.
    std::span<const RiftTypeRecord> types_; // 같은 현재188TYPE의 genus다.
    std::span<const std::uint8_t> players_; // 같은9×180BYTE의+b0를 호출마다 읽는다.
    const double& now_; //0055b4d0의 현재 게임 시각이다.
    const std::uint32_t& outpostType_; //00541250의 현재 분류 제외 번호다.
    std::function<bool()> disabled_; //00594fc8의 현재 값이며 Player 조회 전에 읽는다.
    AiObjectNoticeHooks hooks_; // 선택한 이동/거리/표시의 실제 하위다.
};
}
