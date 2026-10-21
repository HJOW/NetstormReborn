// 원본의 임시 이름과 부모/자식 효과 뒤 원문·현재 할당 길이 재조회를 유지한다.
#include "o/RawDataManagerResize.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace netstorm::o {
namespace {
constexpr std::string_view kTemporaryName="T$e$m$p"; // 0044d290이 생성/이름 교체에 같은 값으로 쓰는 원본 임시 이름이다.
// 비정렬 원본DWORD의 현재 값만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    // 네 바이트를 원본 little endian 순서로 결합한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8);return value;
}
// 지정DWORD만 쓰며 인접 이름/링크를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 원본 필드의 네 바이트 밖은 변경하지 않는다.
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 포인터를 잘라 호스트 주소로 쓰지 않고 같은 실제 노드를 빌린다.
std::span<std::uint8_t> Node(const DataManagerResizeHooks& hooks,std::uint32_t handle) {
    if (!handle || !hooks.selection.stream) throw std::logic_error("DataManager 확장의 실제 노드 조회 미연결");auto bytes=hooks.selection.stream(handle);
    if (bytes.size()<60) throw std::out_of_range("DataManager 확장의 실제60BYTE 누락");return bytes;
}
// 원본 strcpy의 이름/NUL만 쓰며 나머지 이름40BYTE를 초기화하지 않는다.
void Name(std::span<std::uint8_t> node,std::string_view name) {
    name=name.substr(0,name.find('\0'));if (name.size()>=40) throw std::out_of_range("DataManager 실제 이름40BYTE 범위");
    std::copy(name.begin(),name.end(),node.begin()+16);node[16+name.size()]=0;
}
// 실제 접근하는 양의 범위만 조회하고0길이는 잘못된 포인터를 역참조하지 않는다.
std::span<std::uint8_t> Buffer(const DataManagerResizeHooks& hooks,std::uint32_t pointer,std::uint32_t position,std::uint32_t length) {
    if (!length) return {};if (!pointer || !hooks.memory.buffer) throw std::logic_error("DataManager 확장의 실제 버퍼 조회 미연결");auto bytes=hooks.memory.buffer(pointer);
    if (position>bytes.size() || length>bytes.size()-position) throw std::out_of_range("DataManager 확장의 실제 버퍼 물리 범위");return bytes.subspan(position,length);
}
}
// 생성자는 할당 뒤의 현재 입력 이름을 복사하며 NULL은 첫 이름BYTE만0으로 쓴다.
std::uint32_t ConstructDataManagerStream(std::uint32_t stream,std::optional<std::string_view> name,std::int32_t size,const DataManagerResizeHooks& hooks) {
    auto node=Node(hooks,stream);std::fill(node.begin(),node.begin()+16,std::uint8_t{});
    AllocateMemStream(node,static_cast<std::uint32_t>(std::max(size,1)),hooks.memory);
    if (name) Name(node,*name);else node[16]=0;Put(node,0x38,0);return stream;
}
// 원본 생성 부모는 새 this가 아니라 마지막 추가 효과 뒤의 manager.current를 반환한다.
std::uint32_t CreateDataManagerStream(std::span<std::uint8_t> manager,std::string_view name,const DataManagerResizeHooks& hooks) {
    const auto existing=FindDataManagerSection(manager,name,hooks.selection);if (existing) RemoveDataManagerStream(manager,existing,hooks.list);
    if (!hooks.allocateStream) throw std::logic_error("DataManager 섹션 생성의 실제 노드 할당 미연결");const auto stream=hooks.allocateStream(60);
    if (stream) ConstructDataManagerStream(stream,name,0x4000,hooks);AppendDataManagerStream(manager,stream,hooks.list);return Read(manager,8);
}
// 원문 노드는 생성 전의 같은 객체이며 그 data/end만 대상 생성이 끝난 뒤 다시 읽는다.
std::uint32_t CloneDataManagerStream(std::span<std::uint8_t> manager,std::span<std::uint8_t> sourceManager,std::string_view name,const DataManagerResizeHooks& hooks) {
    const auto original=FindDataManagerSection(sourceManager,name,hooks.selection);
    if (!original) { if (!hooks.missingSource) throw std::logic_error("DataManager 복제의 원문 누락 진단 미연결");hooks.missingSource(); }
    const auto source=Node(hooks,original);const auto stream=CreateDataManagerStream(manager,name,hooks);auto target=Node(hooks,stream);
    if (Read(source,0)) CopyMemStream(target,source,hooks.memory);Put(target,4,0);return stream;
}
// 기존 대상의 반납 효과 뒤 원래 이름을 찾으므로 선행 current를 캐시하지 않는다.
std::uint32_t RenameDataManagerStream(std::span<std::uint8_t> manager,std::string_view name,std::string_view previousName,const DataManagerResizeHooks& hooks) {
    const auto target=FindDataManagerSection(manager,name,hooks.selection);if (target) RemoveDataManagerStream(manager,target,hooks.list);
    const auto source=FindDataManagerSection(manager,previousName,hooks.selection);Name(Node(hooks,source),name);return Read(manager,8);
}
// 부모는 생성 전 원문 노드를 보관하되 실제 data/end는 생성이 끝난 뒤 다시 읽는다.
std::uint32_t EnsureDataManagerStreamSize(std::span<std::uint8_t> manager,std::string_view name,std::int32_t minimum,const DataManagerResizeHooks& hooks) {
    const auto original=FindDataManagerSection(manager,name,hooks.selection);
    if (!original) { if (!hooks.missingSource) throw std::logic_error("DataManager 최소 확장의 누락 진단 미연결");hooks.missingSource(); }
    const auto source=Node(hooks,original);if (std::bit_cast<std::int32_t>(Read(source,8))>=minimum) return Read(manager,8);
    if (!hooks.allocateStream) throw std::logic_error("DataManager 임시 stream 실제 할당 미연결");const auto allocated=hooks.allocateStream(60);
    if (allocated) ConstructDataManagerStream(allocated,kTemporaryName,minimum,hooks);auto target=Node(hooks,allocated);
    const auto length=Read(source,8);WriteMemStream(target,Buffer(hooks,Read(source,0),0,length),length,hooks.memory);
    const auto remaining=Read(target,12)-Read(target,4);auto tail=Buffer(hooks,Read(target,0),Read(target,4),remaining);std::fill(tail.begin(),tail.end(),std::uint8_t{});
    const auto end=Read(target,12);Put(target,4,end);if (std::bit_cast<std::int32_t>(Read(target,8))<std::bit_cast<std::int32_t>(end)) Put(target,8,end);
    AppendDataManagerStream(manager,allocated,hooks.list);RenameDataManagerStream(manager,name,kTemporaryName,hooks);return Read(manager,8);
}
}
