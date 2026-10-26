// 원문미작성끝을0으로정의하지않고실제소유할당을한번만만든다.
#include "client/ClientTemplateSectionNames.h"
#include <array>
#include <new>
#include <stdexcept>
#include <utility>
namespace netstorm::client { // 같은owner는여러TemplateStorage와Session이공유할수있다.
ClientTemplateSectionNames::ClientTemplateSectionNames(TemplateSectionNamesAllocator allocate):allocate_(std::move(allocate)) { // 생성자는표할당/쓰기를하지않는다.
    if (!allocate_) allocate_=[](std::size_t size) { return std::unique_ptr<std::uint8_t[]>(new(std::nothrow) std::uint8_t[size]); }; // 괄호/중괄호값초기화없이512를빌린다.
}
std::span<std::uint8_t> ClientTemplateSectionNames::Names() { // 원문부모가현재token을공개한뒤같은할당에만쓴다.
    std::array<std::uint8_t,7> formatted{}; // 원문서식의정의된7BYTE만출력하며미정local256끝을빌리지않는다.
    o::TemplateSectionNamesHooks hooks;hooks.allocate=[this](auto size) { if (size!=o::kTemplateSectionNamesCapacity) throw std::logic_error("Client 이름표 실제512할당계약");bytes_=allocate_(size);return bytes_ ? 1U : 0U; };
    hooks.buffer=[this](auto token)->std::span<std::uint8_t> { if (token!=1 || !bytes_) throw std::out_of_range("Client 이름표 실제NULL할당접근");return {bytes_.get(),o::kTemplateSectionNamesCapacity}; };
    hooks.format=[&formatted](auto index)->std::span<const std::uint8_t> { if (index>=20) throw std::out_of_range("Template Terr원문서식0..19범위");formatted={'T','e','r','r',static_cast<std::uint8_t>('0'+index/10),static_cast<std::uint8_t>('0'+index%10),0};return formatted; }; // 실제선택범위에서는Terr%02d의ASCII/NUL과동일하다.
    const auto token=o::GetTemplateSectionNames(current_,hooks);return hooks.buffer(token);
}
std::uint32_t ClientTemplateSectionNames::CurrentToken() const { return current_; } // 같은현재포인터슬롯의읽기투영이다.
}
