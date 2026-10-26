// Client수명의005c8aa4표한개를파일reader와신규manager초기화가같이빌린다.
#pragma once
#include "o/RawTemplateSectionNames.h"
#include <memory>
namespace netstorm::client { // Session/Storage교체와독립인원문lazy owner다.
using TemplateSectionNamesAllocator=std::function<std::unique_ptr<std::uint8_t[]>(std::size_t)>; // 실제할당또는독립검사의알려진BYTE할당장치다.
class ClientTemplateSectionNames { // 이름표의포인터/변환상태를같은고정주소에보관한다.
public:
    explicit ClientTemplateSectionNames(TemplateSectionNamesAllocator allocate={}); // 할당은아직하지않으며최초Names에서만원문부모를실행한다.
    ClientTemplateSectionNames(const ClientTemplateSectionNames&)=delete; // 살아있는이름포인터와변환상태를복제하지않는다.
    ClientTemplateSectionNames& operator=(const ClientTemplateSectionNames&)=delete; // 기존소비자의alias를다른할당으로바꾸지않는다.
    ClientTemplateSectionNames(ClientTemplateSectionNames&&)=delete; // 고정Client주소를유지한다.
    ClientTemplateSectionNames& operator=(ClientTemplateSectionNames&&)=delete; // 빌린포인터를이동후이전owner에남기지않는다.
    std::span<std::uint8_t> Names(); // 실제512BYTE의동일포인터를lazy생성/반환한다.235BYTE미정끝을게임규칙으로읽지않는다.
    std::uint32_t CurrentToken() const; // 최초0/성공1인호스트불투명token만읽는다.실제주소를DWORD로자르지않는다.
private:
    TemplateSectionNamesAllocator allocate_; // 최초선택시에만실행하는실제또는검사allocator다.
    std::unique_ptr<std::uint8_t[]> bytes_; // 값초기화없는실제512BYTE이며원문이쓴277BYTE만정의한다.
    std::uint32_t current_{}; // 새Client의BSS0이며Session정리에서reset하지않는다.
};
}
