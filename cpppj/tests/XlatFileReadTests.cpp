// 실제 원문 File/정규화/닫기/파서 순서와 기존 번역 장부 보존을 대조한다.
#include "TestSupport.h"
#include "RawSceneSupport.h"
#include "FileProbeTestSupport.h"
#include "client/XlatFileRead.h"
#include "o/BaseFile.h"
#include "o/RawXlat.h"
#include <array>
#include <bit>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

using namespace netstorm::client;
using namespace netstorm::o;
namespace { // 원본 입력 BYTE와 최초 기대값을 분리해 보관한다.
using namespace std::literals; // 입력의 첫NUL 뒤 바이트도 string_view 길이에 포함한다.
constexpr std::uintptr_t kFile=0x10016000,kKey=0x10017000; // 이번 독립 CPU의 File/키 불투명 토큰이다.
constexpr std::string_view kSeed="*\nKey\n-\nSeed\n=\n"; // 다른 언어 또는 중복 진단 앞의 독립 장부 입력이다.
constexpr std::array<std::string_view,14> kSources{{"\0"sv,"header\n*\nKey\n-\nValue\n=\n"sv,"*\nKey\n-\nValue\n="sv,"*\nKey\n-\nValue\n=tail"sv,"*\nKey\n-\nIncomplete"sv,"*\r\nKey\r\n-\r\nValue\r\n=\r\n"sv,"*\r\r\nKey\r\n-\nA\rB\n=\n"sv,"*\nKey\n-\nFirst\n=\n*\nKey\n-\nLast\n=\n"sv,"*\nAB\n-\nOne\n=\n*\nBA\n-\nTwo\n=\n"sv,"*\nKey\n-\n\n=\n"sv,"*\n\n-\nEmptyKey\n=\n"sv,"*\nKey\n-\nValue\n=\n\0*\nHidden\n-\nNo\n=\n"sv,"*\nK\xe4y\n-\nV\xf6lue\n=\n"sv,"*\nMulti\nline\n-\nTwo\nlines\n=\n"sv}}; // 711에서 관측한 동일 독립 입력을 파일 장치에도 공급한다.
std::span<const std::uint8_t> Bytes(std::string_view text) { return {reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}; } // UTF-8로 재해석하지 않는 원문 BYTE 뷰다.
std::string FileFields(const RawBaseFileState& state) { // 같은 여섯 원문 필드를 순서대로 직렬화한다.
    const std::array<std::uintptr_t,6> values{{state.file,state.flags,std::uint32_t(state.offset),std::uint32_t(state.length),state.key,state.keyLength}};std::string result;
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result; // 포인터 토큰의 호스트 폭은 줄이지 않는다.
}
std::string HexOrEmpty(std::span<const std::uint8_t> bytes) { const auto text=netstorm::test::rawscene::Hex(bytes);return text.empty() ? "-":text; } // 빈 Native 버퍼 표현을 구별한다.
template<class Operation> bool Failed(Operation operation) { try { operation(); }catch (const std::exception&) { return true; }return false; } // 실패를 빈 표 성공으로 바꾸지 않는다.
}

