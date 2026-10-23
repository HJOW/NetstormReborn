// 실제 독립 소유자와 원문 helper의 같은 물리·초기화·공유수명을 headless로 확인한다.
#include "TestSupport.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientCoreDataGlobals.h"
#include <algorithm>
#include <array>
#include <bit>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using namespace netstorm::client;
using namespace netstorm::o;
static_assert(!std::is_copy_constructible_v<ClientChatManagerGlobals> && !std::is_move_constructible_v<ClientChatManagerGlobals>); //현재DWORD/할당뷰를고정주소에유지한다.
static_assert(!std::is_copy_constructible_v<ClientCoreDataGlobals> && !std::is_move_constructible_v<ClientCoreDataGlobals>); //자기캐시를가리키는current의복사/이동을금지한다.
// 호출자가 먼저 작성한 원문DWORD만 읽고 미작성 할당BYTE는 읽지 않는다.
std::uint32_t DefinedChatDword(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value=0; // 정의된4BYTE를 little endian으로 읽는다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8); //이함수호출은실제ctor가쓴필드에만한정한다.
    return value;
}
// 테스트에서 명시적으로 NUL까지 작성된 prefix만 읽는다.
std::string DefinedRankPrefix(std::span<const std::uint8_t> bytes) {
    std::string text; //원문setter가쓴NUL앞부분만모은다.
    for (std::size_t offset=kChatManagerRankPrefixOffset;offset<bytes.size();++offset) { if (!bytes[offset]) return text;text+=static_cast<char>(bytes[offset]); } //미작성꼬리로진행하지않는NUL종료계약이다.
    throw std::out_of_range("검사에서작성한rank prefix의NUL없음");
}
// headless 설정/명령 장치만 실제로 기록한다. 생산Client 명령 registry를복원했다는뜻이아니다.
void SupplyChatStartupDevices(ChatManagerPrefixHooks& hooks,std::vector<std::string>& commands) {
    hooks.registerCommand=[&commands](std::string_view name,auto callback,auto flags) { commands.push_back(std::string(name)+":"+std::to_string(callback)+":"+std::to_string(flags)); };
    hooks.readConfig=[](std::string_view)->std::optional<std::string> { return std::nullopt; }; //누락설정이라는명시입력이며실제Config성공을가장하지않는다.
}
}

// 새 실제 할당값은 읽지 않고 크기/주소만 검사한 뒤 명시fill한값과hook소유수명만 읽는다.
TEST_CASE(ClientChatCoreGlobals_ActualAllocationsAndHookLifetime) {
    auto owner=std::make_shared<ClientChatManagerGlobals>();CHECK(owner->Current()==0); //실제currentDWORD는최초한번0이다.
    const auto manager=owner->Allocate(kChatManagerBytes),buffer=owner->Allocate(kChatManagerBufferBytes); //둘은별도실제할당이다.
    CHECK(manager!=0 && buffer!=0 && manager!=buffer);CHECK(owner->Current()==0);
    auto managerBytes=owner->Bytes(manager),bufferBytes=owner->Bytes(buffer); //data/size조회는미초기화값을읽지않는다.
    CHECK(managerBytes.size()==2712);CHECK(bufferBytes.size()==128);CHECK(managerBytes.data()!=bufferBytes.data());
    std::fill(managerBytes.begin(),managerBytes.end(),std::uint8_t{0x6b});std::fill(bufferBytes.begin(),bufferBytes.end(),std::uint8_t{0x7c}); //이후값검사는명시적으로정의한입력이다.
    const auto managerAddress=managerBytes.data();owner->Current()=manager; //소유자의current쓰기와Allocate를구별한다.
    auto hooks=MakeClientChatManagerHooks(owner);std::weak_ptr<ClientChatManagerGlobals> weak=owner;owner.reset(); //장치가같은owner를계속보유한다.
    CHECK(!weak.expired());CHECK(hooks.memory(manager).data()==managerAddress);CHECK(hooks.memory(manager)[13]==0x6b);CHECK(hooks.memory(buffer)[12]==0x7c);
    const auto retained=weak.lock();CHECK(retained->Current()==manager);CHECK(&std::as_const(*retained).Current()==&retained->Current());
    bool missing=false,badSize=false; //실제장치계약오류를가짜할당으로바꾸지않는다.
    try { static_cast<void>(hooks.memory(0)); } catch (const std::out_of_range&) { missing=true; }
    try { static_cast<void>(hooks.allocate(129)); } catch (const std::invalid_argument&) { badSize=true; }
    CHECK(missing);CHECK(badSize);
}

