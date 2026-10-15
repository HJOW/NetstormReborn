// 실제 PATCH의 후보/생성/Player부모 출력과 같은 raw 입력의 전체 결과/효과 순서를 대조한다.
#include "ConstructionSupport.h"
#include "o/RawLocalGeyser.h"
#include <fstream>
#include <sstream>
#include <utility>

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
constexpr std::uint32_t kLocalGeyserCases=221,kLocalPoolCapacity=32768; // 기존220입력과추가공유자원1입력및실제서버/예측영역검사용량이다.
// fixture의 원본 사각형 입력만 제공하며 성공 좌표를 계산하지 않는다.
FortRandomRectangle GeyserRectangle(std::uint32_t profile) {
    constexpr std::array<FortRandomRectangle,5> areas{{{16,24,48,56},{16.25F,24.5F,48.75F,56.25F},{16,24,16.5F,24.5F},{16,24,15,23},{0,0,2,2}}}; // 독립 실행기에 넘긴 같은 명시 사각형이다.
    return areas.at(profile);
}
// 모든 현재 최근목록 칸을 native와 같은 문자열로 기록한다.
std::string GeyserRecent(const SquidPostPopList& recent) {
    std::string result;
    // count밖 stale항목도 보존했는지 확인한다.
    for (const auto value:recent.entries) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
}
// 실행기의9Player flags/region 입력을 실제 원문 표에 기록한다.
void GeyserPlayers(std::span<std::uint8_t> players,std::uint32_t profile) {
    std::array<std::uint32_t,9> flags{},regions{};regions.fill(127); // 비활성/지역없음의 명시 기본 입력이다.
    if (profile==1) { flags[1]=2;regions[1]=1; }
    else if (profile==2) { flags[1]=2;regions[1]=1;flags[2]=32;regions[2]=2; }
    else if (profile==3) { flags[4]=34;regions[4]=4;flags[5]=2;regions[5]=5; }
    else if (profile==4) { flags={0,1,2,32,34,0,0,2,32};regions={127,1,127,3,4,5,127,7,8}; }
    // 같은 원본9기록의 각 BYTE/DWORD만 쓴다.
    for (std::size_t owner=0;owner<9;++owner) { Put(players,owner*0xb4+0x50,flags[owner],1);Put(players,owner*0xb4+0x78,regions[owner]); }
}
// 실제 생성/공간/설정 경계만 관찰한다. 난수/후보/반복/최근/pool의 기대값을 재계산하지 않는다.
struct GeyserEnvironment {
    LocalGeyserState& state; // 실제 모듈과 같은 현재 pool/수량/타입 전역이다.
    SquidPostPopList& recent; // 같은 최근5목록의 mutable 입력이다.
    std::span<std::uint8_t> players; // 같은 raw9Player 기록이다.
    std::span<RiftTypeRecord> types; // 실제 Pop 뒤 cost변경 입력을 적용할 같은 타입표다.
    std::int32_t* sharedPool{}; // 연결검사는같은SquidReward전역주소를공급하며미연결검사는기존값형입력이다.
    std::uint32_t predictable{},mutation{},created{}; // 명시 설정/외부효과/생성SID 순번이다.
    double time{}; // 실제 PE TimeSeed에 넘긴 같은 벽시각이다.
    std::string trace; // 실제 외부 효과 순서다.
    // 원본 fixture와 같은 한 항목의 외부 효과를 기록한다.
    void Add(std::string value) { if (!trace.empty()) trace+='|';trace+=std::move(value); }
    // 외부삭제/Pop효과가실제로변경하는단일현재자원저장소다.
    std::int32_t& Pool() const { return sharedPool ? *sharedPool : state.resourcePool; }
    // 실제 함수에 해당하는 명시 장치/공간 하위만 공급한다.
    LocalGeyserHooks Hooks() {
        LocalGeyserHooks hooks;
        hooks.configInt=[this](std::string_view key,std::int32_t fallback) { CHECK(key=="predictableGeysers");CHECK(fallback==1);Add("Config");return static_cast<std::int32_t>(predictable); };
        hooks.wallSeconds=[this] { Add("Time");return time; };
        hooks.playerBounds=[this](std::uint32_t owner) {
            Add("Bounds:"+std::to_string(owner));
            if (mutation==2 && owner==1) { state.firstPlayerQuantity=1;Put(players,2*0xb4+0x50,32,1);Put(players,2*0xb4+0x78,2); }
            const auto origin=static_cast<float>(owner*16); // native에 직접 입력한 같은 Player별 사각형이다.
            return FortRandomRectangle{origin,origin,origin+32,origin+32};
        };
        hooks.negativeQuantity=[this] { Add("Assert"); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { CHECK(sid.value==9);if (mutation==3) Pool()=12000;Add("Destroy:"+std::to_string(sid.value)+":"+std::to_string(flags)); };
        hooks.confirm=[this](const ConstructionConfirmRequest& request) {
            const std::array<std::uint32_t,10> values{request.type,Bits(request.x),Bits(request.y),request.player,request.argument,request.direction,request.flags,request.timeLow,request.timeHigh,request.quality}; // 실제 원본10인자 비트다.
            std::string text="Confirm:";
            // 요청 인자를 원본 순서로 기록하며 임의 지형을 생성하지 않는다.
            for (std::size_t i=0;i<values.size();++i) { if (i) text+=',';text+=std::to_string(values[i]); }Add(std::move(text));
        };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) {
            const auto sid=static_cast<std::uint16_t>(50+created++); // 원본 관찰 경계가 공급한 같은 명시 factory 반환이다.
            Add("Create:"+std::to_string(type)+":"+std::to_string(flags)+":"+std::to_string(sid));return Sid{sid};
        };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) {
            Add("Pop:"+std::to_string(sid.value)+":"+std::to_string(Bits(x))+":"+std::to_string(Bits(y))+":"+std::to_string(flags));
            if (mutation==1) { Pool()=16777217;types[122].cost=4000;recent.count=4; }
            else if (mutation==3) Pool()+=1000;
        };return hooks;
    }
};
}

