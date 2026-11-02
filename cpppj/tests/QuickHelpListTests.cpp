// 실제 원문 부모/일반입력/이름 FIFO 관측과 파일 수명/부분 실패를 대조한다.
#include "GumpRawSupport.h"
#include "client/QuickHelpList.h"
#include "client/HelpListDisk.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include "o/BaseFile.h"
#include "o/RawFileName.h"
#include <filesystem>
#include <fstream>
#include <bit>
#include <chrono>
#include <stdexcept>

using namespace netstorm::client; //제품의 같은 전체 부모/입력/파일장치다.
using namespace netstorm::test::gumpraw; //fixture의 BYTE/열 해독만 공유한다.
namespace { //원문 기대결과를 계산하지 않는 명시 장치/관측 직렬화다.
std::string TextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } //NUL/CRLF를 포함한 실제 바이트다.
std::string HexText(std::string_view hex) { const auto bytes=Bytes(std::string(hex));return {bytes.begin(),bytes.end()}; } //fixture의 원시 문자열을 그대로 되돌린다.
struct StopBoundary {}; //Native 실패 관측과 같은 경계 직전에서 중단한다. SEH unwind 기대값을 합성하지 않는다.
//Native 부모가 호출하는 외부 장치만 공급한다. 순회/선택/파일 수명 계산은 제품을 실행한다.
struct Parent {
    bool present{},shortWrite{},live{},opened{}; //입력 응답과 현재 지역 BaseFile 수명이다.
    std::int32_t count{}; //현재00541348의 signed 값이다.
    std::uint32_t mutation{},lookups{},cleanups{},constructs{},destructs{},closes{},abandoned{}; //각 실제 호출의 횟수다.
    std::string fail,contents,trace,stoppedTrace,stream="OLD"; //명시 실패점/FILE/부작용의 원문 표기다.
    std::size_t position{}; //명시 FILE의 현재 byte 위치다.
    std::array<std::uint32_t,5> flags{1,0,0x80000000U,0xffffffffU,4}; //원문의 포함 판정에 비트 종류를 추측하지 않는다.
    std::array<std::string,5> names{"Alpha","Beta","Gamma","Delta","Epsilon"}; //두 번째 getter가 반환하는 현재 이름이다.
    const std::uint8_t* identity{}; //같은 지역24BYTE 주소만 관측한다.
    std::function<void(std::string_view)> sink; //전용 HTML 파서가 빌리는 동기 stream이다.
    QuickHelpListContext context;QuickHelpListWindowHooks window; //제품 전체 부모에 연결한 실제 명시 장치다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } //Native의 동일 순서로 효과만 직렬화한다.
    void Checkpoint(std::string_view name) { if (fail==name) { stoppedTrace=trace;throw StopBoundary{}; } } //호스트 unwind 이후 부작용과 Native prefix를 구별한다.
    void Put(std::string_view bytes) { if (shortWrite) bytes=bytes.substr(0,1);const auto end=position+bytes.size();if (contents.size()<end) contents.resize(end);contents.replace(position,bytes.size(),bytes);position=end; } //명시 FILE만 쓰고 부모 선택은 계산하지 않는다.
    void Same(QuickHelpListFile file) { CHECK(live && file.data()==identity); } //다른 지역 객체로 바뀌거나 반납 뒤 재사용하지 않는다.
    void Close() { if (opened) { Add("Close:77:"+stream);opened=false;++closes; } } //명시Close 뒤 destructor는 FILE0을 재닫지 않는다.
    explicit Parent(const std::vector<std::string>& input) { //첫 열의 고정6항 JSON은 쉼표/escape 없는 독립 Native 입력이다.
        present=Number(input[0])!=0;count=std::stoi(input[1]);mutation=Number(input[2]);shortWrite=Number(input[3])!=0;fail=input[4].substr(1,input[4].size()-2);contents=input[5].substr(1,input[5].size()-2);
        context.file.construct=[this](auto file) { CHECK(!live);live=true;identity=file.data();++constructs; };
        context.file.open=[this](auto file,auto name,auto mode,auto create) { Same(file);Add("Open:"+TextHex(name)+':'+std::to_string(mode)+':'+std::to_string(create)+':'+stream);Checkpoint("open");opened=true; };
        context.file.write=[this](auto file,auto bytes,auto offset) { Same(file);CHECK(opened && offset==-1);Add("Write:"+TextHex(bytes)+':'+std::to_string(bytes.size())+":1:"+stream);Put(bytes);Checkpoint("write"); };
        context.file.close=[this](auto file) { Same(file);Close(); };
        context.file.destruct=[this](auto file) { Same(file);Close();live=false;identity=nullptr;++destructs; };
        context.typeCount=[this] { return count; };
        context.flags2=[this](auto type) { Add("Type:"+std::to_string(type)+':'+stream);++lookups;return flags.at(type); };
        context.typeName=[this](auto type) { Add("Type:"+std::to_string(type)+':'+stream);++lookups;if (mutation==1 && lookups==2) names.at(type)="Changed";return names.at(type); };
        context.cleanup=[this] { Add("Cleanup:"+stream);++cleanups;Checkpoint("cleanup");if (cleanups==1) { if (mutation==2) count=1;else if (mutation==3) count=5;else if (mutation==4) flags[1]=1; } };
        context.tell=[this](auto section) { Add("Tell:"+TextHex(section)+':'+stream);Checkpoint("tell"); };
        window.openEmpty=[this] { Add("Help::0:0");return present ? 19U : 0U; };
        window.attachStream=[this](auto help,auto value) { CHECK(help==19 && live);stream="F";sink=std::move(value); };
        window.detachStream=[this](auto help) { CHECK(help==19 && opened);stream="0";sink={}; };
        window.abandonStream=[this](auto help) { CHECK(help==19);sink={};++abandoned; }; //원문의 남은 +bc 토큰 F는 그대로이며 callback 수명만 끝낸다.
        window.show=[this](auto help,auto type,auto topic) { CHECK(help==19);Add("Format:"+TextHex(topic)+':'+stream);Add("Show:"+std::to_string(type)+':'+TextHex(topic)+':'+stream);Checkpoint("show");Put('['+std::string(topic)+']'); }; //동적 HTML은 별도 실제 QuickHelpHtml/통합 검사에서 확인한다.
        window.kill=[this](auto help) { CHECK(help==19 && !opened);Add("Kill:"+stream);Checkpoint("kill"); };
    }
};
//보호 원본과 분리된 임시 디렉터리만 실제 OS 파일 검사에 사용한다.
struct TemporaryDirectory {
    std::filesystem::path path; //각 검사에서 유일하게 생성하는 이름이다.
    TemporaryDirectory() { path=std::filesystem::temp_directory_path()/("netstorm-help-list-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(path); }
    ~TemporaryDirectory() { std::error_code error;std::filesystem::remove_all(path,error); } //생성한 동일 임시 경로만 반납하며 제품/보호 자료는 수정하지 않는다.
};
//실제 디스크 결과를 binary로 읽어 CRLF 변환/기존 tail을 구별한다.
std::string ReadBytes(const std::filesystem::path& path) { std::ifstream stream(path,std::ios::binary);return {std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()}; }
}

TEST_CASE(QuickHelpList_Native142ParentOrderAndFailurePrefixes) { //모든 TYPE getter/flags2/현재개수/이름/짧은 쓰기와 부분실패를 고정한다.
    const auto rows=LoadFixture(NETSTORM_QUICK_HELP_LIST_FIXTURE).rows;CHECK(rows.size()==142);
    for (const auto& row:rows) { //기대 분기/문자열은 Native fixture에서만 읽는다.
        CHECK(row.size()==6);if (row.size()!=6) continue;const auto input=Split(row[0].substr(1,row[0].size()-2),',');CHECK(input.size()==6);if (input.size()!=6) continue;Parent p(input);std::string outcome="ok";
        try { CreateQuickHelpList(p.context,p.window); } catch (const StopBoundary&) { outcome=p.fail; }
        CHECK(outcome==row[1]);CHECK(TextHex(p.contents)==row[2]);CHECK(static_cast<std::uint32_t>(p.count)==Number(row[3]));CHECK(p.stream==row[4]);CHECK((outcome=="ok" ? p.trace : p.stoppedTrace)==row[5]);
        CHECK(!p.live && !p.opened);CHECK(p.constructs==(p.present ? 1U : 0U) && p.destructs==p.constructs);CHECK(p.closes==(p.present && outcome!="open" ? 1U : 0U));CHECK(p.abandoned==(outcome=="write" || outcome=="show" || outcome=="cleanup" ? 1U : 0U));
    }
}

TEST_CASE(QuickHelpList_Native128GeneralGuideGumpReleasePauseConsole) { //원문의 선행 소비와 일반 입력 현재03ffffff/release를 대조한다.
    const auto rows=LoadFixture(NETSTORM_QUICK_HELP_LIST_INPUT_FIXTURE).rows;CHECK(rows.size()==128);
    for (const auto& row:rows) { //Guide/Gump 결과는 실제 상위 어댑터 위치처럼 분배 전에 적용한다.
        CHECK(row.size()==12);if (row.size()!=12) continue;std::string trace;
        if (!Number(row[5]) && !Number(row[6])) CHECK(DispatchCreateHelpList({Number(row[0]),std::bit_cast<std::int32_t>(Number(row[1])),std::bit_cast<std::int32_t>(Number(row[2]))},[&] { trace="create"; }));
        CHECK((trace.empty() ? "-" : trace)==row[9]);CHECK(row[10]=="1" && row[11]=="0"); //정지/Console/Editor/미션 입력은 이 함수의 새 관문이 아니다.
    }
    CHECK(!DispatchCreateHelpList({'C',0,0},{}));CHECK(DispatchCreateHelpList({kCreateHelpListCommand|InputCode::kRelease,0,0},{}));
}

TEST_CASE(QuickHelpList_Native21NamesAndRealStringQueue) { //원문 이름 표/토큰/문자열 고리 결과를 같은 실제 공개 API로 재생한다.
    const auto rows=LoadFixture(NETSTORM_QUICK_HELP_LIST_NAMES_FIXTURE).rows;CHECK(rows.size()==21);
    for (const auto& row:rows) { //문자열/숫자 인자가 모두 큐에 보존되지만 CreateHelpList는 인자를 소비하지 않는다.
        CHECK(row.size()==9);if (row.size()!=9) continue;InputQueue queue;ScriptStringRing strings;std::uint32_t unknown{};
        QueueScriptCommand(HexText(row[1]),queue,strings,{GameMenuScriptCode,[&](auto) { ++unknown; }});CHECK(!unknown && strings.Index()==Number(row[7]));
        const auto expected=Split(row[6],',');CHECK(expected.size()==2);if (expected.size()!=2) continue;const auto event=queue.Pop(false);CHECK(queue.Empty() && event.code==Number(expected[0]) && event.x==event.y && IsGameMenuCommand(event.code));
        const auto value=std::bit_cast<std::uint32_t>(event.y);if (expected[1].starts_with('n')) CHECK(value==Number(expected[1].substr(1)));else CHECK(TextHex(strings.Resolve(value))==expected[1].substr(1));
    }
    CHECK(GameMenuCommandValue(0x542964U)==kCreateHelpListCommand);CHECK(CreateHelpListScriptCode("OtherHelpList")==0xffffffffU);
}

TEST_CASE(QuickHelpList_AbsentHelpFirstNulAndReachedBounds) { //없는 Help는 파일/TYPE를 요구하지 않으며 서식 초과는 선행 효과와 소멸을 보존한다.
    QuickHelpListWindowHooks empty;empty.openEmpty=[] { return 0U; };CreateQuickHelpList({},empty);
    Parent p({"1","1","0","0","\"\"","\"\""});p.names[0]=std::string("Alpha\0ignored",13);CreateQuickHelpList(p.context,p.window);CHECK(p.contents=="<html>[AlphaType]<p></html>");
    Parent tooLong({"1","1","0","0","\"\"","\"\""});tooLong.names[0].assign(252,'X');bool failed{};
    try { CreateQuickHelpList(tooLong.context,tooLong.window); }catch (const std::length_error&) { failed=true; }CHECK(failed && tooLong.contents=="<html>" && tooLong.destructs==1 && tooLong.closes==1 && tooLong.abandoned==1 && tooLong.stream=="F" && tooLong.cleanups==0);
}

TEST_CASE(QuickHelpListDisk_BinaryRPlusBPreservesTailAndSharedFileRing) { //기존 파일을 truncate하지 않으며 실제 전체 Open의10칸 소유자도 재사용한다.
    TemporaryDirectory temp;const auto path=temp.path/"NetStormPieceList.html";{ std::ofstream old(path,std::ios::binary);old<<std::string(64,'Z'); }
    netstorm::o::BaseFileSystem files(temp.path);const auto names=files.FileNameOwner();const auto before=names->Index(); //구성만으로 파일/고리를 건드리지 않는다.
    auto disk=MakeHelpListDiskHooks(temp.path,&files);CHECK(names->Index()==before && ReadBytes(path)==std::string(64,'Z'));
    std::array<std::uint8_t,24> storage{};const QuickHelpListFile file(storage);disk.construct(file);disk.open(file,"NetStormPieceList.html",1,1);disk.write(file,"<html>\n</html>",-1);disk.close(file);disk.destruct(file);
    const auto result=ReadBytes(path);CHECK(result.size()==64 && result.substr(0,14)=="<html>\n</html>" && result.substr(14)==std::string(50,'Z'));CHECK(names->Index()!=before); //원문 binary 모드라 LF가CRLF로 바뀌지 않는다.
    const auto fresh=temp.path/std::filesystem::path(std::u8string(u8"한글"));std::filesystem::create_directory(fresh);auto created=MakeHelpListDiskHooks(fresh);created.construct(file);created.open(file,"NetStormPieceList.html",1,1);created.write(file,"<html></html>",-1);created.close(file);created.destruct(file);CHECK(ReadBytes(fresh/"NetStormPieceList.html")=="<html></html>"); //새 파일은 원문의 r+b 실패→w+b 생성이다.
}

TEST_CASE(QuickHelpListDisk_FailedOpenDestructAndSameDeviceReuse) { //실패한 열기의 같은 지역 객체를 먼저 소멸한 뒤 장치를 다시 사용할 수 있다.
    TemporaryDirectory temp;auto disk=MakeHelpListDiskHooks(temp.path/"absent");std::array<std::uint8_t,24> storage{};const QuickHelpListFile file(storage);disk.construct(file);bool failed{};
    try { disk.open(file,"NetStormPieceList.html",1,1); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && !std::filesystem::exists(temp.path/"absent"));disk.destruct(file);
    std::filesystem::create_directory(temp.path/"absent");disk.construct(file);disk.open(file,"NetStormPieceList.html",1,1);disk.write(file,"ok",-1);disk.close(file);disk.destruct(file);CHECK(ReadBytes(temp.path/"absent/NetStormPieceList.html")=="ok");
}
