// 원문 BMP 바이트와 실제 Unicode 파일·연속 프레임 반납을 검증한다.
#include "TestSupport.h"
#include "client/RawIndexedBitmap.h"
#include "client/IndexedBitmapFile.h"
#include "client/RawCaptureSequence.h"
#include "o/BaseFile.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <map>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // 실제 공유 거부 확인에서 Win32 매크로 충돌을 막는다.
#endif
#include <windows.h>
#endif

using namespace netstorm::client; // 같은 제품 본체/파일 어댑터를 조립한다.
namespace { // Native의 명시 장치와 검사 소유 파일만 제공한다.
constexpr std::uint32_t kPixels=0x11000000,kOther=0x11001000,kPalette=0x11002000,kOtherPalette=0x11003000; //독립원문장치주소다.
std::vector<std::string> Fields(std::string_view text,char delimiter) { // 마지막 필드를 포함한 원시 TSV 분리다.
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find(delimiter,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos:end-begin));if (end==text.npos) return result;begin=end+1; } // 각 구간의 바이트를 보존한다.
}
std::vector<std::uint8_t> Bytes(std::string_view text) { // '-'만 빈 출력으로 해석한다.
    std::vector<std::uint8_t> result;if (text=="-") return result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16))); // 원시 BYTE를 복원한다.
    return result;
}
std::string Hex(std::span<const std::uint8_t> bytes) { // 현재 전체 BYTE를 원문 표기와 대조한다.
    constexpr char digits[]="0123456789abcdef";std::string result;
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // 메모리 순서를 보존한다.
    return result;
}
std::string TextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } // 파일명을 문자 변환 없이 기록한다.
IndexedBitmapScratch Scratch(std::string_view text) { // 세 지역 구간의 초기 물리만 읽는다.
    const auto bytes=Bytes(text);if (bytes.size()!=1078) throw std::runtime_error("BMP 초기 지역 물리 길이");IndexedBitmapScratch result;
    std::copy_n(bytes.begin(),14,result.file.begin());std::copy_n(bytes.begin()+14,40,result.info.begin());std::copy_n(bytes.begin()+54,1024,result.palette.begin());return result;
}
std::string ScratchHex(const IndexedBitmapScratch& scratch) { return Hex(scratch.file)+Hex(scratch.info)+Hex(scratch.palette); } // 원문이 쓰는 세 구간만 직렬화한다.
std::vector<std::vector<std::string>> Fixture() { // 표의 기대 출력은 장치 입력으로 사용하지 않는다.
    std::ifstream input(NETSTORM_INDEXED_BITMAP_FIXTURE);if (!input) throw std::runtime_error("BMP 원문 fixture 열기 실패");std::string line;std::getline(input,line);std::vector<std::vector<std::string>> result;
    while (std::getline(input,line)) { if (!line.empty() && line.back()=='\r') line.pop_back();result.push_back(Fields(line,'\t')); } // 원시 행 순서를 유지한다.
    return result;
}
std::vector<std::string> NormalRow() { // 실제 파일 검사도 독립 원문의4×3/stride8/0 초기 물리를 기대값으로 쓴다.
    for (const auto& row:Fixture()) if (row.size()==13 && row[0]=="0" && row[1]=="0" && row[2]=="4" && row[3]=="3" && row[4]=="8" && row[5]=="0" && row[6]=="1" && row[7]=="0") return row; // 원문 입력으로만 선택한다.
    throw std::runtime_error("BMP 실제 파일의 원문 입력 행 부재");
}
struct Scene { // 파일 경계는 기록하고 실제 헤더/색표/행은 제품 코드가 만든다.
    IndexedBitmapInput input; // 경계 효과와 부모가 공유하는 현재7DWORD다.
    IndexedBitmapScratch scratch; // 처음부터 끝까지 같은 세 지역 배열이다.
    std::map<std::uint32_t,std::vector<std::uint8_t>> memory; // 선언된 화면/색표 물리다.
    std::map<std::string,unsigned> calls; // 명시 장치의 호출 번호다.
    std::vector<std::uint8_t> output; // 실제 write 요청 순서의 바이트다.
    std::string trace; // 선택한 장치와 현재 인자의 전체 순서다.
    unsigned opened{},mode{}; // 원문의 FILE 유무와 명시 변이 번호다.
    explicit Scene(const std::vector<std::string>& row):scratch(Scratch(row.at(8))),opened(static_cast<unsigned>(std::stoul(row.at(6)))),mode(static_cast<unsigned>(std::stoul(row.at(7)))) { // 초기8열과scratchIn만 입력이다.
        input={kPixels,static_cast<std::uint32_t>(std::stoul(row[0])),static_cast<std::uint32_t>(std::stoul(row[1])),static_cast<std::uint32_t>(std::stoul(row[2])),static_cast<std::uint32_t>(std::stoul(row[3])),static_cast<std::uint32_t>(std::stoul(row[4])),kPalette};
        constexpr std::array<std::array<std::uint32_t,4>,4> seeds{{{kPixels,37,11,512},{kOther,19,17,512},{kPalette,13,7,1024},{kOtherPalette,29,3,1024}}}; // Native와 같은 독립 입력 시드다.
        for (const auto& values:seeds) { // 각 선언 물리를 별도로 만든다.
            auto& bytes=memory[values[0]];bytes.resize(values[3]);for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(index*values[1]+values[2]); // 각 행/색/예약 바이트를 구별한다.
        }
    }
    void Event(std::string name,std::vector<std::string> args={}) { // 실제 진입 인자를 먼저 기록한다.
        if (!trace.empty()) trace+=';';trace+=name;for (const auto& arg:args) trace+=':'+arg; // 관측 필드를 순서대로 붙인다.
        const auto trigger=name+std::to_string(++calls[name]);
        if (mode==1 && trigger=="construct1") { input.width=5;input.height=2; }
        if ((mode==2 && trigger=="write1") || (mode==3 && trigger=="write2")) input.palette=kOtherPalette;
        if (mode==4 && trigger=="write3") { input.pixels=kOther;input.x=input.y=1;input.pitch=7; }
        if (mode==5 && trigger=="write4") { input.pitch=3;input.pixels=kOther; }
        if (mode==6 && trigger=="write4") { input.width=17;input.height=1; }
        if (mode==7 && trigger=="write1") { scratch.info[24]=0xa1;scratch.info[25]=0xb2;scratch.info[26]=0xc3;scratch.info[27]=0xd4;scratch.palette[3]=0x5a; }
    }
    IndexedBitmapHooks Hooks() { // 순수 메모리 조회와 실제 파일 경계를 구별한다.
        IndexedBitmapHooks h;h.construct=[this](auto name,auto mode_,auto create) { Event("construct",{TextHex(name),std::to_string(mode_),std::to_string(create)}); };h.isOpen=[this] { Event("open");return opened; };
        h.write=[this](auto bytes,auto offset) { output.insert(output.end(),bytes.begin(),bytes.end());Event("write",{std::to_string(bytes.size()),std::to_string(static_cast<std::uint32_t>(offset))}); };h.destroy=[this] { Event("destroy"); };
        h.memory=[this](auto token,auto offset,auto length)->std::span<const std::uint8_t> { Event("read",{std::to_string(token),std::to_string(offset),std::to_string(length)});const auto& bytes=memory.at(token);if (offset>bytes.size() || length>bytes.size()-offset) throw std::out_of_range("BMP 검사 선언 물리");return std::span(bytes).subspan(offset,length); };return h;
    }
    std::string Arguments() const { return std::to_string(input.pixels)+':'+std::to_string(input.x)+':'+std::to_string(input.y)+':'+std::to_string(input.width)+':'+std::to_string(input.height)+':'+std::to_string(input.pitch)+':'+std::to_string(input.palette); } // 실제 현재7DWORD다.
};
std::filesystem::path TestRoot(std::string_view label) { // 모든 출력은 검사 소유의 새 Unicode 디렉터리에만 둔다.
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();const auto root=std::filesystem::temp_directory_path()/std::filesystem::path(u8"Netstorm BMP 저장 😀")/(std::string(label)+'-'+std::to_string(stamp));std::filesystem::create_directories(root);return root;
}
void Seed(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) { // 기존 끝 보존을 확인할 검사 입력만 만든다.
    std::ofstream file(path,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!file) throw std::runtime_error("BMP 검사 입력 파일 실패");
}
template<class F> bool Rejects(F function) { try { function(); }catch (const std::exception&) { return true; }return false; } // 제품 예외와 앞선 실제 파일 쓰기를 함께 관측한다.
}

