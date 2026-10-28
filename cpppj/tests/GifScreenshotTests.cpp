// 원본 PE 관측의 선택된135입력과 실제 GIF 인덱스/행 간격/사전 감김을 검사한다.
#include "TestSupport.h"
#include "client/GifScreenshot.h"
#include "client/GifImage.h"
#include "client/ViewKeyInput.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <stdexcept>

using namespace netstorm::client;
namespace { // fixture를 기대 계산 없이 읽는 검사 장치다.
std::vector<std::string> Split(std::string_view text,char separator) { // 마지막 빈 칸도 원문 그대로 유지한다.
    std::vector<std::string> result;std::size_t begin{}; // 현재 필드 시작과 원시 문자열 목록이다.
    for (;;) { // TSV 또는 실제 사건열의 구분자만 소비한다.
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1;
    }
}
std::uint32_t Number(std::string_view text) { return static_cast<std::uint32_t>(std::stoull(std::string(text))); } // signed 서식을 포함한 DWORD 입력을 읽는다.
std::string Selected(std::string_view trace) { // 이 제품 단계에서 직접 소유하는 장치 호출만 원문에서 고른다.
    std::string output; // 실제 순서를 바꾸지 않은 선택된 trace다.
    for (const auto& event:Split(trace,';')) { // Guide/Gump/CRT 서식/커서의 기존 경계는 제외한다.
        if (event.starts_with("guide:") || event.starts_with("mission:") || event.starts_with("gump:") || event=="general" || event.starts_with("format:") || event.starts_with("cursor:")) continue;
        if (!output.empty()) output+=';';output+=event;
    }
    return output;
}
}
TEST_CASE(GifScreenshot_Native135InputsAndCurrentPause) { // Ctrl Shift진단32입력은 아직제품구현이아니므로 수락135와 구별한다.
    std::ifstream input(NETSTORM_SCREENSHOT_KEY_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t accepted{},diagnostic{}; // 실제읽은행과 선택/미선택범위다.
    while (std::getline(input,line)) { // 제품에연결한일반/Shift입력만 같은 장치순서로 대조한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with("name\t")) continue;
        const auto row=Split(line,'\t');CHECK(row.size()==20);if (row.size()!=20) continue;
        const auto fields=Split(row[1],':');const auto code=Number(fields[0]); // 실제입력열의 사건코드다.
        if ((code&(InputCode::kControl|InputCode::kShift))==(InputCode::kControl|InputCode::kShift)) { ++diagnostic;continue; }
        ++accepted;auto counter=Number(row[7]),banners=Number(row[6]),pause=Number(row[4]);std::uint32_t checks{},hide{},redraw{};std::string filename,trace; // 입력전역과 명시외부장치의 실제상태다.
        const auto append=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; // 제품의 실제호출만 순서대로쓴다.
        ScreenshotSaveHooks save; // 설정·파일·논리화면을 원문과같은 명시경계로 제공한다.
        save.set=[&](auto key,auto value) { append("set:"+std::string(key)+':'+std::string(value));filename=value; };
        save.read=[&](auto key,auto& path) { path=row[10];path.replace(path.find("{GifOutName}"),12,filename);append("get:"+std::string(key)+':'+path);return true; };
        save.exists=[&](auto path) { const bool exists=checks++<Number(row[8]);append("exists:"+std::string(path)+':'+std::to_string(exists));return exists; };
        save.lock=[&] { append("lock"); };save.unlock=[&] { append("unlock"); };
        save.gif=[&](auto path) { append("gif:"+std::string(path)+":16789504:"+row[11]+':'+row[12]+':'+row[13]); };
        save.log=[&](auto path) { append("log:29:Screen shot written to %s\\n:"+std::string(path)); };
        ViewKeyInputHooks view; // Tell뒤현재정지를 재조회하는동일장치다.
        view.paused=[&] { append("paused:"+std::to_string(pause));return std::bit_cast<std::int32_t>(pause)>0; };
        view.useBanners=[&] { append("banners:"+std::to_string(banners));return banners; };
        view.writeBanners=[&](auto value) { append("config:useBanners=\""+std::to_string(value)+'"');banners=value; };
        if (!Number(row[2]) && !Number(row[3])) { // 선행Guide/Gump가소비하지않은입력만일반단계에준다.
            const InputEvent event{code,17,23}; // 원문이받은현재사건이다.
            DispatchScreenshotKey(event,[&] { static_cast<void>(SaveNumberedScreenshot(counter,save)); },[&](auto section) { append("tell:"+std::string(section));if (Number(row[9])) pause=1; });
            if (!Number(row[5])) static_cast<void>(DispatchViewKeyInput(event,{hide,redraw},view));
        }
        CHECK(counter==Number(row[14]));CHECK(banners==Number(row[15]));CHECK(trace==Selected(row[16]));CHECK(row[17]=="1");
    }
    CHECK(accepted==135 && diagnostic==32);
}
TEST_CASE(GifScreenshot_EncoderPalettePaddingAndDictionaryReset) { // 원래화면과무관한명시인덱스에서코드폭증가/4096사전반납을검사한다.
    constexpr std::uint32_t width=313,height=179,pitch=320; // 홀수폭/행패딩의독립화면입력이다.
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(pitch)*height,253),expected;std::array<std::uint8_t,768> palette{};std::uint32_t random=17; // 난수소비는검사전용이며게임전역을쓰지않는다.
    for (std::size_t index=0;index<palette.size();++index) palette[index]=static_cast<std::uint8_t>(index*71); // 서로다른RGB값을그대로보존한다.
    for (std::uint32_t y=0;y<height;++y) { // 각행의실제폭만기대픽셀에추가한다.
        for (std::uint32_t x=0;x<width;++x) { random=random*1664525U+1013904223U;const auto byte=static_cast<std::uint8_t>(random>>24);pixels[static_cast<std::size_t>(y)*pitch+x]=byte;expected.push_back(byte); } // 투명관문없는모든256색입력이다.
    }
    const auto encoded=EncodeScreenshotGif(width,height,pitch,pixels,palette);const auto decoded=DecodeGif(encoded); // 기존독립읽기경로로첫이미지의실제인덱스를확인한다.
    CHECK(decoded.width==width && decoded.height==height && decoded.indices==expected);CHECK(std::equal(palette.begin(),palette.end(),encoded.begin()+13));
    const std::array<std::uint8_t,1> single{255};const auto one=DecodeGif(EncodeScreenshotGif(1,1,1,single,palette));CHECK(one.indices==std::vector<std::uint8_t>{255});
}
TEST_CASE(GifScreenshot_WriteFailureUnlocksWithoutSuccessLog) { // 파일장치예외때잠금/번호는유지하고성공안내를발화하지않는다.
    std::uint32_t counter=7;std::string order;ScreenshotSaveHooks hooks; // 원문실행장치와실패시점의관측이다.
    hooks.set=[&](auto,auto) { order+='S'; };hooks.read=[&](auto,auto& path) { order+='R';path="unused.gif";return true; };hooks.exists=[](auto) { return false; };
    hooks.lock=[&] { order+='L'; };hooks.gif=[&](auto) { order+='W';throw std::runtime_error("명시파일장치실패"); };hooks.unlock=[&] { order+='U'; };hooks.log=[&](auto) { order+='X'; };
    bool failed{};try { static_cast<void>(SaveNumberedScreenshot(counter,hooks)); } catch (const std::runtime_error&) { failed=true; }
    CHECK(failed && counter==8 && order=="SRLWU");CHECK(!DispatchScreenshotKey({0xa0760000U,0,0},{},{}));
}
TEST_CASE(GifScreenshot_Native256PrimaryPaletteRecords) { // 같은원문기본53비트의실제할당RGB를모든채널에서대조한다.
    std::ifstream input(std::filesystem::path(NETSTORM_SCREENSHOT_KEY_FIXTURE).parent_path()/"screenshot-palette-x86.tsv");CHECK(input.good());if (!input) return;
    std::array<std::uint8_t,256*56> primary{};std::array<std::uint8_t,768> expected{};std::string line;std::size_t count{},precisionDifferences{}; // 원시double입력/실제관측값/읽은행수다.
    while (std::getline(input,line)) { // 독립256행의입력bits와관측53/64결과를구별해읽는다.
        if (line.starts_with("index\t")) continue;const auto row=Split(line,'\t');CHECK(row.size()==10);if (row.size()!=10) continue;const auto index=Number(row[0]);CHECK(index<256);if (index>=256) continue;++count;
        bool differs{}; // 이번호의두정밀도실제결과차이다.
        for (std::size_t channel=0;channel<3;++channel) { // 기록의세입력double과53비트출력BYTE만제품대조에공급한다.
            const auto bits=std::stoull(row[1+channel]);std::memcpy(primary.data()+index*56+24+channel*8,&bits,8);expected[index*3+channel]=static_cast<std::uint8_t>(Number(row[4+channel]));differs|=row[4+channel]!=row[7+channel];
        }
        precisionDifferences+=differs;
    }
    CHECK(count==256 && precisionDifferences==236);CHECK(ScreenshotPrimaryPalette(primary)==expected);
}
TEST_CASE(GifScreenshot_DefaultFileNamespaceAndAbsoluteOverride) { // 원본설정의선행구분자를드라이브루트로오인하지않는다.
    const std::filesystem::path root="H:/game"; // 실제파일을만들지않는기본디스크경로입력이다.
    CHECK(ResolveScreenshotDiskPath(root,"\\ScreenShot/pic0.gif")==root/"screenshot/pic0.gif"); // 기존BaseFile의대소문자무시정규형을같이사용한다.
    CHECK(ResolveScreenshotDiskPath(root,"ScreenShot/pic2.gif")==root/"screenshot/pic2.gif");
    CHECK(ResolveScreenshotDiskPath(root,"C:/chosen/pic9.gif")==std::filesystem::path("C:/chosen/pic9.gif"));
}
