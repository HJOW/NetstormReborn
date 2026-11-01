// Intro 감지·출처·현재 Client 장부와 실제 이름/입력/State 예약을 창 없이 검사한다.
#include "TestSupport.h"
#include "client/IntroMenu.h"
#include "client/ClientMain.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include "o/BaseFile.h"
#include "o/RawBaseFileReadPolicy.h"
#include <array>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <sstream>

using namespace netstorm::client;
using namespace netstorm::o;
namespace { // 보호 자료는 읽고 시험 파일은 작업 경로에만 만든다.
const auto kIntroRoot=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path(); // 저장소/보호 자료와 검사 작업본을 구별하는 기준이다.
}

// 0041ab10의 읽기와 다른 우선순위·출처 값을 성공 mask 전체와 장치 실패에서 확인한다.
TEST_CASE(IntroMenu_PresencePolicyArchiveFirstAndNoDeviceFallback) {
    const std::array<BaseFileReadSite,4> order{BaseFileReadSite::PrimaryArchive,BaseFileReadSite::PrimaryDisk,BaseFileReadSite::SecondaryDisk,BaseFileReadSite::SecondaryArchive}; // 보호 원문이 고르는 네 상대 조회 순서다.
    for (std::uint32_t mask=0;mask<16;++mask) { // 네 장치의 모든 성공/실패 조합에서 최초 성공만 방문해야 한다.
        std::vector<BaseFileReadSite> calls;std::uint32_t expected{};std::size_t wanted=4;
        for (std::size_t index=0;index<order.size();++index) if (mask&(1U<<index)) { expected=static_cast<std::uint32_t>(order[index]);wanted=index+1;break; } // 원문 순서표에서 최초 성공의 출처와 방문 개수만 고른다.
        const auto result=ProbeBaseFileByPolicy("movie\\intro.smk",[&](auto site,auto name) { CHECK(name=="movie\\intro.smk");calls.push_back(site);return (mask&(1U<<(calls.size()-1)))!=0; });
        CHECK(result==expected && calls.size()==wanted && std::equal(calls.begin(),calls.end(),order.begin()));
    }
    for (bool exists:{false,true}) { // 직접 장치의 실패는 아카이브나 다른 디스크를 찾지 않는다.
        std::size_t calls{};CHECK(ProbeBaseFileByPolicy("C:\\movie.smk",[&](auto site,auto) { ++calls;CHECK(site==BaseFileReadSite::DirectDisk);return exists; })==(exists ? 5U : 0U));CHECK(calls==1);
    }
}

// 빈 경로·첫NUL·각 출처·예외의 선행0과 비영 성공의1 정규화를 원문 호출 순서대로 확인한다.
TEST_CASE(IntroMenu_DetectionZeroStoreCurrentPathAndPartialFailure) {
    std::uint32_t value=7;CHECK(DetectIntroMovie(value,[] { return std::string(); },{})==0 && value==0);
    for (auto source:{0U,1U,2U,3U,4U,5U,0xffffffffU}) { // 원문은 출처 번호를 보관하지 않고 비영 여부만 표시1로 바꾼다.
        value=77;std::size_t calls{};const auto result=DetectIntroMovie(value,[&] { CHECK(value==0);return std::string("intro.smk\0ignored",17); },[&](auto path) { CHECK(value==0 && path=="intro.smk");++calls;return source; });CHECK(result==(source ? 1U : 0U) && value==result && calls==1);
    }
    value=9;bool failed{};try { DetectIntroMovie(value,[]()->std::string { throw std::runtime_error("path"); },{}); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && !value);
    value=9;failed=false;try { DetectIntroMovie(value,[] { return "intro.smk"; },[](auto)->std::uint32_t { throw std::runtime_error("probe"); }); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && !value);
}

