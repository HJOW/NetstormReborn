// 같은 SNART 물리표를 제자리 변환하고 현재 이름의 시작 포인터를 빌린다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
namespace netstorm::o { // 원문004a8800/004a8850의 공유 mutable 이름표 도구다.
using SnartSeparatorAssertion=std::function<void()>; // 잘못된 첫 separator의 Snart.cpp:0x46 진단이며 복귀하면 계속한다.
std::span<std::uint8_t> SelectSnartName(std::span<std::uint8_t> names,std::uint32_t index,const SnartSeparatorAssertion& assertion); // 현재 표의 이름 시작부터 같은 물리 끝까지 반환한다. 길이를 미리 읽지 않는다.
std::string_view ReadSnartName(std::span<std::uint8_t> names,std::uint32_t index,const SnartSeparatorAssertion& assertion); // 기존 파일 이미지 reader처럼 현재 첫NUL까지 읽는 편의 경계다.
}
