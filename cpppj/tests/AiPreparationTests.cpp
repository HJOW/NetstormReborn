// 원본 준비의 현재 참조·대상 선택·목록 불일치·x87 경계를 검사한다. 중단 지시에 따라 작성·컴파일만 한다.
#include "o/RawAiPreparation.h"
#include "RawSceneSupport.h"
#include <cmath>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 등록된 메모리에 합성 입력을 넣으며 원본 실행 관측값이나 게임 대체 AI로 쓰지 않는다.
struct PreparationInput {
    RawAiBrainMemory memory{0x42000000,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); }}; // 할당 원문의 명시 검사 입력이다.
    SidPool pool{OriginalEdition::Patch1078,32768}; // 현재 사제 HP·좌표를 보관하는 동일 풀이다.
    std::array<std::uint8_t,1620> players{}; // 같은 Player9 기록이다.
    std::array<std::uint32_t,81> allies{}; // 같은 한 방향 동맹 표다.
    std::array<std::uint32_t,9> priests{}; // 실제 Lookup 하위의 명시 SID 입력이다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},localOwner{8},playerKnown{1},invalidGraph{254}; // 같은 현재 본체와 전역 입력이다.
    std::int32_t noRegion{127};double now{16777216.25}; // 현재 지역 없음 번호와 float 축소를 구별하는 QWORD다.
    std::uint32_t demo{},scans{},assertions{};bool knowledge{},blocking{}; // 선택된 현재 장치 입력과 호출 관찰이다.
    std::map<std::int32_t,std::uint32_t> temples;std::map<std::uint32_t,std::uint32_t> graphs; // 선택되는 하위 결과의 명시 입력이다.
    std::vector<std::uint32_t> priestQueries,graphQueries,spQueries;std::vector<float> additions; // 호출 순서·횟수의 관찰값이다.
    std::vector<std::pair<std::array<float,2>,std::array<float,2>>> distances; // 발 중심 보정이 없는 두 Point 입력을 관찰한다.
    AiPreparationHooks hooks; // 검사마다 필요한 선택 효과만 교체한다.
    // 원본 생성자를 대신했다고 주장하지 않는 명시0 본체와 실제 목록 블록을 준비한다.
    PreparationInput() {
        auto raw=memory.Bytes(token);std::fill(raw.begin(),raw.end(),std::uint8_t{});Field(0x260,1);Field(0x264,2);Field(0x25c,1);Field(0x324,memory.Allocate(160));Field(0x328,40);Field(0x228,std::bit_cast<std::uint32_t>(3.25F));
        // 모든 Player의 현재 지역을 없으며 자기 방향만 동맹으로 명시한다.
        for (std::uint32_t owner=0;owner<9;++owner) { Put(players,owner*180+0x78,127);allies[owner*9+owner]=1; }
        hooks.missingEnemyAssertion=[this] { ++assertions; };
        hooks.priest=[this](std::uint32_t owner) { priestQueries.push_back(owner);if (owner>=priests.size()) throw std::out_of_range("검사 사제 owner 범위");return priests[owner]; };
        hooks.temple=[this](std::int32_t region) { const auto found=temples.find(region);return found==temples.end() ? 0U : found->second; };
        hooks.maxHitPoints=[](Sid) { return 100; };
        hooks.pointDistance=[this](auto source,auto target) { distances.emplace_back(source,target);const double dx=static_cast<double>(source[0])-target[0],dy=static_cast<double>(source[1])-target[1];return std::sqrt(dx*dx+dy*dy); };
        hooks.graph=[this](std::uint32_t sid) { graphQueries.push_back(sid);const auto found=graphs.find(sid);return found==graphs.end() ? invalidGraph : found->second; };
        hooks.regionBounds=[](std::int32_t) { return std::array<float,4>{16,32,64,96}; };
        hooks.storedSp=[this](std::uint32_t owner) { spQueries.push_back(owner);return 0.0F; };
        hooks.addLocalSp=[this](float delta) { additions.push_back(delta); };
        hooks.demo=[this] { return demo; };hooks.knowledgeOpen=[this] { return knowledge; };hooks.blockingGumpOpen=[this] { return blocking; };hooks.initialScan=[this](auto brain) { CHECK(brain==token);++scans; };
    }
    // 같은 실제 입력을 빌리는 준비 몸체를 구성한다.
    std::unique_ptr<RawAiPreparation> Make() { return std::make_unique<RawAiPreparation>(memory,pool,players,allies,AiPreparationGlobals{localOwner,playerKnown,noRegion,[this] { return invalidGraph; },now},hooks); }
    // 본체의 현재 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) { return Get(memory.Bytes(token),at); }
    // 본체의 현재 DWORD만 바꾼다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재 QWORD 시각을 결과 관찰용 double로 해석한다.
    double Time(std::size_t at) { return std::bit_cast<double>((std::uint64_t(Field(at+4))<<32)|Field(at)); }
    // 원본 목록 포인터의 실제 입력 칸에만 쓴다.
    void Geyser(std::size_t index,std::uint32_t value) { Put(memory.Bytes(Field(0x324)),index*4,value); }
    // 현재 Player 참여 DWORD와 같은 풀의 사제 raw HP·좌표를 준비한다.
    Sid Priest(std::uint32_t owner,std::int32_t hp,float x,float y) {
        const auto sid=pool.Allocate(2);priests[owner]=sid.value;Put(players,owner*180+0x2c,1);auto raw=pool.AllocatedBytes(sid);Put(raw,0x1a,std::bit_cast<std::uint32_t>(hp));Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));return sid;
    }
};

