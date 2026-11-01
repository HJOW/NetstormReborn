// 최초 다중 장부와 원문 파일 바이트·동일 키의 언어별 값을 창 없이 대조한다.
#include "TestSupport.h"
#include "o/RawXlat.h"
#include "o/OriginalText.h"
#include "client/ClientMain.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
using namespace netstorm::o;
using namespace netstorm::client;
namespace { //독립 CPU fixture의 기대 바이트만 읽는다.
const auto kXlatRoot=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path(); //보호 원본/이번 작업 출력을 구별하는 기준이다.
std::vector<std::uint8_t> XlatDecode(std::string_view hex) { //최초 관측의 첫NUL 뒤도 복원한다.
    std::vector<std::uint8_t> bytes;if (hex=="-") return bytes;
    for (std::size_t index=0;index<hex.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(index,2)),nullptr,16))); //각 두 글자가 한 BYTE다.
    return bytes;
}
std::string_view XlatView(std::span<const std::uint8_t> bytes) { return {reinterpret_cast<const char*>(bytes.data()),bytes.size()}; } //원문 바이트와 별도 UTF-8 변환을 섞지 않는다.
std::string XlatString(std::span<const std::uint8_t> bytes) { return std::string(XlatView(bytes)); } //부재 파일의 빈 기록에도 같은 문자열 표현을 사용한다.
std::string XlatHex(std::string_view text) { //시작 파일 경계의 실제 인자를 관측한다.
    constexpr std::string_view kDigits="0123456789abcdef";std::string result; //원문 로그와 파일 이름은 소문자hex로 고정한다.
    for (const unsigned char byte:text) { result+=kDigits[byte>>4];result+=kDigits[byte&15U]; } //문자 인코딩/대소문자를 정규화하지 않는다.
    return result;
}
}
// 실제 load/목록/strchr/검색 본체의 관측을 재생하고 원문 전체 파일 버퍼도 비교한다.
TEST_CASE(XlatRuntime_IndependentNativeBuffersAndMergedLookup) {
    std::ifstream file(kXlatRoot/"cpppj/tests/fixtures/xlat-runtime-x86.tsv");CHECK(file.good());std::unique_ptr<RawXlat> xlat;std::string line;std::size_t cases{},loads{},queries{},startup{};
    while (std::getline(file,line)) { //각 사례의 언어 적재 순서와 이후 조회를 그대로 보존한다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);char kind{};row>>kind;
        if (kind=='C') { std::size_t id{};row>>id;CHECK(id==cases++);xlat=std::make_unique<RawXlat>(); }
        else if (kind=='L') {
            std::uint32_t which{},returned{};std::size_t count{};std::string input,buffer;row>>which>>input>>returned>>buffer>>count;CHECK(returned==1 && xlat);const auto bytes=XlatDecode(input),expected=XlatDecode(buffer);xlat->Load(which,bytes);const auto actual=xlat->Buffer(which);CHECK(actual.size()==expected.size() && std::equal(actual.begin(),actual.end(),expected.begin()) && xlat->Size()==count);++loads;
        } else if (kind=='Q') {
            std::int32_t language{};std::string keyHex,resultHex;row>>language>>keyHex>>resultHex;const auto key=XlatDecode(keyHex),expected=XlatDecode(resultHex);auto actual=xlat->Translate(XlatView(key),language);actual=actual.substr(0,actual.find('\0'));CHECK(actual==XlatView(expected));++queries;
        } else {
            CHECK(kind=='I');std::uint32_t mask{};std::string wanted,trace;row>>mask>>wanted;RawXlat table;RawXlatStartupHooks hooks;
            hooks.exists=[&](auto path) { if (!trace.empty()) trace+=',';trace+="exists:"+XlatHex(path);const auto bit=path=="d/xlat.german" ? 0U : path=="d/xlat.english" ? 1U : 2U;return (mask&(1U<<bit))!=0; };
            hooks.read=[&](auto path) { trace+=",open:"+XlatHex(path)+",close";return std::vector<std::uint8_t>{0}; };hooks.missing=[&](auto) { trace+=",log:9"; };InitializeRawXlat(table,hooks);CHECK(trace==wanted);++startup;
        }
        CHECK(!row.fail());
    }
    CHECK(cases==43 && loads==45 && queries==1935 && startup==8);
}
// 파일 부재 뒤 다음 언어로 진행하고 읽기 실패 전 완료한 언어 표/원문 버퍼를 보존한다.
TEST_CASE(XlatRuntime_StartupReadFailurePreservesEarlierLanguages) {
    RawXlat table;std::vector<std::string> paths;RawXlatStartupHooks hooks;hooks.exists=[](auto) { return true; };
    hooks.read=[&](auto path)->std::vector<std::uint8_t> { paths.emplace_back(path);if (path=="d/xlat.english") throw std::runtime_error("read");const std::string source="*\nKey\n-\nGerman\n=\n";return {source.begin(),source.end()}; };
    bool failed{};try { InitializeRawXlat(table,hooks); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && paths.size()==2 && table.Translate("Key",3)=="German" && table.Translate("Key",6)=="Key" && !table.Buffer(3).empty() && table.Buffer(1).empty());
}
// 실제 보호 TARC의97,526BYTE 전체를 원문이 수정한 동일 크기 버퍼/고유 키 수와 대조한다.
TEST_CASE(XlatRuntime_IndependentActualTarcFiles) {
    std::ifstream file(kXlatRoot/"cpppj/tests/fixtures/xlat-assets-x86.tsv");CHECK(file.good());RawXlat xlat;std::string line;std::size_t loads{},queries{},total{};
    while (std::getline(file,line)) { //원시 자산과 기대 버퍼를 임의 정규화하지 않는다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);char kind{};row>>kind;
        if (kind=='C') { std::uint32_t id{};row>>id;CHECK(id==0); }
        else if (kind=='L') {
            std::uint32_t which{},returned{};std::size_t size{};std::string sourceHex,bufferHex;row>>which>>sourceHex>>returned>>bufferHex>>size;CHECK(returned==1);const auto source=XlatDecode(sourceHex),expected=XlatDecode(bufferHex);xlat.Load(which,source);const auto actual=xlat.Buffer(which);CHECK(actual.size()==expected.size() && std::equal(actual.begin(),actual.end(),expected.begin()) && xlat.Size()==size);total+=source.size();++loads;
        } else {
            CHECK(kind=='Q');std::int32_t language{};std::string keyHex,resultHex;row>>language>>keyHex>>resultHex;const auto key=XlatDecode(keyHex),expected=XlatDecode(resultHex);auto actual=xlat.Translate(XlatView(key),language);actual=actual.substr(0,actual.find('\0'));CHECK(actual==XlatView(expected));++queries;
        }
        CHECK(!row.fail());
    }
    CHECK(loads==3 && queries==40 && total==97526);
}
// 실제 TARC의 세 언어를 Client 수명에 읽고 선택 변경 뒤 파일을 다시 읽지 않는 번역 경로를 확인한다.
TEST_CASE(XlatRuntime_ActualClientArchiveAndLanguageChanges) {
    ClientOptions options;options.gameDirectory=kXlatRoot/"originals";options.noAudio=true;Client client(options);auto& config=client.Configuration();config.Configuration().AppendArguments("currentLanguage=german;LanguageSpec=\"d/language.{LOCAL.1}\"",{});client.InterpretLanguageOptions();
    CHECK(client.Translate("Loading")=="Loading");client.InitializeNativeTranslations();CHECK(client.Translate("Loading")=="Ladevorgang");
    config.Set("currentLanguage","portuguese");CHECK(client.LanguageNumber()==6 && client.Translate("Loading")=="los");const auto portuguese=client.Translate("Loading");
    config.Set("currentLanguage","french");CHECK(client.LanguageNumber()==2 && client.Translate("Loading")=="Loading"); //원문 시작은 French 파일을 읽지 않는다. 존재하는 다른 표를 임의 추가하지 않는다.
    config.Set("currentLanguage","german");CHECK(client.Translate("Loading")=="Ladevorgang");config.Set("currentLanguage","english");CHECK(client.Translate("Loading")=="Loading");config.Set("currentLanguage","portuguese");CHECK(client.Translate("Loading")==portuguese);
    CHECK(client.Translate("사용자😀")=="사용자😀"); //원문 밖 UTF-8 이름은1252 표의 문자열로 잘못 디코딩하지 않는다.
}
