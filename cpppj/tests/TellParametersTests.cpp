// 원본Tell부모1070관측과같은공개파서/실제param Config수명을대조한다.
#include "RawSceneSupport.h"
#include "client/TellParameters.h"
#include "o/ConfigInterface.h"
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// fixture의바이트를UTF-8/현재로케일변환없이되돌린다.
std::string DecodeTellHex(std::string_view hex) {
    if (hex.size()%2) throw std::logic_error("Tell관측hex길이");std::string result; // 원본바이트저장소다.
    // 각두자리hex를원본한BYTE로읽는다.
    for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16));return result;
}
// 문자열의첫NUL뒤를보존한입력과C문자열경계의관측을구별한다.
std::string EncodedTell(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 현재Config치환/Append장치만원문과동일한명시프로필로공급한다.
struct TellParameterScene {
    std::uint32_t profile{}; // 기대결과가아닌명시Config치환경계입력이다.
    std::map<std::uint32_t,std::string> values; // 앞선Append가후속치환에노출하는같은명시현재값이다.
    std::string trace; // 실제선택된경계호출순서다.
    // 실제치환/Append효과를한번기록한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 원문도구의현재외부Config반환값만공급한다.
    TellParameterHooks Hooks() {
        TellParameterHooks hooks;hooks.substitute=[&](std::string_view text) {
            Add("S,"+EncodedTell(text));if (profile==1) return '['+std::string(text)+']';
            if (profile==2) return text=="{param.1}" ? values[1] : std::string(text);
            if (profile==3) return text=="EMPTY" ? std::string() : "Q\";=\n"+std::string(text);
            if (profile==4) return std::string("N\0TAIL",6);return std::string(text);
        };
        hooks.append=[&](std::string_view line) {
            Add("A,"+EncodedTell(line));const auto equal=line.find('=');if (equal==line.npos || equal+3>line.size()) throw std::logic_error("명시TellAppend서식"); // 원문printf가완성한문자열만읽는다.
            values[static_cast<std::uint32_t>(std::stoul(std::string(line.substr(0,equal))))]=std::string(line.substr(equal+2,line.size()-equal-3));
        };return hooks;
    }
    // 실제공개부모가반환한이름과전체효과만직렬화한다.
    std::string Run(std::string_view source) { const auto name=ParseTellParameters(source,Hooks());return EncodedTell(name)+'|'+trace; }
};
}
// 부모/실제토큰/CRT의모든1070관측과전체바이트/호출순서를비교한다.
TEST_CASE(TellParameters_Native1070) {
    const auto rows=LoadFixture(NETSTORM_TELL_PARAMETERS_FIXTURE).rows;CHECK(rows.size()==1070);
    // 각행은새장치와실제공개제품몸체로독립실행한다.
    for (const auto& row:rows) { CHECK(row.size()==4);TellParameterScene scene;scene.profile=Number(row[2]);CHECK(scene.Run(DecodeTellHex(row[1]))==row[3]); }
}
// 같은Registry의지역param을본문기간에유지하고이전인자가후속치환에보이는지검사한다.
TEST_CASE(TellParameters_ActualConfigLifetimeAndSequentialSubstitution) {
    netstorm::o::ConfigInterface configuration([](std::string_view)->std::optional<std::vector<std::uint8_t>> { return std::nullopt; });configuration.Configuration().Assign("width=800\n"); // 실제전역Config를유효하게한다.
    netstorm::o::Config outer(configuration.Registry(),"param");outer.Append("1=\"outer\"");const auto before=configuration.Registry().Objects().size(); // 바깥같은이름객체를지역Tell이일시가린다.
    {
        netstorm::o::Config parameters(configuration.Registry(),"param");const auto name=ParseTellParameters("SETSIZE({width},{param.1})",{
            [&](std::string_view text) { return configuration.SubstituteLocal(text); },[&](std::string_view line) { parameters.Append(line); }}); // 실제00441740과같은Append본문을연결한다.
        CHECK(name=="SETSIZE" && parameters.GetRaw("1")=="800" && parameters.GetRaw("2")=="800");CHECK(configuration.SubstituteLocal("{param.1},{param.2}")=="800,800");
        CHECK(configuration.Registry().Objects().size()==before+1); // 임시local은각치환뒤반납되고현재param만본문동안유지된다.
    }
    CHECK(configuration.Registry().Objects().size()==before && configuration.SubstituteLocal("{param.1}")=="outer");
}
// 선택하지않은의존은요구하지않고원본지역배열범위를넘는무효복사를쓰기전에거부한다.
TEST_CASE(TellParameters_MissingEffectsAndPhysicalLimit) {
    CHECK(ParseTellParameters("About",{})=="About" && ParseTellParameters("Foo(\"\",next)",{})=="Foo");bool missing=false,oversize=false; // 두독립실패경계다.
    try { static_cast<void>(ParseTellParameters("Foo(value)",{})); }catch (const std::logic_error&) { missing=true; }
    try { static_cast<void>(ParseTellParameters(std::string(1024,'A'),{})); }catch (const std::length_error&) { oversize=true; }
    CHECK(missing && oversize);std::string nul="About";nul+='\0';nul+=std::string(1024,'X');CHECK(ParseTellParameters(nul,{})=="About");
}
