// 원문 언어 번호/옵션 prefix와 같은 실제 Client의 동기 설정 변경을 창 없이 검사한다.
#include "TestSupport.h"
#include "client/LanguageOptions.h"
#include "client/ClientMain.h"
#include <bit>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
using namespace netstorm::client;
namespace { //원시 fixture의 문자열/부분효과를 그대로 읽는다.
const auto kLanguageRoot=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path(); //보호 입력/작업 출력을 구분하는 저장소다.
std::string LanguageDecode(std::string_view hex) { //각 BYTE를 첫NUL 뒤까지 복원한다.
    std::string result;if (hex=="-") return result;
    for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)); //ASCII 정규화를 하지 않는다.
    return result;
}
std::string LanguageHex(std::string_view text) { //용어표 장치가 받은 실제 이름을 동일 형식으로 관측한다.
    constexpr std::string_view kDigits="0123456789abcdef";std::string result; //원시 BYTE를 소문자 두 자리로 기록한다.
    for (const unsigned char byte:text) { result+=kDigits[byte>>4];result+=kDigits[byte&15U]; } //빈 이름은 명시 '-'다.
    return result.empty() ? "-" : result;
}
}
// 4096 LANGID/13이름/546옵션 입력의 실제 CPU 관측과 결과·두 load의 현재 전역을 대조한다.
TEST_CASE(LanguageOptions_IndependentNative4655) {
    std::ifstream file(kLanguageRoot/"cpppj/tests/fixtures/language-options-x86.tsv");CHECK(file.good());std::string line;std::size_t count{};
    while (std::getline(file,line)) { //기대값은 제품 구현 대신 고정 원문 관측에서 읽는다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);char kind{};row>>kind;
        if (kind=='D') { std::uint32_t id{},result{};row>>id>>result;CHECK(DetectOriginalLanguage(static_cast<std::uint16_t>(id))==result); }
        else if (kind=='F') { std::string hex;std::uint32_t result{};row>>hex>>result;CHECK(FindOriginalLanguage(LanguageDecode(hex))==result); }
        else {
            CHECK(kind=='O');std::uint32_t detected{},before{},shown{},changed{},after{},published{};std::string hex,wanted;row>>detected>>before>>shown>>hex>>changed>>after>>published>>wanted;
            auto current=std::bit_cast<std::int32_t>(before),view=std::bit_cast<std::int32_t>(shown);std::string trace;const auto raw=LanguageDecode(hex);
            const auto result=ApplyOriginalLanguageOption(detected,current,view,{[&] { return raw; },[&](auto name) { if (!trace.empty()) trace+=';';trace+=LanguageHex(name)+','+std::to_string(std::bit_cast<std::uint32_t>(current))+','+std::to_string(std::bit_cast<std::uint32_t>(view)); }});
            CHECK(result==(changed!=0) && std::bit_cast<std::uint32_t>(current)==after && std::bit_cast<std::uint32_t>(view)==published && (trace.empty() ? "-" : trace)==wanted);
        }
        CHECK(!row.fail());++count;
    }
    CHECK(count==4655);
}
// 첫 load 뒤 callback이 현재 값을 바꾸거나 두 번째 load가 실패해도 원문 부분효과를 되돌리지 않는다.
TEST_CASE(LanguageOptions_CurrentWritesReloadOrderAndFailures) {
    std::int32_t current=-1,published=-1;std::size_t loads{};
    CHECK(ApplyOriginalLanguageOption(0,current,published,{[] { return "german"; },[&](auto name) { CHECK(name=="german" && published==-1);++loads;if (loads==1) { CHECK(current==3);current=6; }else CHECK(current==6); }}));CHECK(current==6 && published==3 && loads==2);
    current=-1;published=7;loads=0;bool failed{};
    try { ApplyOriginalLanguageOption(0,current,published,{[] { return "french"; },[&](auto) { if (++loads==2) throw std::runtime_error("load"); }}); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && current==2 && published==7 && loads==2);
    CHECK(!ApplyOriginalLanguageOption(0,current,published,{[] { return "french"; },{}}) && published==7); //원문 current가 같으면 실패한 두 번째 읽기도 임의 재시도하지 않는다.
}
// 실제 Config/LanguageSpec/Client callback을 사용하며 파일/창/월드/네트워크는 만들지 않는다.
TEST_CASE(LanguageOptions_ActualClientConfigAndScenarioPath) {
    ClientOptions options;options.gameDirectory=kLanguageRoot/"originals";options.noAudio=true;Client client(options);auto& config=client.Configuration();
    config.Configuration().AppendArguments("LanguageSpec=\"language.{LOCAL.1}\";missionSpec=\"D/{LOCAL.1}.{languageTag}\";currentLanguage=german",{});
    client.InterpretLanguageOptions();CHECK(client.LanguageNumber()==3);
    config.Set("currentLanguage","french");CHECK(client.LanguageNumber()==2);config.Set("currentLanguage","French");CHECK(client.LanguageNumber()==0); //unknown 대소문자는 이전 프랑스어 번호를 유지하지 않는다.
    config.Set("currentLanguage","portuguese");CHECK(client.LanguageNumber()==6);config.Set("currentLanguage","");CHECK(client.LanguageNumber()==0);
    config.Set("languageTag","french");CHECK(config.PathSpec("missionSpec","TEST02")=="D/TEST02.french"); //UI 선택과 사용자가 명시한 시나리오 spec을 자동으로 덮지 않는다.
}
