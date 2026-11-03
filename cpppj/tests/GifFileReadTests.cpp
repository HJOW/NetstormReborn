// 원문 파일부모의 최초 관측과 현재 backing GIF·Decode 실패 전 치수를 대조한다.
#include "TestSupport.h"
#include "FileProbeTestSupport.h"
#include "client/GifFileRead.h"
#include "client/GifImage.h"
#include "client/ClientKencloudGlobals.h"
#include "o/BaseFile.h" // 실제 VFS와 원문 키 상수의 완전 정의를 독립 검사에 제공한다.
#include "o/ConfigInterface.h"
#include "o/SpStore.h"
#include <array>
#include <bit>
#include <chrono>
#include <fstream>
#include <sstream>
using namespace netstorm::client;
using namespace netstorm::o;
namespace { // 새 scratch 입력만 쓰고 기존 fixture/원본을 수정하지 않는다.
constexpr std::uintptr_t kFile=0x12200000,kKey=0x12300100,kInput=0x12302000,kBitmap=0x12310000; // Native 관측장치의 불투명 토큰이다.
std::vector<std::uint8_t> GifInput(std::uint16_t width,std::uint16_t height,std::uint8_t color,bool invalid=false) { // 작은 Native 행과 같은 명시 코드 입력이며 기대값은 계산하지 않는다.
    std::vector<std::uint8_t> result{'G','I','F','8','9','a'};const auto word=[&](auto value) { result.push_back(static_cast<std::uint8_t>(value));result.push_back(static_cast<std::uint8_t>(value>>8)); }; // 실제 리틀엔디언 WORD 입력이다.
    word(width);word(height);result.insert(result.end(),{0,0,0,0x2c});word(0);word(0);word(width);word(height);result.push_back(0);result.push_back(invalid ? 1 : 2);
    std::uint64_t bits{};std::size_t shift{};for (std::size_t index=0;index<std::size_t(width)*height;++index) { bits|=std::uint64_t(4) << shift;shift+=3;bits|=std::uint64_t(color)<<shift;shift+=3; } // 매 픽셀 앞 clear로 입력 코드 폭3을 고정한다.
    bits|=std::uint64_t(5)<<shift;shift+=3;const auto count=(shift+7)/8;result.push_back(static_cast<std::uint8_t>(count));for (std::size_t index=0;index<count;++index) result.push_back(static_cast<std::uint8_t>(bits>>(index*8))); // 실제 payload BYTE를 낮은 비트부터 쓴다.
    result.insert(result.end(),{0,0x3b});return result;
}
std::string Numbers(const RawBaseFileState& state) { const std::array<std::uintptr_t,6> values{{state.file,state.flags,static_cast<std::uint32_t>(state.offset),static_cast<std::uint32_t>(state.length),state.key,state.keyLength}};std::string text;for (const auto value:values) { if (!text.empty()) text+=',';text+=std::to_string(value); }return text; } // 현재 여섯 필드의 Native DWORD 기록이다.
std::filesystem::path GifWork() { return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-gif-live-reader-781/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/std::filesystem::path(u8"그림😀"); } // 각 검사의 새 Unicode scratch다.
template<class Function> bool GifFailure(Function operation) { try { operation(); }catch (const std::exception&) { return true; }return false; } // 명시 실패를 빈 성공 이미지로 대신하지 않는다.
}

