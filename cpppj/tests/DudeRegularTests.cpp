// 제품 전 원본1597개 정기/하위 입력과3개 정적 초기화 입력을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDudeRegular.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 명시 큰 효과 경계는 native와 같은 인자로 기록하며 나머지 분기/WORD/프레임/RNG는 실제 모듈을 쓴다.
TEST_CASE(DudeRegular_Native1600) {
    SidPool pool(OriginalEdition::Patch1078,24000,false);std::size_t rows{},initializers{};
    // 원문에 쓰인 현재 부모/목표 번호를 실제 client 목록에서 확보한다.
    for (std::uint16_t number=5;number<=53;++number) CHECK(pool.Allocate(2)==Sid{number});
    std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));frames[70]=RiftTypeFrames({{'A','P',0,0},{'A','P',1,0},{'A','P',2,0},{'B','P',0,0},{'B','P',1,0},{'I','P',0,0}});
    const auto slot=pool.Slot(Sid{50});std::span<std::uint8_t> raw(const_cast<std::uint8_t*>(slot.data()),slot.size());
    // 최초1600행과53/64비트 결과를 빠짐없이 비교한다. C++ 실행은 통합 검증 시 수행한다.
    for (const auto& row:LoadFixture(NETSTORM_DUDE_REGULAR_FIXTURE).rows) {
        CHECK(row.size()==3);if (row.size()!=3) return;CHECK(row[1]==row[2]);const auto fields=Split(row[0],',');CHECK(fields.size()==17);if (fields.size()!=17) return;std::array<std::uint32_t,17> input{};
        // unsigned 원문 비트를 보존하고 signed count는 실제 호출 때만 해석한다.
        for (std::size_t index=0;index<input.size();++index) input[index]=Number(fields[index]);const auto expected=Split(row[1],'|');CHECK(expected.size()==6);if (expected.size()!=6) return;
        if (input[0]==6) {
            const auto native=Split(expected[5],',');CHECK(native.size()==4);if (native.size()!=4) return;SquidPostPopList population;InitializeDudePopulation(population);
            CHECK(Number(native[0])==0x508c04U && Number(native[1])==1);CHECK(population.entries.size()==Number(native[2]) && population.count==Number(native[3]));++initializers;++rows;continue; //registry/atexit 토큰이나 미사용 heap을 호스트 바이트 복제로 표시하지 않는다.
        }
        std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x508cc8);raw[10]=70;raw[11]=static_cast<std::uint8_t>(input[4]);Put(raw,12,0x5102,2);Put(raw,14,std::bit_cast<std::uint32_t>(20.75F));Put(raw,18,input[16]);raw[34]=2;Put(raw,36,input[13]);
        // 목표 진단은 일반 목표 이동 wrapper의 원본 입력이며 free/dead에서도 raw 읽기를 허용한다.
        for (std::uint16_t number=51;number<=53;++number) { const auto saved=pool.Slot(Sid{number});std::span<std::uint8_t> target(const_cast<std::uint8_t*>(saved.data()),saved.size());std::fill(target.begin(),target.end(),std::uint8_t{});target[10]=71;target[11]=static_cast<std::uint8_t>(input[15]); }
        GameRandom random(input[12]);SquidPostPopList providers;providers.entries={51,52,53};providers.count=input[10];auto providerIndex=std::bit_cast<std::int32_t>(input[11]);bool checking=input[5]!=0;std::string trace;
        const auto event=[&](std::string name,std::initializer_list<std::uint32_t> values) {
            if (!trace.empty()) trace+=';';trace+=name;
            // 명시 경계의 전체 DWORD 인자를 순서대로 보존한다.
            for (const auto value:values) trace+=':'+std::to_string(value);
        };
        DudeRegularHooks hooks;
        hooks.checkSpace=[&](Sid sid) { event("space",{sid.value});return input[7]!=0; };
        hooks.findPath=[&](Sid sid,std::uint32_t type) { event("lookup",{sid.value,type,0,0});return Sid{static_cast<std::uint16_t>(input[6] ? 52 : 0)}; };
        hooks.task=[&](Sid sid,std::uint32_t task,float delay) { event("task",{sid.value,task,std::bit_cast<std::uint32_t>(delay)});Put(raw,12,task,2); };
        hooks.nearestResidence=[&](Sid sid) { const auto current=pool.Slot(sid);event("nearest",{Get(current,14),Get(current,18),0x59a9d8,current[34]});return Sid{static_cast<std::uint16_t>(input[8] ? 51 : 0)}; }; //59a9d8은 원문 목록 토큰이며 호스트 주소로 사용하지 않는다.
        hooks.entrance=[&](Sid sid) { event("point",{sid.value});return RawPathPoint{51.25F,52.75F}; };
        hooks.move=[&](Sid sid,RawPathPoint point,bool quickly) { event("move",{sid.value,std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y),static_cast<std::uint32_t>(quickly)});return input[9]!=0; };
        hooks.setFrame=[&](Sid sid,std::int32_t frame,std::uint32_t flags) { event("frame",{sid.value,std::bit_cast<std::uint32_t>(frame),flags});Put(raw,36,std::bit_cast<std::uint32_t>(frame)); };
        hooks.destroy=[&](Sid sid,std::uint32_t flags) { event("destroy",{sid.value,flags});raw[11]|=2; };
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { event("unpop",{sid.value,flags});raw[11]|=4;if (input[14]) { Put(raw,14,std::bit_cast<std::uint32_t>(42.5F));Put(raw,18,std::bit_cast<std::uint32_t>(43.5F));Put(raw,36,3); } };
        hooks.pop=[&](Sid sid,RawPathPoint point,std::uint32_t flags) { event("pop",{sid.value,std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y),flags});Put(raw,14,std::bit_cast<std::uint32_t>(point.x));Put(raw,18,std::bit_cast<std::uint32_t>(point.y));raw[11]&=static_cast<std::uint8_t>(~4U); };
        hooks.assertion=[&](std::uint32_t line) { event("assert",{line}); };
        RawDudeRegular body(pool,frames,random,{checking,providers,providerIndex},std::move(hooks));const auto count=std::bit_cast<std::int32_t>(input[2]);std::uint32_t result{};
        switch (input[0]) {
        case 0:result=std::bit_cast<std::uint32_t>(body.Handle(Sid{50},input[1],input[2],std::bit_cast<float>(input[3])));break;
        case 1:result=std::bit_cast<std::uint32_t>(body.Wander(Sid{50},count,input[1]!=0));break;
        case 2:result=std::bit_cast<std::uint32_t>(body.Return(Sid{50},Sid{static_cast<std::uint16_t>(input[6] ? 52 : 0)},count));break;
        case 3:result=std::bit_cast<std::uint32_t>(body.Drop(Sid{50},Sid{static_cast<std::uint16_t>(input[6] ? 52 : 0)},count));break;
        case 4:result=std::bit_cast<std::uint32_t>(body.Fall(Sid{50},count));break;
        case 5:result=static_cast<std::uint32_t>(body.MoveTo(Sid{50},Sid{static_cast<std::uint16_t>(input[8] ? 51 : 0)},input[1]!=0));break;
        default:CHECK(false);return;
        }
        const auto observed=std::to_string(result)+'|'+(trace.empty() ? std::string("-") : trace)+'|'+Hex(raw)+'|'+std::to_string(random.State())+'|'+std::to_string(std::bit_cast<std::uint32_t>(providerIndex))+"|-";
        CHECK(observed==row[1]);++rows;
    }
    CHECK(rows==1600 && initializers==3);
}
