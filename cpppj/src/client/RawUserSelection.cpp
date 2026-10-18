// 디컴파일의 두 인자 표기 대신 실제 스택의 세 인자와 원본 쓰기·재조회 순서를 보존한다.
#include "client/RawUserSelection.h"
#include <bit>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client {
namespace {
// 선택된 하위가 없으면 그 위치에서 실패하여 선행 원본 효과를 감추지 않는다.
template<class F> const F& Need(const F& hook) {
    if (!hook) throw std::logic_error("실제 사용자 선택 하위 미연결");return hook;
}
}
// 두 판본의 stride나 TYPE 수를 혼합하지 않는다.
RawUserSelection::RawUserSelection(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,UserSelectionState state,UserSelectionHooks hooks)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("사용자 선택10.78 풀/TYPE 오류");
}
// 0번 슬롯은 유효 범위 검사에만 사용하며 실제 선택 객체로 읽지 않는다.
std::span<const std::uint8_t> RawUserSelection::Raw(std::uint32_t sid) const {
    if (!sid || sid>=pool_.Capacity()) throw std::out_of_range("사용자 선택 SID 범위 오류");return pool_.Slot(o::Sid{static_cast<std::uint16_t>(sid)});
}
// 부모/현재 TYPE를 매 호출 시점에서 조회한다.
const o::RiftTypeRecord& RawUserSelection::Type(std::uint32_t type) const {
    if (type>=types_.size()) throw std::out_of_range("사용자 선택 TYPE 범위 오류");return types_[type];
}
// 원본 낮은 주소부터 필요한 폭만 읽는다.
std::uint32_t RawUserSelection::Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value=0;
    // little endian 원문을 호스트 정렬과 무관하게 조립한다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);
    return value;
}
// 4d5490은 현재 signed 개수를 통지 후 다시 읽으며 죽음/반납 비트만 제외한다.
void RawUserSelection::ClearMultiple() const {
    // callback이 개수/다음 항목을 바꾸면 다음 반복에서 그 현재 값을 사용한다.
    for (std::int32_t index=0;index<state_.multipleCount;++index) {
        if (static_cast<std::size_t>(index)>=state_.multiple.size()) throw std::out_of_range("사용자 다중 선택 배열 범위 오류");
        const auto number=state_.multiple[static_cast<std::size_t>(index)];
        if (!number || number>=pool_.Capacity() || (Raw(number)[11]&3)) continue;
        const o::Sid sid{static_cast<std::uint16_t>(number)};Need(hooks_.selectNotice)(sid,0);Need(hooks_.refresh)(sid);
    }
    state_.multipleCount=0;
}
// 종속 next는 가상 효과 이전에 캡처한다. ProcessFinder는70이상 자산만 건너뛴다.
void RawUserSelection::ClearPrevious(o::Sid sid) const {
    auto next=Read(Raw(sid.value),6,2);std::unordered_set<std::uint32_t> visited; // 손상된 순환만 호스트에서 명시 거부한다.
    // 현재 살아 있는 프로세스뿐 아니라 원본이 방문하는 모든 물리 후보를 같은 순서로 검사한다.
    while (next) {
        const auto number=next;if (!visited.insert(number).second) throw std::logic_error("사용자 선택 프로세스 체인 순환");
        const auto raw=Raw(number);next=Read(raw,4,2);const auto type=raw[10];
        if (type>=10 && type<70 && (type==Need(hooks_.manaProcess)() || type==Need(hooks_.rangeProcess)()))
            Need(hooks_.processNotice)(o::Sid{static_cast<std::uint16_t>(number)},3,0,0);
    }
    Need(hooks_.selectNotice)(sid,0);Need(hooks_.refresh)(sid);state_.selected=0;
}
// flags와 기억 좌표 사이에 추가 조회나 임의 정수 반올림을 넣지 않는다.
void RawUserSelection::Remember(std::uint32_t x,std::uint32_t y,std::uint32_t type) const {
    if (Type(type).flags2&0x30000U) { state_.rememberedPoint={x,y};state_.mouseType=type; }
}
// forceSelect는 flags1&08000000의 선택 허용, activateMove는 모드5 활성화에만 관여한다.
std::uint32_t RawUserSelection::Select(std::uint32_t number,std::uint32_t forceSelect,std::uint32_t activateMove) const {
    // dais가 같은 위치의 유효 제단으로 위임하면 최초 관문과 다중 선택 정리부터 다시 수행한다.
    for (;;) {
        if ((number && Need(hooks_.paused)()) || (!Need(hooks_.enabled)() && !number) || Need(hooks_.blocked)()) return 0;
        ClearMultiple();if (state_.selected) { Raw(state_.selected);ClearPrevious(o::Sid{static_cast<std::uint16_t>(state_.selected)}); }
        if (!number || (Raw(number)[11]&2)) { Need(hooks_.highlight)(0);return 0; }
        const o::Sid sid{static_cast<std::uint16_t>(number)};Need(hooks_.dump)(sid);
        if (Type(Raw(number)[10]).flags1&0x10000000U) Need(hooks_.showMana)(sid,0);
        if (Need(hooks_.rangeType)(Raw(number)[10])) Need(hooks_.showRange)(sid,0);
        if ((Type(Raw(number)[10]).flags2&0x30000U) && Need(hooks_.content)(sid).value) Need(hooks_.showRange)(sid,0);
        if (Raw(number)[10]==Need(hooks_.dais)()) {
            const auto altar=Need(hooks_.altar)(sid); // 원본40ea40은 상태 없이 물리 범위만 확인한다.
            if (altar.value && altar.value<pool_.Capacity()) { number=altar.value;continue; }
        }
        if (!(Type(Raw(number)[10]).flags1&0x08000000U) || forceSelect) {
            state_.selected=number;Need(hooks_.selectNotice)(sid,1);Need(hooks_.refresh)(sid);
            const auto raw=Raw(number);const auto x=Read(raw,14),y=Read(raw,18); // 비교 전 원본 float 두 개의 비트를 캡처한다.
            if (!(std::bit_cast<float>(x)==std::bit_cast<float>(state_.rememberedPoint[0]) && std::bit_cast<float>(y)==std::bit_cast<float>(state_.rememberedPoint[1]))) state_.mouseType=0;
            if (Raw(number)[40]&9) Remember(Read(Raw(number),14),Read(Raw(number),18),Raw(number)[10]);
            if ((Type(Raw(number)[10]).flags2&0x30000U) && !(Raw(number)[40]&1)) {
                if (!state_.selected) Need(hooks_.assertion)();
                if (state_.selected) {
                    if (!Need(hooks_.moving)(sid) && activateMove && (Need(hooks_.editor)() || Raw(number)[34]==Need(hooks_.localOwner)())) Need(hooks_.highlight)(5);
                    Need(hooks_.movementPreview)(sid);
                }
            }
            Need(hooks_.sound)(Read(Raw(number),14),Read(Raw(number),18),"select.wav",0);
        }
        return number;
    }
}
// 4d5c60은 정지/콘솔 관문 없이 enabled만 읽으며 기존 단일 선택 가상 통지를 대신 호출하지 않는다.
void RawUserSelection::Clear() const {
    if (Need(hooks_.enabled)()) { ClearMultiple();state_.selected=0;Need(hooks_.highlight)(0); }
}
// 보호 PE의 Carrier+ac는 선택 결과와 무관하게 마지막 공통 Open의 전체DWORD를 반환한다.
std::uint32_t RawUserSelection::CarrierMenu(o::Sid sid,const InputEvent& event,const std::function<std::uint32_t(o::Sid,const InputEvent&)>& common) const {
    if (!(Raw(sid.value)[40]&1)) static_cast<void>(Select(sid.value,0,0));return Need(common)(sid,event);
}
// Archer는 둘째 인자를1로 바꾸지 않는다. 실제 셋째1은 이동 모드의 별도 분기를 제어한다.
std::uint32_t RawUserSelection::ArcherMenu(o::Sid sid,const InputEvent& event,const std::function<std::uint32_t(o::Sid,const InputEvent&)>& common) const {
    if (!(Raw(sid.value)[40]&1)) static_cast<void>(Select(sid.value,0,1));return Need(common)(sid,event);
}
}
