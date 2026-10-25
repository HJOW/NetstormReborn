// 실제 PATCH Player9/방향별81DWORD 전체 출력과 로그인/AI 수명 효과를 독립 기계어 fixture에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPlayerParticipation.h"
#include <algorithm>
#include <array>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 전체 raw 출력은 낮은 바이트부터 hex로 기록한다.
std::string ParticipationHex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef"; // 바이트의상위/하위4비트에대응한다.
    std::string result;
    // 입력의순서를변경하지않고모든바이트를관찰한다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result;
}
// 명시 입력의 DWORD를 little endian으로 쓴다.
void ParticipationPut(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    // 원본정수폭네바이트만변경한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 원본 상태·효과를 준비하며 기대 로그인/동맹/AI 판단을 계산하지 않는다.
struct ParticipationScene {
    std::array<std::uint8_t,1620> players{}; // 실제폭9×180BYTE의입력/출력저장소다.
    std::array<std::uint32_t,81> allies{}; // 서로다른방향을보존하는실제전체행렬이다.
    std::array<std::uint8_t,9> hidden{}; // 실제Chat숨김전역의명시입력이다.
    PlayerParticipationState state; // 호출마다참조할현재모드다.
    PlayerParticipationHooks hooks; // 원본외부효과의인자/순서관찰이다.
    std::string trace; // 실제발생한효과의순서다.
    // 독립 입력13개를 현재raw자료에만 적용한다.
    explicit ParticipationScene(const std::vector<std::string>& fields) {
        CHECK(fields.size()==13 || fields.size()==14);const auto number=[&](std::size_t index) { return static_cast<std::uint32_t>(std::stoll(fields[index])); }; // fixture의DWORD입력을감김변환한다.
        const auto owner=std::stoi(fields[0]);state.playerKnown=number(6);state.localOwner=number(7);state.metamap=number(8)!=0;state.battle=number(9)!=0;state.localTransport=number(10)!=0;
        // 실제Player이름/Subscriber/IP와독립방향별표를준비한다.
        for (std::size_t player=0;player<9;++player) {
            const auto name="P"+std::to_string(player);std::copy(name.begin(),name.end(),players.begin()+static_cast<std::ptrdiff_t>(player*180+12));ParticipationPut(players,player*180+44,static_cast<std::uint32_t>(player+100));
            std::fill_n(players.begin()+static_cast<std::ptrdiff_t>(player*180+48),30,static_cast<std::uint8_t>('Z'));players[player*180+78]='~';players[player*180+79]=127;hidden[player]=static_cast<std::uint8_t>((number(11)>>player)&1);
            // 초기자료는대칭으로완성한동맹이아닌입력패턴이며원본과C++에똑같이공급한다.
            for (std::size_t target=0;target<9;++target) allies[player*9+target]=player==target ? 1U : (player+target)%3==0 ? 7U : 0U;
        }
        const auto selected=owner>=0 && owner<9 ? static_cast<std::size_t>(owner) : 0U;
        ParticipationPut(players,selected*180+80,number(3));ParticipationPut(players,selected*180+84,number(2));ParticipationPut(players,selected*180+176,number(4));ParticipationPut(players,selected*180+44,number(5));
        if (fields.size()==14) { const auto length=number(13);std::fill_n(players.begin()+static_cast<std::ptrdiff_t>(selected*180+12),length,static_cast<std::uint8_t>('N'));players[selected*180+12+length]=0; }
        hooks.assertion=[this](PlayerParticipationAssertion kind) { Add(std::string("A,")+(kind==PlayerParticipationAssertion::BrainAlreadyAssigned ? 'B' : 'M')); };
        hooks.assignConnection=[this](std::int32_t pending,std::int32_t target) { Add("M,"+std::to_string(static_cast<std::uint32_t>(pending))+','+std::to_string(static_cast<std::uint32_t>(target))); };
        hooks.rejectLogin=[this](std::int32_t pending) { Add("R,"+std::to_string(static_cast<std::uint32_t>(pending))); };hooks.disconnect=[this](std::int32_t pending) { Add("D,"+std::to_string(static_cast<std::uint32_t>(pending))); };
        hooks.clearChat=[this](std::uint32_t channel) { Add("X,"+std::to_string(channel)); };
        hooks.addChat=[this](std::uint32_t subscriber,std::string_view name,std::uint32_t channel) { const auto bytes=std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(name.data()),name.size());Add("C,"+std::to_string(subscriber)+','+ParticipationHex(bytes)+','+std::to_string(channel)); };
        hooks.sendAlliance=[this](std::uint32_t server,std::uint32_t player,std::uint32_t mask) { Add("S,"+std::to_string(server)+','+std::to_string(player)+','+std::to_string(mask)); };
        hooks.destructBrain=[this](std::uint32_t token) { Add("K,"+std::to_string(token)); };hooks.freeBrain=[this](std::uint32_t token) { Add("F,"+std::to_string(token)); };
    }
    // 원본효과의기대모델대신실제호출만기록한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 현재저장소를빌리는처리기를만들며게임객체/brain을생성하지않는다.
    RawPlayerParticipation Runtime() { return {OriginalEdition::Patch1078,players,allies,hidden,state,hooks}; }
    // 방향별표전체를원본DWORD바이트순서로직렬화한다.
    std::string AllyBytes() const {
        std::array<std::uint8_t,324> bytes{};
        // 각cell의전체DWORD를0/1로정규화하지않고검사한다.
        for (std::size_t index=0;index<allies.size();++index) ParticipationPut(bytes,index*4,allies[index]);return ParticipationHex(bytes);
    }
};
}
// native의전체기록1620BYTE/전체행렬324BYTE와모든외부효과를고유입력608개에서비교한다.
TEST_CASE(PlayerParticipation_Native608) {
    std::size_t count=0;
    // 원본이반환한모든raw출력과분기효과를입력순서대로대조한다.
    for (const auto& row:LoadFixture(NETSTORM_PLAYER_PARTICIPATION_FIXTURE).rows) {
        if (row[0]=="kind") continue;
        CHECK(row.size()==6);const auto fields=Split(row[1],',');ParticipationScene scene(fields);auto runtime=scene.Runtime();const auto owner=static_cast<std::uint32_t>(std::stoll(fields[0]));
        if (row[0]=="Login") CHECK(runtime.CheckLogin(std::stoi(fields[1]),std::stoi(fields[0]))==(Number(row[2])!=0));
        else if (row[0]=="Alliance") runtime.SetAlliance(owner,static_cast<std::uint32_t>(std::stoll(fields[2])));
        else if (row[0]=="Add") runtime.AddAlly(owner,static_cast<std::uint32_t>(std::stoll(fields[1])));
        else if (row[0]=="Attach") runtime.AttachBrain(owner,static_cast<std::uint32_t>(std::stoll(fields[12])));
        else if (row[0]=="Remove") runtime.RemoveBrain(owner);else if (row[0]=="RemoveAll") runtime.RemoveAllBrains();else CHECK(false);
        CHECK(ParticipationHex(scene.players)==row[3]);CHECK(scene.AllyBytes()==row[4]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[5]);++count;
    }
    CHECK(count==608);
}
// 원본 수신288개 입력의 전체 Player/방향표/현재 packet과 모든 효과를 두 정밀도 원문에 대조한다.
TEST_CASE(PlayerParticipation_AlliancePacketNative288) {
    std::size_t count{}; // 실제로 재생한 고유 원본 입력 수다.
    // 기대 출력은 C++ 작성 전에 동결한 원본 전체 함수의 반환만 사용한다.
    for (const auto& row:LoadFixture(NETSTORM_PLAYER_ALLIANCE_PACKET_FIXTURE).rows) {
        if (row[0]=="input") continue;
        const auto values=Split(row[0],',');CHECK(values.size()==5 && row.size()==9);
        if (values.size()!=5 || row.size()!=9) return;
        const std::vector<std::string> fields{values[0],"1","1515870810","0","0","0",values[2],values[3],values[4],"0","0","341","7002"};
        ParticipationScene scene(fields);auto runtime=scene.Runtime();std::array<std::uint8_t,41> storage;storage.fill(0xa7);
        auto packet=std::span<std::uint8_t>(storage).subspan(16,9);packet[0]=1;packet[1]=0x3d;packet[2]=9;packet[3]=0;packet[4]=static_cast<std::uint8_t>(std::stoul(values[0]));ParticipationPut(packet,5,static_cast<std::uint32_t>(std::stoull(values[1])));
        runtime.ReceiveAlliance(packet,[&](std::span<const std::uint8_t> bytes) { CHECK(bytes.data()==packet.data());scene.Add("B,"+ParticipationHex(bytes)); });
        const std::vector<std::string> actual{ParticipationHex(scene.players),scene.AllyBytes(),scene.trace.empty() ? "-" : scene.trace,ParticipationHex(packet)};
        CHECK(actual==std::vector<std::string>(row.begin()+1,row.begin()+5));CHECK(actual==std::vector<std::string>(row.begin()+5,row.end()));
        CHECK(std::all_of(storage.begin(),storage.begin()+16,[](auto byte) { return byte==0xa7; }) && std::all_of(storage.begin()+25,storage.end(),[](auto byte) { return byte==0xa7; }));++count;
    }
    CHECK(count==288);
}
// 같은brain토큰은파괴/반납경계가모두끝날때까지조회되며flags20은해제뒤에도원본대로유지된다.
TEST_CASE(PlayerParticipation_BrainLifetimeAndDirectionalRows) {
    ParticipationScene scene(Split("1,1,0,0,0,0,0,1,0,0,0,0,7002",','));auto runtime=scene.Runtime();runtime.AttachBrain(1,7002);CHECK(runtime.Brain(1)==7002);
    scene.hooks.destructBrain=[&](std::uint32_t token) { CHECK(token==7002 && runtime.Brain(1)==7002); };scene.hooks.freeBrain=[&](std::uint32_t token) { CHECK(token==7002 && runtime.Brain(1)==7002); };
    auto actual=scene.Runtime();actual.RemoveBrain(1);CHECK(actual.Brain(1)==0 && (scene.players[180+80]&0x20));
    actual.SetAlliance(1,1U<<2);CHECK(scene.allies[1*9+2]==1 && scene.allies[2*9+1]==7);actual.SetAlliance(2,0);CHECK(scene.allies[1*9+2]==1 && scene.allies[2*9+1]==0);
}
