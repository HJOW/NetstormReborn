// 원본이 보존하는 파일명 뒤 바이트와 헤더 부가 필드를 임의로 초기화하지 않는다.
#include "o/RawTemplateLoad.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::size_t kManagerBytes=276,kHeaderBytes=20,kNameOffset=12,kNameBytes=256; // 10.78 manager/Template/파일명 배열의 실제 폭이다.
// 원본의 비정렬 DWORD를 같은 저장소에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    // 네 BYTE만 낮은 비트부터 합친다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(8*index);return value;
}
// 원본이 쓰는 지정 DWORD만 갱신한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 주변 필드를 보존하고 네 BYTE에만 쓴다.
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 선택한 효과가 없는 경우 성공으로 채우지 않고 그 호출 지점에서 거부한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Template Load의 실제 하위 효과 미연결");return function; }
}

// 원본 생성자에서 쓰지 않는 파일명 뒤 공간과 manager 밖 바이트는 보존한다.
void ConstructTemplateDataManager(std::span<std::uint8_t> manager,std::string_view fileName,std::span<std::uint8_t,4> global) {
    const auto nul=fileName.find('\0'),length=nul==std::string_view::npos ? fileName.size() : nul;
    if (manager.size()<kManagerBytes || length>=kNameBytes) throw std::invalid_argument("Template 생성자의 실제 manager/파일명 범위");
    Put(manager,4,0);Put(manager,8,0);Put(manager,0,0x514690U);Put(manager,0x10c,0);Put(manager,0x110,0xffffffffU);Put(global,0,0);
    // 입력 C 문자열의 첫 NUL까지 한 BYTE씩 순서대로 복사한다.
    for (std::size_t index=0;index<=length;++index) manager[kNameOffset+index]=index==length ? 0 : static_cast<std::uint8_t>(fileName[index]);
}

// 파일명은 별도 metadata 문자열 대신 실제 manager 배열에서 빌린다.
std::string_view TemplateDataManagerFileName(std::span<const std::uint8_t> manager) {
    if (manager.size()<kManagerBytes) throw std::invalid_argument("Template 파일명 getter의 manager276 누락");
    const auto bytes=manager.subspan(kNameOffset,kNameBytes);const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});
    if (end==bytes.end()) throw std::out_of_range("Template 파일명의 실제 NUL 누락");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}

// 하위 효과 뒤 원본이 다시 읽는 ptr/readonly/current를 같은 헤더에서 재조회한다.
void LoadTemplateFile(std::span<std::uint8_t> header,std::string_view fileName,const TemplateLoadHooks& hooks) {
    if (header.size()!=kHeaderBytes) throw std::invalid_argument("Template Load의 실제20BYTE 헤더 누락");
    const auto old=Read(header,0);
    if (old) { const auto name=Need(hooks.fileName)(old);if (!Need(hooks.sameName)(name,fileName)) Need(hooks.wrongName)();return; }
    Put(header,4,0);auto manager=Need(hooks.allocate)(kManagerBytes);if (manager) manager=Need(hooks.construct)(manager,fileName);Put(header,0,manager);
    const bool readOnly=Read(header,8)!=0 || Need(hooks.readOnly)(Need(hooks.fileName)(manager));Put(header,8,readOnly ? 1U : 0U);Put(header,16,0);
    if (Need(hooks.readFile)(Read(header,0))) { if (Read(header,0) && Need(hooks.isCurrent)()) Need(hooks.refreshTitle)();return; }
    const auto failed=Read(header,0);if (failed) { Need(hooks.lifetime.destroy)(failed);Need(hooks.lifetime.release)(failed); }Put(header,0,0);
}
}
