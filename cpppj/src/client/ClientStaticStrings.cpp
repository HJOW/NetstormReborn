// 실제 고리/CRT 장치이며 Editor의 예약30을 문자열 한도로 오해하지 않는다.
#include "client/ClientStaticStrings.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <limits>
#include <new>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

namespace netstorm::client { // 소유자는 모든 동기 callback과 빌린 문자열보다 오래 살아야 한다.
o::StaticStringGlobals ClientStaticStrings::Globals() { return {size_,base_,current_,last_}; } // 단일 실제 슬롯의 참조다.
void ClientStaticStrings::Initialize(std::uint32_t size) { o::InitializeStaticStrings(size,Globals(),Hooks()); } // 실제 시작 호출만 고정 크기를 결정한다.
std::uint32_t ClientStaticStrings::Allocate(std::uint32_t size) {
    const auto next=std::uint64_t(nextBase_)+size+4096U;if (next>std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("임시 고리 주소 토큰 소진");
    std::unique_ptr<std::uint8_t[]> bytes(new(std::nothrow) std::uint8_t[size]);if (!bytes) return 0;const auto result=nextBase_;allocations_.emplace(result,Allocation{std::move(bytes),size});nextBase_=static_cast<std::uint32_t>(next);return result;
} // Raw 초기화가 실제 작성하기 전에는 새 할당을 임의0채움하지 않는다.
std::span<std::uint8_t> ClientStaticStrings::Tail(std::uint32_t address) {
    auto found=allocations_.upper_bound(address);if (found==allocations_.begin()) throw std::out_of_range("임시 고리 실제 할당 없음");--found;const auto offset=address-found->first;if (offset>found->second.size) throw std::out_of_range("임시 고리 실제 주소 범위");return {found->second.bytes.get()+offset,found->second.size-offset};
} // 주소 토큰의 같은 현재 물리만 반환한다.
std::span<std::uint8_t> ClientStaticStrings::Memory(std::uint32_t address,std::uint32_t size) { auto bytes=Tail(address);if (size>bytes.size()) throw std::out_of_range("임시 고리 원문 접근 물리 폭");return bytes.first(size); } // store 이전의 주소/폭 검사다.
o::StaticStringHooks ClientStaticStrings::Hooks() {
    o::StaticStringHooks hooks;hooks.allocate=[this](auto size) { return Allocate(size); };hooks.memory=[this](auto address,auto size) { return Memory(address,size); };hooks.assertion=[](auto line) { std::clog<<"Staticstring 원문 진단:"<<line<<'\n'; };return hooks;
} // 실제 CRT 서식만 선택한 호출자가 추가한다.
std::span<std::uint8_t> ClientStaticStrings::Reserve(std::uint32_t size) { const auto address=o::ReserveStaticString(size,Globals(),Hooks());return Tail(address); } // Native 결과주소부터 실제 끝까지 빌린다.
std::span<const std::uint8_t> ClientStaticStrings::FormatInteger(std::uint32_t size,std::span<const std::uint8_t> pattern,std::int32_t value) {
    std::uint32_t source=0x10000000U;const auto pointer=reinterpret_cast<std::uintptr_t>(pattern.data()); // 외부 번역 주소와 같은 실제 고리 안의 주소를 구별한다.
    for (const auto& [base,allocation]:allocations_) { const auto start=reinterpret_cast<std::uintptr_t>(allocation.bytes.get());if (pointer>=start && pointer-start<allocation.size) { source=base+static_cast<std::uint32_t>(pointer-start);break; } } // 같은 목적/서식 주소면 Raw 부모가 CRT를 생략한다.
    auto hooks=Hooks();hooks.format=[this,pattern,value](auto destination,auto) {
        const auto end=std::find(pattern.begin(),pattern.end(),std::uint8_t{});if (end==pattern.end()) throw std::out_of_range("임시 고리 정수 서식의 NUL 범위");const std::string text(reinterpret_cast<const char*>(pattern.data()),static_cast<std::size_t>(end-pattern.begin()));
        static const std::regex conversion(R"(%(?:%|[-+ #0]*[0-9]*(?:\.[0-9]*)?[diuoxXc]))");std::size_t offset{},arguments{}; // 값인자 하나의 정수/문자/리터럴%만 실제 CRT에 전달한다.
        while ((offset=text.find('%',offset))!=text.npos) { std::cmatch match;if (!std::regex_search(text.c_str()+offset,text.c_str()+text.size(),match,conversion,std::regex_constants::match_continuous)) throw std::logic_error("임시 고리 한 정수 외 CRT 서식 미연결");if (match.str()!="%%" && ++arguments>1) throw std::logic_error("임시 고리 실제 값인자 하나 초과");offset+=match.length(); } // 미공급 인자를 임의0이나 가짜 문자열로 채우지 않는다.
        const auto length=std::snprintf(nullptr,0,text.c_str(),value);if (length<0) throw std::runtime_error("임시 고리 실제 CRT 정수 서식 실패");std::vector<char> formatted(static_cast<std::size_t>(length)+1);std::snprintf(formatted.data(),formatted.size(),text.c_str(),value);
        for (std::size_t index=0;index<formatted.size();++index) Memory(destination+static_cast<std::uint32_t>(index),1)[0]=static_cast<std::uint8_t>(formatted[index]); // 예약 폭을 넘어도 같은 실제 할당 안에서는 NUL까지 작성하고 범위 중단의 앞선 쓰기를 보존한다.
    };
    return Tail(o::FormatStaticString(size,source,Globals(),hooks)); // wrapper가 캡처한 반환주소와 실제 고리 수명을 유지한다.
}
}
