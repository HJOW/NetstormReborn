// 최초 원문92행의 File/직접 팔레트 prefix와 실제 현재 VFS 입구를 대조한다.
#include "TestSupport.h"
#include "RawSceneSupport.h"
#include "FileProbeTestSupport.h"
#include "client/PaletteFileRead.h"
#include "o/BaseFile.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <string>
#include <vector>

using namespace netstorm::client;
using namespace netstorm::o;
namespace { // 기대값은 최초 Native 바이트에서만 읽으며 보호 원본에는 쓰지 않는다.
constexpr std::uintptr_t kFile=0x12200000,kKey=0x12300100; // 독립 CPU의 File/키 불투명 토큰이다.
std::string FileFields(const RawBaseFileState& state) { // 원문 여섯 필드를 Native DWORD 표기로 직렬화한다.
    const std::array<std::uintptr_t,6> fields{{state.file,state.flags,std::uint32_t(state.offset),std::uint32_t(state.length),state.key,state.keyLength}};
    std::string text;for (const auto value:fields) { if (!text.empty()) text+=',';text+=std::to_string(value); }return text; // 원문 순서를 바꾸거나 현재 포인터를 절단하지 않는다.
}
template<class Operation> bool Failed(Operation operation) { // 명시 실패를 빈 팔레트 성공으로 바꾸지 않는다.
    try { operation(); }catch (const std::exception&) { return true; }return false;
}
}

TEST_CASE(PaletteFileRead_Native92DirectWritesPrefixAndFileLifetime) { // saved1024/파일/효과열만 대조하며 Native의 별도 OS·색검색 열은 Reader 검증으로 합산하지 않는다.
    using namespace netstorm::test::rawscene;
    std::ifstream input(NETSTORM_PALETTE_FILE_READ_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) { // 원문 입력은 재생만 하며 제품 출력으로 기대값을 만들지 않는다.
        const auto row=Split(line,'\t');CHECK(row.size()==7);if (row.size()!=7) return;
        const auto fields=Split(row[0],',');CHECK(fields.size()==10);if (fields.size()!=10) return;
        const auto length=Number(fields[0]),key=Number(fields[1]),sub=Number(fields[2]),partial=Number(fields[3]),scenario=Number(fields[7]),failRead=Number(fields[8]),returnAssert=Number(fields[9]);
        std::vector<std::uint8_t> stored(length);for (std::size_t index=0;index<stored.size();++index) stored[index]=static_cast<std::uint8_t>(((index*29+7)&255U)^(key ? static_cast<std::uint8_t>(std::string_view("abc")[index%3]):0U)); // 명시 원문 입력 파일만 준비한다.
        std::array<std::uint8_t,1024> saved;saved.fill(0xa5);std::array<std::uint8_t,4096> scratch{};std::string events,outcome="ok";std::int32_t position{};std::uint32_t reads{};
        RawBaseFileState state{kFile,0x65656565,std::bit_cast<std::int32_t>(0x65656565U),std::bit_cast<std::int32_t>(0x65656565U),kKey,3};RawBaseFileIOHooks io;PaletteFileReadHooks hooks;
        const auto event=[&](std::string text) { if (!events.empty()) events+=';';events+=text; }; // 실제 장치 진입 순서만 기록한다.
        hooks.open=[&](auto& current,auto name,auto mode,auto create) { CHECK(name=="palette.col" && mode==0 && create==1 && !current.key && !current.keyLength);event("O");current={scenario==2 ? 0U:kFile,0x80U|sub,sub ? 37:0,sub ? static_cast<std::int32_t>(length):0,key ? kKey:0,key ? 3U:0U};position=sub ? 37:0;if (scenario==1) throw std::runtime_error("open"); };
        io.tell=[&](auto file) { CHECK(file==state.file);event("T");return position; };
        io.seek=[&](auto file,auto offset,auto origin) { CHECK(file==state.file);event("S:"+std::to_string(offset)+":"+std::to_string(origin));position=origin==2 ? (sub ? 37:0)+static_cast<std::int32_t>(length):origin==1 ? position+offset:offset; };
        io.read=[&](auto file,auto bytes,auto size,auto count) {
            if (!file) { event("R:null:"+std::to_string(size)+":"+std::to_string(count));throw std::runtime_error("nullRead"); } // 가상 CRT의 명시 실패이며 실제 NULL CRT 성공을 합성하지 않는다.
            CHECK(file==kFile);const auto start=position-(sub ? 37:0);CHECK(start>=0);const auto requested=std::size_t(size)*count;
            const auto remaining=std::max<std::int64_t>(0,std::int64_t(length)-partial-start);const auto actual=std::min<std::size_t>(requested,static_cast<std::size_t>(remaining));CHECK(bytes.size()>=requested);
            ++reads;event("R:"+std::to_string(size)+":"+std::to_string(count)+":"+std::to_string(actual));if (scenario==3 && reads==failRead) throw std::runtime_error("read");
            if (actual) std::copy_n(stored.begin()+start,actual,bytes.begin());position+=static_cast<std::int32_t>(actual);
        }; // 짧은 읽기의 남은 saved_/현재 암호 작업 물리를 채우거나 재시도하지 않는다.
        io.key=[](auto token,auto size) { CHECK(token==kKey && size==3);return std::span(reinterpret_cast<const std::uint8_t*>("abc"),3); };
        io.close=[&](auto file) { CHECK(file==kFile);event("C");if (scenario==5) throw std::runtime_error("close"); };
        io.freeKey=[&](auto token) { CHECK(token==kKey);event("K"); };
        hooks.assertion=[&](auto sourceLine) { event("D:"+std::to_string(sourceLine));if (!returnAssert) throw std::runtime_error("assertion"); };io.assertion=hooks.assertion;
        hooks.apply=[&] { CHECK(state.file==kFile);event("A");if (scenario==4) throw std::runtime_error("apply"); }; // 실제 Screen 후단은 기존 적용 검사와 정적 접합으로 별도 확인한다.
        try { ReadPaletteFile(state,"palette.col",saved,io,hooks,scratch); }catch (const std::runtime_error& failure) { outcome=failure.what(); }
        CHECK(outcome==row[1] && FileFields(state)==row[2] && Hex(saved)==row[3] && events==row[4]);++rows;
    }
    CHECK(rows==92);
}

