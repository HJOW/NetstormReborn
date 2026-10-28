// 원문 전체덤프/재귀/로그와 두파일단계, 실제디스크 장치를 검사한다.
#include "TestSupport.h"
#include "client/RawObjectDump.h"
#include "client/ObjectDumpDisk.h"
#include "o/RawObjectTrace.h"
#include "o/RawObjectTraceMain.h"
#include "o/BaseFile.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 기대출력은원문fixture에서만읽고입력과장치반환을따로구성한다.
using namespace netstorm::client;using namespace netstorm::o;
// 원시바이트를NUL/ANSI값까지보존하여16진수열로기록한다.
std::string Hex(std::string_view bytes) { constexpr char digits[]="0123456789abcdef";std::string out;for (const unsigned char b:bytes) { out+=digits[b>>4];out+=digits[b&15]; }return out; } // 전체BYTE를앞에서부터쌍으로기록한다.
// 빈값을포함한입력16진수쌍을바이트로되돌린다.
std::string Unhex(std::string_view text) { std::string out;for (std::size_t i=0;i<text.size();i+=2) out+=static_cast<char>(std::stoul(std::string(text.substr(i,2)),nullptr,16));return out; } // 모든쌍을원문순서로읽는다.
// 탭/노드구분자의원문열을누락없이분리한다.
std::vector<std::string> Split(const std::string& text,char separator='\t') { std::stringstream stream(text);std::string field;std::vector<std::string> result;while (std::getline(stream,field,separator)) result.push_back(field);return result; } // 현재구분자의모든열을분리한다.
// 원문입력과최종열을한번읽고각검사에같은바이트를전달한다.
std::vector<std::vector<std::string>> Rows() {
    std::ifstream input(NETSTORM_OBJECT_DUMP_FIXTURE);CHECK(bool(input));std::string line;std::vector<std::vector<std::string>> rows;
    while (std::getline(input,line)) { // 같은원문56행을인코딩변환없이읽는다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;rows.push_back(Split(line));
    }
    return rows;
}
// 컴파일된UTF-8작업경로를시스템ANSI변환없이현재filesystem경로로읽는다.
std::filesystem::path WorkDirectory() { const std::string_view path=NETSTORM_OBJECT_DUMP_WORK;return std::filesystem::path(std::u8string(path.begin(),path.end())); }
// 실제풀/TYPE/trace전역과명시파일/예외장치를소유하는합성입력장면이다.
struct DumpScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원문50BYTE의동일한물리풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재TYPE이름/속성표다.
    TraceLogState log;std::uint32_t count{},suppress{},depth{},checking{},metamap{},land=97,fringe=102,simple=121;bool boss=true; // 실제같은주소를빌리는현재전역이다.
    TraceLogStream openResult{};std::uint32_t mutation{},throwStage{},throwKind{},destructs{};bool changed{},overrideRead{}; // 명시외부변형/예외선택과소멸관측이다.
    std::string events,contents,finalFile,readBytes;std::vector<std::uint8_t> allocation;std::array<std::uint8_t,24> file{}; // 실제장치기록/두파일내용/할당/같은지역File물리다.
    // 입력만전체물리슬롯의바이트를설치한다.
    std::span<std::uint8_t> Raw(std::uint32_t sid) { const auto bytes=pool.Bytes();return std::span(const_cast<std::uint8_t*>(bytes.data()),bytes.size()).subspan(sid*50,50); }
    // 실제장치가기록하는현재level/덤프모드/FILE이다.
    std::string State() const { return std::to_string(static_cast<std::uint32_t>(log.level))+":"+std::to_string(suppress)+":"+std::to_string(log.stream); }
    // 앞선기록이있을때만장치구분자를추가한다.
    void Event(const std::string& value) { if (!events.empty()) events+=';';events+=value; }
    // 독립입력열만설치하고기대출력열은읽지않는다.
    explicit DumpScene(const std::vector<std::string>& row) {
        auto bytes=pool.Bytes();std::fill_n(const_cast<std::uint8_t*>(bytes.data()),bytes.size(),std::uint8_t{});
        for (std::uint32_t sid=0;sid<24000;++sid) Raw(sid)[11]=1; // 미사용 물리 슬롯을 명시 free 입력으로 둔다.
        for (std::size_t kind=0;kind<types.size();++kind) types[kind].name="kind"+std::to_string(kind); // 원문과 같은 독립 TYPE 이름 입력이다.
        types[82].flags1=0x810;count=static_cast<std::uint32_t>(std::stoul(row.at(0)));log.level=std::stoi(row.at(1));suppress=static_cast<std::uint32_t>(std::stoul(row.at(2)));
        openResult=static_cast<TraceLogStream>(std::stoull(row.at(3)));mutation=static_cast<std::uint32_t>(std::stoul(row.at(4)));log.stream=777;file.fill(0xa5);
        overrideRead=row.at(5)!="-";if (overrideRead) readBytes=Unhex(std::string_view(row.at(5)).substr(1));
        if (row.at(6)!="-") for (const auto& node:Split(row.at(6),'/')) { // 지정된 원문50BYTE만 설치한다.
            const auto colon=node.find(':');const auto sid=static_cast<std::uint32_t>(std::stoul(node.substr(0,colon)));const auto raw=Unhex(std::string_view(node).substr(colon+1));CHECK(raw.size()==50);std::copy(raw.begin(),raw.end(),Raw(sid).begin());
        }
    }
    // EH 정적 계약의 지정 위치에서만 정확한 typed 또는 일반 호스트 예외를 던진다.
    void MaybeThrow(std::uint32_t stage) { if (throwStage==stage) { if (throwKind==1) throw ObjectDumpUnableToOpen{};throw std::runtime_error("합성 외부 파일 예외"); } }
    // 같은 현재 전역을 부모와 재귀 양쪽에 빌린다.
    ObjectDumpGlobals Globals() { return {suppress,count,land,fringe,simple}; }
    // 실제 원문 재귀→현재VT→같은 공유 로그 본체를 연결한다.
    void Trace(std::uint32_t number,const TraceLogHooks& logger) {
        TraceRawObjectToLog(pool,types,suppress,{depth,count,checking,metamap,boss},log,logger,number,[] { throw std::logic_error("합성 dead 진단"); });
    }
    // console/fprintf/물리wt 변환을 독립 경계로 관측한다.
    TraceLogHooks LogHooks() {
        TraceLogHooks hooks;hooks.console=[&](auto line) { Event("C:"+Hex(line));if (line=="\n" && !changed) { changed=true;if (mutation==1) count=5;else if (mutation==2) count=25;else if (mutation==3) Raw(7)[11]=1;else if (mutation==4) Raw(7)[10]=0; } };
        hooks.write=[&](auto stream,auto line) { Event("W:"+std::to_string(stream)+":"+Hex(line));if (stream==1000) for (const auto c:line) { if (c=='\n') contents+='\r';contents+=c; } }; // 모든BYTE를 FILE의 text변환과 같은 순서로 쓴다.
        hooks.flush=[](auto) { throw std::logic_error("요청하지 않은 flush"); };return hooks;
    }
    // 원문의 각 파일 경계와 같은24BYTE this를 관측하고 반환값을 고정한다.
    ObjectDumpHooks Hooks() {
        ObjectDumpHooks hooks;hooks.path=[&] { Event("P:"+State());return std::string("out/squids.txt"); };
        hooks.open=[&](auto path,auto mode) { Event("O:"+Hex(path)+":"+Hex(mode)+":"+State());return openResult; };
        hooks.openFailure=[&] { Event("A:"+Hex("traceStream != NULL")+":"+State());if (mutation==5) log.stream=1000; };
        hooks.close=[&](auto stream) { Event("X:"+std::to_string(stream)+":"+State()); };
        hooks.trace=[&](auto sid) { const auto logger=LogHooks();Trace(sid,logger); };
        hooks.read=[&](auto path,auto first,auto second) { Event("R:"+Hex(path)+":"+std::to_string(first)+":"+std::to_string(second)+":"+State());MaybeThrow(1);const auto& bytes=overrideRead ? readBytes : contents;return std::vector<std::uint8_t>(bytes.begin(),bytes.end()); };
        hooks.allocate=[&](auto size) { Event("N:"+std::to_string(size)+":"+State());MaybeThrow(2);allocation.resize(static_cast<std::size_t>(size)+1);return std::span<std::uint8_t>(allocation); };
        hooks.construct=[&](auto physical) { CHECK(physical.data()==file.data());Event("B:"+State());MaybeThrow(3); };
        hooks.openWrite=[&](auto physical,auto path,auto mode,auto extra) { CHECK(physical.data()==file.data());Event("Q:"+Hex(path)+":"+std::to_string(mode)+":"+std::to_string(extra)+":"+State());MaybeThrow(4); };
        hooks.write=[&](auto physical,auto bytes,auto offset) { CHECK(physical.data()==file.data());const std::string_view text(reinterpret_cast<const char*>(bytes.data()),bytes.size());Event("D:"+Hex(text)+":"+std::to_string(offset)+":"+State());MaybeThrow(5);finalFile.assign(text);return static_cast<std::uint32_t>(bytes.size()); };
        hooks.destruct=[&](auto physical) { CHECK(physical.data()==file.data());++destructs;Event("Z:"+State());MaybeThrow(6); };return hooks;
    }
    // 전체 장치/현재 상태/최종 파일을 원문과 같은 형식으로 기록한다.
    std::string Result() const { return events+"|"+State()+"|"+Hex(finalFile); }
    // 합성 부모 호출을 같은 물리에서 실행한다.
    void Run() { const auto logger=LogHooks();DumpRawObjects(pool,types,log,Globals(),logger,Hooks(),file); }
};
}

