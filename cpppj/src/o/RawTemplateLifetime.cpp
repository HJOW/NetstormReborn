// 큰 소멸 효과 뒤 현재/뒤 칸을 다시 읽으며 원본이 저장한 manager 포인터는 유지한다.
#include "o/RawTemplateLifetime.h"
#include <algorithm>
#include <stdexcept>
namespace netstorm::o {
namespace {
constexpr std::size_t kHeaderBytes=20,kTemplateCount=9; // 원본 Template 한 기록과 전역 배열의 실제 폭이다.
// 비정렬 포인터/빌림DWORD를 같은 바이트에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t result{};
    // 지정DWORD의 네 바이트만 읽는다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(bytes[at+index])<<(index*8);return result;
}
}
// 다른 폭은 원본 밖 쓰기 대신 명시적으로 거부한다.
void InitializeTemplateHeader(std::span<std::uint8_t> header) { if (header.size()!=kHeaderBytes) throw std::invalid_argument("Template 실제20BYTE 헤더 누락");std::fill(header.begin(),header.end(),std::uint8_t{}); }
// 소멸이 헤더 포인터/빌림 값을 바꾸어도 처음 저장한 같은 manager를 free한다.
void ReleaseTemplateHeader(std::span<std::uint8_t> header,const TemplateLifetimeHooks& hooks) {
    if (header.size()!=kHeaderBytes) throw std::invalid_argument("Template 실제20BYTE 반납 헤더 누락");const auto manager=Read(header,0);
    if (manager && !Read(header,4)) { if (!hooks.destroy || !hooks.release) throw std::logic_error("Template 실제 소멸/할당 반납 미연결");hooks.destroy(manager);hooks.release(manager); }
    InitializeTemplateHeader(header);
}
// 빈 포인터는 부가DWORD를 지우지 않는 전역 정리의 자연 반환이다.
void ClearTemplateHeaders(std::span<std::uint8_t> headers,const TemplateLifetimeHooks& hooks) {
    if (headers.size()!=kTemplateCount*kHeaderBytes) throw std::invalid_argument("Template 실제9개 헤더 누락");
    // 앞 소멸 효과가 바꾼 뒤 칸의 현재 포인터와 빌림 값을 순서대로 읽는다.
    for (std::size_t slot=0;slot<kTemplateCount;++slot) { const auto header=headers.subspan(slot*kHeaderBytes,kHeaderBytes);if (Read(header,0)) ReleaseTemplateHeader(header,hooks); }
}
}
