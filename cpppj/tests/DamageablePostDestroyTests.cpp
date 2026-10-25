// 삭제후실제부모몸체의독립원본기록을현재HP/중심/finder/다리검색과대조한다.
#include "RawSceneSupport.h"
#include "o/RawDamageablePostDestroy.h"
#include <sstream>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본격자에서관찰한부모/주변후보와합성가상표시경계의기록값이다.
constexpr std::array<std::uint16_t,7> kPostIds{50,60,61,62,63,64,65};
constexpr std::uint32_t kPostVtable=0x11307000;
// 다음행의free/dead값이할당API를막지않도록검사입력만전체슬롯에직접쓴다.
std::span<std::uint8_t> PostInputSlot(SidPool& pool,std::uint16_t sid) {
    const auto raw=pool.Slot(Sid{sid});return {const_cast<std::uint8_t*>(raw.data()),raw.size()};
}
// 원본fixture의효과/전체raw저장소를계산없이문자열로보존하는장면이다.
struct DamageablePostReplayScene {
    SidPool pool; // 실제부모/다리/HP후보의단일raw저장소다.
    SquidHash hash; // 실제4단계버킷과각현재raw next를읽는다.
    std::vector<RiftTypeRecord> types; // 명시원본입력의타입metadata다.
    std::vector<RiftTypeFrames> frames; // 금간검색에쓰는원본과같은4개합성코드다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 기존중심어댑터의필수공간참조다.
    SquidPostPopState bookkeeping; // HP기존몸체에같은장부를공급하며이격자에서는비용/SP를바꾸지않는다.
    SquidRewardState hpState; // 실제MaxHP의현재특수타입/사제quarter전역이다.
    GameRandom random{12345}; // HP몸체의SP저장소가빌리는전역이며이격자에서는소비하지않는다.
    ScrambledSpStore store; // 패치HP처리기의필수SP참조이며Initialize는요청하지않는다.
    DamageablePostDestroyState state; // 각원본전역과판본별제외타입기록이다.
    std::vector<std::string> events; // 명시하위효과의원본순서기록이다.
    std::array<std::uint32_t,18> input{}; // 고정fixture입력DWORD이며기대출력을포함하지않는다.
    std::unique_ptr<SquidReward> hp; // 완료된HasHitPoints/MaxHitPoints를직접호출한다.
    std::unique_ptr<RawSquidNeighbors> footprint; // 완료된현재발자국중심을직접호출한다.
    std::unique_ptr<RawDamageablePostDestroy> post; // 위자료를같은풀에서참조하는부모몸체다.
    bool patch; // frame/owner/extra폭의판본구분이다.
    // 필요한원본자료와실제풀번호5..127을초기화한다.
    explicit DamageablePostReplayScene(OriginalEdition edition):pool(edition,32768,false),
        types(edition==OriginalEdition::Patch1078 ? 188U : 171U),frames(types.size(),RiftTypeFrames({})),
        store(random,[] { return 12345U; }),patch(edition==OriginalEdition::Patch1078) {
        // 원본분석장면의대상을실제할당영역으로준비한다.
        for (std::uint16_t sid=5;sid<=127;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // 현재판본에서읽거나쓰는필드의정확한폭이다.
    std::size_t Extra() const { return patch ? 40U : 35U; }
    // 현재소유자BYTE의원본위치다.
    std::size_t Owner() const { return patch ? 34U : 32U; }
    // 현재프레임필드의원본위치다.
    std::size_t Frame() const { return patch ? 36U : 34U; }
    // 패치는DWORD이고CD는BYTE인프레임필드폭이다.
    std::size_t FrameWidth() const { return patch ? 4U : 1U; }
    // opcode와같은입력raw/버킷을구성하며HP/finder결과를미리만들지않는다.
    void Prepare(const std::array<std::uint32_t,18>& values) {
        input=values;hash.Reset();events.clear();post.reset();hp.reset();footprint.reset();
        std::fill(types.begin(),types.end(),RiftTypeRecord{});
        // 프레임은같은실제코드표순서로금간첫검색/단단한판정을공급한다.
        for (auto& table:frames) table=RiftTypeFrames({{'A','P',1,0},{'A','P',11,32},{'A','P',21,64},{'A','P',11,96}});
        // 각원본후보의좌표/상태/footprint/해시단계를입력순서로설정한다.
        for (const auto sid:kPostIds) {
            const auto type=sid==50 ? input[4] : (sid<62 ? 90U : (sid==62 ? 92U : (sid<65 ? 82U : 94U)));
            auto raw=PostInputSlot(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kPostVtable);
            raw[10]=static_cast<std::uint8_t>(type);raw[11]=sid==50 ? 2 : 0;raw[Extra()]=static_cast<std::uint8_t>(sid==50 ? input[3] : (sid==61 ? 1U : 0U));
            raw[Owner()]=2;Put(raw,12,0x7355,2);
            const auto x=sid==50 ? std::bit_cast<float>(input[16]) : (sid==60 ? 49.25F : (sid==61 ? 49.5F : (sid==63 ? 49.0F : (sid==65 ? 50.5F : 50.0F))));
            const auto y=sid==50 ? std::bit_cast<float>(input[17]) : (sid==60 ? 49.25F : (sid==61 ? 49.5F : (sid==63 || sid==64 ? 49.0F : (sid==65 ? 50.5F : 50.0F))));
            Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));
            Put(raw,Frame(),sid==63 ? input[11]%4 : (sid==64 ? 1U : 0U),FrameWidth());
            auto& record=types[type];record.flags1=type==90 ? 0x10U : 0U;record.flags2=type==82 ? 4U : (type==94 ? 0x1000000U : 0U);
            record.maxHitPoints=100;record.group=0;record.footX=sid==50 ? static_cast<std::int32_t>(input[9]) : (sid==65 ? 3 : 1);
            record.footY=sid==50 ? static_cast<std::int32_t>(input[10]) : (sid==65 ? 3 : 1);
            if (sid!=50) { const auto level=(sid==63 || sid==64) ? 0 : 1;auto& bucket=hash.Bucket(level,x,y);Put(raw,4,bucket,2);bucket=sid; }
        }
        auto& source=types[input[4]];source.flags1=input[5];source.flags2=input[6];source.group=std::bit_cast<std::int32_t>(input[7]);source.maxHitPoints=std::bit_cast<std::int32_t>(input[8]);
        state.battle=input[1]!=0;state.boss=input[2]!=0;state.dudeCount=std::bit_cast<std::int32_t>(input[12]);state.damageEvent=input[13];state.bridgeCrackLife=input[14];
        hp=std::make_unique<SquidReward>(pool,types,bookkeeping,hpState,patch ? &store : nullptr);
        footprint=std::make_unique<RawSquidNeighbors>(pool,hash,spots,types,frames);
        post=std::make_unique<RawDamageablePostDestroy>(pool,hash,types,frames,*hp,*footprint,state,Hooks());
    }
    // 이격자는외부몸체의실행완료를주장하지않고필수원본경계의호출/인자/지정변경만대조한다.
    DamageablePostDestroyHooks Hooks() {
        DamageablePostDestroyHooks hooks;
        hooks.dropDude=[this](Sid sid,std::int32_t index,bool randomPoint) {
            events.push_back("D:"+std::to_string(sid.value)+":"+std::to_string(index)+":"+std::to_string(randomPoint));
            if (input[15]==1) PostInputSlot(pool,50)[10]=91;
            if (input[15]==6) PostInputSlot(pool,50)[Owner()]=3;
        };
        hooks.dropIsland=[this](float x,float y,std::uint32_t flags,std::uint32_t owner) {
            events.push_back("I:"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y))+":"+std::to_string(flags)+":"+std::to_string(owner));
            if (input[15]==2) PostInputSlot(pool,50)[Extra()]=1;
        };
        hooks.regular=[this](Sid sid,std::uint32_t event,float payload,std::uint32_t count) {
            events.push_back("R:"+std::to_string(sid.value)+":"+std::to_string(event)+":"+std::to_string(std::bit_cast<std::uint32_t>(payload))+":"+std::to_string(count));
            if (input[15]==4) PostInputSlot(pool,61)[11]=2;
        };
        hooks.hasBridgeFall=[this](Sid sid) { events.push_back("F:"+std::to_string(sid.value));return input[11]==3; };
        hooks.notifySurface=[this](Sid sid) { events.push_back("N:"+std::to_string(sid.value));if (input[15]==3) Put(PostInputSlot(pool,sid.value),Frame(),2,FrameWidth()); };
        hooks.displayUpdate=[this](Sid sid) {
            events.push_back("U:"+std::to_string(sid.value)+":"+std::to_string(Get(PostInputSlot(pool,sid.value),Frame(),FrameWidth())));
            if (input[15]==5) Put(PostInputSlot(pool,sid.value),12,0xfedc,2);
        };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { events.push_back("X:"+std::to_string(sid.value)+":"+std::to_string(flags)); };
        hooks.base=[this](Sid sid,std::uint32_t flags) { events.push_back("B:"+std::to_string(sid.value)+":"+std::to_string(flags)); };
        return hooks;
    }
    // 관찰한모든경계와현재raw후보를원본행형식그대로직렬화한다.
    std::string Events() const {
        std::string value;
        // 모든경계요청은누락없이원본발생순서로연결한다.
        for (const auto& event:events) { if (!value.empty()) value+=';';value+=event; }return value.empty() ? "-" : value;
    }
    // 관찰한전체후보의원본stride바이트와SID를같이저장한다.
    std::string Raw() const {
        std::string value;
        // 원본행의고정SID순서를바꾸거나바뀌지않은필드를생략하지않는다.
        for (const auto sid:kPostIds) { if (!value.empty()) value+=';';value+=std::to_string(sid)+":"+Hex(pool.Slot(Sid{sid})); }return value;
    }
};
}

