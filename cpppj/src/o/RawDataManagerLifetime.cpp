// 원본의 효과 뒤 head 재조회와 소멸/free 사이의 현재 바이트를 보존한다.
#include "o/RawDataManagerLifetime.h"
#include <stdexcept>
namespace netstorm::o {
namespace {
constexpr std::uint32_t kBaseVtable=0x00507dd8; // 0044da60이 처음 쓰는 10.78 DataManager base 가상 표 식별자다.
// 비정렬된 원본DWORD만 읽고 호스트 포인터로 해석하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    // 지정한 네 바이트를 낮은 비트부터 합친다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8);
    return value;
}
// 현재DWORD만 써 인접한 이름/링크/예약 필드를 유지한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 네 바이트의 실제 원본 순서만 갱신한다.
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8));
}
}
// free 효과가 pointer/pos를 바꾸어도 반환 뒤 같은16BYTE를 원본 순서로 비운다.
void DestroyMemStream(std::span<std::uint8_t> stream,const std::function<void(std::uint32_t)>& releaseBuffer) {
    if (stream.size()<16) throw std::invalid_argument("MemStream 소멸의 실제16BYTE 누락");
    const auto buffer=Read(stream,0);
    if (buffer) { if (!releaseBuffer) throw std::logic_error("MemStream 실제 버퍼 반납 미연결");releaseBuffer(buffer); }
    Put(stream,0,0);Put(stream,12,0);Put(stream,8,0);Put(stream,4,0);
}
// 빈 목록도 vptr를 쓰며 current를0으로 만드는 별도 초기화를 수행하지 않는다.
void DestroyDataManager(std::span<std::uint8_t> manager,const DataManagerLifetimeHooks& hooks) {
    if (manager.size()<12) throw std::invalid_argument("DataManager 소멸의 실제12BYTE 누락");
    auto head=Read(manager,4);Put(manager,0,kBaseVtable);
    // 앞선 버퍼/free 효과가 바꾼 현재 head를 다음 순회에서 다시 읽는다.
    while (head) {
        if (!hooks.stream || !hooks.releaseStream) throw std::logic_error("DataManager 실제 섹션 조회/반납 미연결");
        const auto node=Read(manager,4);auto stream=hooks.stream(node);
        if (stream.size()<60) throw std::invalid_argument("DataManager 실제60BYTE 섹션 노드 누락");
        Put(manager,4,Read(stream,0x38));DestroyMemStream(stream,hooks.releaseBuffer);hooks.releaseStream(node);
        head=Read(manager,4);
    }
}
}