// 재초기화는 같은현재슬롯을바꾸되이전manager/내부할당과이미작성한prefix를계속보유한다.
TEST_CASE(ClientChatCoreGlobals_ReinitializePreservesEarlierAllocations) {
    auto owner=std::make_shared<ClientChatManagerGlobals>();auto hooks=MakeClientChatManagerHooks(owner); //실제할당/CRTcopy를사용한다.
    std::vector<std::string> commands,keys;SupplyChatStartupDevices(hooks,commands); //명시headless장치이며게임은실행하지않는다.
    std::uint32_t subscriber=0xffffffffU,active=0; //원문공통startup의실제Subscriber초기값이다.
    hooks.readConfig=[&](std::string_view key)->std::optional<std::string> { keys.emplace_back(key);return key.ends_with('2')?"Back":"Front"; };
    InitializeChatManager({owner->Current(),subscriber,active},hooks);const auto first=owner->Current(); //실제생성자가쓴정의필드만읽는다.
    const auto firstBuffer=DefinedChatDword(owner->Bytes(first),8);auto oldManager=owner->Bytes(first);auto oldBuffer=owner->Bytes(firstBuffer); //내부할당값자체는아직읽지않는다.
    std::fill(oldBuffer.begin(),oldBuffer.end(),std::uint8_t{0x5a});oldManager[0x244]='Q';oldManager[0x245]=0; //후속보존을검사할실제정의입력이다.
    const auto managerAddress=oldManager.data(),bufferAddress=oldBuffer.data(); //새할당뒤에도주소가같아야한다.
    InitializeChatManager({owner->Current(),subscriber,active},hooks);const auto second=owner->Current(); //이전token을자동free하지않는다.
    CHECK(first!=second);CHECK(owner->Bytes(first).data()==managerAddress);CHECK(owner->Bytes(firstBuffer).data()==bufferAddress);CHECK(owner->Bytes(first)[0x244]=='Q');CHECK(owner->Bytes(firstBuffer)[127]==0x5a);
    CHECK(DefinedChatDword(owner->Bytes(second),4)==32);CHECK(owner->Bytes(second)[0x244]==0);CHECK(DefinedRankPrefix(owner->Bytes(first))=="Q");
    CHECK(keys==std::vector<std::string>({"-1chatFormat","-1chatFormat2","-1chatFormat","-1chatFormat2"}));CHECK(commands.size()==6);
    CHECK(commands[0]=="format:4382192:1");CHECK(commands[1]=="back:4382192:1");CHECK(commands[2]=="front:4382448:1");
}

// 실제Chat/Core두몸체를연결하고미선택active0과선택된필수registry누락을정직하게구별한다.
TEST_CASE(ClientChatCoreGlobals_RealHelpersAndRequiredExternalBoundaries) {
    auto chat=std::make_shared<ClientChatManagerGlobals>();auto core=std::make_shared<ClientCoreDataGlobals>();auto hooks=MakeClientChatManagerHooks(chat); //실제물리장치만자동연결한다.
    std::uint32_t subscriber=0xffffffffU,metamap=0;PlayerWorldState player; //실제외부DWORD/Player상태를별도사본동기화없이빌린다.
    bool rejected=false; //registry미공급도생성자이전선행검사로바꾸지않는다.
    try { InitializeChatManager({chat->Current(),subscriber,core->Active()},hooks); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected);CHECK(chat->Current()!=0);CHECK(chat->Bytes(chat->Current())[0x244]==0); //명령도달전생성부분효과는이미남는다.
    RefreshChatManagerRankPrefix({chat->Current(),subscriber,core->Active()},hooks,{},{}); //실제active0이므로core/formatter가미공급이어도자연반환한다.
    std::vector<std::string> commands;SupplyChatStartupDevices(hooks,commands);InitializeChatManager({chat->Current(),subscriber,core->Active()},hooks);
    std::array<std::uint8_t,1620> players{};core->Players()=players;core->Select(player,metamap,1,1);core->Active()=1;players[0xb4+0x70]=5; //실제현재Player+4rank다.
    hooks.core=[&]()->std::span<const std::uint8_t> { return core->Read(player,metamap,{}); };hooks.maximumRank=[] { return 31; };hooks.minorCount=[] { return 3; }; //실제getter와명시rank개수입력이다.
    std::array<std::uint8_t,256> output{},temporary{};temporary.fill(0xa5); //원문지역미정물리는검사에서명시한다.
    RefreshChatManagerRankPrefix({chat->Current(),subscriber,core->Active()},hooks,output,temporary);CHECK(DefinedRankPrefix(chat->Bytes(chat->Current()))=="~[Iicon.C0]~[Iicon.B2]");
}

