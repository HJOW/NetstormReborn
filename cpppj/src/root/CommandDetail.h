// 실제87h의 signed 숫자/모듈러 페이지 이동과 대문자X 쌍 입력을 공유한다.
#pragma once
#include "root/ChallengeDetail.h"
#include "root/CommandPages.h"
#include "root/SharedFiles.h"

namespace netstorm::root::command_detail { // 원문 CRT/필드·현재 페이지 치환을 위한 내부 부품이다.
inline std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // signed %d/원문 CMP의 같은DWORD를 보존한다.
inline std::int32_t Add(std::int32_t value,std::int32_t amount) { return Signed(std::bit_cast<std::uint32_t>(value)+std::bit_cast<std::uint32_t>(amount)); } // x86 덧셈/감산의 하위DWORD를 사용해 signed overflow를 피한다.
inline std::optional<std::pair<std::int32_t,std::int32_t>> Pair(const std::string& text) { const auto split=text.find('X');if (split==text.npos) return std::nullopt;return std::pair{std::atoi(text.c_str()),std::atoi(text.c_str()+split+1)}; } //44b7d0의 대문자X가 없으면 count0,있으면 빈항목도0/첫두개만사용한다. 세번째 이후X는 atoi가 멈추므로 무시된다.
inline void Navigation(std::string& page,std::int32_t first,std::int32_t last,std::uint32_t count,std::string_view rows) {
    CommandPages::ReplaceInteger(page,"{START}",first);CommandPages::ReplaceInteger(page,"{END}",last);CommandPages::ReplaceInteger(page,"{AMOUNT}",6);CommandPages::ReplaceInteger(page,"{MAXGUILDS}",Signed(count));CommandPages::ReplaceInteger(page,"{BACKSTART}",Add(first,-6));CommandPages::ReplaceInteger(page,"{BACKEND}",Add(last,-6));CommandPages::ReplaceInteger(page,"{NEXTSTART}",Add(first,6));CommandPages::ReplaceInteger(page,"{NEXTEND}",Add(last,6));CommandPages::Replace(page,"{CURRENTLYOPEN}",rows);
} //453479/45391e의 실제 치환 순서와6개 단위 페이지 이동이다.
}
