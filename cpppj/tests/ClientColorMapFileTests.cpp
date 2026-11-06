// 현재 CRT 캐시와 실제 원문9표의 합성·부분파일·실패 수명을 검사한다. 원본/fixture는 읽기만 한다.
#include "TestSupport.h"
#include "FileProbeTestSupport.h"
#include "client/ClientColorSortInput.h"
#include "client/OwnerColorTableState.h"
#include "client/Screen.h"
#include "o/RawColorSortPrimary.h"
#include "o/RawColorMapCache.h"
#include "o/BaseFile.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace { // 새 검사별 Unicode 작업 폴더와 최초 Native 입력만 사용한다.
using namespace netstorm::client;
constexpr std::array<std::size_t,9> kFileOrder{2,7,6,4,3,5,1,0,8}; // 원문43c4e0의9read/write 순서이며 주소 순서와 다르다.
std::filesystem::path Work() { // 중단 자료를 덮지 않고 매검사 새 폴더를 만든다.
    const auto path=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-resume-967/io"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/u8"캐시😀";
    std::filesystem::create_directories(path);return path;
}
void Write(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) { // 이미 만든 이번 작업 폴더의 파일만 작성한다.
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!out) throw std::runtime_error("색표 검사 파일 작성 실패");
}
std::vector<std::uint8_t> Read(const std::filesystem::path& path) { // 캐시 파일의 정확한 원시 바이트를 재적재한다.
    std::ifstream in(path,std::ios::binary);if (!in) throw std::runtime_error("색표 검사 파일 열기 실패");return {std::istreambuf_iterator<char>(in),{}};
}
std::vector<std::string> Split(std::string_view text,char separator) { // 최초 TSV의 마지막 칸까지 순서대로 분리한다.
    std::vector<std::string> out;std::size_t start{};
    for (;;) { const auto end=text.find(separator,start);out.emplace_back(text.substr(start,end==text.npos?text.npos:end-start));if (end==text.npos) return out;start=end+1; } // 빈 칸도 보존한다.
}
std::vector<std::uint8_t> Unhex(const std::string& text) { // 자료를 제품 계산 없이 바이트로만 해석한다.
    std::vector<std::uint8_t> bytes(text.size()/2);for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>(std::stoul(text.substr(i*2,2),nullptr,16));return bytes; // 원시 BYTE 순서다.
}
struct NativeInput { // 최초53비트 원문 입력과 아홉 출력이다.
    std::array<std::uint32_t,256> logical{}; // 원문 초기RGBX다.
    std::array<ColorMap,9> expected{}; // 원문 주소 순서의9표다.
    std::uint32_t red{}; // 같은 관측의 현재 명명 빨강이다.
    NativeInput() { // 기존 fixture를 바꾸거나 새 제품 결과로 기대값을 생성하지 않는다.
        std::ifstream in(std::filesystem::path(__FILE__).parent_path()/"fixtures/colorsort-fixed-input-x86.tsv");std::string line;
        while (std::getline(in,line)) { // 첫53비트 행만 입력으로 선택한다. 전체6행 대조는 기존 검사다.
            if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto fields=Split(line,'\t');if (fields.at(0)!="639") continue;
            red=static_cast<std::uint32_t>(std::stoul(fields.at(2)));const auto bytes=Unhex(fields.at(3));
            for (std::size_t i=0;i<logical.size();++i) for (std::size_t b=0;b<4;++b) logical[i]|=std::uint32_t(bytes.at(i*4+b))<<(8*b); // 실제little endian RGBX다.
            const auto maps=Split(fields.at(6),':');for (std::size_t i=0;i<expected.size();++i) { const auto value=Unhex(maps.at(i));std::copy(value.begin(),value.end(),expected[i].begin()); } // 원문 표만 복사한다.
            return;
        }
        throw std::runtime_error("색표 Native53 입력 없음");
    }
};
void Capture(ClientColorSortInput& owner,const NativeInput& native) { // 미작성padding도 명시 원시 입력으로 준다.
    std::array<std::uint8_t,netstorm::o::kColorSortPrimaryBytes> allocation;allocation.fill(0xa5);owner.CaptureWithPrimary(native.logical,allocation);
}
std::vector<std::uint8_t> Pack(const std::array<ColorMap,9>& maps) { // 명시 파일 순서만 적용하며 각 표의 내용은 다시 계산하지 않는다.
    std::vector<std::uint8_t> out;for (const auto index:kFileOrder) out.insert(out.end(),maps[index].begin(),maps[index].end());return out; // 원문9개256BYTE다.
}
}

// 캐시miss의 실제 생성/쓰기와 두번째 새 소유자의hit가 같은 Native9표/현재 소유자 표를 유지한다.
TEST_CASE(ClientColorMapFile_NativeMissPersistsAndFreshOwnerHitsSameBytes) {
    NativeInput native;const auto root=Work(),path=root/L"!color.dat";netstorm::o::BaseFileSystem files(root);ClientColorSortInput first;Capture(first,native);std::uint32_t redraw{},calls{};
    first.InitializeCache(files,path,native.red,[&] { ++calls;first.OwnerTables()->Initialize(first.Primary(),188,82,redraw); });
    CHECK(first.MapsReady() && first.Maps()==native.expected && calls==1 && first.OwnerTables()->Ready());const auto expected=Pack(native.expected);CHECK(Read(path)==expected && expected.size()==2304);
    constexpr std::array<std::uint8_t,8> colors{84,249,184,140,150,112,156,222}; // 최초PE의8색을 각 물리행에 대조한다.
    for (std::size_t row=0;row<8;++row) for (std::size_t color=0;color<256;++color) { // hit와무관한부모접두전체4096BYTE다.
        CHECK(first.OwnerSolid()[row*256+color]==colors[row]);CHECK(first.OwnerHighlight()[row*256+color]==(first.Primary()->Lightness(color)>0.800000011920929?colors[row]:static_cast<std::uint8_t>(color)));
    }
    ClientColorSortInput second;Capture(second,native);second.InitializeCache(files,path,255,[&] { ++calls; }); // hit에서 다른 빨강으로 다시 계산하면 원문destroy 표와 달라진다.
    CHECK(calls==2 && second.Maps()==native.expected && Read(path)==expected);
}