// 같은원본221행을기존독립상태/실제공유자원두backend에서결과/전체저장소/효과순서까지대조한다.
void CheckLocalGeyserFixture(bool shared) {
    std::ifstream input(NETSTORM_LOCALGEYSER_FIXTURE);if (!input) throw std::runtime_error("로컬간헐천 fixture 없음");
    std::string line;std::uint32_t count=0;
    // 사례마다 실제 원본과 같은 독립 raw 저장소를 구성한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=26) throw std::runtime_error("로컬간헐천 fixture 열 수 오류");++count;
        SidPool pool(OriginalEdition::Patch1078,kLocalPoolCapacity);SquidHash hash;std::vector<std::uint8_t> spots(65536,static_cast<std::uint8_t>(Number(row[3]))),players(9*0xb4);
        std::vector<RiftTypeRecord> types(188);types[122].cost=2000;types[84].footX=types[84].footY=256;
        std::vector<RiftTypeFrames> frames;frames.reserve(188);
        // 프레임표는 현재 타입 번호의 같은 원문 자료로 제공한다.
        for (std::size_t i=0;i<188;++i) frames.emplace_back(std::vector<FrameCode>{});
        if (Number(row[8])) {
            auto raw=InputRaw(pool,Sid{9});Put(raw,8,0,2);raw[10]=84;raw[40]=0;Put(raw,14,Bits(255));Put(raw,18,Bits(255));
            // 모든 검색점에서 같은 명시 empty결과9가 실제typedfinder로 나오도록 넓은 발자국을 입력한다.
            for (int y=0;y<16;++y) for (int x=0;x<16;++x) hash.Cell(3,x,y)=9;
        }
        const auto surfaceProfile=Number(row[15]); // 현재hash0/프레임문자 무늬의 명시 입력이다.
        if (surfaceProfile) {
            const std::string pattern=surfaceProfile==1 ? "FFFFFFFFF" : surfaceProfile==2 ? "FBICANGDH" : "fbicangdh"; // native에 직접 넣은 같은9문자다.
            std::vector<FrameCode> codes;
            // 후보첫3×3과같은고정raw입력이며원본출력으로좌표를역산하지않는다.
            for (std::size_t i=0;i<9;++i) {
                const auto sid=static_cast<std::uint16_t>(20+i);auto raw=InputRaw(pool,Sid{sid});raw[10]=97;Put(raw,36,static_cast<std::uint32_t>(i));
                hash.Cell(0,31+static_cast<int>(i/3),35+static_cast<int>(i%3))=sid;codes.push_back({static_cast<std::uint8_t>(pattern[i]),65,static_cast<std::uint8_t>(i),0});
            }frames[97]=RiftTypeFrames(std::move(codes));
        }
        SquidPostPopList recent;recent.count=3;recent.entries={31,32,33,34,35,36,37};LocalGeyserState state;
        state.boss=Number(row[5])!=0;state.fort=Number(row[6])!=0;state.resourcePool=std::bit_cast<std::int32_t>(Number(row[7]));
        state.firstPlayerQuantity=static_cast<std::int32_t>(std::stoll(row[12]));state.lastPlayerQuantity=static_cast<std::int32_t>(std::stoll(row[13]));
        GeyserPlayers(players,Number(row[9]));if (Number(row[14])==2) { Put(players,0xb4+0x50,2,1);Put(players,0xb4+0x78,1); }
        GeyserEnvironment environment{state,recent,players,types};environment.predictable=Number(row[10]);environment.time=std::bit_cast<double>(std::stoull(row[11]));environment.mutation=Number(row[14]);
        std::int32_t sharedPool=state.resourcePool; // 실제전역의명시초기입력이며Bind시의복사/예산snapshot이아니다.
        if (shared) { state.resourcePool=-777;environment.sharedPool=&sharedPool; }
        RawLocalGeyser body(pool,hash,spots,types,frames,players,recent,state,environment.Hooks());CHECK(&body.Pool()==&pool);CHECK(&body.Recent()==&recent);
        if (shared) { body.BindResourcePool(sharedPool);CHECK(&body.ResourcePool()==&sharedPool); }
        GameRandom random(Number(row[1]));LocalGeyserPoint point{Float(0xdeadbeef),Float(0xcafebabe)};std::uint32_t result=0;
        if (row[0]=="T") result=body.TryCandidate(random,GeyserRectangle(Number(row[2])),point);
        else if (row[0]=="P") body.Populate(static_cast<std::int32_t>(std::stoll(row[4])),random,GeyserRectangle(Number(row[2])));
        else if (row[0]=="G") body.Generate();else throw std::runtime_error("로컬간헐천 fixture 연산 오류");
        CHECK(result==Number(row[16]));CHECK(Bits(point.x)==Number(row[17]));CHECK(Bits(point.y)==Number(row[18]));CHECK(random.State()==Number(row[19]));
        CHECK(recent.count==Number(row[20]));CHECK(GeyserRecent(recent)==row[21]);CHECK(std::bit_cast<std::uint32_t>(body.ResourcePool())==Number(row[22]));
        if (shared) CHECK(state.resourcePool==-777);
        // 외부호출이없는행은fixture의빈열sentinel과비교하며실제관찰내용을바꾸지않는다.
        CHECK(state.firstPlayerQuantity==static_cast<std::int32_t>(std::stoll(row[23])));CHECK(environment.created==Number(row[24]));CHECK((environment.trace.empty() ? "-" : environment.trace)==row[25]);
    }
    CHECK(count==kLocalGeyserCases);
}