// 강제 대상과 첫 비동맹 선택이 서로 다른 하위 호출/조기 반환 계약을 유지한다.
TEST_CASE(AiPreparation_ForcedAndDirectionalEnemySelection) {
    PreparationInput input;auto preparation=input.Make();input.Field(0x240,0xffffffffU);preparation->SelectEnemy(input.token);CHECK(input.Field(0x264)==0xffffffffU && input.priestQueries.empty());
    input.Field(0x240,0);input.priests[1]=10;input.priests[2]=11;input.priests[3]=12;input.allies[1*9+2]=1;input.allies[3*9+1]=1;preparation->SelectEnemy(input.token);CHECK(input.Field(0x264)==3 && input.priestQueries==std::vector<std::uint32_t>({1,2,3}));
    input.priestQueries.clear();input.hooks.priest=[&](auto owner) { input.priestQueries.push_back(owner);if (owner==1) input.Field(0x260,2);return owner==1 ? 10U : 0U; };preparation=input.Make();preparation->SelectEnemy(input.token);CHECK(input.Field(0x264)==1 && input.priestQueries==std::vector<std::uint32_t>({1}));
}

// 같은 최초 스캔 위치·그래프/범위·두 수량 판정과 QWORD 시각을 합쳐 대조한다.
TEST_CASE(AiPreparation_CombinedPreparationUsesCurrentWorldAndOneScan) {
    PreparationInput input;input.localOwner=1;input.demo=1;input.knowledge=true;input.hooks.blockingGumpOpen=[]()->bool { throw std::logic_error("지식 창 참이면 차단 창 조회 금지"); };Put(input.players,180+0x78,3);Put(input.players,360+0x78,4);input.temples={{3,100},{4,200}};input.graphs={{100,7},{200,8},{700,7},{701,8}};input.Field(0x238,4);input.Field(0x220,std::bit_cast<std::uint32_t>(6.0F));input.Field(0x25c,0);input.Field(0x234,3);input.Field(0x230,2);input.Field(0x2e4,2);input.Field(0x2b0,0xdeadbeef);
    input.hooks.initialScan=[&](auto brain) { CHECK(brain==input.token);CHECK(input.Field(0x29c)==0 && input.Field(0x2a0)==0 && input.spQueries.size()==1);++input.scans;input.Geyser(0,700);input.Geyser(1,701);input.Field(0x338,2);input.Field(0x32c,2); };
    auto preparation=input.Make();preparation->Update(input.token);CHECK(input.Time(0x5a8)==16777219.5 && input.scans==1 && input.Field(0x25c)==1);CHECK(input.Field(0x294)==100 && input.Field(0x2a8)==200 && input.Field(0x298)==7 && input.Field(0x2ac)==8 && input.Field(0x5f8)==7);CHECK(input.Field(0x604)==4 && input.Field(0x5fc)==1 && input.Field(0x600)==std::bit_cast<std::uint32_t>(6.0F));CHECK(std::bit_cast<float>(input.Field(0x2b0))==40 && std::bit_cast<float>(input.Field(0x2b4))==64);CHECK(input.Field(0x2cc)==0 && input.Field(0x2d0)==0 && input.additions.empty());
    CHECK(input.graphQueries==std::vector<std::uint32_t>({100,200,700,701,700,701}));input.Field(0x2d8,12001);input.hooks.storedSp=[&](auto) { return 12001.0F; };preparation=input.Make();preparation->Update(input.token);CHECK(input.scans==1 && input.Field(0x2d0)==1 && &preparation->Pool()==&input.pool);
}