TEST_CASE(XlatFileRead_Native252InstallCloseBufferAndMergedLanguages) { // 실제 File필드·닫기 버퍼·파싱 버퍼/조회와 효과열을 모두 대조한다.
    using namespace netstorm::test::rawscene;std::ifstream input(NETSTORM_XLAT_FILE_READ_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) { // 최초 원문 기대값은 읽기만 하며 제품 출력으로 다시 쓰지 않는다.
        const auto row=Split(line,'\t');CHECK(row.size()==10);if (row.size()!=10) return;const auto p=Split(row[0],',');CHECK(p.size()==7);if (p.size()!=7) return;
        const auto source=Number(p[0]),which=Number(p[1]),key=Number(p[2]),sub=Number(p[3]),partial=Number(p[4]),scenario=Number(p[5]),seed=Number(p[6]);RawXlat table;
        if (seed || scenario==9) table.Load(scenario==9 ? which : which!=3 ? 3U:6U,Bytes(kSeed)); // 입력인 선행 장부만 구성하며 현재 결과를 미리 공급하지 않는다.
        const auto payload=Bytes(kSources.at(source));std::vector<std::uint8_t> stored(payload.begin(),payload.end());
        if (key) for (std::size_t index=0;index<stored.size();++index) stored[index]^=static_cast<std::uint8_t>(std::string_view("abc")[index%3]); // 파일 저장 BYTE만 명시 키로 만든다.
        std::array<std::uint8_t,4096> scratch{};RawBaseFileState state{0x65656565,0x65656565,std::bit_cast<std::int32_t>(0x65656565U),std::bit_cast<std::int32_t>(0x65656565U),0x65656565,0x65656565};RawBaseFileIOHooks io;XlatFileReadHooks hooks;std::string events,outcome="ok",returned="-",closed="-";std::int32_t position{};
        const auto event=[&](std::string text) { if (!events.empty()) events+=';';events+=text; }; // 원문 장치 진입만 순서대로 기록한다.
        hooks.hasBuffer=[&](auto index) { return table.HasBuffer(index); };
        hooks.path=[&](auto literal) { CHECK(literal=="case");const auto path=std::string(".\\d\\")+std::string(literal);event("F:"+netstorm::test::rawscene::Hex(Bytes(path)));return path; };
        hooks.exists=[&](auto path) { CHECK(path==".\\d\\case");event("P");return scenario!=1; };
        hooks.log=[&](auto reason,auto) { event(reason==XlatFileFailure::Missing ? "L:missing":"L:length"); };
        hooks.open=[&](auto& current,auto path,auto mode,auto create) { CHECK(path==".\\d\\case" && mode==0 && create==1 && !current.key && !current.keyLength);event("O");current={scenario==3 ? 0U:kFile,0x80U|sub,sub ? 37:0,sub ? scenario==4 ? -1:static_cast<std::int32_t>(payload.size()):0,key ? kKey:0,key ? 3U:0U};position=sub ? 37:0;if (scenario==2) throw std::runtime_error("open"); };
        io.tell=[&](auto file) { CHECK(file==kFile);event("T");return position; };
        io.seek=[&](auto file,auto offset,auto origin) { CHECK(file==kFile);event("S:"+std::to_string(offset)+":"+std::to_string(origin));position=origin==2 ? scenario==4 ? -1:(sub ? 37:0)+static_cast<std::int32_t>(payload.size()):offset; };
        io.read=[&](auto file,auto bytes,auto size,auto count) { CHECK(file==kFile);const auto start=position-(sub ? 37:0);CHECK(start>=0);const auto available=std::max<std::int64_t>(0,std::int64_t(payload.size())-partial-start);const auto actual=std::min<std::size_t>(std::size_t(size)*count,static_cast<std::size_t>(available));CHECK(bytes.size()>=std::size_t(size)*count);event("R:"+std::to_string(size)+":"+std::to_string(count)+":"+std::to_string(actual));if (scenario==6) throw std::runtime_error("read");if (actual) std::copy_n(stored.begin()+start,actual,bytes.begin());position+=static_cast<std::int32_t>(actual); };
        io.key=[](auto token,auto size) { CHECK(token==kKey && size==3);return Bytes("abc"); };
        io.close=[&](auto file) { CHECK(file==kFile);closed=HexOrEmpty(table.Buffer(which));event("C");if (scenario==7) throw std::runtime_error("close"); };
        io.freeKey=[&](auto token) { CHECK(token==kKey);event("K"); };
        hooks.releaseExisting=[](auto) { throw std::logic_error("원문 중복 진단을 무시하는 입력은 이 검사 대상이 아님"); };
        hooks.allocate=[&](auto index,auto size) { CHECK(index==which && size==payload.size());event("N:"+std::to_string(size));if (scenario==5) throw std::runtime_error("allocation");return XlatFileAllocation{1,table.AllocateFileBuffer(index,size)}; };
        hooks.normalize=[&](auto index) { CHECK(state.file==kFile);table.NormalizeFileBuffer(index); };
        hooks.parse=[&](auto index) { CHECK(!state.file && !state.key);event("Q");if (scenario==8) throw std::runtime_error("parse");table.ParseFileBuffer(index); };
        hooks.assertion=[&](auto sourceLine) { event("D:"+std::to_string(sourceLine));throw std::runtime_error("assertion"); };io.assertion=hooks.assertion;
        try { returned=std::to_string(ReadXlatFile(state,which,"case",io,hooks,scratch)); }catch (const std::exception& failure) { outcome=failure.what(); }
        const auto translations=HexOrEmpty(Bytes(table.Translate("Key",3)))+","+HexOrEmpty(Bytes(table.Translate("Key",6)));
        CHECK(outcome==row[1] && returned==row[2] && FileFields(state)==row[3] && std::uint32_t(table.HasBuffer(which))==Number(row[4]) && HexOrEmpty(table.Buffer(which))==row[5] && closed==row[6] && events==row[7] && table.Size()==Number(row[8]) && translations==row[9]);++rows;
    }
    CHECK(rows==252);
}