// 두사용자같은sharedowner는실제슬롯/캐시를공유하고새owner는별도최초0을구성한다.
TEST_CASE(ClientChatCoreGlobals_CoreAliasSharedOwnerAndFreshOwner) {
    std::array<std::uint8_t,1620> players{};PlayerWorldState player;std::uint32_t metamap=0; //실제빌린자료가모든owner사용자보다오래산다.
    auto first=std::make_shared<ClientCoreDataGlobals>();auto second=first; //여러Session처럼동일한실제owner를보유하는사용자다.
    CHECK(first->Current().empty());CHECK(first->Players().empty());CHECK(first->Active()==0);CHECK(std::all_of(first->Cached().begin(),first->Cached().end(),[](auto value){return value==0;}));
    CHECK(&first->Current()==&second->Current());CHECK(&first->Active()==&second->Active());CHECK(first->Cached().data()==second->Cached().data());
    first->Players()=players;first->Select(player,metamap,8,3);first->Active()=71;auto current=second->Read(player,metamap,{}); //같은현재포인터를재조회한다.
    CHECK(current.data()==players.data()+8*0xb4+0x6c);current[4]=207;CHECK(players[8*0xb4+0x70]==207);first.reset(); //한사용자반납뒤에도다른사용자가실제owner를유지한다.
    second->Select(player,metamap,0,1);CHECK(second->Current().empty());CHECK(second->Active()==0);CHECK(second->Cached()[4]==207);CHECK(player.playerKnown==1);
    metamap=1;second->Select(player,metamap,1,1);current=second->Read(player,metamap,{});CHECK(current.data()==second->Cached().data());current[8]=88;CHECK(second->Cached()[8]==88);
    ClientCoreDataGlobals fresh;CHECK(fresh.Current().empty());CHECK(fresh.Active()==0);CHECK(fresh.Cached()[4]==0);CHECK(fresh.Cached().data()!=second->Cached().data());
}

// owner adapter도원문진단뒤current를재조회하고다음비영선택은같은현재Player base슬롯을읽는다.
TEST_CASE(ClientChatCoreGlobals_CoreDiagnosticAndCurrentPlayersSlot) {
    std::array<std::uint8_t,1620> firstPlayers{},nextPlayers{};PlayerWorldState player;std::uint32_t metamap=0;ClientCoreDataGlobals owner; //실제서로다른Player물리다.
    owner.Players()=firstPlayers;owner.Select(player,metamap,1,1);metamap=1;std::uint32_t assertions=0; //진단에서실제owner포인터를바꾼다.
    auto current=owner.Read(player,metamap,{[&](auto line) { ++assertions;CHECK(line==85);owner.Current()=owner.Cached();owner.Active()=33; }});
    CHECK(assertions==1);CHECK(current.data()==owner.Cached().data());CHECK(owner.Active()==33);
    owner.Players()=nextPlayers;metamap=0;owner.Select(player,metamap,8,7);current=owner.Read(player,metamap,{});CHECK(current.data()==nextPlayers.data()+8*0xb4+0x6c);CHECK(owner.Active()==33);CHECK(player.templateOwner==7);
}
