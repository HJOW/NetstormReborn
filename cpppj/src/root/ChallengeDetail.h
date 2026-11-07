// CCh의 비정렬 실제 필드와 allocator/미작성 바이트를 공유한다.
#pragma once
#include "root/ChallengeRuntime.h"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace netstorm::root::challenge_detail { // 원문 바이트 작성의 내부 경계이며 별도 상태 사본이 없다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("R CCh의 선택된 실제 상위 장치 미연결");return function; } // 선택한 실제 장치만 요구한다.
inline std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("R CCh 필드 읽기 물리 범위");std::uint32_t result{};
    for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[at+byte])<<(byte*8);return result; // 낮은 BYTE부터 실제 x86 폭으로 읽는다.
} // 원문 비정렬 BYTE/WORD/DWORD 조회다.
inline void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("R CCh 필드 쓰기 물리 범위");
    for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 지정한 실제 폭만 쓴다.
} // 그 외 scratch/기존 레코드 바이트는 덮지 않는다.
inline std::string Text(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>=bytes.size()) throw std::out_of_range("R CCh 문자열 시작 범위");const auto first=bytes.begin()+at,end=std::find(first,bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("R CCh 문자열의 실제 NUL 없음");return {reinterpret_cast<const char*>(bytes.data()+at),static_cast<std::size_t>(end-first)};
} // 실제 물리 안의 첫NUL까지 소유 복사한다.
inline void Copy(std::span<std::uint8_t> to,std::size_t at,std::span<const std::uint8_t> from) {
    if (at>to.size() || from.size()>to.size()-at) throw std::out_of_range("R CCh 실제 복사 범위");std::memmove(to.data()+at,from.data(),from.size());
} // 원문 겹침 가능한 실제 바이트 복사다.
inline void PutText(std::span<std::uint8_t> bytes,std::size_t at,std::string_view text) {
    const auto length=text.find('\0');text=text.substr(0,length);Copy(bytes,at,{reinterpret_cast<const std::uint8_t*>(text.data()),text.size()});Put(bytes,at+text.size(),0,1);
} // C문자열의 NUL까지 쓰고 뒤쪽 기존 바이트는 보존한다.
inline void Header(std::span<std::uint8_t> bytes,std::uint8_t type,std::uint16_t length) { Put(bytes,0,1,1);Put(bytes,1,type,1);Put(bytes,2,length,2); } //444ba0은 version&7f를 전체 BYTE에 쓰며 이전 highbit를 보존하지 않는다.
inline std::vector<std::uint8_t> Packet(std::uint8_t type,std::size_t capacity,std::uint16_t length=4) { std::vector<std::uint8_t> bytes(capacity,0xff);Header(bytes,type,length);return bytes; } //00482970의 실제ff allocator/최소4 header다.
inline void AllocationInput(std::span<std::uint8_t> bytes) { void* allocation=std::malloc(bytes.size());if (!allocation) throw std::bad_alloc();std::memcpy(bytes.data(),allocation,bytes.size());std::free(allocation); } // 스택 미작성 부분은 현재 CRT 입력으로 보존하며 원본 스택 바이트 자체 일치를 주장하지 않는다.
}
