// 채집 handler의 실제 두 PE 결과를 현재 raw 수량/난수/표면과 효과 순서로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGeyserMined.h"
#include "o/RawPacketDispatch.h"
#include "o/RawSquidDestroy.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// fixture 16진수 이미지의 byte를 같은 raw 폭으로 읽는다.
std::vector<std::uint8_t> MinedBytes(const std::string& text) {
    std::vector<std::uint8_t> result;
    // 두 hex 글자마다 한 byte를 만든다.
    for (std::size_t i=0;i<text.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
// 실제 native 효과 요청 사이에 동일한 구분자를 둔다.
std::string MinedEvents(const std::vector<std::string>& events) {
    std::string text;
    // 발생 순서대로 각 효과 인자를 붙인다.
    for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
    return text.empty() ? "-" : text;
}
}

// free 자연 반환·WORD 감김·현재cost/권한·난수·callback 뒤 표면 변경을 두 PE와 대조한다.
TEST_CASE(rawGeyserMined_native_word_random_and_empty_spawn) {
    const auto fixture=LoadFixture(NETSTORM_GEYSERMINED_FIXTURE);std::size_t checked=0;
    // 각 입력을 새 같은풀/hash/난수 위에서 실행한다. 기대 수량이나 분기는 계산하지 않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==6);const auto input=Split(row[1],',');CHECK(input.size()==7);const auto quantity=Number(input[0]),cost=Number(input[1]),seed=Number(input[2]),free=Number(input[3]),boss=Number(input[4]),surface=Number(input[5]),mutation=Number(input[6]);
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;SidPool pool(edition,32768,true);SquidHash hash;
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        // 원본 격리 입력 번호5..8을 실제 client free list로 확보한다.
        for (std::uint16_t sid=5;sid<=8;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        auto unused=pool.AllocatedBytes(Sid{7});std::fill(unused.begin(),unused.end(),std::uint8_t{});
        auto raw=pool.AllocatedBytes(Sid{6});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=122;raw[11]=static_cast<std::uint8_t>(free);Put(raw,12,quantity,2);Put(raw,14,std::bit_cast<std::uint32_t>(40.75F));Put(raw,18,std::bit_cast<std::uint32_t>(41.25F));
        types[154].cost=std::bit_cast<float>(cost);pool.AllocatedBytes(Sid{8})[10]=static_cast<std::uint8_t>(surface);hash.Cell(0,40,41)=surface ? 8 : 0;GameRandom random(seed);GeyserMinedState state;state.boss=boss!=0;std::vector<std::string> events;
        GeyserMinedHooks hooks;
        hooks.sound=[&](float x,float y,std::string_view name,std::uint32_t flags) {
            CHECK(name.starts_with("geyserMined") && name.ends_with(".wav") && flags==0);const auto choice=std::stoul(std::string(name.substr(11,1)));
            events.push_back("S:"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(choice)+','+std::to_string(flags));
            if (mutation==1) { Put(raw,12,400,2);state.nuggetType=155;types[155].cost=199.5F;state.boss=true; }
            else if (mutation==2) { Put(raw,14,std::bit_cast<std::uint32_t>(42.25F));Put(raw,18,std::bit_cast<std::uint32_t>(43.75F));Put(raw,12,200,2); }
            else if (mutation>=4 && mutation<=9) {
                const float cellX=mutation==4 ? -0.25F : mutation==5 ? 256.0F : mutation==6 ? std::bit_cast<float>(0x7fc00000U) : mutation==7 ? 4294967296.0F : mutation==8 ? -4294967296.0F : std::bit_cast<float>(0x7f800000U);
                const float cellY=mutation==4 ? 255.75F : mutation<=6 ? 41.25F : 0.0F;Put(raw,14,std::bit_cast<std::uint32_t>(cellX));Put(raw,18,std::bit_cast<std::uint32_t>(cellY));Put(raw,12,200,2);
            }
            else if (mutation>=10) { raw[10]=82;raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=mutation>=12 ? 8 : 0;state.checking=mutation==11 || mutation==12; }
        };
        hooks.destroy=[&](Sid sid,std::uint32_t flags) {
            CHECK(sid==Sid{6} && flags==0x1000);events.push_back("D:"+std::to_string(flags));raw[11]|=2;
            if (mutation==3) {
                const int x=static_cast<int>(std::bit_cast<float>(Get(raw,14))),y=static_cast<int>(std::bit_cast<float>(Get(raw,18)));
                hash.Cell(0,x,y)=surface==97 ? 0 : 8;pool.AllocatedBytes(Sid{8})[10]=97;
            }
        };
        hooks.create=[&](std::uint32_t type,std::uint32_t flags) {
            CHECK(type==84 && flags==0);events.push_back("C:"+std::to_string(type)+','+std::to_string(flags));auto born=pool.AllocatedBytes(Sid{7});std::fill(born.begin(),born.end(),std::uint8_t{});born[10]=static_cast<std::uint8_t>(type);born[11]=4;return Sid{7};
        };
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) {
            CHECK(sid==Sid{7} && flags==0);events.push_back("P:"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(flags));auto born=pool.AllocatedBytes(sid);Put(born,14,std::bit_cast<std::uint32_t>(x));Put(born,18,std::bit_cast<std::uint32_t>(y));born[11]=0;
        };
        if (mutation>=4 && mutation<=9) {
            const int x=mutation==5 ? 40 : 0,y=mutation==4 ? 255 : mutation<=6 ? 41 : 0;hash.Cell(0,x,y)=surface ? 8 : 0;
        }
        RawGeyserMined handler(pool,hash,types,random,state,std::move(hooks));const std::array<std::uint8_t,6> packet{0xab,0xcd,0xef,0xff,6,0};
        if (row[3].ends_with(";A")) { CHECK(Throws([&] { handler.HandlePacket(packet); }));events.push_back("A"); }
        else handler.HandlePacket(packet);
        CHECK(random.State()==Number(row[2]) && MinedEvents(events)==row[3]);const auto expected=MinedBytes(row[4]);CHECK(expected.size()==raw.size());
        // 원본에서 가상 표만 격리 주소로 주입했으므로 그것을 제외한 모든 raw byte를 비교한다.
        CHECK(std::equal(raw.begin()+4,raw.end(),expected.begin()+4));const auto born=pool.Slot(Sid{7});const auto expectedBorn=MinedBytes(row[5]);CHECK(expectedBorn.size()==born.size());CHECK(std::equal(born.begin()+4,born.end(),expectedBorn.begin()+4));++checked;
    }
    CHECK(checked>16);
}