// 알려진 표를 실제 파일에서 읽고 화면 명암이 같은 현재 캐시/후속재생성 표를 소비하는지 확인한다.
TEST_CASE(ClientColorMapFile_SentinelHitFeedsCurrentScreenMapsWithoutRebuild) {
    NativeInput native;const auto root=Work(),path=root/L"!color.dat";netstorm::o::BaseFileSystem files(root);std::array<ColorMap,9> maps{};
    for (std::size_t row=0;row<maps.size();++row) for (std::size_t color=0;color<256;++color) maps[row][color]=static_cast<std::uint8_t>(row*17+color); // 계산으로우연히일치하지않는명시행이다.
    auto bytes=Pack(maps);bytes.push_back(0x77);Write(path,bytes);ClientColorSortInput owner;Capture(owner,native);unsigned calls{};owner.InitializeCache(files,path,native.red,[&] { ++calls; });
    CHECK(owner.Maps()==maps && calls==1 && Read(path)==bytes);const auto* same=&owner.Maps(); // 후행바이트를무시하고파일을다시쓰지않는다.
    Screen screen(nullptr,nullptr,640,480);screen.BindColorMaps([&] { const auto& current=owner.Maps();return GumpShadeMaps{current[4],current[6]}; }); // Init/창/표면을생성하지않고실제조회만실행한다.
    CHECK(screen.ShadeMaps().bright==maps[4] && screen.ShadeMaps().dark==maps[6]);
    owner.Rebuild(native.red);CHECK(&owner.Maps()==same && owner.Maps()==native.expected && Read(path)==bytes);
    CHECK(screen.ShadeMaps().bright==native.expected[4] && screen.ShadeMaps().dark==native.expected[6]);
}

// 실제짧은fread는첫목적표접두만바꾸며원문부모는나머지표를재생성하거나파일을고치지않는다.
TEST_CASE(ClientColorMapFile_ActualShortReadKeepsZeroTailAndFile) {
    NativeInput native;const auto root=Work(),path=root/L"!color.dat";netstorm::o::BaseFileSystem files(root);const std::vector<std::uint8_t> bytes{7,11,19};Write(path,bytes);ClientColorSortInput owner;Capture(owner,native);unsigned calls{};
    owner.InitializeCache(files,path,native.red,[&] { ++calls; });CHECK(calls==1 && owner.MapsReady() && Read(path)==bytes);
    for (std::size_t row=0;row<9;++row) for (std::size_t color=0;color<256;++color) CHECK(owner.Maps()[row][color]==(row==2 && color<bytes.size()?bytes[color]:0)); // BSS0과실제파일접두를구분한다.
}

// 미생성부모의reader실패→재계산→writer실패를숨기지않으며상위폴더나성공파일을합성하지않는다.
TEST_CASE(ClientColorMapFile_MissingParentPreservesOwnerAndBuildPrefixButFails) {
    NativeInput native;const auto root=Work(),path=root/L"missing"/L"!color.dat";netstorm::o::BaseFileSystem files(root);ClientColorSortInput owner;Capture(owner,native);unsigned calls{};bool failed=false;
    try { owner.InitializeCache(files,path,native.red,[&] { ++calls; }); }catch (const netstorm::o::ColorCacheUnableToOpen&) { failed=true; }
    CHECK(failed && calls==1 && owner.Maps()==native.expected && !std::filesystem::exists(path.parent_path()));
}

// 등록snapshot과다른현재암호화TAFF를9번순차로읽어FILE/키위치/마지막반납을검증한다.
TEST_CASE(ClientColorMapFile_CurrentEncryptedArchiveDoesNotUseRegisteredPayloadSnapshot) {
    NativeInput native;const auto root=Work(),path=root/L"cache.tarc";const std::vector<std::uint8_t> first(2304,31),second(2304,147); // 같은metadata와서로다른명시내용이다.
    const auto archive=[](std::vector<std::uint8_t> payload) { // 실제원문키로payload만XOR하고등록헤더의암호bit를설정한다.
        netstorm::o::ApplyXor(payload);auto bytes=netstorm::test::fileprobe::Taff("d/live.dat",payload);netstorm::test::rawscene::Put(bytes,0x18,2);return bytes;
    };
    Write(path,archive(first));netstorm::o::BaseFileSystem files(root);CHECK(files.RegisterArchive(path));const auto snapshot=files.Read("d/live.dat");CHECK(snapshot==first);
    Write(path,archive(second));ClientColorSortInput owner;Capture(owner,native);unsigned calls{};owner.InitializeCache(files,"d/live.dat",native.red,[&] { ++calls; });
    CHECK(calls==1 && snapshot==first);for (const auto& map:owner.Maps()) CHECK(std::all_of(map.begin(),map.end(),[](auto byte) { return byte==147; })); // rawFile은등록시payload사본으로대체되지않는다.
    std::filesystem::rename(path,root/L"closed.tarc");CHECK(std::filesystem::exists(root/L"closed.tarc")); // FILE/키반납뒤의실제rename도성공해야한다.
}
