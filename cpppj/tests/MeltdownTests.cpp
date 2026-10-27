// 독립 기계어의 요청·피해·가변 패킷 전체를 제품 몸체에 재생한다.
#include "RawSceneSupport.h"
#include "o/RawMeltdown.h"
#include <iostream>

using namespace netstorm::test::rawscene;
namespace { // 명시 입력과 큰 외부 효과만 공급한다.
struct MeltdownScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true};SquidHash hash; // 현재 raw와 실제 검색 공간이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 원문188TYPE 입력이다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); // 보호 프레임 입력이다.
    std::array<std::uint32_t,81> alliances{};MeltdownState state; // 실제 동맹/헤더와 재조회 상태다.
    std::array<std::uint8_t,220> packet{};std::vector<std::uint32_t> c; // 독립 입력과 효과 버퍼다.
    std::uint32_t boss{},local{513},visuals{};std::string trace; // 큰 효과의 재진입 관측이다.
    std::span<std::uint8_t> Raw(std::uint16_t sid) { const auto raw=pool.Slot(Sid{sid});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 실제 슬롯 입력만 쓴다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> args={}) { // 원문 효과 순서를 같은 DWORD 표기로 기록한다.
        if (!trace.empty()) trace+=';';trace+=name;for (const auto value:args) trace+=':'+std::to_string(value); // 전체 인자를 보존한다.
    }
    void Text(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 패킷/음향의 원래 문자열이다.
    void Prepare(std::string_view input) { // 명시 입력만 설치하며 판단/기대값을 계산하지 않는다.
        c.clear();for (const auto& field:Split(input,',')) c.push_back(Number(field)); // 원문 DWORD23개다.
        CHECK(c.size()==23);trace.clear();visuals=0;boss=c[16];local=513;types.assign(188,{});frames.assign(188,RiftTypeFrames({}));alliances.fill(0);state=MeltdownState{};
        state.boss=[this] { return boss; };state.localOwner=[this] { return local; };state.alliances=alliances;state.request.protocol=state.effect.protocol=static_cast<std::uint8_t>(c[20]);state.effect.length=static_cast<std::uint16_t>(c[21]);alliances[c[13]*9+c[14]]=c[15];
        for (std::uint16_t sid=50;sid<160;++sid) { // 전체 입력 후보를 같은 바이트로 초기화한다.
            auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);raw[10]=sid==50 ? 75 : 70;raw[11]=sid==50 ? 0 : static_cast<std::uint8_t>(c[4]);raw[34]=sid==50 ? 2 : static_cast<std::uint8_t>(c[14]);raw[40]=static_cast<std::uint8_t>(c[5]);Put(raw,14,c[8]);Put(raw,18,c[9]);
        }
        if (c[0]==3) Raw(50)[11]=static_cast<std::uint8_t>(c[4]);
        types[75].flags2=0x200;types[75].footX=types[75].footY=3;types[70].flags2=c[1];types[70].footX=std::bit_cast<std::int32_t>(c[6]);types[70].footY=std::bit_cast<std::int32_t>(c[7]);frames[70]=RiftTypeFrames({FrameCode{74,0,1,static_cast<std::uint8_t>(c[3])}});
        packet.fill(255);Put(packet,0,1,1);Put(packet,1,0x75,1);Put(packet,2,4,2);Put(packet,4,c[10]);Put(packet,8,c[11]);Put(packet,12,c[12],1);Put(packet,13,c[13],1);Put(packet,14,50,2);Put(packet,16,c[17]);
        for (std::size_t index=0;index<100;++index) Put(packet,20+index*2,60+static_cast<std::uint32_t>(index),2); // 원문 WORD 목록이다.
        if (c[0]==3) Put(packet,4,50,2);
    }
    MeltdownHooks Hooks() { // 원문과 같은 명시 큰 효과만 기록한다.
        MeltdownHooks hooks;hooks.hasHitPoints=[this](Sid sid) { Event("hp",{sid.value});return c[2]!=0; };hooks.assertion=[this](auto line) { Event("assert",{line}); };
        hooks.send=[this](auto target,std::span<const std::uint8_t> bytes) { Text("send:"+std::to_string(target)+':'+Hex(bytes)); };
        hooks.find=[this](float x,float y,std::int32_t radius,BombTargetList& list) {
            Event("find",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),std::bit_cast<std::uint32_t>(radius)});CHECK(list.capacity==100 && list.count==0);
            for (std::size_t index=0;index<c[17];++index) list.storage[index]=15000+static_cast<std::uint32_t>(index); // 독립 검색 경계가 제공한 번호다.
            list.count=std::bit_cast<std::int32_t>(c[17]);if (c[18]==1) { Put(Raw(50),14,std::bit_cast<std::uint32_t>(80.25F));Raw(50)[34]=7; }
            if (c[18]==3) { state.effect.protocol=0x83;state.effect.length=6; }
        };
        hooks.broadcast=[this](std::span<std::uint8_t> bytes) { Text("broadcast:"+Hex(bytes));if (c[18]==2) { local=769;Put(bytes,14,99,2); } };
        hooks.receive=[this](auto owner,auto connection,std::span<const std::uint8_t> bytes) { Text("receive:"+std::to_string(owner)+':'+std::to_string(connection)+':'+Hex(bytes)); };
        hooks.destroy=[this](Sid sid,auto flags) { Event("destroy",{sid.value,flags}); };
        hooks.damage=[this](Sid sid,auto amount,auto owner,Sid parent) {
            Event("damage",{sid.value,std::bit_cast<std::uint32_t>(amount),owner,parent.value});if (c[18]==1) Put(Raw(60),14,std::bit_cast<std::uint32_t>(200.F));if (c[18]==2) Raw(60)[11]=2;
            if (c[0]==4 && c[18]==3) { Put(packet,16,1);Put(packet,13,7,1); }
        };
        hooks.ground=[this](float x,float y) { Event("ground",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)}); };
        hooks.edge=[this](Sid sid,float x,float y) { Event("edge",{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)}); };
        hooks.visual=[this](float x,float y,auto variant,float interval) {
            Event("visual",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),variant,std::bit_cast<std::uint32_t>(interval)});if (c[18]==1 && ++visuals==1) { Put(packet,12,2,1);Put(packet,4,std::bit_cast<std::uint32_t>(100.F)); }
        };
        hooks.sound=[this](std::string_view name) { Text("sound:"+std::string(name)+":0:0:0:0:0");if (c[18]==2) Put(packet,16,1); };
        return hooks;
    }
};
}
// 두 정밀도의 기계어 결과를 전체 trace/raw/packet과 각각 대조한다.
TEST_CASE(Meltdown_Native442RequestDamageEffectAndWriter) {
    MeltdownScene scene;std::size_t count{},diagnostics{}; // 실패 입력은 최초3행만 별도 표시한다.
    for (const auto& row:LoadFixture(NETSTORM_MELTDOWN_FIXTURE).rows) { // 기존 기대값을 재계산하거나 바꾸지 않는다.
        CHECK(row.size()==9);scene.Prepare(row[0]);RawMeltdown body(scene.pool,scene.hash,scene.types,scene.frames,scene.state,scene.Hooks());std::uint32_t result{};
        const auto op=scene.c[0];
        if (op==0) body.Command(Sid{50},scene.c[19],[&](Sid sid,auto code) { scene.Event("base",{sid.value,code}); });
        else if (op==1) result=body.MayTarget(Sid{60});
        else if (op==2) body.Apply(std::bit_cast<float>(scene.c[10]),std::bit_cast<float>(scene.c[11]),std::bit_cast<std::int32_t>(scene.c[12]),scene.c[13],Sid{60},Sid{50});
        else if (op==3) body.ReceiveRequest(scene.packet);
        else if (op==4) body.ReceiveEffect(scene.packet);
        else RawMeltdown::FinalizePacket(scene.packet,scene.c[22]!=0);
        const std::array<std::string,4> actual{std::to_string(result),scene.trace.empty() ? "-" : scene.trace,Hex(scene.Raw(60)),Hex(scene.packet)};
        for (std::size_t field=0;field<actual.size();++field) { // 기대값 두 정밀도와 제품 전체 출력을 비교한다.
            if (actual[field]!=row[1+field] && diagnostics++<3) std::cout<<"Meltdown 입력 "<<row[0]<<" 필드 "<<field<<"\n실제 "<<actual[field]<<"\n원문 "<<row[1+field]<<'\n';
            CHECK(actual[field]==row[1+field]);CHECK(row[1+field]==row[5+field]);
        }
        ++count;
    }
    CHECK(count==442);
}
// 실제 검색의 엄격한 거리/예약 SID/abstract/dead/한도와 같은 해시 순서를 검사한다.
TEST_CASE(Meltdown_FindStrictBoundarySidBandsAndOutputLimit) {
    MeltdownScene scene;scene.Prepare("0,512,1,0,0,0,3,3,1112014848,1112014848,1111621632,1111621632,4,2,1,0,1,3,0,50331896,1,0,1");
    const std::array<std::uint16_t,8> ids{1,4,5,14999,15000,15001,15002,15003}; // 예약/서버 경계와 거리의 명시 입력이다.
    scene.types[70].footX=scene.types[70].footY=2;
    const std::array<float,8> x{50,50,50,50,53,52.999996F,50,50},y{50,50,50,50,54,54,50,50}; // 거리5의 정확한 경계와 안쪽 입력이다.
    for (std::size_t index=0;index<ids.size();++index) { // 실제 해시에 정해진 후보만 등록한다.
        auto raw=scene.Raw(ids[index]);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=70;Put(raw,14,std::bit_cast<std::uint32_t>(x[index]+1.F));Put(raw,18,std::bit_cast<std::uint32_t>(y[index]+1.F));if (index==6) raw[40]=1;if (index==7) raw[11]=2;
        auto& head=scene.hash.Bucket(1,x[index]+1.F,y[index]+1.F);Put(raw,4,head,2);head=ids[index];
    }
    RawMeltdown body(scene.pool,scene.hash,scene.types,scene.frames,scene.state,scene.Hooks());std::array<std::uint32_t,100> storage{};BombTargetList result{storage,100,0};body.Find(50,50,4,result);
    std::vector<std::uint32_t> found(storage.begin(),storage.begin()+result.count);std::sort(found.begin(),found.end());CHECK((found==std::vector<std::uint32_t>{1,4,15001,15003}));
    result.capacity=1;result.count=0;body.Find(50,50,4,result);CHECK(result.count==1);
}
