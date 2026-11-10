// 원문 0054db38 helpWord 캐시(441300/4413c0)의 NULL/빈 값/대소문자/교체 규칙을 게임 없이 확인한다.
#include "TestSupport.h"
#include "client/ClientHelpWord.h"
#include "client/ClientLocalStateGlobals.h"
#include "o/OriginalText.h"
using namespace netstorm::client;

// 설정 키가 없거나 빈 문자열이면 최초 NULL 캐시는 NULL로 남고 반환은 false다.
TEST_CASE(ClientHelpWord_MissingOrEmptyKeepsNull) {
    ClientHelpWord cache;CHECK(!cache.Get().has_value() && !cache.Interpret(std::nullopt) && !cache.Interpret(std::string()) && !cache.Get().has_value() && cache.Generation()==0);
}
// 이미 비NULL이면 키 부재/빈 값은 같은 문자열과 정체성(generation)을 유지한다.
TEST_CASE(ClientHelpWord_MissingOrEmptyKeepsExisting) {
    ClientHelpWord cache;CHECK(cache.Interpret(std::string("Help")) && cache.Generation()==1);
    CHECK(!cache.Interpret(std::nullopt) && !cache.Interpret(std::string()) && cache.Get() && *cache.Get()=="Help" && cache.Generation()==1);
}
// 대소문자만 다르면 옛 표기를 유지하고 성공(true)으로 끝난다.
TEST_CASE(ClientHelpWord_CaseOnlyDifferenceKeepsOldSpelling) {
    ClientHelpWord cache;cache.Interpret(std::string("Help"));CHECK(cache.Interpret(std::string("hELP")) && *cache.Get()=="Help" && cache.Generation()==1);
}
// 다른 문자열이면 교체하고 교체 횟수를 늘린다.
TEST_CASE(ClientHelpWord_DifferentTextReplacesAndCounts) {
    ClientHelpWord cache;cache.Interpret(std::string("Help"));CHECK(cache.Interpret(std::string("Info")) && *cache.Get()=="Info" && cache.Generation()==2);
}
// 외부에서 설치한 비영 빈 문자열은 NULL과 구별되며 비어 있는 설정값이 그것을 NULL로 합치지 않는다.
TEST_CASE(ClientHelpWord_NonNullEmptyIsDistinctFromNull) {
    ClientHelpWord cache;CHECK(cache.Interpret(std::string("x")));CHECK(!cache.Interpret(std::string()) && cache.Get().has_value());
}
// 한글 표시 바이트는 ASCII 대소문자 접기를 적용하지 않고 정확히 같을 때만 같은 문구로 본다.
TEST_CASE(ClientHelpWord_KoreanDisplayBytesCompareExactly) {
    // UTF-8 "정보"(U+C815 U+BCF4)의 원문 바이트를 표시 바이트로 부호화한다.
    const auto bytes=netstorm::o::EncodeDisplayText("\xEC\xA0\x95\xEB\xB3\xB4");const std::string korean(bytes.begin(),bytes.end());
    ClientHelpWord cache;CHECK(cache.Interpret(korean) && cache.Get() && *cache.Get()==korean && cache.Generation()==1);
    CHECK(cache.Interpret(korean) && cache.Generation()==1); // 같은 바이트는 옛 할당을 유지한다.
    CHECK(ClientHelpWord::EqualsIgnoreCase("ABC","abc") && !ClientHelpWord::EqualsIgnoreCase("abc","abd") && !ClientHelpWord::EqualsIgnoreCase("ab","abc"));
}
// 상태 전역의 캐시 주소는 호출마다 같고 소비자가 읽는 값과 옵션 해석이 쓰는 값이 같은 객체다.
TEST_CASE(ClientHelpWord_LocalStateGlobalsSharesOneCache) {
    ClientLocalStateGlobals globals;auto* const first=&globals.HelpWord();CHECK(first==&globals.HelpWord() && !globals.HelpWord().Get());
    first->Interpret(std::string("Help"));const ClientLocalStateGlobals& view=globals;CHECK(view.HelpWord().Get() && *view.HelpWord().Get()=="Help");
}