// 정상4412입력의전체raw·현재진단/경계순서를두원본판본모두대조한다.
TEST_CASE(DamageablePostDestroy_ReplaysBothNativeFullEffects) {
    const auto fixture=LoadFixture(NETSTORM_DAMAGEABLEPOSTDESTROY_FIXTURE);std::size_t checked=0;
    // 고유원본행만재생하며두x87관찰이나기존helper진입은추가완료로세지않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==6);const auto fields=Split(row[2],',');CHECK(fields.size()==18);std::array<std::uint32_t,18> input{};
        // 지정한원본DWORD입력만파싱하고기대출력을재계산하지않는다.
        for (std::size_t index=0;index<input.size();++index) input[index]=Number(fields[index]);
        DamageablePostReplayScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);scene.Prepare(input);
        if (row[1]=="Post") scene.post->PostDestroy(Sid{50},input[0]);else scene.post->Drops(Sid{50},input[6],input[0]!=0);
        const bool same=scene.Events()==row[3] && scene.Raw()==row[4] && row[5]=="0";CHECK(same);
        if (!same) { std::fprintf(stderr,"DamageablePost %s %s row%zu input%s\nactual%s\nexpect%s\n",row[0].c_str(),row[1].c_str(),checked,row[2].c_str(),scene.Events().c_str(),row[3].c_str());break; }
        CHECK(&scene.post->Pool()==&scene.pool && scene.random.State()==12345 && !scene.store.Initialized());++checked;
    }
    CHECK(checked==fixture.rows.size());
}