TEST_CASE(GifFileRead_Native56ExactZeroDimensionsDecodeAndCleanup) { // Native 최초56행의 전체 효과열/현재필드/선택 실제픽셀을 재생한다.
    using namespace netstorm::test::rawscene;std::ifstream file(NETSTORM_GIF_FILE_READ_FIXTURE);CHECK(file.good());std::string line;std::getline(file,line);std::size_t rows{};
    while (std::getline(file,line)) { // 모든 행은 원문 CPU에서 만든 기대값을 그대로 사용한다.
        const auto row=Split(line,'\t');CHECK(row.size()==8);if (row.size()!=8) return;const auto fields=Split(row[0],',');CHECK(fields.size()==8);if (fields.size()!=8) return;
        const auto w=Number(fields[0]),h=Number(fields[1]),color=Number(fields[2]),key=Number(fields[3]),sub=Number(fields[4]),partial=Number(fields[5]),scenario=Number(fields[6]),actual=Number(fields[7]);const auto payload=GifInput(static_cast<std::uint16_t>(w),static_cast<std::uint16_t>(h),static_cast<std::uint8_t>(color),scenario==6);
        auto stored=payload;if (key) for (std::size_t index=0;index<stored.size();++index) stored[index]^=std::string_view("abc")[index%3]; // Native 명시 키의 저장 BYTE만 작성한다.
        RawBaseFileState state{kFile,0x65656565,std::bit_cast<std::int32_t>(0x65656565U),std::bit_cast<std::int32_t>(0x65656565U),kKey,3};RawBaseFileIOHooks io;GifFileReadHooks hooks;std::string events,inputHex="-",outcome="ok";std::vector<std::uint8_t> input,pixels(std::size_t(w)*h,0xcd);std::array<std::uint8_t,4096> scratch{};std::int32_t width=std::bit_cast<std::int32_t>(0xdeadbeefU),height=0x12345678,position{};std::size_t used{},available=payload.size()-partial,actualRows{};
        const auto event=[&](std::string text) { if (!events.empty()) events+=';';events+=text; }; // 실제 호출 순서의 한 효과만 추가한다.
        hooks.open=[&](auto& current,auto name,auto mode,auto create) { CHECK(name=="input.gif" && mode==0 && create==1 && !current.key && !current.keyLength);event("O");current={scenario==2 ? 0U:kFile,0x80U|sub,sub ? 37:0,sub ? static_cast<std::int32_t>(payload.size()):0,key ? kKey:0,key ? 3U:0U};position=sub ? 37:0;if (scenario==1) throw std::runtime_error("open"); };
        io.tell=[&](auto current) { CHECK(current==kFile);event("T");return position; };io.seek=[&](auto current,auto offset,auto origin) { CHECK(current==kFile);event("S:"+std::to_string(origin));position=origin==2 ? static_cast<std::int32_t>(payload.size()):offset; };
        io.read=[&](auto current,auto bytes,auto size,auto count) { CHECK(current==kFile && count==1);const auto read=std::min<std::size_t>(size,available);if (!key) CHECK(std::all_of(bytes.begin(),bytes.end(),[](auto value) { return value==0; }));event("R:"+std::to_string(size)+":"+std::to_string(read));if (scenario==4) throw std::runtime_error("read");std::copy_n(stored.begin()+static_cast<std::ptrdiff_t>(used),read,bytes.begin());used+=read;available-=read;position+=static_cast<std::int32_t>(read); };
        io.key=[](auto token,auto size) { CHECK(token==kKey && size==3);return std::span(reinterpret_cast<const std::uint8_t*>("abc"),3); };io.close=[&](auto current) { CHECK(current==kFile);event("C");if (scenario==8) throw std::runtime_error("close"); };io.freeKey=[&](auto token) { CHECK(token==kKey);event("K"); };
        hooks.allocateInput=[&](auto size) { event("I:"+std::to_string(size));if (scenario==3) throw std::runtime_error("input");input.assign(size,0xcd);return GifFileAllocation{kInput,input}; };
        hooks.allocateBitmap=[&](auto size)->std::uintptr_t { CHECK(width==static_cast<std::int32_t>(w) && height==static_cast<std::int32_t>(h) && size==w*h && state.file==kFile);event("B:"+std::to_string(size));if (scenario==5) throw std::runtime_error("bitmap");return kBitmap; };
        hooks.decode=[&](auto token,auto bytes,auto currentWidth,auto currentHeight) { CHECK(token==kBitmap && state.file==kFile && currentWidth==w && currentHeight==h && width==static_cast<std::int32_t>(w) && height==static_cast<std::int32_t>(h));event("X");inputHex=Hex(bytes);if (scenario==6) throw std::runtime_error("decode");if (actual) { pixels=DecodeBackgroundGif(bytes).indices;actualRows=h; } }; // 예외경계와 정상 실제LZW 픽셀 관측을 구별한다.
        hooks.releaseInput=[&](auto token) { CHECK(token==kInput);event("F");if (scenario==7) throw std::runtime_error("inputFree"); };hooks.assertion=[&](auto sourceLine) { event("D:"+std::to_string(sourceLine));throw std::runtime_error("assertion"); };
        try { CHECK(ReadGifFile(state,"input.gif",width,height,io,hooks,scratch)==kBitmap); }catch (const std::runtime_error& failure) { outcome=failure.what(); }
        CHECK(outcome==row[1] && std::to_string(static_cast<std::uint32_t>(width))+","+std::to_string(static_cast<std::uint32_t>(height))==row[2] && Numbers(state)==row[3] && events==row[4] && inputHex==row[5] && (pixels.empty() ? "-":Hex(pixels))==row[6] && actualRows==Number(row[7]));++rows;
    }
    CHECK(rows==56);
}

