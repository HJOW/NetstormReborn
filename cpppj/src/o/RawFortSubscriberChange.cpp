// 현재 manager/fort 관문 뒤 원문 strcpy와 설정·제목·ID·저장·통지 순서만 소유한다.
#include "o/RawFortSubscriberChange.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o { // 원본 Fort Subscriber 선택 몸체의 명시 경계다.
namespace { // 선택된 경계와 실제 NUL 접근을 검사하는 내부 장치다.
// 선택되지 않은 하위를 빈 성공으로 채우지 않는다.
template<class Function> const Function& Need(const Function& effect) { if (!effect) throw std::logic_error("Fort Subscriber 변경 실제 장치 누락");return effect; }
// 실제 handle의 첫 NUL까지만 읽으며 저장소 밖의 원문 접근은 host에서 거부한다.
std::span<const std::uint8_t> CString(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("Fort Subscriber handle NUL 누락");return bytes.first(static_cast<std::size_t>(end-bytes.begin())+1);
}
}
// Template 부재는 fortMode조차 읽지 않는다. 큰 설정/제목/writer 뒤에는 현재 owner/handle을 다시 읽는다.
void ChangeFortSubscriber(std::uint32_t id,std::optional<std::span<const std::uint8_t>> name,std::uint32_t& my,const FortSubscriberChangeHooks& hooks) {
    if (!Need(hooks.manager)(Need(hooks.owner)()) || !Need(hooks.fortMode)()) return;
    if (name) {
        auto destination=Need(hooks.handle)();std::size_t index{};
        for (;;) { // 원문 strcpy처럼 NUL까지 순서대로 쓰며 물리 범위 실패 전의 바이트를 유지한다.
            if (index>=name->size() || index>=destination.size()) throw std::out_of_range("Fort Subscriber strcpy 물리 범위 부족");
            const auto byte=(*name)[index];destination[index++]=byte;if (!byte) break;
        }
        const auto current=CString(Need(hooks.handle)());Need(hooks.configure)("fort.mySubHandle",{reinterpret_cast<const char*>(current.data()),current.size()-1});Need(hooks.title)();
    }
    my=id;const auto current=Need(hooks.handle)();const auto owner=Need(hooks.owner)();Need(hooks.subscriber)(owner,id,current);Need(hooks.notify)(Need(hooks.owner)());
}
}