// 원문00435de0의 독립 CPU 관측84행을 같은 명시 파일 장치로 재생해 전역/반환/조회 횟수를 대조한다.
TEST_CASE(IntroMenu_IndependentNativeDetection84) {
    std::ifstream input(kIntroRoot/"cpppj/tests/fixtures/intro-menu-x86.tsv");CHECK(input.good());std::string line;std::size_t count{};
    while (std::getline(input,line)) { // 원시 fixture의 모든 입력을 읽고 기대값을 제품에서 계산하지 않는다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);std::string hex;std::uint32_t source{},initial{},returned{},available{},probes{};row>>hex>>source>>initial>>returned>>available>>probes;CHECK(!row.fail());std::string path;
        if (hex!="-") for (std::size_t index=0;index<hex.size();index+=2) path+=static_cast<char>(std::stoul(hex.substr(index,2),nullptr,16)); // 첫NUL 뒤도 원본 입력 그대로 복원한다.
        std::uint32_t value=initial,calls{};const auto result=DetectIntroMovie(value,[&] { return path; },[&](auto) { ++calls;return source; });CHECK(result==returned && value==available && calls==probes);++count;
    }
    CHECK(count==84);
}

// 실제 상대/절대/한글 파일과 TAFF 메타데이터를 사용한다. 영상 내용의 디코드/Window 실행은 없다.
TEST_CASE(IntroMenu_ActualFileProbeAndClientStartupOwner) {
    const auto work=kIntroRoot/"extracted/cpp-intro-menu-699/files";std::filesystem::create_directories(work);const auto file=work/std::filesystem::path(u8"인트로😀.smk");{ std::ofstream stream(file,std::ios::binary);CHECK(stream.good()); } // 빈 파일도 rb 존재 성공이며 유효한 영상 여부는 열기 부모의 별도 책임이다.
    const auto text=file.u8string();const std::string path(text.begin(),text.end());BaseFileSystem files(work);CHECK(files.Probe(path)==5 && files.Probe("missing.smk")==0);CHECK(files.RegisterArchive(kIntroRoot/"originals/netstorm.tarc"));CHECK(files.Probe("d/tell.english")==2);
    ClientOptions options;options.gameDirectory=kIntroRoot/"originals";options.noAudio=true;Client client(options);const auto owner=client.LocalStateGlobals();const auto* address=&owner->IntroAvailable();CHECK(!*address);client.Configuration().Set("introMovie",path);client.DetectIntroAvailability();CHECK(*address==1);
    client.Configuration().Set("introMovie",path+".missing");CHECK(*address==1);client.DetectIntroAvailability();CHECK(!*address && address==&owner->IntroAvailable()); // Config 알림/다른 State는 최초 감지값을 자동 재계산하지 않는다.
}

// 이름→실제 문자열/정수 큐→일반 입력이 원문처럼 두 DWORD만 바꾸고 나머지144BYTE 블록을 보존한다.
TEST_CASE(IntroMenu_NameQueueAndNativeStateRequest) {
    CHECK(GameMenuScriptCode("PlayIntro")==kPlayIntroCommand && GameMenuScriptCode("pLaYiNtRo")==kPlayIntroCommand && GameMenuCommandValue(0x542a54U)==kPlayIntroCommand);
    InputQueue queue;ScriptStringRing strings;QueueScriptCommand("PlayIntro,0",queue,strings,{GameMenuScriptCode,{}});const auto event=queue.Pop(false);CHECK(event.code==kPlayIntroCommand && event.x==0 && event.y==0);
    for (auto code:{kPlayIntroCommand,kPlayIntroCommand|InputCode::kRelease,kPlayIntroCommand|InputCode::kControl,0x02000001U}) { // release/다른 명령은 State를 그대로 두고 수정키는 명령 실행을 막지 않는다.
        std::array<LocalMissionStateBlock,5> blocks;
        for (auto& block:blocks) block.fill(0x12345678U); // 원문 다섯 블록을 모두 같은 비영 값으로 채워 잘못된 범위의0쓰기도 드러낸다.
        const auto before=blocks;LocalMissionStateStorage state{blocks[0],blocks[1],blocks[2],blocks[3],blocks[4]};
        const bool selected=(code&0x03ffffffU)==kPlayIntroCommand;CHECK(DispatchPlayIntroInput({code,-7,99},state)==selected);
        auto expected=before;if (selected && !(code&InputCode::kRelease)) { expected[4][0]=39;expected[4][35]=1; }CHECK(blocks==expected);
    }
}