TEST_CASE(GifFileRead_CurrentEncryptedArchiveKeepsLegacySnapshot) { // metadata는 공유하고 새 Native Open만 현재 backing FILE을 읽는다.
    using namespace netstorm::test::fileprobe;using netstorm::test::rawscene::Put;using netstorm::test::rawscene::Get;const auto root=GifWork();const auto a=GifInput(3,2,0),b=GifInput(3,2,3);
    const auto archive=[&](auto payload) { auto bytes=Taff("d/live.gif",payload);Put(bytes,0x18,2);const auto offset=Get(bytes,0x28);for (std::size_t index=0;index<payload.size();++index) bytes[offset+index]^=kXorKey[index%kXorKey.size()];return bytes; }; // 명시TAFF 원문만 scratch에 암호화한다.
    Write(root/"live.tarc",archive(a));BaseFileSystem files(root);CHECK(files.RegisterArchive(root/"live.tarc"));const auto old=files.Read("d/live.gif");Write(root/"live.tarc",archive(b));std::int32_t width=-7,height=-9;const auto image=ReadBackgroundGifFileImage(files,"d/live.gif",width,height);
    CHECK(width==3 && height==2 && image.indices==std::vector<std::uint8_t>(6,3) && files.Read("d/live.gif")==old && DecodeBackgroundGif(old).indices==std::vector<std::uint8_t>(6,0));
}

TEST_CASE(GifFileRead_ValidDimensionsRemainBeforeInvalidLzwFailure) { // 실제 hostdecoder 실패를 없던 파일/정상 이미지로 바꾸지 않는다.
    using namespace netstorm::test::fileprobe;const auto root=GifWork();Write(root/"d/bad.gif",GifInput(3,2,3,true));BaseFileSystem files(root);std::int32_t width=-7,height=-9;
    CHECK(GifFailure([&] { ReadBackgroundGifFileImage(files,"d/bad.gif",width,height); }) && width==3 && height==2);width=-11;height=-13;
    CHECK(GifFailure([&] { ReadBackgroundGifFileImage(files,"d/missing.gif",width,height); }) && width==-11 && height==-13); // 필수Open 실패는 치수 출력을 쓰지 않는다.
    Write(root/"d/good.gif",GifInput(2,1,0));const auto image=ReadBackgroundGifFileImage(files,"d/good.gif",width,height);CHECK(width==2 && height==1 && image.indices==std::vector<std::uint8_t>(2,0)); // 실패 뒤 같은 VFS의 정상 File 수명도 독립적이다.
}

TEST_CASE(GifFileRead_KencloudCurrentArchiveUsesExistingPixelsAnd512Contract) { // 실제 Kencloud 장치는 같은주소/난수/512검사를 유지한다.
    using namespace netstorm::test::fileprobe;using netstorm::test::rawscene::Put;const auto root=GifWork();const auto first=Gif(512,512,17),second=Gif(512,512,29);Write(root/"cloud.tarc",Taff("d/cloud.gif",first));BaseFileSystem files(root);CHECK(files.RegisterArchive(root/"cloud.tarc"));const auto old=files.Read("d/cloud.gif");Write(root/"cloud.tarc",Taff("d/cloud.gif",second));ConfigInterface config(ConfigInterface::FileReader{});config.Set("CloudFileSpec","d/cloud.gif");GameRandom random;random.SetState(0x89abcdefU);ClientKencloudGlobals cloud([](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); });
    cloud.Build(files,config,random);const auto globals=cloud.Globals();CHECK(globals.cache && cloud.Allocations()==1 && std::all_of(cloud.Memory(globals.cache).begin(),cloud.Memory(globals.cache).end(),[](auto value) { return value==29; }) && files.Read("d/cloud.gif")==old);
}