// 실제 원문56입력의 전체 재귀/로그/파일 순서와 최종 상태를 비교한다.
TEST_CASE(ObjectDump_IndependentNative56FullTraceFilesAndCurrentBounds) {
    const auto rows=Rows();CHECK(rows.size()==56);
    for (const auto& row:rows) { DumpScene scene(row);scene.Run();CHECK(scene.Result()==row.at(7)); } // 모든 독립 입력을 실제 제품 본체로 실행한다.
}

// 정확한 EX_UNABLE_TO_OPEN_FILE은open/write에서만 잡고 나머지는File을unwind하며 현재모드를 유지한다.
TEST_CASE(ObjectDump_TypedRegionUnwindAndRestorationFollowNativeEhMetadata) {
    const auto row=Rows().at(0);
    for (std::uint32_t stage=1;stage<=6;++stage) for (std::uint32_t kind=1;kind<=2;++kind) { // 여섯 경계와 typed/일반 두 예외를 구별한다.
        DumpScene scene(row);scene.throwStage=stage;scene.throwKind=kind;bool threw=false;
        try { scene.Run(); }catch (...) { threw=true; }
        const bool caught=kind==1 && (stage==4 || stage==5);CHECK(threw!=caught);
        CHECK(scene.destructs==(stage>=4 ? 1U : 0U));
        CHECK(scene.suppress==((caught || stage==6) ? 0U : 1U));CHECK(scene.log.level==((caught || stage==6) ? 29 : 30));
    }
}