// 원본 등록/단일 분배부터 같은풀의 실제 factory/공통가상삭제/Pop과 현재표면까지 고갈 흐름을 연결한다.
TEST_CASE(rawGeyserMined_real_packet_factory_destroy_and_space) {
    // 두 판본의 수량WORD와 서버생성/공간 체인을 같은 순서로 실행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,32768,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        types[122].footX=types[122].footY=types[84].footX=types[84].footY=1;types[154].cost=200;
        SquidFactory factory(pool,types);SquidPop pop(pool,hash,spots);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
        const auto geyser=factory.Create(122,2);Put(pool.AllocatedBytes(geyser),12,200,2);CHECK(pop.Pop(geyser,types[122],1,1,40.75F,41.25F,0)==RawPopResult::Registered);
        GameRandom random;GeyserMinedState state;std::uint32_t sounds=0,created=0,recorded=0;Sid born;GeyserMinedHooks hooks;PacketConnection connection;connection.sender=17;
        hooks.sound=[&](float,float,std::string_view,std::uint32_t flags) { CHECK(flags==0 && connection.depth==1 && recorded==1);++sounds; };
        hooks.destroy=[&](Sid sid,std::uint32_t flags) {
            CHECK(sid==geyser && flags==0x1000);SquidDestroyHooks effects;effects.selected=[] { return Sid{}; };
            effects.emit=[&](const SquidDestroyEvent& event) {
                if (event.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();
                else if (event.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy();
                else throw std::logic_error("공통채집삭제 검사에서 다른파생효과는 미실행 경계");
            };
            destroy.Destroy(sid,flags,effects);
        };
        hooks.create=[&](std::uint32_t type,std::uint32_t flags) { CHECK(type==84 && flags==0);born=factory.Create(type,flags);++created;return born; };
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(sid==born && flags==0);CHECK(pop.Pop(sid,types[84],1,1,x,y,flags)==RawPopResult::Registered); };
        RawGeyserMined handler(pool,hash,types,random,state,std::move(hooks));PacketDispatchHooks packetHooks;
        packetHooks.now=[] { return 723U; };
        packetHooks.record=[&](PacketConnection& current,std::span<const std::uint8_t> packet,std::uint16_t length) {
            CHECK(&current==&connection && current.depth==0 && current.received==723 && length==6 && packet[1]==0x5b);++recorded;
        };
        packetHooks.terminate=[](PacketConnection&) { throw std::logic_error("간헐천 채집의 mismatch0은 버전 종료를 요청하지 않는다"); };
        packetHooks.close=[](PacketConnection&) { throw std::logic_error("간헐천 채집은 이 연결을 닫지 않는다"); };
        packetHooks.release=[](PacketConnection&) { throw std::logic_error("간헐천 채집은 이 연결을 해제하지 않는다"); };
        RawPacketDispatch dispatch(edition,std::move(packetHooks));RegisterGeyserMined(dispatch,handler);
        CHECK(&handler.Pool()==&pool);const auto& registration=dispatch.Registration(0x5b);CHECK(registration.length==6 && registration.version==1 && registration.mismatch==0 && !registration.observer && registration.receive);
        const std::array<std::uint8_t,6> packet{0xff,0x5b,6,0,static_cast<std::uint8_t>(geyser.value),static_cast<std::uint8_t>(geyser.value>>8)};
        CHECK(dispatch.Validate(packet) && dispatch.Dispatch(connection,packet));CHECK(connection.depth==0 && connection.received==723 && recorded==1);
        CHECK(sounds==1 && created==1 && born.value>=pool.Layout().serverFirst);CHECK(pool.Slot(geyser)[11]&1);CHECK(hash.Bucket(1,40.75F,41.25F)==born.value);
        CHECK(Get(pool.Slot(born),14)==std::bit_cast<std::uint32_t>(40.75F) && Get(pool.Slot(born),18)==std::bit_cast<std::uint32_t>(41.25F));
        CHECK(Throws([&] { handler.HandlePacket({}); }));CHECK(Throws([&] { RawGeyserMined missing(pool,hash,types,random,state,{}); }));
    }
}