TEST_CASE(PaletteFileRead_CurrentArchiveAndLooseRgbPreserveSnapshotAndReservedBytes) { // 실제 현재 backing과 예약 BYTE를 독립 입력으로 확인한다.
    using namespace netstorm::test::fileprobe;
    const auto root=Work()/"palette-786";std::vector<std::uint8_t> first(1024,17),second(1024,29);Write(root/"palette.tarc",Taff("d/live.col",first));BaseFileSystem files(root);CHECK(files.RegisterArchive(root/"palette.tarc"));const auto snapshot=files.Read("d/live.col");
    Write(root/"palette.tarc",Taff("d/live.col",second));std::array<std::uint8_t,1024> saved;saved.fill(0xa5);std::uint32_t applied{};
    ReadNativePaletteFile(files,"d/live.col",saved,[&] { ++applied;CHECK(std::equal(saved.begin(),saved.end(),second.begin())); });CHECK(applied==1 && files.Read("d/live.col")==snapshot && snapshot==first);
    std::vector<std::uint8_t> rgb(776);for (std::size_t index=0;index<256;++index) { rgb[8+index*3]=std::uint8_t(index);rgb[9+index*3]=std::uint8_t(index+1);rgb[10+index*3]=std::uint8_t(index+2); } // 헤더 내용이 아닌 길이로 선택하는 원본 RGB 입력이다.
    Write(root/"d/rgb.col",rgb);saved.fill(0xa5);ReadNativePaletteFile(files,"d/rgb.col",saved,[&] { ++applied; });CHECK(applied==2);
    for (std::size_t index=0;index<256;++index) CHECK(saved[index*4]==std::uint8_t(index+2) && saved[index*4+1]==std::uint8_t(index+1) && saved[index*4+2]==std::uint8_t(index) && saved[index*4+3]==0xa5); // 같은 저장 칸의 예약 바이트는 파일로 덮지 않는다.
}

TEST_CASE(PaletteFileRead_MissingAndBadLengthStopBeforeApplyWithExistingSaved) { // 필수 Open/잘못된 길이 실패를 정상 이미지나 apply로 바꾸지 않는다.
    using namespace netstorm::test::fileprobe;
    const auto root=Work()/"palette-failure-786";BaseFileSystem files(root);std::array<std::uint8_t,1024> saved;saved.fill(0x71);const auto before=saved;bool applied{};
    CHECK(Failed([&] { ReadNativePaletteFile(files,"d/missing.col",saved,[&] { applied=true; }); }) && !applied && saved==before);
    const std::vector<std::uint8_t> bad(775,9);Write(root/"d/bad.col",bad);
    CHECK(Failed([&] { ReadNativePaletteFile(files,"d/bad.col",saved,[&] { applied=true; }); }) && !applied && saved==before);
    const std::vector<std::uint8_t> good(1024,23);Write(root/"d/bad.col",good);ReadNativePaletteFile(files,"d/bad.col",saved,[&] { applied=true; });CHECK(applied && std::all_of(saved.begin(),saved.end(),[](auto value) { return value==23; })); // 실패 뒤 같은 VFS와 파일을 다시 읽는다.
}
