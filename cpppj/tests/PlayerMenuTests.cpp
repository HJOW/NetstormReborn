// PATCH Player 부모 전체의 실제292입력과 정적 버퍼/GUI 효과를 비교한다.
#include "RawSceneSupport.h"
#include "client/RawPlayerMenu.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 원문13개 독립 입력 열이다.
enum Input { Target,Modes,Flags,Priest,Allied,Hide,Allow,Money,Ignored,Whisper,Games,Address,Mutation };
// 원본 문자열의 바이트hex다.
std::string TextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 실제 기록/현재 getter·GUI 경계만 조립한다. 예상 메뉴는 계산하지 않는다.
struct PlayerScene {
    std::array<std::uint32_t,13> input{}; // 원문 독립 입력이다.
    std::array<std::uint8_t,1620> players{}; // 같은9 Player 원문이다.
    std::array<std::uint8_t,40*460> meta{}; // 같은40개 실제 metadata다.
    std::array<std::uint8_t,4800> chat{}; // 실제100개 채팅 원문이다.
    std::array<std::uint32_t,81> alliances{}; // 방향별 동맹이다.
    std::array<std::uint8_t,9> hidden{}; // 같은 숨김 BYTE9다.
    std::array<std::uint32_t,9> priests{}; // 현재 사제 ownerState다.
    PlayerIdentityReads reads; // 메뉴/식별자에 공유하는 현재 전역이다.
    std::unique_ptr<RawPlayerIdentity> identity; // 같은 기록을 빌리는 실제 조회기다.
    PlayerMenuState state; // 원본 명령/정적 버퍼다.
    PlayerMenuHooks hooks; // 현재 조회/효과 관찰 경계다.
    std::uint32_t owner{},subscriber{},taken{}; // raw target 행·실제 DWORD·GUI 스택 입력이다.
    std::string trace; // 실제 호출 효과 순서다.
    // 고정 원문 입력을 채우며 원본 반환/메뉴 기대값은 사용하지 않는다.
    explicit PlayerScene(const std::vector<std::string>& row) {
        // 원문DWORD/float비트를 그대로 보존한다.
        for (std::size_t index=0;index<input.size();++index) input[index]=Number(row[index]);
        owner=input[Target]==0 ? 1U : 2U;subscriber=input[Target]==0 ? 0xfedcba98U : input[Target]==1 ? 0x80000002U : 0xdeadbeefU;
        Put(players,180+44,0xfedcba98);Text(players,180+12,"Local1");players[180+80]=0x23;
        Put(players,2*180+44,0x80000002);Text(players,2*180+12,"Alpha");Put(players,8*180+44,0xfffffff0);Text(players,8*180+12,"LAST");
        Put(players,3*180+44,0x80000003);Text(players,3*180+12,"Buddy");players[3*180+80]=0x23;
        players[owner*180+80]=static_cast<std::uint8_t>(input[Flags]);priests[owner]=input[Priest];Put(players,180+84,(input[Allied]&16) ? 1U<<owner : 0);
        // 숨김 표와 서로 다른 두 방향의 동맹을 독립 입력으로 채운다.
        for (std::uint32_t index=0;index<9;++index) hidden[index]=(input[Hide]&(1U<<index)) ? 1 : 0;
        alliances[owner*9+3]=(input[Allied]&1)!=0;alliances[3*9+owner]=(input[Allied]&2)!=0;alliances[owner*9+8]=(input[Allied]&4)!=0;alliances[8*9+owner]=(input[Allied]&8)!=0;
        // metadata1/2와 실제 필드 배치의 독립 버전/주소 입력이다.
        for (std::uint32_t index=1;index<=2;++index) {
            auto entry=std::span(meta).subspan(index*460,460);Put(entry,0,index);Put(entry,4,index==1 ? 0xfedcba98U : 0x80000002U);Text(entry,38,index==1 ? "LocalUser" : "Meta2");entry[100]=1;Put(entry,176,1,2);Put(entry,178,78,2);Put(entry,84,input[Games],2);Text(entry,416,"udp,127.0.0.1:6800");
        }
        if (input[Address]) Text(players,owner*180+48,"zlocal,127.0.0.1:6800");
        reads.localOwner=[] { return 1U; };reads.subscriber=[] { return 0xfedcba98U; };reads.address=[] { return 0xc0a8005eU; };reads.name=[] { return std::string_view("LocalUser"); };
        reads.battle=[this] { return (input[Modes]&1)!=0; };reads.editor=[this] { return (input[Modes]&8)!=0; };reads.metamap=[this] { return (input[Modes]&4)!=0; };reads.unknownName=[] { return std::string_view("SOMEONE"); };
        identity=std::make_unique<RawPlayerIdentity>(OriginalEdition::Patch1078,players,meta,chat,reads);
        state.sendMission.fill(static_cast<char>(0xa5));state.sendLobby.fill(0x5a);Store(state.sendMission,"OLD_MISSION");Store(state.sendLobby,"OLD_LOBBY");
        hooks.mission=[this] { return (input[Modes]&2)!=0; };hooks.fort=[this] { return (input[Modes]&16)!=0; };hooks.allowSend=[this] { return (input[Allow]&1)!=0; };hooks.allowOffer=[this] { return (input[Allow]&2)!=0; };hooks.allowBreak=[this] { return (input[Allow]&4)!=0; };
        hooks.assertion=[this](PlayerMenuAssertion kind) { Add(std::string("!,")+(kind==PlayerMenuAssertion::MetamapNumber ? "1" : "0")); };
        hooks.parseAddress=[this](std::string_view text,std::uint32_t port) { Add("A,"+TextHex(text)+','+std::to_string(port)); };
        hooks.reliability=[this](std::uint32_t index) { Add("R,"+std::to_string(index));if (input[Mutation]&8) input[Modes]&=~4U;return 90+index; };
        hooks.ignored=[this](std::uint32_t value) { Add("G,"+std::to_string(value));return input[Ignored]!=0; };hooks.whispered=[this](std::uint32_t value) { Add("W,"+std::to_string(value));return static_cast<std::uint8_t>(input[Whisper]); };
        hooks.money=[this](std::uint32_t value) { Add("$,"+std::to_string(value));if (input[Mutation]&4) { players[owner*180+80]=0;input[Allow]&=~1U; }return std::bit_cast<float>(input[Money]); };
        hooks.translate=[this](std::string_view key) { Add("T,"+TextHex(key));if ((input[Mutation]&1) && key=="Allied with ~i%s~.") Text(players,3*180+12,"Changed");if ((input[Mutation]&2) && key=="Version: ~i%d.%d~.\n") Put(meta,owner*460+176,9,2);return std::string(key); };
        hooks.print=[](std::string_view pattern,std::span<const PriestMenuArgument> args) { std::string text;std::size_t argument=0;
            // 이 명시 CRT 경계는%s/%d/%%의 실제 인자 순서만 처리한다.
            for (std::size_t index=0;index<pattern.size();++index) { if (pattern[index]!='%') { text+=pattern[index];continue; }++index;CHECK(index<pattern.size());if (pattern[index]=='%') { text+='%';continue; }CHECK(argument<args.size());if (pattern[index]=='s') text+=std::get<std::string>(args[argument++]);else { CHECK(pattern[index]=='d');text+=std::to_string(std::get<std::int32_t>(args[argument++])); } }CHECK(argument==args.size());return text;
        };
        hooks.beginFancy=[this] { Add("F"); };hooks.beginVertical=[this] { Add("V"); };hooks.separator=[this] { Add("X"); };hooks.title=[this](std::string_view text) { Add("D,"+TextHex(text)); };hooks.header=[this](std::string_view text) { Add("H,"+TextHex(text)); };
        hooks.item=[this](std::string_view text,std::uint32_t command,std::uint32_t parameter,std::uint32_t include,std::uint32_t enabled) { Add("I,"+TextHex(text)+','+std::to_string(command)+','+std::to_string(parameter)+','+std::to_string(include)+','+std::to_string(enabled)); };
        hooks.checked=[this](std::uint32_t checked,std::string_view text,std::uint32_t command,std::uint32_t parameter) { Add("C,"+std::to_string(checked)+','+TextHex(text)+','+std::to_string(command)+','+std::to_string(parameter)); };
        hooks.takeMenu=[this] { const auto token=100+ ++taken;Add("K,"+std::to_string(token));return token; };hooks.submenu=[this](std::string_view text,std::uint32_t child) { Add("S,"+TextHex(text)+','+std::to_string(child)); };
    }
    // 원문 문자열/NUL만 쓰며 뒤의 표식을 지우지 않는다.
    static void Text(std::span<std::uint8_t> bytes,std::size_t offset,std::string_view text) { std::copy(text.begin(),text.end(),bytes.begin()+offset);bytes[offset+text.size()]=0; }
    // 정적 버퍼는 기존 이름 뒤 표식을 보존하며 NUL까지만 갱신한다.
    static void Store(std::span<char> bytes,std::string_view text) { std::copy(text.begin(),text.end(),bytes.begin());bytes[text.size()]=0; }
    // 실제 효과를 발생 순서대로 연결한다.
    void Add(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 같은 실제 Identity/표/현재 전역/버퍼를 빌린다.
    RawPlayerMenu Menu() { return RawPlayerMenu(*identity,alliances,hidden,priests,state,hooks); }
    // 두 정적 버퍼 전체를 원문과 같은 바이트로 읽는다.
    std::string Buffers() const { return Hex({reinterpret_cast<const std::uint8_t*>(state.sendMission.data()),104})+Hex({reinterpret_cast<const std::uint8_t*>(state.sendLobby.data()),104}); }
};
}
// 실제 PATCH 전체 부모292입력의 반환/정적208바이트/모든 조회/GUI 효과와 대조한다.
TEST_CASE(PlayerMenu_Native292) {
    std::size_t count=0;
    // 기대값은 실제 기계어에서 얻었으며 생성/분기 계산을 복제하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_PLAYER_MENU_FIXTURE).rows) {
        if (row[0]=="target") continue;CHECK(row.size()==16);PlayerScene scene(row);const auto result=scene.Menu().Open(scene.subscriber);const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (trace!=row[15]) std::fprintf(stderr,"Player 메뉴 차이 target%s modes%s mutation%s\n실제 %s\n원문 %s\n",row[0].c_str(),row[1].c_str(),row[12].c_str(),trace.c_str(),row[15].c_str());
        CHECK(std::to_string(result)==row[13]);CHECK(scene.Buffers()==row[14]);CHECK(trace==row[15]);++count;
    }CHECK(count==292);
}
// 같은 토큰은 이후 갱신된 실제 정적 문자열을 읽으며 누락 효과는 성공으로 바뀌지 않는다.
TEST_CASE(PlayerMenu_StaticTextLifetimeAndRequiredQueries) {
    const auto rows=LoadFixture(NETSTORM_PLAYER_MENU_FIXTURE).rows;PlayerScene scene(rows.at(1));auto menu=scene.Menu();CHECK(menu.Open(scene.subscriber)!=0);const auto token=scene.state.missionTextToken;CHECK(menu.TextParameter(token)=="asksend(2)");
    Put(scene.players,2*180+44,0);Put(scene.players,8*180+44,scene.subscriber);menu.Open(scene.subscriber);CHECK(menu.TextParameter(token)=="asksend(8)");
    PlayerScene missing(rows.at(1));missing.hooks.money={};CHECK(Throws([&] { missing.Menu().Open(missing.subscriber); }));
}
