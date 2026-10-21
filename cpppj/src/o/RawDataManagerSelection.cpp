// 진단·비교 뒤의 현재 포인터 재조회와 다른 노드/필드 보존을 원본 순서대로 처리한다.
#include "o/RawDataManagerSelection.h"
#include <algorithm>
#include <stdexcept>
namespace netstorm::o {
namespace {
// 지정DWORD만 읽으며 정렬되지 않은 원본 필드도 허용한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t result{};
    // 같은 네 바이트만 낮은 비트부터 합친다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(bytes[at+index])<<(index*8);
    return result;
}
// current/pos의 네 바이트만 변경한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 인접 필드와 양쪽 보호 바이트는 변경하지 않는다.
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 동일 노드의 실제 최소 폭을 확인하고 원본의 잘못된 메모리를 성공으로 바꾸지 않는다.
std::span<std::uint8_t> Node(std::uint32_t handle,const DataManagerSelectionHooks& hooks) {
    if (!handle || !hooks.stream) throw std::logic_error("DataManager 현재 노드/조회 연결 누락");
    auto bytes=hooks.stream(handle);if (bytes.size()<60) throw std::out_of_range("DataManager 실제60BYTE 노드 누락");return bytes;
}
// C문자열은 첫NUL까지만 사용한다. 물리적 이름40BYTE 밖 읽기는 허용하지 않는다.
std::string_view Name(std::span<const std::uint8_t> node) {
    const auto bytes=node.subspan(16,40);const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});
    if (end==bytes.end()) throw std::out_of_range("DataManager 이름40BYTE 내 NUL 누락");
    return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
// 필요 진단을 생략하지 않으며 반환한 뒤의 실제 저장소를 호출자가 재조회한다.
void Assert(const DataManagerSelectionHooks& hooks,DataManagerSelectionAssertion kind) {
    if (!hooks.assertion) throw std::logic_error("DataManager 선택의 실제 진단 미연결");hooks.assertion(kind);
}
// CRT의 NUL 이전 공식 ASCII 이름만 지원하며 현재 프로세스 로캘에 의존하지 않는다.
unsigned char Lower(unsigned char byte) {
    if (byte>=128) throw std::invalid_argument("DataManager 비ASCII 이름 CRT 로캘 미복원");
    return byte>='A' && byte<='Z' ? static_cast<unsigned char>(byte+('a'-'A')) : byte;
}
}
// 공식 섹션 이름의 첫 차이 부호와 NUL 종료를 CRT 경계와 동일하게 처리한다.
int CompareDataManagerSectionNames(std::string_view first,std::string_view second) {
    first=first.substr(0,first.find('\0'));second=second.substr(0,second.find('\0'));
    // 매 바이트를 읽는 순서대로 비교하고 일치하는 접두 뒤 길이를 비교한다.
    for (std::size_t index=0;index<std::min(first.size(),second.size());++index) {
        const auto left=Lower(static_cast<unsigned char>(first[index])),right=Lower(static_cast<unsigned char>(second[index]));
        if (left!=right) return left>right ? 1 : -1;
    }
    return first.size()==second.size() ? 0 : first.size()>second.size() ? 1 : -1;
}
// 비교에서 current가 바뀌어도 캡처한 노드 대신 원본처럼 현재 값으로 진행한다.
std::uint32_t FindDataManagerSection(std::span<std::uint8_t> manager,std::string_view name,const DataManagerSelectionHooks& hooks) {
    if (manager.size()<12) throw std::invalid_argument("DataManager 실제12BYTE 누락");
    Put(manager,8,Read(manager,4));
    // 원본처럼 현재 포인터가0이 될 때 실패하고 첫 일치에서 반환한다.
    while (Read(manager,8)) {
        const auto node=Node(Read(manager,8),hooks);const auto currentName=Name(node);
        const auto comparison=hooks.compare ? hooks.compare(name,currentName) : CompareDataManagerSectionNames(name,currentName);
        if (!comparison) return Read(manager,8);
        Put(manager,8,Read(Node(Read(manager,8),hooks),0x38));
    }
    return 0;
}
// 쓰기 허용은 비영DWORD이며 진단 이후의 현재 노드에만 pos0을 쓴다.
std::uint32_t SelectDataManagerSection(std::span<std::uint8_t> manager,std::string_view name,std::int32_t writable,const DataManagerSelectionHooks& hooks) {
    FindDataManagerSection(manager,name,hooks);
    if (!Read(manager,8)) Assert(hooks,DataManagerSelectionAssertion::MissingCurrent);
    if (!writable) {
        const auto node=Node(Read(manager,8),hooks);
        if (!Read(node,0) || !Read(node,8)) Assert(hooks,DataManagerSelectionAssertion::EmptyRead);
    }
    Put(Node(Read(manager,8),hooks),4,0);return Read(manager,8);
}
}
