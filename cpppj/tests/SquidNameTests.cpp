// 객체 가상 이름의 현재 언어/TYPE·치환/Factory 접미사 순서를 검사한다. 작성·컴파일만 한다.
#include "o/RawSquidName.h"
#include "RawSceneSupport.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 같은 실제 슬롯/TYPE와 현재 언어·설정 장치를 준비한다.
struct NameScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true};std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188);Sid parent{}; //현재원문/설명표/부모다.
    std::uint32_t language{1};SquidNameHooks hooks;std::vector<std::string> trace; //현재언어/선택효과/실제조회순서다.
    // 이름 하위의 효과만 명시하며 제품의 언어/가상 분기를 대신하지 않는다.
    NameScene() {
        parent=pool.Allocate(2);auto raw=pool.AllocatedBytes(parent);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x501dd0);raw[10]=85;types[85].description="raw";
        hooks.language=[this] { trace.push_back("language");return language; };hooks.translate=[this](std::string_view text) { trace.push_back("translate:"+std::string(text));return text==" Level %s" ? std::string(text) : "translated:"+std::string(text); };hooks.substitute=[this](std::string_view text) { trace.push_back("substitute");return "cfg:"+std::string(text); };hooks.level=[this](Sid sid) { CHECK(sid==parent);trace.push_back("level");return std::uint32_t(pool.Slot(sid)[11]>>5); };
    }
    // 같은 현재 원문과 선택 효과를 빌린다.
    RawSquidName Make() { return RawSquidName(pool,types,hooks); }
};
}
// 언어0은한번,1은두번조회하고다른언어일때만설명번역을선택한다.
TEST_CASE(SquidName_BaseLanguageReadOrderAndSubstitution) {
    NameScene input;input.language=0;CHECK(input.Make().Name(input.parent)=="cfg:raw" && input.trace==std::vector<std::string>({"language","substitute"}));input.trace.clear();input.language=1;CHECK(input.Make().Name(input.parent)=="cfg:raw" && input.trace==std::vector<std::string>({"language","language","substitute"}));input.trace.clear();input.language=2;CHECK(input.Make().Name(input.parent)=="cfg:translated:raw" && input.trace==std::vector<std::string>({"language","language","translate:raw","substitute"}));
}
// Factory는base치환뒤패턴을항상번역하고그효과뒤현재레벨을읽는다.
TEST_CASE(SquidName_FactorySuffixAfterBaseAndCurrentLevel) {
    NameScene input;Put(input.pool.AllocatedBytes(input.parent),0,0x509148);input.hooks.translate=[&](std::string_view text) { CHECK(text==" Level %s");input.trace.push_back("pattern");input.pool.AllocatedBytes(input.parent)[11]=64;return std::string(text); };CHECK(input.Make().Name(input.parent)=="cfg:raw Level III");CHECK(input.trace==std::vector<std::string>({"language","language","substitute","pattern","level"}));
}
// TYPE는언어조회전에선택하지만설명바이트는그효과뒤현재값을사용한다.NUL/버퍼오류도분리한다.
TEST_CASE(SquidName_CurrentTypeFieldAndSelectedBounds) {
    NameScene input;input.hooks.language=[&] { input.types[85].description=std::string("current\0ignored",15);input.pool.AllocatedBytes(input.parent)[10]=86;return 0U; };input.types[86].description="other";CHECK(input.Make().Name(input.parent)=="cfg:current");input.hooks.substitute=[](auto) { return std::string(80,'x'); };CHECK(Throws([&] { input.Make().Name(input.parent); }));Put(input.pool.AllocatedBytes(input.parent),0,0);input.hooks={};CHECK(Throws([&] { input.Make().Name(input.parent); }));
}
// 원본로마숫자/sentinel과실제%s/%%/폭서식을보존한다.
TEST_CASE(SquidName_EntriesRomanSentinelAndCrtSuffix) {
    CHECK(SquidNameEntry(0x501dd0)==0x4ace20 && SquidNameEntry(0x509148)==0x454d30 && SquidNameEntry(0x509210)==0x454d30);CHECK(Throws([] { SquidNameEntry(0); }));CHECK(SquidLevelName(0)=="I" && SquidLevelName(9)=="X" && SquidLevelName(10)==std::string(1,static_cast<char>(0xff)));NameScene input;Put(input.pool.AllocatedBytes(input.parent),0,0x509148);input.hooks.translate=[](auto) { return std::string(" %% [%4s]"); };CHECK(input.Make().Name(input.parent)=="cfg:raw % [   I]");input.hooks.translate=[](auto) { return std::string(256,'x'); };CHECK(Throws([&] { input.Make().Name(input.parent); }));
}
