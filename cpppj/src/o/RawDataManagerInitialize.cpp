// 초기화 앞/중간의 외부 효과를 되돌리거나 표를 사전 문자열 목록으로 복사하지 않는다.
#include "o/RawDataManagerInitialize.h"
#include <stdexcept>
namespace netstorm::o { // 같은 rawmanager와 mutable표의 원문 부모다.
namespace { // 선택한 하위와 고정DWORD 읽기만 확인한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("DataManager 초기화의 선택된 실제 하위 미연결");return function; } // 도달한 경계만 요구한다.
std::uint32_t Root(std::span<const std::uint8_t> manager) { // 현재 고정this의+4DWORD를 읽는다.
    if (manager.size()<8) throw std::out_of_range("DataManager 초기화의 실제 root 물리 범위");std::uint32_t value{};
    // 현재 네 BYTE를 little endian으로만 합친다.
    for (std::size_t at=0;at<4;++at) value|=std::uint32_t(manager[4+at])<<(at*8);return value;
}
}
void InitializeDataManager(std::span<std::uint8_t> manager,const DataManagerInitializeHooks& hooks) { // 진단이 owner/표를 바꿔도 실제 그 뒤 위치에서 조회한다.
    if (Root(manager)) Need(hooks.assertion)(DataManagerInitializeAssertion::RootNotEmpty);auto names=Need(hooks.names)();
    // 반환표 포인터는 고정하되 미래 이름의 내용은 각Create 뒤 같은 표에서 다시 읽는다.
    for (std::uint32_t index=0;;++index) { const auto name=SelectSnartName(names,index,[&hooks] { Need(hooks.assertion)(DataManagerInitializeAssertion::NameSeparator); });if (!name[0]) return;Need(hooks.create)(name); }
}
}