// 절반 HP 경계·전체 동맹 거리 방문·현재 적 조기 반환과 HP 선저장을 확인한다.
TEST_CASE(AiPreparation_VulnerableEnemySelectionKeepsNativeOrder) {
    PreparationInput input;input.Field(0x264,4);input.Field(0x248,0x100);const auto enemy=input.Priest(2,10,11,10);input.Priest(1,100,10,10);input.Priest(3,100,50,10);input.allies[1*9+3]=1;
    input.hooks.maxHitPoints=[&](Sid sid) { CHECK(sid==enemy);Put(input.pool.AllocatedBytes(sid),0x1a,100);return 100; };auto preparation=input.Make();preparation->Update(input.token);CHECK(input.Field(0x264)==2 && input.distances.size()==2);if (input.distances.size()==2) CHECK((input.distances[0].first==std::array<float,2>({10,10}) && input.distances[0].second==std::array<float,2>({11,10}) && input.distances[1].first==std::array<float,2>({50,10})));
    PreparationInput boundary;boundary.Field(0x264,4);boundary.Field(0x248,0x100);boundary.Priest(2,50,11,10);boundary.Make()->Update(boundary.token);CHECK(boundary.Field(0x264)==4 && boundary.distances.empty());
    PreparationInput current;current.Field(0x248,0x100);current.Priest(2,-1,11,10);current.Priest(5,-2,12,10);current.hooks.pointDistance=[](auto,auto)->double { throw std::logic_error("현재 취약 적이면 동맹 거리 조회 금지"); };current.Make()->Update(current.token);CHECK(current.Field(0x264)==2 && current.priestQueries==std::vector<std::uint32_t>({2,1,2}));
}

// FILD 정수 비교와 float 차액 저장을 구분하고 두 번째 SP 읽기의 현재 owner/지갑을 보존한다.
TEST_CASE(AiPreparation_LocalSpRechecksOwnerAndPreservesIntegerPrecision) {
    PreparationInput input;input.localOwner=1;input.Field(0x2d8,16777217);input.hooks.storedSp=[&](auto owner) { input.spQueries.push_back(owner);if (input.spQueries.size()==1) input.localOwner=2;else input.Field(0x2d8,16777219);return 16777216.0F; };input.Make()->Update(input.token);CHECK(input.spQueries==std::vector<std::uint32_t>({1,2}) && input.additions==std::vector<float>({3.0F}));
    PreparationInput unknown;unknown.localOwner=1;unknown.playerKnown=0;unknown.Field(0x2d8,9);unknown.hooks.storedSp=[](auto)->float { throw std::logic_error("PlayerKnown0은 분산 SP 읽기 금지"); };unknown.Make()->Update(unknown.token);CHECK(unknown.additions==std::vector<float>({9.0F}));
}

//324 포인터·338 개수의 실제 조합과 하위 뒤의 현재 개수/포인터/기준 그래프 재조회를 확인한다.
TEST_CASE(AiPreparation_GeyserCountUsesNativeMismatchedFields) {
    PreparationInput input;input.Field(0x32c,0xffffffffU);input.Field(0x338,2);input.Field(0x298,7);input.Geyser(0,700);input.Geyser(1,701);const auto replacement=input.memory.Allocate(8);Put(input.memory.Bytes(replacement),4,702);
    input.hooks.graph=[&](auto sid) { input.graphQueries.push_back(sid);if (sid==700) { input.Field(0x324,replacement);input.Field(0x298,8); }return 8U; };auto preparation=input.Make();CHECK(preparation->CountAttachedGeysers(input.token)==2 && input.graphQueries==std::vector<std::uint32_t>({700,702}));
    input.Field(0x338,0);input.Field(0x324,0xdeadbeef);CHECK(preparation->CountAttachedGeysers(input.token)==0);input.Field(0x338,2);input.Field(0x324,input.memory.Allocate(4));CHECK(Throws([&] { preparation->CountAttachedGeysers(input.token); }));
}

// 선택되지 않은 누락 하위는 요구하지 않고 선택된 지점에서는 앞선 실제 쓰기를 유지한 채 오류를 낸다.
TEST_CASE(AiPreparation_SelectedMissingHooksAndPhysicalContracts) {
    PreparationInput input;input.hooks.regionBounds={};input.hooks.maxHitPoints={};input.hooks.pointDistance={};input.hooks.graph={};input.Make()->Update(input.token);CHECK(input.Field(0x298)==254 && input.Field(0x2ac)==254 && input.Field(0x2d0)==1);
    input.Field(0x25c,0);input.hooks.initialScan={};CHECK(Throws([&] { input.Make()->Update(input.token); }));CHECK(input.Field(0x25c)==0 && input.Field(0x5fc)==1);
    input.Field(0x264,0);input.hooks.priest=[](auto) { return 0U; };input.hooks.missingEnemyAssertion={};CHECK(Throws([&] { input.Make()->Update(input.token); }));CHECK(input.Field(0x264)==0);
    const auto shortBody=input.memory.Allocate(16);CHECK(Throws([&] { input.Make()->SelectEnemy(shortBody); }));CHECK(Throws([&] { RawAiPreparation bad(input.memory,input.pool,std::span(input.players).first(1619),input.allies,{input.localOwner,input.playerKnown,input.noRegion,[&] { return input.invalidGraph; },input.now},{}); }));
}
}