// count의낮은WORD를mask하지않고실제ADD의carry와payload0을확인한다.
TEST_CASE(DamageablePostDestroy_PackedCountAndRequiredCreationBoundary) {
    DamageablePostReplayScene scene(OriginalEdition::Patch1078);
    std::array<std::uint32_t,18> input{0x110000,1,1,0,89,0x10,0,0,131073,3,3,0,0,6699,5,0,
        std::bit_cast<std::uint32_t>(50.5F),std::bit_cast<std::uint32_t>(50.5F)};scene.Prepare(input);
    scene.post->PostDestroy(Sid{50},input[0]);bool scheduled=false;
    // 원본count는owner1*65536+65537=131073이며payload는0으로유지된다.
    for (const auto& event:scene.events) if (event=="R:60:6699:0:131073") scheduled=true;
    CHECK(scheduled);
    auto hooks=scene.Hooks();hooks.dropDude={};CHECK(Throws([&] { RawDamageablePostDestroy invalid(scene.pool,scene.hash,scene.types,scene.frames,*scene.hp,*scene.footprint,scene.state,hooks); }));
}

// 첫 다리 약화의 표면 통지가 권한을 바꾸면 뒤의 금 간 다리 삭제는 현재 권한을 따른다.
TEST_CASE(DamageablePostDestroy_SharedBossRecheckedBetweenBridges) {
    // 두 판본의 실제 finder와 프레임 약화 몸체로 같은 순서의 후보를 읽는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        DamageablePostReplayScene scene(edition);scene.Prepare({0,1,1,0,90,0x10,0,2,100,1,1,0,0,6699,5,0,std::bit_cast<std::uint32_t>(50.F),std::bit_cast<std::uint32_t>(50.F)});bool boss=true;scene.state.sharedBoss=&boss;
        CHECK(scene.hash.Bucket(0,49,49)==63 && scene.hash.Bucket(0,50,49)==64 && Get(scene.pool.Slot(Sid{63}),4,2)==0 && Get(scene.pool.Slot(Sid{64}),4,2)==0); // 두 좌표의 원래 독립 버킷에 같은 SID를 중복 연결하지 않는다.
        auto hooks=scene.Hooks();const auto notify=hooks.notifySurface;std::size_t notified{},destroyed{};hooks.notifySurface=[&](Sid sid) { notify(sid);++notified;boss=false; };hooks.destroy=[&](Sid,std::uint32_t) { ++destroyed; };RawDamageablePostDestroy post(scene.pool,scene.hash,scene.types,scene.frames,*scene.hp,*scene.footprint,scene.state,std::move(hooks));post.PostDestroy(Sid{50},0);CHECK(notified==1 && destroyed==0 && !boss);
        const auto firstFrame=Get(scene.pool.Slot(Sid{63}),scene.Frame(),scene.FrameWidth());CHECK(firstFrame==1);boss=true;scene.state.boss=false;post.PostDestroy(Sid{50},0);CHECK(destroyed==2); // 이미 약화된 두 다리는 현재 boss가 모두 삭제한다.
        if (destroyed!=2) std::printf("공유 권한 다리 입력: 판본%d, 첫 프레임%u, 최종 프레임%u/%u, 통지%zu, 삭제%zu, boss%d\n",static_cast<int>(edition),firstFrame,Get(scene.pool.Slot(Sid{63}),scene.Frame(),scene.FrameWidth()),Get(scene.pool.Slot(Sid{64}),scene.Frame(),scene.FrameWidth()),notified,destroyed,boss); // 실패한 신규 입력의 실제 프레임과 효과 횟수를 보존한다.
    }
}
