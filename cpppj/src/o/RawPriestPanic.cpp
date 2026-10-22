// 도달하지 않은 경계를 요구하지 않고 원본의 현재 읽기·임시 쓰기·재시도를 보존한다.
#include "o/RawPriestPanic.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 선택된 실제 하위 누락을 빈 성공으로 숨기지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("사제 panic 실제 하위 미연결");return hook; }
// 원본 CRT의 정상 signed DWORD 절삭 범위를 확인한다.
std::int32_t SpeedInteger(float value) { if (!std::isfinite(value) || double(value)<-2147483648.0 || double(value)>=2147483648.0) throw std::out_of_range("사제 panic 속도 CRT 정상범위");return static_cast<std::int32_t>(value); }
}
RawPriestPanic::RawPriestPanic(const SidPool& pool,std::span<DudeNoticeTypeData> speeds,PriestPanicHooks hooks):pool_(pool),speeds_(speeds),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || speeds.size()!=188) throw std::invalid_argument("사제 panic PATCH/188TYPE 필요");
}
std::uint32_t RawPriestPanic::Read(Sid sid,std::size_t at,std::size_t width) const {
    const auto raw=pool_.Slot(sid);std::uint32_t result{};for (std::size_t i=0;i<width;++i) result|=std::uint32_t(raw[at+i])<<(8*i);return result; // 실제 BYTE 폭만 조합한다.
}
void RawPriestPanic::Run(Sid priest) const {
    const auto temple=Need(hooks_.temple)(std::bit_cast<float>(Read(priest,14)),std::bit_cast<float>(Read(priest,18)));
    if (temple.value) {
        const auto hp=std::bit_cast<std::int32_t>(Read(priest,26)); // 최대HP 조회 앞의 현재HP snapshot이다.
        if (hp<Need(hooks_.maximum)(priest) && Read(temple,34,1)==Read(priest,34,1)) {
            const auto kind=Read(priest,10,1);if (kind>=speeds_.size()) throw std::out_of_range("사제 panic 현재TYPE 범위");auto& type=speeds_[kind];const auto saved=SpeedInteger(type.speed);type.speed=type.speed+type.speed;
            const auto path=Need(hooks_.target)(priest,temple);type.speed=static_cast<float>(saved);if (path) return;
        }
    }
    const bool direct=!Need(hooks_.land)(std::bit_cast<float>(Read(priest,14)),std::bit_cast<float>(Read(priest,18)));
    for (;;) { // 원본은 영역이 맞는 첫 실제 경로 시도까지 임의 위치를 계속 고른다.
        const auto x=std::bit_cast<float>(Read(priest,14)),y=std::bit_cast<float>(Read(priest,18)); // 두난수 호출 전의 두 좌표를 함께 보존한다.
        const auto candidateX=static_cast<float>(Need(hooks_.random)(-20,20)+x),candidateY=static_cast<float>(Need(hooks_.random)(-20,20)+y);
        bool valid=direct;
        if (!valid && Need(hooks_.land)(candidateX,candidateY)) {
            const auto candidateRegion=Need(hooks_.region)(candidateX,candidateY); // 후보 영역 효과 뒤의 현재 부모 좌표로 두 번째 영역을 읽는다.
            const auto currentRegion=Need(hooks_.region)(std::bit_cast<float>(Read(priest,14)),std::bit_cast<float>(Read(priest,18)));valid=candidateRegion==currentRegion;
        }
        if (valid) { static_cast<void>(Need(hooks_.point)(priest,candidateX,candidateY));return; }
    }
}
}
