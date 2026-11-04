// 원본의 목록→검색→확보→HP→예약→소유자 상태 쓰기 순서를 보존한다.
#include "o/RawPriestPostPop.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 원문00492240은 범위 밖을 진단한 뒤 쓰기를 생략하며9DWORD 중 지정한1..8 한칸만 바꾼다.
void ResetPriestOwnerState(std::array<std::uint32_t,kPlayerCount+1>& states,std::uint32_t player,const std::function<void(std::uint32_t)>& assertion) {
    if (player<1 || player>kPlayerCount) {
        if (!assertion) throw std::logic_error("사제 owner 상태 초기화 진단 미연결");assertion(player);return;
    }
    states[player]=0;
}
// 후반 조합의 풀 혼합 검사를 위해 읽기 전용 참조를 반환한다.
const SidPool& RawPriestPostPop::Pool() const { return pool_; }
namespace {
// 판본 공통 raw 필드 위치와 원본 회복 분모/배율이다. 배율은 두 PE에서 float 50.0이다.
constexpr std::size_t kType=10,kOriginalOwner=12;
// PATCH 물리 슬롯의 extra(+0x28)와 현재 소유자(+0x22) 위치다(단위862 PATCH 접두 분리 때 이름을 붙였다).
constexpr std::size_t kPatchExtra=40,kPatchOwner=34;
constexpr std::int32_t kRegenDivisor=6;
constexpr double kRegenScale=50.0;
// 가상 표 DWORD를 정렬과 무관하게 읽는다.
std::uint32_t Vtable(std::span<const std::uint8_t> raw) {
    std::uint32_t result=0;
    // 낮은 바이트부터 합친다.
    for (std::size_t i=0;i<4;++i) result|=static_cast<std::uint32_t>(raw[i])<<(8*i);
    return result;
}
// 00414450와 CD 몸체의 inline 목록: 용량이 남을 때만 중복을 검색하고 끝에 넣는다.
// CD 사전 계약(count<=용량 확인 뒤)의 기존 독립 목록 경로에서만 사용한다.
void AddUnique(SquidPostPopList& list,Sid sid) {
    if (list.count>=list.entries.size()) return;
    // 활성 구간 밖의 오래된 같은 번호는 중복으로 취급하지 않는다.
    for (std::uint32_t i=0;i<list.count;++i) if (list.entries[i]==sid.value) return;
    list.entries[list.count++]=sid.value;
}
// PATCH00414450(&005954c4,sid): 처음 읽은 signed count가 signed 용량보다 작을 때만 현재 count와 비교하며 중복을 찾고,
// 저장 위치는 처음 읽은 count를 쓰며 그 뒤 현재 count를 한 번 증가시킨다. 음수/짧은 배열 저장은 실제 역참조 위치의 호스트 물리 경계다.
// (2026-11-04 단위862: 사제 접두의 일괄 count 검사를 원문 signed 관문으로 바꾸며 추가했다.)
void AppendUniquePriest(SquidPostPopList& list,Sid sid) {
    const auto used=list.Count();const auto capacity=list.Capacity(); // 원문 함수 머리에서 읽은 +8/+4 두 DWORD다.
    if (capacity>0xffffffffU) throw std::out_of_range("사제 목록 DWORD 용량 물리 범위 오류");
    if (std::bit_cast<std::int32_t>(used)>=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(capacity))) return;
    if (std::bit_cast<std::int32_t>(used)>0) {
        const auto storage=list.Storage();std::uint32_t index{}; // 원문이 검색 시작 때 읽은 배열 포인터와 현재 검색 번호다.
        do { // 첫 항목은 최초 양수 count로 들어오며 이후 반복 조건은 매번 현재 count를 다시 읽는다.
            if (index>=storage.size()) throw std::out_of_range("사제 목록 중복 검색 물리 배열 범위 오류");
            if (storage[index]==sid.value) return;
            ++index;
        } while (std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(list.Count()));
    }
    if (std::bit_cast<std::int32_t>(used)<0) throw std::out_of_range("사제 목록 음수 저장 위치");
    const auto storage=list.Storage();if (used>=storage.size()) throw std::out_of_range("사제 목록 실제 저장 범위 오류");
    storage[used]=sid.value;list.SetCount(list.Count()+1U);
}
// PATCH에서 도달한 외부 장치만 요구하고, 호출 중 연결이 바뀌어도 선택한 callable 사본의 수명을 유지한다(단위862 추가).
template<class Function> Function PriestNeed(const Function& function) {
    if (!function) throw std::logic_error("사제 postPop 선택된 실제 하위 미연결");
    return function;
}
}
// 풀 혼합은 두 판본 모두 효과 전에 거부한다. 훅 사전 요구는 CD의 기존 계약에만 남긴다.
RawPriestPostPop::RawPriestPostPop(SidPool& pool,const SquidReward& hp,PriestPostPopState& state,PriestPostPopHooks hooks,const std::uint32_t& priestType)
    :pool_(pool),hp_(hp),state_(state),hooks_(std::move(hooks)),priestType_(priestType) {
    if (&hp.Pool()!=&pool) throw std::invalid_argument("사제 postPop/최대 HP의 SID 풀이 다릅니다");
    if (pool.Edition()!=OriginalEdition::Patch1078 && (!hooks_.findRegular || !hooks_.reserveRegular || !hooks_.addRegular))
        throw std::invalid_argument("사제 회복 예약 효과 누락");
}
// 최초 등록의 공간 쓰기 전에 접두 계약을 확인한다. 상태/목록/예약은 바꾸지 않는다.
void RawPriestPostPop::Validate(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (raw[kType]!=kPriestType || Vtable(raw)!=(patch ? kPatchPriestVtable : kCdPriestVtable))
        throw std::invalid_argument("사제 postPop 가상 표/타입 불일치");
    if (state_.Priests().count>state_.Priests().entries.size()) throw std::invalid_argument("사제 목록 개수 오류");
}
// x87은 int / 6을 0방향으로 절삭한 뒤 곱하고 float으로 저장한다. int를 먼저 float으로 줄이지 않는다.
// 분자는 현재 사제 TYPE DWORD(0049a840/0049fce0, CD 00444720)이고 그 뒤 객체 최대 HP(004adcb0/CD004aed30)를 읽는다.
float RawPriestPostPop::RegenPayload(Sid sid) const {
    const float numerator=static_cast<float>(static_cast<double>(hp_.TypeHitPoints(priestType_)/kRegenDivisor)*kRegenScale);
    // 원본 FILD의 정수 분모를 유지하고 나눗셈 결과만 float으로 저장한다.
    return static_cast<float>(static_cast<double>(numerator)/static_cast<double>(hp_.MaxHitPoints(sid)));
}
// PATCH004950f0의 접두다. 실제 가상 진입은 TYPE/가상 표/할당/목록 사전 검사 없이 이 몸체로 들어온다.
void RawPriestPostPop::PrefixPatch(Sid sid,std::uint32_t flags) const {
    if (pool_.Slot(sid)[kPatchExtra]&9U) return;
    if (flags&1U) {
        AppendUniquePriest(state_.Priests(),sid);
        // 이미 있는 회복은 확보/HP 조회를 생략한다. 확보 실패도 HP 조회와 생성만 생략한다.
        if (!PriestNeed(hooks_.findRegular)(sid,kPriestRegenEvent) && PriestNeed(hooks_.reserveRegular)()) {
            const auto payload=RegenPayload(sid);
            PriestNeed(hooks_.addRegular)(sid,kPriestRegenEvent,payload);
        }
    }
    // 예약 효과 이후 현재 원래 소유자 WORD와 현재 소유자 BYTE를 다시 읽고 유효1..8 한 칸만 0으로 만든다.
    const auto current=pool_.Slot(sid);const auto owner=current[kPatchOwner];
    if ((current[kOriginalOwner]&0x7f)==owner && owner>0 && owner<=kPlayerCount) state_.OwnerState()[owner]=0;
}
// 목록/회복 접두 뒤 전체 Carrier/보호막 처리는 RawPriestPostPopTail이 이어 준다.
void RawPriestPostPop::Prefix(Sid sid,std::uint32_t flags) const {
    if (pool_.Edition()==OriginalEdition::Patch1078) { PrefixPatch(sid,flags);return; }
    Validate(sid);const auto raw=pool_.AllocatedBytes(sid);
    if (raw[35]&9) return;
    if (flags&1) {
        AddUnique(state_.Priests(),sid);
        if (!hooks_.findRegular(sid,kPriestRegenEvent) && hooks_.reserveRegular()) hooks_.addRegular(sid,kPriestRegenEvent,RegenPayload(sid));
    }
    // 예약 훅 이후 다시 읽는다. 원본도 현재 소유자를 이 지점에서 조회한다.
    const auto current=pool_.Slot(sid);
    const auto owner=current[32];
    if ((current[kOriginalOwner]&0x7f)==owner && owner>0 && owner<=kPlayerCount) ResetPriestOwnerState(state_.OwnerState(),owner); // 같은 유효owner0쓰기 본문을 데이터 전용 정리와 공유한다.
}
// ProcessForm·Kernel을 가진 실제 호스트와 합성한다. reserve는 원본 할당 실패를 주입할 경계다.
PriestPostPopHooks MakePriestPostPopProcessHooks(SquidProcessHost& host) {
    return {
        [&host](Sid sid,std::uint32_t event) { return host.FindEvent(sid,event)!=nullptr; },
        [] { return true; },
        [&host](Sid sid,std::uint32_t event,float payload) { host.AddRegular(sid,event,payload); }
    };
}
}