// 기존값형독립fixture의현재동작을유지한다.
TEST_CASE(LocalGeyser_IndependentPeFixture) { CheckLocalGeyserFixture(false); }
// 현재보상전역을직접참조하고삭제환급/Pop뒤현재값을다시읽으며사본은수정하지않는다.
TEST_CASE(LocalGeyser_SharedResourcePoolPeFixture) { CheckLocalGeyserFixture(true); }

// 실제 생성이 누락된 상태를 간헐천 생성 완료로 세지 않는다.
TEST_CASE(LocalGeyser_RejectsMissingCreation) {
    SidPool pool(OriginalEdition::Patch1078,kLocalPoolCapacity);SquidHash hash;std::vector<std::uint8_t> spots(65536),players(9*0xb4);std::vector<RiftTypeRecord> types(188);
    std::vector<RiftTypeFrames> frames;
    // 현재 판본의 모든 빈 프레임 슬롯을 명시한다.
    for (std::size_t i=0;i<188;++i) frames.emplace_back(std::vector<FrameCode>{});
    SquidPostPopList recent;recent.entries.resize(5);LocalGeyserState state;GeyserEnvironment environment{state,recent,players,types};auto hooks=environment.Hooks();hooks.create={};bool rejected=false;
    try { const RawLocalGeyser body(pool,hash,spots,types,frames,players,recent,state,std::move(hooks)); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