// 14/40/1024BYTE·미정 필드·홀수 행·콜백 뒤 현재 인자를 전체 원문 출력에 대조한다.
TEST_CASE(IndexedBitmap_Native336HeadersPaletteRowsAndCurrentArguments) {
    std::size_t count{};for (const auto& row:Fixture()) { // 원문 각 입력은 독립된 물리를 사용한다.
        CHECK(row.size()==13);if (row.size()!=13) return;Scene scene(row);const auto before=scene.memory;const auto hooks=scene.Hooks();WriteRawIndexedBitmap("capture.bmp",scene.input,scene.scratch,hooks);
        if (scene.trace!=row[12]) std::printf("BMP mismatch row=%zu mode=%s trace=%s\n",count,row[7].c_str(),scene.trace.c_str());
        CHECK(ScratchHex(scene.scratch)==row[9] && (scene.output.empty() ? "-":Hex(scene.output))==row[10] && scene.Arguments()==row[11] && scene.trace==row[12]);CHECK(scene.memory==before);++count;
    }
    CHECK(count==336);
}

// 실제 wide CRT와 공통 BaseFile의 생성/비절삭/공유 거부를 같은 원문 바이트로 검사한다.
TEST_CASE(IndexedBitmap_ActualUnicodeFileCreationTailAndOpenFailure) {
    const auto row=NormalRow();Scene scene(row);const auto expected=Bytes(row[10]);const auto root=TestRoot("tail");const auto created=root/std::filesystem::path(u8"새 프레임 😀.bmp"),existing=root/std::filesystem::path(u8"이전 프레임 😀.bmp");
    const IndexedBitmapArea area{0,0,4,3,8};WriteIndexedBitmapFile(created,scene.memory[kPixels],area,scene.memory[kPalette],scene.scratch);CHECK(netstorm::o::ReadFileBytes(created)==expected);
    std::vector<std::uint8_t> previous(expected.size()+29,0x69);Seed(existing,previous);scene.scratch=Scratch(row[8]);WriteIndexedBitmapFile(existing,scene.memory[kPixels],area,scene.memory[kPalette],scene.scratch);std::copy(expected.begin(),expected.end(),previous.begin());CHECK(netstorm::o::ReadFileBytes(existing)==previous);
    const auto missing=root/std::filesystem::path(u8"없는 폴더")/"image.bmp";CHECK(Rejects([&] { WriteIndexedBitmapFile(missing,scene.memory[kPixels],area,scene.memory[kPalette],scene.scratch); }));CHECK(!std::filesystem::exists(missing.parent_path()));
#ifdef _WIN32
    const auto lock=CreateFileW(existing.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);CHECK(lock!=INVALID_HANDLE_VALUE);
    if (lock!=INVALID_HANDLE_VALUE) { CHECK(Rejects([&] { WriteIndexedBitmapFile(existing,scene.memory[kPixels],area,scene.memory[kPalette],scene.scratch); }));CloseHandle(lock);CHECK(netstorm::o::ReadFileBytes(existing)==previous); }
#endif
    std::printf("BMP_FILE_ROOT %s\n",netstorm::platform::PathToUtf8(root).c_str());
}