// 채집 소리 효과 뒤의 현재 boss로 고갈 삭제와 Empty 생성 여부를 정한다.
TEST_CASE(rawGeyserMined_shared_boss_rechecked_after_sound) {
    // 두 판본의 실제 수량 WORD 감소와 finder를 외부 효과 관측에 연결한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,32768,true);SquidHash hash;std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);types[154].cost=1;
        // 원본 분석 장면의 client 번호를 실제 풀에서 확보한다.
        for (std::uint16_t sid=5;sid<=8;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        auto raw=pool.AllocatedBytes(Sid{6});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=122;Put(raw,12,1,2);Put(raw,14,std::bit_cast<std::uint32_t>(40.75F));Put(raw,18,std::bit_cast<std::uint32_t>(41.25F));bool boss=true,nextBoss=false;GeyserMinedState state;state.sharedBoss=&boss;GameRandom random(123);std::size_t destroyed{},created{},popped{};GeyserMinedHooks hooks;hooks.sound=[&](float,float,std::string_view,std::uint32_t) { boss=nextBoss; };hooks.destroy=[&](Sid sid,std::uint32_t flags) { CHECK(sid==Sid{6} && flags==0x1000);++destroyed;raw[11]|=2; };hooks.create=[&](std::uint32_t type,std::uint32_t flags) { CHECK(type==84 && flags==0);++created;return Sid{7}; };hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(sid==Sid{7} && x==40.75F && y==41.25F && flags==0);++popped; };RawGeyserMined mined(pool,hash,types,random,state,std::move(hooks));mined.Handle(Sid{6});CHECK(Get(raw,12,2)==0 && destroyed==0 && created==0 && popped==0);Put(raw,12,1,2);nextBoss=true;state.boss=false;mined.Handle(Sid{6});CHECK(Get(raw,12,2)==0 && destroyed==1 && created==1 && popped==1);
    }
}
