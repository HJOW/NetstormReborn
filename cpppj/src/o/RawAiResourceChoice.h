// AI의 간헐천 선택·신전 귀환과 포획 운반자/끊긴 간헐천 목록을 같은 본체에서 처리한다.
#pragma once
#include "o/RawAiMovement.h"

namespace netstorm::o {
// 이미 복원된 실제 중심/그래프/내용물/이동과 원본 로그·진단을 빌린다.
struct AiResourceChoiceHooks {
    std::function<RawPathPoint(Sid)> center; //004adea0의 현재 신전 발 중심이다.
    std::function<std::uint32_t(Sid)> graph; //00462bc0의 실제 현재 그래프이며 SID0 필터는 caller 위치에서만 적용한다.
    std::function<std::int32_t(Sid)> crystals; //004acc60(actor,20h,현재5412c0)의 실제 내용물 총수다.
    std::function<void(std::uint32_t,Sid,Sid)> target; //004116d0(token,actor,target,0)의 같은 실제 이동이다.
    std::function<void(std::uint32_t,Sid)> home; //004118a0의 같은 실제 신전 복귀다.
    std::function<void(std::uint32_t,Sid)> captureLog; //004105b0(token,29,PriestGetter)의 현재 장치 로그다.
    std::function<void(std::uint32_t,Sid)> disconnectedLog; //현재532f3e4 종류로 끊긴 간헐천을 기록한다.
    std::function<void(std::string_view)> assertion; //원본의 유효 간헐천 진단이다. 반환하면 남은 흐름을 계속한다.
};
// 별도 공략/대상 복사본 없이 현재330/348/384 목록과 실제 월드를 사용한다.
class RawAiResourceChoice {
public:
    RawAiResourceChoice(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::function<std::uint32_t()> invalidGraph,AiResourceChoiceHooks hooks); //같은 PATCH/188TYPE/현재 무효 그래프를 빌린다.
    RawPathPoint HomeCenter(std::uint32_t token) const; //004112b0:현재 신전 중심 또는 본체268/26c 원문이다.
    Sid Select(std::uint32_t token,std::size_t list,RawPathPoint point,std::uint32_t excludedGraph,std::uint32_t requiredGraph) const; //00410a30:거리제곱 signed절삭→그래프→엄격한 최소값/제외·필수 그래프다.
    void CollectOrHome(std::uint32_t token,Sid actor) const; //00411900:현재 신전/유효 그래프와 크리스털/수집능력에 따라 실제 수집 또는 귀환한다.
    bool HasFlyingCaptureCarrier(std::uint32_t token) const; //00410980:현재384 목록의 비행 genus만 검사한다.
    void RememberCaptureCarrier(std::uint32_t token,Sid actor) const; //004109d0:중복이면 반환,full이면 추가만 생략하고 현재 로그를 처리한다.기한은 바꾸지 않는다.
    void ReclassifyDisconnected(std::uint32_t token) const; //00411350:현재330의 다른 그래프 간헐천을 모두 제거하고348에 순서대로 옮긴다.
    const SidPool& Pool() const; //같은 실제 월드 연결을 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재 살아 있는 실제0xb08 본체만 허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //지정 DWORD의 현재값을 읽는다.
    std::int32_t Count(std::uint32_t token,std::size_t list) const; //목록+8의 signed count다.
    std::uint32_t Item(std::uint32_t token,std::size_t list,std::int32_t index) const; //현재 실제 포인터의 DWORD를 읽는다.
    void Append(std::uint32_t token,std::size_t list,std::uint32_t value) const; //full에서는 포인터를 읽지 않고 일반 중복을 유지한다.
    void Remove(std::uint32_t token,std::size_t list,std::uint32_t value) const; //모든 중복을 안정 압축하고 inactive 꼬리는 보존한다.
    Sid Object(std::uint32_t value) const; //DWORD를 물리 검사 뒤 WORD SID로 바꾼다.0도 실제 슬롯이다.
    std::uint32_t Genus(std::uint32_t value) const; //현재 TYPE의 순수 flags2를 읽는다.
    RawAiBrainMemory& memory_; //생성/자산/다른 행동과 같은 실제 본체·목록 소유자다.
    const SidPool& pool_; //현재 위치/TYPE를 읽는 유일한 실제 풀이다.
    std::span<const RiftTypeRecord> types_; //현재188개 TYPE이며 보조 genus를 합성하지 않는다.
    std::function<std::uint32_t()> invalidGraph_; //같은 장부의 현재 무효 그래프를 선택된 원본 위치에서 읽는다.
    AiResourceChoiceHooks hooks_; //선택된 실제 중심/그래프/내용물/이동/로그 경계다.
};
}