// FILE0은 표면을 읽지 않으며 물리 오류 뒤에도 실제 FILE을 반납한다.
TEST_CASE(IndexedBitmap_SelectedFileBranchAndPhysicalFailureCleanup) {
    IndexedBitmapInput input;IndexedBitmapScratch scratch;scratch.file.fill(0xa7);const auto before=ScratchHex(scratch);std::size_t constructs{},destroys{};IndexedBitmapHooks h;h.construct=[&](auto,auto mode,auto create) { CHECK(mode==1 && create==1);++constructs; };h.isOpen=[] { return 0U; };h.destroy=[&] { ++destroys; };WriteRawIndexedBitmap("unused.bmp",input,scratch,h);CHECK(constructs==1 && destroys==1 && ScratchHex(scratch)==before);
    const auto row=NormalRow();Scene scene(row);const auto root=TestRoot("partial"),path=root/"partial.bmp";CHECK(Rejects([&] { WriteIndexedBitmapFile(path,scene.memory[kPixels],{0,0,4,3,8},{},scene.scratch); }));const auto bytes=netstorm::o::ReadFileBytes(path),expected=Bytes(row[10]);CHECK(bytes==std::vector<std::uint8_t>(expected.begin(),expected.begin()+54));
#ifdef _WIN32
    const auto exclusive=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);CHECK(exclusive!=INVALID_HANDLE_VALUE);if (exclusive!=INVALID_HANDLE_VALUE) CloseHandle(exclusive); // 예외 뒤 남은 FILE이 없어야 배타적 재열기가 된다.
