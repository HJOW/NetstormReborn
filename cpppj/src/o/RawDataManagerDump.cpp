// 가상호출/trace뒤현재물리를다시읽고SNART이름표의실제종료BYTE를사용한다.
#include "o/RawDataManagerDump.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace netstorm::o { // 10.78의공용DataManager덤프몸체이며출력장치는호출자가공급한다.
namespace { // 고정DWORD읽기와원본C문자열/선택된장치계약만검사한다.
constexpr std::int32_t kDumpTraceLevel=29; // 원문0044cf20의모든출력이사용하는0x1d수준이다.
// 호출된실제경계가없으면그시점앞의물리변경을보존한채거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("DataManager덤프의선택된실제경계미연결");return function; }
// 이름/파일명/prefix는원본처럼현재첫NUL이전만사용한다.
std::string_view CString(std::string_view text) { return text.substr(0,text.find('\0')); }
// 선택한SNART이름포인터에서현재첫NUL까지읽으며저장소밖원본읽기는거부한다.
std::string_view Name(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("DataManager덤프의현재이름NUL누락");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
// 실제stream+8의네BYTE만읽고data/pos/alloc과인접이름은보존한다.
std::uint32_t End(std::span<const std::uint8_t> stream) {
    if (stream.size()<60) throw std::out_of_range("DataManager덤프의실제60BYTE노드누락");std::uint32_t value{};
    // 같은네BYTE를원문little endian순서로조합한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(stream[8+index])<<(index*8);return value;
}
// 고정서식%04d의부호/최소폭만CRT로변환하며게임formatter의다른서식을추가하지않는다.
std::string Total(std::uint32_t value) {
    std::array<char,32> bytes{};const auto length=std::snprintf(bytes.data(),bytes.size(),"%04d",std::bit_cast<std::int32_t>(value));
    if (length<0 || static_cast<std::size_t>(length)>=bytes.size()) throw std::out_of_range("DataManager덤프의고정signed서식범위");return {bytes.data(),static_cast<std::size_t>(length)};
}
}
// 이름표포인터는한번만캡처하되각검색/trace뒤같은표의앞으로진행할이름내용은다시읽는다.
void DumpDataManager(std::span<std::uint8_t> manager,std::optional<std::string_view> prefix,const DataManagerDumpHooks& hooks) {
    if (manager.size()<276) throw std::invalid_argument("DataManager덤프의실제276BYTE manager누락");
    const auto file=Need(hooks.fileName)(manager);Need(hooks.trace)(kDumpTraceLevel,"File "+std::string(CString(file))+": "+std::string(CString(prefix.value_or(std::string_view{})))+"\n");
    auto names=Need(hooks.names)(manager);std::uint32_t total{},index{};
    // 원문은이름표의현재첫BYTE가0일때종료하며정상순회중에는노드end합계의DWORD감김을유지한다.
    for (;;) {
        const auto selected=SelectSnartName(names,index,hooks.separatorAssertion);if (!selected[0]) { Need(hooks.trace)(kDumpTraceLevel,Total(total)+" bytes total.\n");return; }
        const auto current=FindDataManagerSection(manager,Name(selected),hooks.selection);
        if (current) total+=End(Need(hooks.selection.stream)(current));
        else Need(hooks.trace)(kDumpTraceLevel,"ERROR: Missing section "+std::string(Name(selected))+"\n"); //비교뒤같은이름포인터의현재BYTE를로그인수로다시읽는다.
        ++index;
    }
}
}
