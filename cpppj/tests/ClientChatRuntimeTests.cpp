// 실제 Client 명령표/Config/Chat 할당과 보호 icon/현재 Core를 같은 Native 부모에 연결해 확인한다.
#include "DudeSpawnSupport.h"
#include "client/ClientChatRuntime.h"
#include "client/ClientMain.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientDotCommandGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/RawSurfaceWorld.h"
#include <algorithm>
#include <stdexcept>

using namespace netstorm::client;
namespace { // 창/게임 실행 없이 실제 장기 owner와 보호 자료를 읽는다.
// 실제 Client 생성자는 archive만 읽고 Run의 별도 창/설정 저장은 실행하지 않는다.
ClientOptions ChatRuntimeOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";return options; }
// Native 부모가 이미 작성한 DWORD만 읽고 미정 할당 꼬리는 관측하지 않는다.
std::uint32_t ChatRuntimeWord(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[offset+byte])<<(8*byte);return value; // 네 실제 little endian BYTE다.
}
// 실제 CRT/접두 setter가 작성한 첫 NUL까지 읽는다.
std::string ChatRuntimeText(std::span<const std::uint8_t> bytes,std::size_t offset) {
    const auto text=bytes.subspan(offset);const auto end=std::find(text.begin(),text.end(),std::uint8_t{});if (end==text.end()) throw std::out_of_range("Chat 실제 작성 문자열의 NUL 없음");return {reinterpret_cast<const char*>(text.data()),static_cast<std::size_t>(end-text.begin())};
}
}

// 현재 signed Subscriber 키와 실제 명령 등록/두 고정 strncpy/마지막 강제NUL을 함께 확인한다.
TEST_CASE(ClientChatRuntime_StartupUsesRealRegistryConfigAndTruncation) {
    Client client(ChatRuntimeOptions());client.Configuration().Set("-1chatFormat",std::string(70,'F'));client.Configuration().Set("-1chatFormat2",std::string(520,'G'));
    CHECK(client.ChatManagerGlobals()->Current()==0 && ChatRuntimeWord(client.DotCommandGlobals()->Bytes(),0xa04)==1);InitializeClientChatManager(client);
    const auto manager=client.ChatManagerGlobals()->Bytes(client.ChatManagerGlobals()->Current()),commands=client.DotCommandGlobals()->Bytes();
    CHECK(ChatRuntimeWord(manager,4)==32 && ChatRuntimeWord(manager,8)!=0 && manager[0x244]==0);CHECK(ChatRuntimeText(manager,0x14)==std::string(59,'F') && ChatRuntimeText(manager,0x50)==std::string(499,'G'));
    CHECK(ChatRuntimeWord(commands,0xa04)==4 && ChatRuntimeText(commands,32)=="format" && ChatRuntimeText(commands,64)=="back" && ChatRuntimeText(commands,96)=="front");
    CHECK(ChatRuntimeWord(commands,0x804)==0x42ddf0 && ChatRuntimeWord(commands,0x808)==0x42ddf0 && ChatRuntimeWord(commands,0x80c)==0x42def0);CHECK(ChatRuntimeWord(commands,0x904)==kDotCommandNamesToken+32 && ChatRuntimeWord(commands,0x90c)==kDotCommandNamesToken+96);
}

// 두 UTF8 설정은 실제 원문 ANSI 바이트로 변환하며 누락은 빈 문자열/전체 padding이다.
TEST_CASE(ClientChatRuntime_OriginalEncodingAndMissingFormat) {
    Client client(ChatRuntimeOptions());client.Configuration().Set("-1chatFormat","é");InitializeClientChatManager(client);const auto manager=client.ChatManagerGlobals()->Bytes(client.ChatManagerGlobals()->Current());
    CHECK(manager[0x14]==0xe9 && manager[0x15]==0 && manager[0x50]==0 && manager[0x242]==0 && manager[0x243]==0);
}

// 재시작은 현재 ID를 다시 읽고 이전 실제 manager를 반납하지 않으며 같은 명령표에 추가한다.
TEST_CASE(ClientChatRuntime_ReinitializeRetainsOldManagerAndCurrentSubscriber) {
    Client client(ChatRuntimeOptions());InitializeClientChatManager(client);const auto previous=client.ChatManagerGlobals()->Current();const auto before=client.ChatManagerGlobals()->Bytes(previous);const auto* address=before.data();before[0x244]='P';before[0x245]=0;
    client.SubscriberGlobals()->MySubscriber()=7;client.Configuration().Set("7chatFormat","Now");InitializeClientChatManager(client);
    CHECK(client.ChatManagerGlobals()->Current()!=previous && client.ChatManagerGlobals()->Bytes(previous).data()==address && ChatRuntimeText(client.ChatManagerGlobals()->Bytes(previous),0x244)=="P");CHECK(ChatRuntimeText(client.ChatManagerGlobals()->Bytes(client.ChatManagerGlobals()->Current()),0x14)=="Now" && ChatRuntimeWord(client.DotCommandGlobals()->Bytes(),0xa04)==7);
}

// 두 실제 관문에서 빈 Core를 조회하지 않는다. 비영 active를 임의0으로 만들지 않는다.
TEST_CASE(ClientChatRuntime_NaturalManagerAndActiveGates) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto surface=MakeRawSurfaceStorage(assets.game);Client client(ChatRuntimeOptions());client.CoreDataGlobals()->Active()=1;
    RefreshClientChatRankPrefix(client,assets.game,*surface);CHECK(client.ChatManagerGlobals()->Current()==0 && client.CoreDataGlobals()->Active()==1);
    InitializeClientChatManager(client);client.CoreDataGlobals()->Active()=0;const auto manager=client.ChatManagerGlobals()->Bytes(client.ChatManagerGlobals()->Current());manager[0x244]='K';manager[0x245]=0;
    RefreshClientChatRankPrefix(client,assets.game,*surface);CHECK(ChatRuntimeText(manager,0x244)=="K" && client.CoreDataGlobals()->Current().empty());
}