#endif
    std::printf("BMP_PARTIAL_ROOT %s\n",netstorm::platform::PathToUtf8(root).c_str());
}

// 실제 별도 할당으로 두 프레임을 보존하고 기존 연속 저장 부모가 새 파일 어댑터에 전달한다.
TEST_CASE(CaptureSequence_ActualHeapSnapshotsAndIndexedFiles) {
    const auto row=NormalRow();Scene source(row);std::vector<std::uint8_t> pixels(source.memory[kPixels].begin(),source.memory[kPixels].begin()+24);std::map<std::uint32_t,std::vector<std::uint8_t>> heap;std::uint32_t next=10;CaptureSequenceState state;CaptureSequenceScreen screen{4,3,8,1,2};const auto root=TestRoot("sequence");const auto prefix=netstorm::platform::PathToUtf8(root/"sequence");std::size_t locks{},unlocks{},files{};CaptureSequenceHooks h;
    h.lock=[&] { ++locks; };h.unlock=[&] { ++unlocks; };h.allocate=[&](auto size) { const auto token=next++;heap[token].resize(size);return token; };h.memory=[&](auto token,auto length)->std::span<std::uint8_t> { auto& bytes=token==1 ? pixels:heap.at(token);if (length>bytes.size()) throw std::out_of_range("실제 연속 저장 검사 물리");return bytes; };
    h.prefix=[&] { return prefix; };h.bitmap=[&](auto name,auto token,auto x,auto y,auto width,auto height,auto pitch,auto palette) { CHECK(palette==2 && pitch==4);IndexedBitmapScratch work;WriteIndexedBitmapFile(netstorm::platform::PathFromUtf8(name),heap.at(token),{x,y,width,height,pitch},source.memory[kPalette],work);++files; };h.release=[&](auto token) { CHECK(heap.erase(token)==1); };
    h.report=[&](auto first,auto last) { CHECK(first==prefix+"000.bmp" && last==prefix+"002.bmp");return std::string("written"); };h.notice=[](auto message,auto width,auto height) { CHECK(message=="written" && width==0 && height==0); };
    StartCaptureSequence(state);AppendCaptureSequenceFrame(state,screen,h);std::fill(pixels.begin(),pixels.end(),std::uint8_t{0x5a});AppendCaptureSequenceFrame(state,screen,h);std::fill(pixels.begin(),pixels.end(),std::uint8_t{0xff});CHECK(heap.size()==2 && state.count==2);StopCaptureSequence(state,screen,h);
    auto first=Bytes(row[10]);const std::array<std::uint8_t,12> payload{51,88,125,162,159,196,233,14,11,48,85,122};std::copy(payload.begin(),payload.end(),first.begin()+1078);auto second=first;std::fill(second.begin()+1078,second.end(),std::uint8_t{0x5a}); // 원문 형식의 헤더 기대값과 명시된 두 스냅샷의 픽셀을 대조한다.
    CHECK(netstorm::o::ReadFileBytes(root/"sequence000.bmp")==first && netstorm::o::ReadFileBytes(root/"sequence001.bmp")==second);CHECK(!std::filesystem::exists(root/"sequence002.bmp") && files==2 && locks==2 && unlocks==2 && heap.empty() && state.active==0 && state.count==2 && state.frames[0]==0 && state.frames[1]==0);
    std::printf("BMP_SEQUENCE_ROOT %s\n",netstorm::platform::PathToUtf8(root).c_str());
}
