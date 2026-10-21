// 원본의 선행 링크·압축/소멸 뒤 재조회와 stale current를 보존한다.
#include "o/RawDataManagerList.h"
#include <stdexcept>
namespace netstorm::o {
namespace {
// 정렬되지 않은DWORD 필드의 현재 바이트만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    // 같은 네 바이트를 원본 순서로 결합한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8);return value;
}
// 지정한 링크/current DWORD만 변경한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 인접 필드와 보호 바이트를 보존한다.
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 원본 포인터를 호스트 주소로 자르지 않고 같은 실제60BYTE에서 조회한다.
std::span<std::uint8_t> Node(const DataManagerListHooks& hooks,std::uint32_t handle) {
    if (!handle || !hooks.lifetime.stream) throw std::logic_error("DataManager 목록의 실제 노드 조회 미연결");auto bytes=hooks.lifetime.stream(handle);
    if (bytes.size()<60) throw std::out_of_range("DataManager 목록의 실제60BYTE 누락");return bytes;
}
// 원본 보고 뒤 계속하는 진단은 소비자가 실제 수리/중단 동작을 선택한다.
void Missing(const DataManagerListHooks& hooks) { if (!hooks.missingPredecessor) throw std::logic_error("DataManager 목록의 선행 링크 진단 미연결");hooks.missingPredecessor(); }
}
// 빈 목록은 새 노드 조회나 압축 없이 head/current 두 필드만 쓴다.
void AppendDataManagerStream(std::span<std::uint8_t> manager,std::uint32_t stream,const DataManagerListHooks& hooks) {
    if (manager.size()<12) throw std::invalid_argument("DataManager 추가의 실제12BYTE 누락");auto tail=Read(manager,4);
    if (tail) {
        // 원본처럼 현재 next를 따라 마지막 노드를 찾는다.
        while (const auto next=Read(Node(hooks,tail),0x38)) tail=next;
        if (!hooks.compact) throw std::logic_error("DataManager 마지막 stream 압축 미연결");hooks.compact(tail);Put(Node(hooks,tail),0x38,stream);
    } else Put(manager,4,stream);
    Put(manager,8,stream);
}
// 해제 도중 외부 효과가 바꾼 current는 별도로0으로 만들지 않는다.
void RemoveDataManagerStream(std::span<std::uint8_t> manager,std::uint32_t stream,const DataManagerListHooks& hooks) {
    if (manager.size()<12) throw std::invalid_argument("DataManager 제거의 실제12BYTE 누락");auto previous=Read(manager,4);
    if (stream==previous) Put(manager,4,Read(Node(hooks,stream),0x38));
    else {
        if (!Read(Node(hooks,previous),0x38)) Missing(hooks);
        else {
            // 목표의 직전 노드를 유지하고 다음 링크가0이면 같은 선행 노드로 진단한다.
            for (;;) { const auto next=Read(Node(hooks,previous),0x38);if (next==stream) break;previous=next;if (!Read(Node(hooks,previous),0x38)) break; }
            if (!Read(Node(hooks,previous),0x38)) Missing(hooks);
        }
        const auto current=Read(Node(hooks,previous),0x38);Put(Node(hooks,previous),0x38,Read(Node(hooks,current),0x38));
    }
    if (stream) { DestroyMemStream(Node(hooks,stream),hooks.lifetime.releaseBuffer);if (!hooks.lifetime.releaseStream) throw std::logic_error("DataManager 실제 노드 반납 미연결");hooks.lifetime.releaseStream(stream); }
}
}
