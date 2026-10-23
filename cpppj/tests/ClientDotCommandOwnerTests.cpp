// 실제 Client의 명령 표 소유권만 검사하며 Run 초기화·명령 실행 완료로 세지 않는다.
#include "TestSupport.h"
#include "client/ClientMain.h"
#include "client/ClientDotCommandGlobals.h"
#include <filesystem>
#include <memory>

using namespace netstorm::client;
namespace {
// 기존 headless 검사처럼 보호 archive를 읽기만 하고 창·설정 저장을 실행하지 않는다.
ClientOptions DotClientOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";return options; }
// 같은 인접 표에서 현재 count DWORD를 읽으며 별도 상태로 보유하지 않는다.
std::uint32_t DotOwnerCount(const ClientDotCommandGlobals& owner) {
    const auto bytes=owner.Bytes();std::uint32_t result{}; // 실제 count의 네 바이트를 읽는다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(bytes[0xa04+index])<<(index*8); // 원문 little endian 순서다.
    return result;
}
// 실제 등록 부모가 요구하는 NUL 포함 입력 물리다.
std::span<const std::uint8_t> FormatName() { static constexpr std::uint8_t name[]{'f','o','r','m','a','t',0};return name; } // format 이름은 검사 명시 입력이며 실제 startup callback을 대신하지 않는다.
}
// Client 생성에서 한 번 만든 정적 say와 그 뒤 등록을 같은 getter로 관측한다.
TEST_CASE(ClientDotCommandOwner_ActualClientKeepsSameTableAcrossConfiguration) {
    Client client(DotClientOptions());const auto owner=client.DotCommandGlobals();const auto* address=owner->Bytes().data(); // 실제 Client가 소유한 한 표다.
    CHECK(owner && DotOwnerCount(*owner)==1 && owner->Bytes()[0]=='s');
    CHECK(owner->Register(FormatName(),0x12345678,0)==1);client.Configuration().SetInt("readOnlyForts",1);
    CHECK(client.DotCommandGlobals()==owner && owner->Bytes().data()==address && DotOwnerCount(*owner)==2 && owner->Bytes()[32]=='f');
}
// Client 반납 뒤 명시 소비자의 공유 참조가 표를 보존하며 새 Client만 count1로 시작한다.
TEST_CASE(ClientDotCommandOwner_RetainedTableAndFreshClientAreSeparate) {
    std::shared_ptr<ClientDotCommandGlobals> retained;std::weak_ptr<ClientDotCommandGlobals> weak; // 외부 소비자 수명을 명시한다.
    { Client client(DotClientOptions());retained=client.DotCommandGlobals();weak=retained;CHECK(retained->Register(FormatName(),0x12345678,1)==1); }
    CHECK(!weak.expired() && DotOwnerCount(*retained)==2);
    { Client fresh(DotClientOptions());CHECK(fresh.DotCommandGlobals()!=retained && DotOwnerCount(*fresh.DotCommandGlobals())==1 && DotOwnerCount(*retained)==2); }
    retained.reset();CHECK(weak.expired()); // 마지막 소비자 반납 뒤 빌린 표를 읽지 않는다.
}