// 보호 icon.type의 B10/C14는 실제 최대139다. 현재 Client 캐시의 rank를 Native formatter가 제한한다.
TEST_CASE(ClientChatRuntime_ActualCoreAndProtectedIconCounts) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto surface=MakeRawSurfaceStorage(assets.game);surface->ownerMode.challenge=true;Client client(ChatRuntimeOptions());InitializeClientChatManager(client);
    auto& core=*client.CoreDataGlobals();const std::uint32_t metamap=1;core.Select(client.GameplayGlobals()->player,metamap,1,1);core.Active()=1;const auto* selected=core.Current().data();core.Cached()[4]=255;
    RefreshClientChatRankPrefix(client,assets.game,*surface);CHECK(ChatRuntimeText(client.ChatManagerGlobals()->Bytes(client.ChatManagerGlobals()->Current()),0x244)=="~[Iicon.C12]~[Iicon.B9]" && core.Current().data()==selected && core.Active()==1);
    core.Cached()[4]=15;RefreshClientChatRankPrefix(client,assets.game,*surface);CHECK(ChatRuntimeText(client.ChatManagerGlobals()->Bytes(client.ChatManagerGlobals()->Current()),0x244)=="~[Iicon.C0]~[Iicon.B5]");
}

// 자산 원문 사본 대신 같은 현재 전역 TYPE의 B/C 개수 변경을 즉시 읽고0 divisor의 실제 fault도 유지한다.
TEST_CASE(ClientChatRuntime_CurrentTypeCountsAndNativeDivideFault) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto surface=MakeRawSurfaceStorage(assets.game);surface->ownerMode.challenge=true;Client client(ChatRuntimeOptions());InitializeClientChatManager(client);
    auto& core=*client.CoreDataGlobals();const std::uint32_t metamap=1;core.Select(client.GameplayGlobals()->player,metamap,1,1);core.Active()=1;core.Cached()[4]=255;const auto kind=assets.game.Find("icon").block+kFirstAssetTypeNumber;
    surface->frames[kind]=RiftTypeFrames({{'B',0,1,0},{'B',0,2,0},{'B',0,3,0},{'C',0,1,0},{'C',0,2,0}});RefreshClientChatRankPrefix(client,assets.game,*surface);CHECK(ChatRuntimeText(client.ChatManagerGlobals()->Bytes(client.ChatManagerGlobals()->Current()),0x244)=="~[Iicon.C0]~[Iicon.B2]");
    surface->frames[kind]=RiftTypeFrames({{'C',0,1,0}});bool fault{};try { RefreshClientChatRankPrefix(client,assets.game,*surface); }catch (const std::domain_error&) { fault=true; }CHECK(fault && ChatRuntimeText(client.ChatManagerGlobals()->Bytes(client.ChatManagerGlobals()->Current()),0x244)=="~[Iicon.C0]~[Iicon.B2]");
}

// 실제 설정은 정리된 캡처 전체를 보존하고 back manager는59BYTE로 제한하며 재초기화도 같은 값을 읽는다.
TEST_CASE(ClientChatRuntime_FormatLimitsConfigAndReinitialize) {
    Client client(ChatRuntimeOptions());client.SubscriberGlobals()->MySubscriber()=7;InitializeClientChatManager(client);const auto owner=client.ChatManagerGlobals();const auto* capture=owner->FormatCapture().data();
    const auto format="~0~r"+std::string(70,'X');CHECK(ApplyClientChatFormat(client,false,format)==0);std::string value;client.Configuration().ReadString("7chatFormat",value);
    CHECK(value=="~r"+std::string(70,'X') && ChatRuntimeText(owner->Bytes(owner->Current()),0x14)=="~r"+std::string(57,'X'));
    CHECK(ApplyClientChatFormat(client,true,"prefix~g~B suffix")==0);value.clear();client.Configuration().ReadString("7chatFormat2",value);CHECK(value=="~g~B" && ChatRuntimeText(owner->Bytes(owner->Current()),0x50)=="~g~B");
    const auto previous=owner->Current();InitializeClientChatManager(client);CHECK(owner->Current()!=previous && owner->FormatCapture().data()==capture);
    CHECK(ChatRuntimeText(owner->Bytes(owner->Current()),0x14)=="~r"+std::string(57,'X') && ChatRuntimeText(owner->Bytes(owner->Current()),0x50)=="~g~B");
}

// 실제 좁은 서식의 비ASCII는 Config 경계에서 UTF8로 바꾸고 실패한 검색은 현재 manager/공통 캡처를 보존한다.
TEST_CASE(ClientChatRuntime_FormatEncodingAndIllegalInputPreserveMemory) {
    Client client(ChatRuntimeOptions());InitializeClientChatManager(client);const auto owner=client.ChatManagerGlobals();const std::string original="~rCaf\xe9";CHECK(ApplyClientChatFormat(client,true,original)==0);
    std::string value;client.Configuration().ReadString("-1chatFormat2",value);CHECK(value=="~rCafé" && ChatRuntimeText(owner->Bytes(owner->Current()),0x50)==original);
    const auto before=owner->Bytes(owner->Current());const std::vector<std::uint8_t> manager(before.begin(),before.end()),capture(owner->FormatCapture().begin(),owner->FormatCapture().end());
    CHECK(ApplyClientChatFormat(client,false,"plain without style")==0);
    CHECK(std::equal(manager.begin(),manager.end(),owner->Bytes(owner->Current()).begin()) && std::equal(capture.begin(),capture.end(),owner->FormatCapture().begin()));
}
