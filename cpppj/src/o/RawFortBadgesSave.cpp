// 초기 count 포인터와 현재 stream 주소를 분리하고 인라인 쓰기·진단·증가 순서를 보존한다.
#include "o/RawFortBadgesSave.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 선택되지 않은 장치는 요구하지 않으며 도달한 누락 경계는 앞선 효과 뒤에 실패한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("Badges 저장의 실제 하위 경계 미연결");return function;
}
// 실제 비정렬 little endian DWORD만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Badges DWORD 실제 물리 범위");std::uint32_t value{};
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; // 원본 네 BYTE 폭만 합친다.
}
// 해당 stream DWORD만 쓰고 다른 필드를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Badges DWORD 쓰기 실제 범위");
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 낮은 BYTE부터 네 개만 쓴다.
}
// 원본 EDI가 처음 보관한 할당/위치다. 뒤의 stream.data/pos 변경과 무관하게 같은 BYTE를 사용한다.
struct SavedCounter {
    std::span<std::uint8_t> allocation; // 저장 완료까지 살아 있는 최초 버퍼 할당이다.
    std::uint32_t position{}; // 최초 data에 더한 원래 pos다. 뒤에 새 pos를 복사하지 않는다.
};
// 실제 BYTE 쓰기 직전에 물리 범위만 확인하고 원문 밖 메모리를 대신 쓰지 않는다.
void WriteByte(std::span<std::uint8_t> bytes,std::uint32_t position,std::uint8_t value) {
    if (position>=bytes.size()) throw std::out_of_range("Badges 출력 BYTE 실제 물리 범위");bytes[position]=value;
}
// BYTE를 쓴 뒤에 다시 읽은 현재 pos/end로 signed 비교·DWORD 감김 갱신을 수행한다.
std::uint32_t Advance(std::span<std::uint8_t> fields) {
    const auto next=Read(fields,4)+1U,end=Read(fields,8);Put(fields,4,next);
    if (std::bit_cast<std::int32_t>(next)>std::bit_cast<std::int32_t>(end)) Put(fields,8,next);return next;
}
}
// 타입/미정 padding을 변경하지 않으며 중복은 원본처럼 처음 기록에서 검색을 끝낸다.
std::uint32_t ReadFortBadgeCount(std::span<const std::uint8_t> badges,std::uint32_t type) {
    if (badges.size()!=kFortBadgeRecordCount*kFortBadgeRecordStride) throw std::invalid_argument("Badge getter의 실제28x220BYTE 장부 누락");
    for (std::size_t index=0;index<kFortBadgeRecordCount;++index) { // type255 이상도 DWORD로 비교하며 BYTE로 절삭하지 않는다.
        const auto at=index*kFortBadgeRecordStride;if (Read(badges,at)==type) return Read(badges,at+kFortBadgeCountOffset);
    }
    return 0;
}
// count getter 뒤에도 반환 DWORD는 보관하며 각 진단 뒤 주소만 현재 필드에서 다시 읽는다.
void SaveFortBadges(std::int32_t slot,const FortBadgesSaveHooks& hooks) {
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(FortBadgesSaveAssertion::MissingTemplate);
    const auto manager=Need(hooks.manager)(slot);const auto stream=Need(hooks.create)(manager,"Badges");SavedCounter counter;
    Need(hooks.access)(stream,[&](auto fields,const auto& memory) {
        const auto position=Read(fields,4),data=Read(fields,0),end=Read(fields,8);counter={Need(memory.buffer)(data),position};
        const auto next=position+1U;Put(fields,4,next);if (std::bit_cast<std::int32_t>(next)>std::bit_cast<std::int32_t>(end)) Put(fields,8,next);
        WriteByte(counter.allocation,counter.position,0); // 초기 count BYTE는 원문처럼 용량 진단 없이 마지막에 쓴다.
    });
    for (std::uint32_t type=0;type<255;++type) { // 전체255회 조회하며 type255를 포함하지 않는다.
        const auto count=Need(hooks.count)(type);if (!count) continue;
        Need(hooks.access)(stream,[&](auto fields,const auto& memory) {
            if (std::bit_cast<std::int32_t>(Read(fields,4)+1U)>std::bit_cast<std::int32_t>(Read(fields,12))) Need(hooks.assertion)(FortBadgesSaveAssertion::WriteCapacity);
            const auto typePosition=Read(fields,4),typeData=Read(fields,0);WriteByte(Need(memory.buffer)(typeData),typePosition,static_cast<std::uint8_t>(type));
            const auto afterType=Advance(fields); // 원문 EAX의 다음 위치를 유지해 두 번째 용량 비교에 사용한다.
            if (std::bit_cast<std::int32_t>(afterType+1U)>std::bit_cast<std::int32_t>(Read(fields,12))) Need(hooks.assertion)(FortBadgesSaveAssertion::WriteCapacity);
            const auto countData=Read(fields,0),countPosition=Read(fields,4);WriteByte(Need(memory.buffer)(countData),countPosition,static_cast<std::uint8_t>(count));
            Advance(fields);WriteByte(counter.allocation,counter.position,static_cast<std::uint8_t>(counter.allocation[counter.position]+1U)); // count 쓰기·pos/end 뒤 최초 BYTE를 감김 증가한다.
        });
    }
}
}