// 명시 격리 디렉터리에서 원문과 같은 텍스트→바이너리 파일 처리를 실행한다.
TEST_CASE(ObjectDump_RealDiskPipelineMatchesIndependentNativeBytes) {
    const auto row=Rows().at(6);const auto expected=Unhex(std::string_view(row.at(7)).substr(row.at(7).rfind('|')+1));
    for (const auto& leaf:{std::filesystem::path("disk"),std::filesystem::path(u8"disk-한글")}) { // ASCII/한글경로각각새장면에서실제wt→바이너리쓰기를검증한다.
        DumpScene scene(row);const std::filesystem::path directory=WorkDirectory()/leaf;std::filesystem::create_directories(directory);std::vector<std::string> console; // 이 단위의 격리 출력만 만든다.
        const auto logger=MakeObjectDumpDiskLogHooks([&](auto line) { console.emplace_back(line); });
        const auto hooks=MakeObjectDumpDiskHooks(directory,[&](auto sid) { scene.Trace(sid,logger); },[] { throw std::logic_error("격리 파일 열기 실패"); });
        DumpRawObjects(scene.pool,scene.types,scene.log,scene.Globals(),logger,hooks,scene.file);
        const auto bytes=ReadFileBytes(directory/"squids.txt");CHECK(std::string(bytes.begin(),bytes.end())==expected);CHECK(scene.log.stream==0 && scene.log.level==7 && scene.suppress==0 && !console.empty());
    }
}

// 상위 디렉터리가 없으면 임의 생성하지 않고 원문 assert복귀/모드유지를 따른다.
TEST_CASE(ObjectDump_RealOpenFailureDoesNotCreateParentOrRestoreModes) {
    DumpScene scene(Rows().at(0));const auto directory=WorkDirectory()/"missing-parent-558";CHECK(!std::filesystem::exists(directory));std::uint32_t failures{};
    const auto hooks=MakeObjectDumpDiskHooks(directory,[](auto) { throw std::logic_error("선택하지 않은 trace"); },[&] { ++failures; });
    DumpRawObjects(scene.pool,scene.types,scene.log,scene.Globals(),{},hooks,scene.file);
    CHECK(failures==1 && scene.log.stream==0 && scene.log.level==30 && scene.suppress==1 && !std::filesystem::exists(directory));
}
