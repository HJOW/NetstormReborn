// PE 정적 표 전체와 같은 장기 owner의 실제 등록·부분중단을 headless로 대조한다.
#include "client/ClientDotCommandGlobals.h"
#include "TestSupport.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using namespace netstorm::client;
namespace {
// 같은 물리 표의 DWORD를 읽으며 별도 현재 count를 만들지 않는다.
std::uint32_t ReadDword(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value{};
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[offset+index])<<(index*8); // 실제 little endian 바이트 순서다.
    return value;
}
// 명령 입력은 NUL을 포함한 명시 ASCII 바이트로 전달한다.
std::span<const std::uint8_t> Name(const char* text,std::size_t size) { return {reinterpret_cast<const std::uint8_t*>(text),size}; }
// 제품 소유자 작성 전에 PE에서 동결한 전체 정적 표만 읽는다.
std::vector<std::uint8_t> StaticImage() {
#ifdef NETSTORM_DOT_COMMAND_STATIC_IMAGE_FIXTURE
    const std::filesystem::path path(NETSTORM_DOT_COMMAND_STATIC_IMAGE_FIXTURE); // 공용 CMake 등록 시 실제 fixture 절대 경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/dot-command-static-image.hex"; // 격리 컴파일은 같은 소스의 fixture를 읽는다.
#endif
    std::ifstream file(path);const std::string hex((std::istreambuf_iterator<char>(file)),{}); // 원시 hex와 마지막 개행을 그대로 읽는다.
    if (!file || hex.size()!=kDotCommandStateBytes*2+1 || hex.back()!='\n') throw std::runtime_error("점 명령 정적 이미지 읽기 실패");
    std::vector<std::uint8_t> result;result.reserve(kDotCommandStateBytes);
    for (std::size_t index=0;index<kDotCommandStateBytes;++index) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(index*2,2),nullptr,16))); // 최초 PE의 모든 바이트를 복원한다.
    return result;
}
}
// count0 가정이나 say 한 필드만의 대조 대신 인접 표2568BYTE 전체를 실제 정적 PE와 비교한다.
TEST_CASE(ClientDotCommandGlobals_EntireStaticPeImageAndSameAddress) {
    static_assert(!std::is_copy_constructible_v<ClientDotCommandGlobals> && !std::is_move_constructible_v<ClientDotCommandGlobals>); // 빌린 주소는 owner 수명 동안 고정이다.
    ClientDotCommandGlobals owner;const auto& readOnly=owner;const auto expected=StaticImage(); // 새 Client 하나의 실제 최초 입력이다.
    CHECK(owner.Bytes().size()==expected.size() && std::equal(owner.Bytes().begin(),owner.Bytes().end(),expected.begin()));
    CHECK(owner.Bytes().data()==readOnly.Bytes().data() && ReadDword(owner.Bytes(),0xa04)==1);
}
// 서로 다른 소비자가 같은 표에 등록해도 최초 say와 앞선 등록을 보존한다.
TEST_CASE(ClientDotCommandGlobals_SharedRegistrationsPreserveSayAndEarlierSlots) {
    auto owner=std::make_shared<ClientDotCommandGlobals>();auto second=owner; // 두 소비자가 단일 장기 표를 빌린다.
    const auto expected=StaticImage();const auto* address=owner->Bytes().data(); // 최초 say와 고정 물리를 관측한다.
    CHECK(owner->Register(Name("format",7),0x12345678,0)==1);
    CHECK(second->Register(Name("back",5),0x87654321,1)==1);
    CHECK(second->Bytes().data()==address && ReadDword(second->Bytes(),0xa04)==3);
    CHECK(std::equal(expected.begin(),expected.begin()+32,second->Bytes().begin()) && ReadDword(second->Bytes(),0x800)==0x0042f1e0);
    CHECK(ReadDword(second->Bytes(),0x804)==0x12345678 && ReadDword(second->Bytes(),0x808)==0x87654321);
    CHECK(ReadDword(second->Bytes(),0x904)==kDotCommandEmptyToken && ReadDword(second->Bytes(),0x908)==kDotCommandNamesToken+64 && ReadDword(second->Bytes(),0x90c)==0);
    owner.reset();CHECK(second->Bytes().data()==address && ReadDword(second->Bytes(),0xa04)==3); // 첫 소비자 반납으로 재초기화하지 않는다.
    ClientDotCommandGlobals fresh;CHECK(ReadDword(fresh.Bytes(),0xa04)==1 && ReadDword(second->Bytes(),0xa04)==3); // 다른 Client의 초기 상태와 구별한다.
}
// 원문 copy가 이름을 쓴 다음 중단하면 같은 표의 부분효과를 되돌리지 않는다.
TEST_CASE(ClientDotCommandGlobals_CopyFailureKeepsActualPartialBytes) {
    ClientDotCommandGlobals owner;DotCommandRegisterHooks hooks; // 실제 CRT copy에 명시 중단만 추가한다.
    hooks.copy=[](auto destination,auto source,auto count) { CopyDotCommandName(destination,source,count);throw std::logic_error("명시 복사 뒤 중단"); };
    bool stopped{};try { owner.Register(Name("front",6),0x12345678,1,hooks); }catch(const std::logic_error&) { stopped=true; }
    CHECK(stopped && ReadDword(owner.Bytes(),0xa04)==1 && owner.Bytes()[32]=='f' && owner.Bytes()[36]=='t');
    CHECK(ReadDword(owner.Bytes(),0x804)==0 && ReadDword(owner.Bytes(),0x904)==0); // callback/표시/count는 아직 도달 전이다.
}
