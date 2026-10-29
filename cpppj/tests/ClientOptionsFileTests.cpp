// 원문 바이트 유지와 유니코드 경로의 실제 설정 왕복·쓰기 실패 수명을 검사한다.
#include "TestSupport.h"
#include "client/ClientOptionsFile.h"
#include "o/BaseFile.h"
#include <stdexcept>

namespace { // UTF-8 입력은 코드 페이지에 의존하지 않는 실제 바이트로 만든다.
using namespace netstorm::client;using namespace netstorm::o; // 같은 제품 인코더/설정 로더를 사용한다.
std::string UnicodeOptions() { const std::u8string text=u8"mQdsTfirst=0\r\nInstallDir=\"H:\\한글\\😀\"\r\n";return {text.begin(),text.end()}; } // 서명 뒤 첫 키가 가려지는 원문 규칙도 그대로 유지한다.
}
TEST_CASE(ClientOptionsFile_OriginalBytesAndUnicodeRoundTrip) {
    const std::string original="mQdsTName=\"caf\xc3\xa9 \xe2\x82\xac\"\r\n";CHECK(EncodeClientOptions(original)==EncodeConfigFile(original));
    const auto text=UnicodeOptions();auto bytes=EncodeClientOptions(text);CHECK(bytes[0]==0 && bytes[2]==0);auto plain=bytes;ApplyXor(plain);
    CHECK(std::string(plain.begin(),plain.begin()+5)=="mQdsT");CHECK(plain[5]==0xef && plain[6]==0xbb && plain[7]==0xbf);
    CHECK(ConfigText::FromBytes(bytes).GetRaw("InstallDir")=="H:\\한글\\😀");ConfigRegistry registry;Config config(registry);config.LoadBytes(bytes);CHECK(config.Get("InstallDir")=="H:\\한글\\😀");
}
TEST_CASE(ClientOptionsFile_InvalidUtf8RemainsRejected) {
    for (const std::string bad:{std::string("mQdsT\xc0\xaf"),std::string("mQdsT\xed\xa0\x80"),std::string("mQdsT\xf4\x90\x80\x80")}) { // 과장 인코딩/서로게이트/최대값 초과를 각각 거부한다.
        bool failed=false;try { static_cast<void>(EncodeClientOptions(bad)); }catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
    }
}
TEST_CASE(ClientOptionsFile_SaveFailureDoesNotRetryOrClearChanges) {
    ConfigInterface config({});config.Startup({});config.Set("InstallDir","H:\\한글\\😀");unsigned writes{};bool failed=false;
    try { SaveClientOptions(config,[&](auto,auto) { ++writes;throw std::invalid_argument("쓰기 실패 관측"); }); }catch (const std::invalid_argument&) { failed=true; }
    CHECK(failed && writes==1 && config.Configuration().changed);std::vector<std::uint8_t> output;
    SaveClientOptions(config,[&](auto name,auto bytes) { CHECK(name=="d\\options.cfg");output.assign(bytes.begin(),bytes.end()); });CHECK(!config.Configuration().changed && !output.empty());
    CHECK(ConfigText::FromBytes(output).GetRaw("InstallDir")=="H:\\한글\\😀");
}
