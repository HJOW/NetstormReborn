// 같은manager의가상파일명/이름표와실제current검색을보존하는원문0044cf20이다.
#pragma once
#include "o/RawDataManagerSelection.h"
#include "o/RawSnart.h"
#include <optional>

namespace netstorm::o { // 클라이언트로그장치와공용DataManager의실제물리를분리한다.
struct DataManagerDumpHooks { // 가상호출과출력만외부경계이며목록검색/끝합계는원문부모가수행한다.
    std::function<std::string_view(std::span<std::uint8_t>)> fileName; // 가상+0의현재NUL파일명을빌린다.첫trace전에호출한다.
    std::function<std::span<std::uint8_t>(std::span<std::uint8_t>)> names; // 첫trace뒤가상+4를한번호출해같은mutableSNART표포인터를고정한다.
    DataManagerSelectionHooks selection; // 같은실제60BYTE노드와CRT비교를FindDataManagerSection에제공한다.버퍼내용/pos는읽지않는다.
    SnartSeparatorAssertion separatorAssertion; // 실제SNART변환의잘못된첫separator진단이다.선택하지않으면필요없다.
    std::function<void(std::int32_t,std::string_view)> trace; // level29의완성문자열을같은실제공유trace부모로전달한다.필터는장치가판정한다.
};
// 0044cf20: 파일로그→가상이름표→각이름검색/누락로그→signed표시합계로그다.전체manager는276BYTE이상이며prefix nullopt는원문NULL의빈문자열이다.
// 합계는end의하위DWORD로감기며비교/로그예외앞의current·이름표변이를되돌리지않는다.근거:docs/exe/cpp-datamanager-dump-20261101.md.
void DumpDataManager(std::span<std::uint8_t> manager,std::optional<std::string_view> prefix,const DataManagerDumpHooks& hooks);
}
