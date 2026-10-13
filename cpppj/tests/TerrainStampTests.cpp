// 두 보호PE의 독립 초기프레임/stamp 결과를 같은 raw pool/두 난수/공유Choose와 비교한다.
#include "RawSceneSupport.h"
#include "o/RawTerrainStamp.h"
#include "o/RawSquidRegion.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 함수에 넣은 실제 타입 번호와 factory가 반환하기 시작하는 공간 번호다.
constexpr std::uint32_t kTerrainIsle=97,kTerrainEdge=98,kTerrainFringe=102,kTerrainOther=94;
constexpr std::uint16_t kTerrainBorn=40,kTerrainEnd=127;
// 원본이 읽는 코드 저장순서이며 판단/선택 결과는 계산하지 않는다.
RiftTypeFrames TerrainCodes(std::uint32_t dataset,bool fringe) {
    std::vector<FrameCode> codes;
    if (dataset==1 || (dataset==4 && fringe)) return RiftTypeFrames({});
    if (dataset==4) dataset=0;
    if (dataset==2) {
        // 최대256 후보를 넘는 동일code300개를 원본과 같은 순서로 준다.
        for (std::uint32_t index=0;index<300;++index) codes.push_back({'A','A',static_cast<std::uint8_t>(index),0});
        return RiftTypeFrames(std::move(codes));
    }
    // 정확검색/정규화/side,A/side,wildcard를 서로 다른 sparse표에서도 검사한다.
    const auto append=[&codes,fringe](std::uint8_t side,std::uint8_t variant) {
        constexpr std::array<std::uint8_t,4> flags{0,2,0x80,1}; // 실제 fringe signedflags 입력이다.
        // 같은 code의8개 후보는 patch quartile과CD전체 선택의 차이를 가진다.
        for (std::uint32_t index=0;index<8;++index) codes.push_back({side,variant,static_cast<std::uint8_t>(index),fringe ? flags[index%4] : static_cast<std::uint8_t>(1)});
    };
    if (dataset==0) {
        // 물리 코드의 side외부/variant내부 순서도 실제 테이블과 동일하다.
        for (std::uint8_t side='A';side<='P';++side)
            for (std::uint8_t variant='A';variant<='P';++variant) append(side,variant);
    } else for (const auto side:std::array<std::uint8_t,5>{'A','C','F','J','P'}) append(side,'A'); // sparse코드의 원본 입력 순서다.
    return RiftTypeFrames(std::move(codes));
}
// 현재 fixture를 결과 재계산 없이 읽는다.
const std::vector<std::vector<std::string>>& StampRows() {
    static const auto rows=LoadFixture(NETSTORM_TERRAINSTAMP_FIXTURE).rows;return rows;
}
// 원본의 현재 지도·두 난수·전역cache와 단일 raw 슬롯을 조립한다.
struct TerrainStampScene {
    SidPool pool; // 같은 factory/owner/Pop/region의 raw 저장소다.
    std::vector<RiftTypeRecord> types; // 현재 genus 표의 입력이다.
    std::vector<RiftTypeFrames> frames; // 실제 코드/J첫 프레임 입력이다.
    std::array<std::uint8_t,65536> codes{},regions{}; // 원본과 같은 X열 우선 지도다.
    std::array<std::int32_t,256> mapping{},aliases{}; // 문자별 타입/대응문자 DWORD다.
    TerrainStampMap map; // 위 실제 지도 span과 현재 context를 공유한다.
    TerrainStampState state; // 초기 stamp만 쓰는 전역99cache다.
    GameRandom random; // 실제 코드/fringe 후보가 소비한다.
    std::uint32_t crtState{}; // cache생성의 독립 CRT holdrand다.
    std::vector<std::uint32_t> input; // 원본 fixture의 입력이며 기대값을 만들지 않는다.
    std::vector<std::string> events; // 외부 factory/가상 효과의 실제 인자/순서다.
    std::uint16_t born{}; // 현재 factory가 반환할 슬롯 번호다.
    std::uint32_t assertions{}; // 실제 assertion 경계의 호출 수다.
    bool patch; // raw frame/owner의 현재 판본 배치다.
    std::string kind; // Frame 또는Stamp 진입점이다.
    std::unique_ptr<RawTerrainStamp> stamp; // 위 단일 저장소를 쓰는 복원 모듈이다.
    // 명시 client번호를 확보하고 결과 판단 전에 원본 입력만 준비한다.
    explicit TerrainStampScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        frames(types.size(),RiftTypeFrames({})),patch(edition==OriginalEdition::Patch1078) {
        // fixture가 생성할 모든 SID는 행별로 재사용한다.
        for (std::uint16_t sid=5;sid<=kTerrainEnd;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        map.codes=codes;map.regions=regions;map.typeMapping=mapping;map.aliases=aliases;
    }
    // 지도 밖 이웃은 쓰지 않고 원본이 bounds검사로 배제하게 둔다.
    void Cell(std::int32_t x,std::int32_t y,std::uint8_t code,std::uint8_t region=5) {
        if (x<0 || x>=256 || y<0 || y>=256) return;
        const auto index=static_cast<std::size_t>(x)*256+static_cast<std::size_t>(y);codes[index]=code;regions[index]=region;
    }
    // 필수 하위 경계의 동일 입력/생성 raw 효과를 설치한다. 전체 파생 factory/Pop을 복원한 검사로 세지 않는다.
    TerrainStampHooks Hooks() {
        TerrainStampHooks hooks;
        hooks.create=[this](std::uint32_t type,std::uint32_t predictable) {
            const auto sid=Sid{born++};auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(type);
            events.push_back("C:"+std::to_string(sid.value)+':'+std::to_string(type)+':'+std::to_string(predictable));return sid;
        };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) {
            pool.AllocatedBytes(sid)[patch ? 34 : 32]=static_cast<std::uint8_t>(owner);events.push_back("O:"+std::to_string(sid.value)+':'+std::to_string(owner));
        };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) {
            auto raw=pool.AllocatedBytes(sid);const auto bx=std::bit_cast<std::uint32_t>(x),by=std::bit_cast<std::uint32_t>(y);Put(raw,14,bx);Put(raw,18,by);
            events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(bx)+':'+std::to_string(by)+':'+std::to_string(flags));
        };
        hooks.setRegion=[this](Sid sid,std::int32_t region) {
            events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(static_cast<std::uint32_t>(region)));
            SetSquidRegion(pool,types,sid,static_cast<std::uint32_t>(region),static_cast<std::uint32_t>(state.noRegion),
                [this](SquidRegionAssert) { ++assertions;events.push_back("A"); });
        };
        hooks.chunkRegion=[this](std::int32_t x,std::int32_t y) { events.push_back("H:"+std::to_string(x)+':'+std::to_string(y));return std::int8_t{6}; };
        hooks.assertion=[this](TerrainStampAssertion) { ++assertions;events.push_back("A"); };
        return hooks;
    }
    // 모든 입력을 원본과 같은 raw지도/타입/캐시/두 난수로 구성한다.
    void Prepare(const std::vector<std::string>& row) {
        kind=row[1];input.clear();events.clear();assertions=0;born=kTerrainBorn;codes.fill(0);regions.fill(0);mapping.fill(0);aliases.fill(0);state={};map.lastSid={};
        // 입력의 DWORD비트를 그대로 읽는다.
        for (const auto& number:Split(row[2],',')) input.push_back(Number(number));
        // 원본의 case별 raw초기화와 같은, 사전에 확보한 슬롯을 비운다.
        for (std::uint16_t sid=5;sid<=kTerrainEnd;++sid) { auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        const auto dataset=input[kind=="Frame" ? 7 : 1],cache=input[kind=="Frame" ? 8 : 2];
        crtState=input[kind=="Frame" ? 5 : 3];random.SetState(input[kind=="Frame" ? 6 : 4]);
        if (cache) {
            state.variants.emplace();
            // 이미 존재하는 전역cache는 명시 입력이며 새 CRT소비가 아니다.
            for (std::size_t index=0;index<99;++index) (*state.variants)[index]=static_cast<std::int32_t>((index*17+3)%32768);
        }
        // 독립 원본에 넣은 네 타입의 코드/genus를 같은 생산표에 전달한다.
        for (const auto type:{kTerrainIsle,kTerrainEdge,kTerrainFringe,kTerrainOther}) { types[type]={};types[type].flags2=type==kTerrainIsle ? 2 : 0;frames[type]=TerrainCodes(dataset,type==kTerrainFringe); }
        mapping['x']=kTerrainIsle;mapping['=']=kTerrainEdge;mapping['?']=kTerrainOther;mapping['h']=kTerrainIsle;
        aliases['x']='x';aliases['=']='=';aliases['?']='?';aliases['h']='x';
        if (kind=="Frame") {
            CHECK(input.size()==10);const auto x=static_cast<std::int32_t>(input[3]),y=static_cast<std::int32_t>(input[4]);map.bounds={0,0,255,255};map.makePredictable=1;map.excludedFlags=0;
            Cell(x,y,static_cast<std::uint8_t>(input[0]));
            // native입력과 같은 여덟 이웃mask/alias/지역 불일치만 넣는다.
            for (std::size_t index=0;index<kNeighborCells.size();++index) {
                const auto [dx,dy]=kNeighborCells[index];const auto mask=input[index%2 ? 2 : 1];
                Cell(x+dx,y+dy,static_cast<std::uint8_t>((mask&(1U<<(index/2))) ? 'h' : '?'),static_cast<std::uint8_t>(input[9] ? 6 : 5));
            }
        } else {
            CHECK(input.size()==8);map.bounds={50,53,52,55};map.makePredictable=input[6];map.excludedFlags=input[5];
            Cell(50,53,'x');Cell(51,53,'x');Cell(50,54,'x');Cell(51,54,'x');
            if (input[0]==1) Cell(51,53,'?');
            if (input[0]==2) { Cell(50,53,'h');mapping['h']=0; }
            if (input[0]==3) Cell(51,54,'h',6);
            if (input[0]==4) map.bounds[2]=50;
        }
        stamp=std::make_unique<RawTerrainStamp>(pool,types,frames,map,random,crtState,state,Hooks());
    }
};
// 실제 프레임·두난수·99cache·현재raw생성·mapping·외부효과 순서를 전수 대조한다.
void ReplayTerrainStamp(const char* edition) {
    TerrainStampScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 저장된 native결과와 생산모듈의 현재 결과만 비교한다.
    for (const auto& row:StampRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==12);scene.Prepare(row);std::uint32_t result=0;
        if (scene.kind=="Frame") result=static_cast<std::uint32_t>(scene.stamp->FrameAt(static_cast<std::int32_t>(scene.input[3]),static_cast<std::int32_t>(scene.input[4])));
        else scene.stamp->Stamp('x');
        std::string cache,effects,raw;
        if (scene.state.variants) {
            // 현재99cache의 각DWORD를 native의 실제 관찰과 비교한다.
            for (const auto value:*scene.state.variants) { if (!cache.empty()) cache+=',';cache+=std::to_string(value); }
        } else cache="-";
        // 현재 외부 효과의 인자/호출 순서를 같은 문자열로 보존한다.
        for (const auto& event:scene.events) { if (!effects.empty()) effects+=';';effects+=event; }if (effects.empty()) effects="-";
        // 생성한 모든 raw 슬롯의type/frame/owner/region/좌표를 실제 폭으로 읽는다.
        for (std::uint16_t sid=kTerrainBorn;sid<scene.born;++sid) {
            if (!raw.empty()) raw+=';';const auto slot=scene.pool.Slot(Sid{sid});raw+=std::to_string(sid)+':'+std::to_string(slot[10])+':'+
                std::to_string(Get(slot,scene.patch ? 36 : 34,scene.patch ? 4 : 1))+':'+std::to_string(slot[scene.patch ? 34 : 32])+':'+
                std::to_string(Get(slot,8,2))+':'+std::to_string(Get(slot,14))+':'+std::to_string(Get(slot,18));
        }if (raw.empty()) raw="-";
        const auto mapping=std::to_string(scene.mapping['x'])+','+std::to_string(scene.mapping['X'])+','+std::to_string(scene.mapping['=']);
        const bool same=result==Number(row[3]) && scene.random.State()==Number(row[4]) && scene.crtState==Number(row[5]) && cache==row[6] && effects==row[7] && raw==row[8] &&
            scene.map.lastSid.value==Number(row[9]) && mapping==row[10] && scene.assertions==Number(row[11]);
        CHECK(same);++count;
        if (!same) { std::printf("%s TerrainStamp 행 %zu %s %s\n기대 frame/rng/crt %s/%s/%s effects %s raw %s\n실제 %u/%u/%u effects %s raw %s\n",
            edition,count,row[1].c_str(),row[2].c_str(),row[3].c_str(),row[4].c_str(),row[5].c_str(),row[7].c_str(),row[8].c_str(),result,scene.random.State(),scene.crtState,effects.c_str(),raw.c_str());break; }
    }
    CHECK(count==780);
}
}
// 패치의 theme0 초기검색·직접DWORDframe·지역/fringe생성을 독립 원본과 비교한다.
TEST_CASE(TerrainStamp_ReplaysPatch) { ReplayTerrainStamp("originals"); }
// CD의 전체 후보·BYTEframe·실제CRTcache/stamp를 독립 원본과 비교한다.
TEST_CASE(TerrainStamp_ReplaysCd) { ReplayTerrainStamp("originalCD"); }
// 같은 모듈에서 cache가 재사용될 때 CRT가 다시 소비되지 않으며 미연결 하위 효과를 거부한다.
TEST_CASE(TerrainStamp_PreservesCacheAndRejectsMissingEffects) {
    TerrainStampScene scene(OriginalEdition::Patch1078);const auto& row=StampRows().front();scene.Prepare(row);
    scene.stamp->FrameAt(50,53);const auto crt=scene.crtState;const auto cache=*scene.state.variants;scene.stamp->FrameAt(50,53);
    CHECK(scene.crtState==crt);CHECK(*scene.state.variants==cache);
    bool rejected=false;
    try { RawTerrainStamp missing(scene.pool,scene.types,scene.frames,scene.map,scene.random,scene.crtState,scene.state,{}); }
    catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
