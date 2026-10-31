// 지역 이미지의 DataManager 덤프를 창 없이 실제 Client 필터/FILE/공유 버퍼/NULL Chat에 연결해 검사한다.
#include "TestSupport.h"
#include "DudeSpawnSupport.h"
#include "client/CampaignTemplateStorage.h"
#include "client/ClientMain.h"
#include "client/ClientDiagnostics.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <stdexcept>

namespace { // 실제 임시 CRT 파일만 소유하며 로그 장치의 본문을 검사 대체물로 바꾸지 않는다.
using namespace netstorm::client;
using namespace netstorm::o;
struct TemplateDiagnosticFile { // 같은 Client FILE 슬롯을 빌린 뒤 임시 파일을 닫기 전에 원래 슬롯을 복원한다.
    TraceLogState& state; // 실제 Client의 장기 로그 전역이다.
    TraceLogStream previous; // 검사 시작 전에 사용하던 FILE 토큰이다.
    FILE* file; // 이 검사만 소유하는 실제 CRT 임시 파일이다.
    explicit TemplateDiagnosticFile(TraceLogState& value):state(value),previous(value.stream),file(std::tmpfile()) { // 새 파일이 열린 뒤에만 현재 FILE을 바꾼다.
        if (!file) throw std::runtime_error("Template 진단 검사 임시 FILE 열기 실패");state.stream=reinterpret_cast<TraceLogStream>(file);
    }
    ~TemplateDiagnosticFile() { state.stream=previous;std::fclose(file); } // Client의 빌림을 복원하고 자체 파일만 반납한다.
    std::string Text() { // 실제 출력 파일을 처음부터 읽으며 게임 데이터에는 쓰지 않는다.
        std::rewind(file);std::string text;std::array<char,256> bytes{};
        for (;;) { const auto count=std::fread(bytes.data(),1,bytes.size(),file);text.append(bytes.data(),count);if (count<bytes.size()) break; } // EOF까지 현재 CRT 출력만 읽는다.
        return text;
    }
};
}

TEST_CASE(TemplateDiagnosticClient_UsesSameFilterFileBufferAndActualNullChat) { // 실제 level29 장치가 필터되면 공유 상태를 보존하고, 활성화되면 두 원문 로그를 같은 FILE에 쓴다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=assets.files.Read("d/thewarbegins.fort");ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));
    auto& globals=client.DiagnosticGlobals();CampaignTemplateStorage storage;storage.BindDiagnosticLog([&client](auto level,auto text) { client.WriteDiagnosticLog(level,text); });TemplateFileLoadHooks load;
    load.read=[&](auto) { return input; };load.typeCount=[] { return 188; };globals.log.level=28;globals.log.buffer.fill('q');const auto before=globals.log.buffer;
    const auto filtered=storage.ReadLoginImage("source.fort",load,0);CHECK(filtered.end>72 && !globals.log.stream && globals.log.buffer==before && !storage.OwnedImages());
    TemplateDiagnosticFile file(globals.log);globals.log.level=29;globals.log.indent=2;globals.log.flush=1;globals.traceToChat=1; // 실제 NULL Chat은 서식과 현재 포인터를 조회한 뒤 UI 추가를 생략한다.
    const auto image=storage.ReadLoginImage("source.fort",load,0);std::array<char,128> total{};std::snprintf(total.data(),total.size(),"  %04d bytes total.\n",static_cast<int>(image.end-72));
    CHECK(image.Bytes().size()==filtered.Bytes().size() && std::equal(image.Bytes().begin(),image.Bytes().end(),filtered.Bytes().begin()));
    CHECK(file.Text()=="  File NONAME: Stripped Image:\n"+std::string(total.data()));CHECK(std::string(globals.log.buffer.data())==total.data() && globals.log.stream==reinterpret_cast<TraceLogStream>(file.file));
    CHECK(globals.log.level==29 && globals.log.indent==2 && globals.log.flush==1 && !storage.OwnedImages());
}
