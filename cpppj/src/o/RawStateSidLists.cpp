// 현재 등록 표와 목록 개수의 실제 수명을 공유하며 데이터/용량은 Native 정리에서 보존한다.
#include "o/RawStateSidLists.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o { //실제 PATCH 등록 표의 순서와 count 쓰기를 복원한다.
namespace { // Native 정적 생성 순서와 현재 물리의 연결만 지정한다.
constexpr std::array<std::uint32_t,9> kHeaders{0x55a4d0,0x55a4ec,0x55a708,0x592f80,0x5950d8,0x5954c0,0x59a9d4,0x5caf28,0x5e1898}; //0044fb20의 실제 아홉 this다.
// 짧은 호스트 뷰를 원문 고정20칸으로 잘못 역참조하지 않는다.
void RequireRegistry(std::span<const std::uint32_t> headers) { if (headers.size()<kStateSidRegistryCapacity) throw std::invalid_argument("State SID 등록 표의20DWORD 누락"); }
}
void RegisterStateSidList(std::span<std::uint32_t> headers,std::uint32_t header) {
    RequireRegistry(headers);
    for (std::size_t index=0;index<kStateSidRegistryCapacity;++index) { if (!headers[index]) { headers[index]=header;return; } } //첫 빈 칸에서 즉시 반환한다.
}
void UnregisterStateSidList(std::span<std::uint32_t> headers,std::uint32_t header) {
    RequireRegistry(headers);
    for (std::size_t index=0;index<kStateSidRegistryCapacity;++index) { if (headers[index]==header) headers[index]=0; } //첫 일치에서 끝내지 않는다.
}
void ResetRegisteredStateSidLists(std::span<const std::uint32_t> headers,const std::function<void(std::uint32_t)>& resetCount) {
    RequireRegistry(headers);
    for (std::size_t index=0;index<kStateSidRegistryCapacity;++index) { //앞선 실제 count store가 뒤 포인터를 바꾸면 다음 조회에 반영한다.
        const auto header=headers[index];if (header) { if (!resetCount) throw std::logic_error("State SID 개수 정리의 실제 쓰기 미연결");resetCount(header); }
    }
}
RawStateSidLists::RawStateSidLists() {
    factories.entries.resize(500);dudes.entries.resize(30);outposts.entries.resize(100);providers.entries.resize(256);
    geysers.entries.resize(100);priests.entries.resize(16);residences.entries.resize(50);temples.entries.resize(20);
    for (const auto header:kHeaders) RegisterStateSidList(registeredHeaders,header); //정적 생성자가 원문 순서로 같은20칸 표에 등록한다.
}
void RawStateSidLists::ResetCount(std::uint32_t header,std::int32_t& multipleCount) {
    switch (header) {
    case 0x55a4d0:factories.count=0;return;
    case 0x55a4ec:dudes.count=0;return;
    case 0x55a708:outposts.count=0;return;
    case 0x592f80:providers.count=0;return;
    case 0x5950d8:geysers.count=0;return;
    case 0x5954c0:priests.count=0;return;
    case 0x59a9d4:residences.count=0;return;
    case 0x5caf28:multipleCount=0;return;
    case 0x5e1898:temples.count=0;return;
    default:throw std::out_of_range("State SID 등록 표의 미연결 실제 헤더");
    }
}
void RawStateSidLists::PrepareGeneralWorld() {
    for (auto* list:{&factories,&dudes,&outposts,&providers,&geysers,&priests,&residences,&temples}) { //기존 독립 일반 World의 새 목록 의미를 같은 주소에서 유지한다.
        std::fill(list->entries.begin(),list->entries.end(),0U);list->count=0;
    }
}
}