TEST_CASE(XlatFileRead_CurrentArchiveStartupKeepsSnapshotAndMissingOrder) { // 현재 backing과 고정 세 파일 순서를 Unicode VFS에서 확인한다.
    using namespace netstorm::test::fileprobe;const auto root=Work()/"xlat-787";const auto first=Bytes("*\nKey\n-\nOld\n=\n"),second=Bytes("*\nKey\n-\nNew\n=\n");Write(root/"xlat.tarc",Taff("d/xlat.german",first));BaseFileSystem files(root);CHECK(files.RegisterArchive(root/"xlat.tarc"));const auto snapshot=files.Read("d/xlat.german");Write(root/"xlat.tarc",Taff("d/xlat.german",second));RawXlat table;std::vector<std::string> missing;
    InitializeNativeXlat(table,files,[&](auto reason,auto path) { CHECK(reason==XlatFileFailure::Missing);missing.emplace_back(path); });
    CHECK(table.Translate("Key",3)=="New" && table.Translate("Key",6)=="Key" && table.HasBuffer(3) && !table.HasBuffer(1) && !table.HasBuffer(6));CHECK(missing==std::vector<std::string>({".\\d\\xlat.english",".\\d\\xlat.portuguese"}) && files.Read("d/xlat.german")==snapshot && std::equal(snapshot.begin(),snapshot.end(),first.begin()));
    Write(root/"d/xlat.portuguese",Bytes("*\nKey\n-\nPort\n=\n"));CHECK(LoadNativeXlatFile(table,files,6,"xlat.portuguese",[](auto,auto) { CHECK(false); }));CHECK(table.Translate("Key",3)=="New" && table.Translate("Key",6)=="Port");
}

TEST_CASE(XlatFileRead_EmptyNativeInstallGuardAndDuplicatePreserveOtherLanguage) { // 길이0의 원문 할당밖 쓰기를 성공으로 합성하지 않고 설치 접두·다른 언어를 보존한다.
    using namespace netstorm::test::fileprobe;const auto root=Work()/"xlat-empty-787";Write(root/"d/empty.xlat",{});BaseFileSystem files(root);RawXlat table;table.Load(3,Bytes(kSeed));
    CHECK(Failed([&] { LoadNativeXlatFile(table,files,1,"empty.xlat",[](auto,auto) { CHECK(false); }); }));CHECK(table.HasBuffer(1) && table.Buffer(1).empty() && table.Translate("Key",3)=="Seed");
    const auto before=files.FileNameOwner()->Index();CHECK(Failed([&] { LoadNativeXlatFile(table,files,1,"empty.xlat",[](auto,auto) { CHECK(false); }); }));CHECK(files.FileNameOwner()->Index()==before && table.Translate("Key",3)=="Seed"); // 중복 진단은 경로/Probe보다 먼저이며 legacyLoad(empty)는 여전히 설치 전에 거부한다.
    RawXlat legacy;CHECK(Failed([&] { legacy.Load(1,{}); }) && !legacy.HasBuffer(1));
}
