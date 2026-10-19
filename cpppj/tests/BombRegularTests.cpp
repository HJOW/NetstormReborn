// 제품과독립인PE741입력의시간·대상 패킷·재진입·전체raw를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBombRegular.h"
#include <filesystem>
#include <iostream>

using namespace netstorm::test::rawscene;
namespace {
// 원본 실행기의명시 하위 입력만공급하고시간/패킷 판단은제품이실행한다.
struct BombScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 동일부모50의물리raw다.
    std::vector<BombRegularType> types=std::vector<BombRegularType>(188); // 원본TYPE의두필드 입력이다.
    BombRegularState state; // 하위효과가바꾸는현재전역이다.
    std::vector<std::uint32_t> input; // 동결된13개DWORD 입력이다.
    std::string trace; // 실제하위호출순서와인자다.
    // 검증입력/명시효과만물리슬롯에기록한다.
    std::span<std::uint8_t> Raw() { const auto raw=pool.Slot(Sid{50});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 효과인자의DWORD비트와순서를원문표현으로기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 각원본인자의폭을보존한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 원문입력만설치하며기대시간/패킷/반환값을계산하지않는다.
    void Prepare(std::string_view text) {
        input.clear();trace.clear();
        // 입력13칸을signed변환없이DWORD로읽는다.
        for (const auto& value:Split(text,',')) input.push_back(Number(value));
        CHECK(input.size()==13);auto raw=Raw();std::fill(raw.begin(),raw.end(),std::uint8_t{});
        raw[10]=168;raw[11]=static_cast<std::uint8_t>(input[6]);Put(raw,12,input[4],2);
        types[168]={std::bit_cast<std::int32_t>(input[12]),std::bit_cast<std::int32_t>(input[3])};types[169]={99,1};
        state.boss=input[5];state.localOwner=input[11];state.effect={0x5f,static_cast<std::uint8_t>(input[9]),static_cast<std::uint16_t>(input[10])};
    }
    // native의큰외부효과와재진입입력만같게연결한다.
    BombRegularHooks Hooks() {
        BombRegularHooks hooks;
        // QA통지는현재TYPE를바꿀수있으며프레임은그뒤TYPE를사용해야한다.
        hooks.notice=[this](Sid sid,std::uint32_t code,std::uint32_t value) { Event("notice",{sid.value,code,value});if (input[8]==1) Raw()[10]=169; };
        // 초기프레임효과뒤권한변경을동일하게공급한다.
        hooks.frame=[this](Sid sid,std::uint32_t frame,std::uint32_t flags) { Event("frame",{sid.value,frame,flags});Put(Raw(),36,frame);if (input[8]==2) state.boss=0; };
        // 반복애니메이션의현재TYPE/권한변경만공급한다.
        hooks.animate=[this](Sid sid,std::uint32_t step,std::uint32_t flags) { Event("animate",{sid.value,step,flags});if (input[8]==3) Raw()[10]=169;if (input[8]==4) state.boss=0; };
        // 실제명시확보경계의크기를기록한다.
        hooks.allocate=[this](std::uint32_t bytes) { Event("allocate",{bytes});return true; };
        // 기대대상을계산하지않고실행기의고정입력목록을반환한다.
        hooks.targets=[this](Sid sid,std::span<std::uint32_t> out) {
            Event("targets",{sid.value,static_cast<std::uint32_t>(out.size())});
            // 독립입력대상번호60..159를원래순서로공급한다.
            for (std::size_t i=0;i<input[7];++i) out[i]=60+static_cast<std::uint32_t>(i);
            if (input[8]==5) { state.effect.protocol=3;state.effect.length=6;state.localOwner=513; }
            return input[7];
        };
        // 배포전전체버퍼와배포뒤로컬수신의변경을구별한다.
        hooks.broadcast=[this](std::span<std::uint8_t> packet) { Event("broadcast",{});trace+=':'+Hex(packet);if (input[8]==6) { state.localOwner=769;Put(packet,10,222,2); } };
        // 수신효과의dead변경은뒤의Regular반환에반영되어야한다.
        hooks.receive=[this](std::uint32_t owner,std::uint32_t connection,std::span<const std::uint8_t> packet) { Event("receive",{owner,connection});trace+=':'+Hex(packet);if (input[8]==7) Raw()[11]=2; };
        // 해제효과의dead변경은이미캡처한반환에영향을주지않는다.
        hooks.release=[this](std::uint32_t bytes) { Event("release",{bytes});if (input[8]==8) Raw()[11]=2; };
        return hooks;
    }
};
}
// 두x87정밀도원문을모두검사하며첫불일치만진단한다.
TEST_CASE(BombRegular_Native741Inputs) {
    BombScene scene;std::size_t count{},diagnostics{};
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-regular-x86.tsv").string();
    // 매행의독립입력으로실제제품호출뒤반환/효과/전체raw를비교한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==7);if (row.size()!=7) continue;
        scene.Prepare(row[0]);RawBombRegular regular(scene.pool,scene.types,scene.state,scene.Hooks());
        const auto actual=std::bit_cast<std::uint32_t>(regular.Handle(Sid{50},scene.input[0],scene.input[1],std::bit_cast<float>(scene.input[2])));
        const auto trace=scene.trace.empty() ? "-" : scene.trace,raw=Hex(scene.Raw());
        if ((actual!=Number(row[1]) || trace!=row[2] || raw!=row[3]) && diagnostics++<3) std::cout<<"Bomb입력 "<<row[0]<<"\n실제 "<<actual<<' '<<trace<<' '<<raw<<"\n원본 "<<row[1]<<' '<<row[2]<<' '<<row[3]<<'\n';
        CHECK(actual==Number(row[1]));CHECK(trace==row[2]);CHECK(raw==row[3]);CHECK(row[1]==row[4] && row[2]==row[5] && row[3]==row[6]);++count;
    }
    CHECK(count==741);
}
// 미도달경계는요구하지않고실제진행경계누락/할당실패/예외해제를검사한다.
TEST_CASE(BombRegular_LazyBoundariesAndCleanup) {
    BombScene scene;scene.Prepare("0,0,1065353216,1,0,0,0,0,0,1,10,1,17");
    BombRegularHooks hooks;hooks.frame=[](Sid,std::uint32_t,std::uint32_t) {}; // 비권한초기화는프레임이외의하위를사용하지않는다.
    RawBombRegular lazy(scene.pool,scene.types,scene.state,hooks);CHECK(&lazy.Pool()==&scene.pool);CHECK(lazy.Handle(Sid{50},0,0,1.0F)==1.0F);
    scene.state.boss=1;scene.types[168].castTime=-1;
    CHECK(Throws([&] { lazy.Handle(Sid{50},0,0,1.0F); }));
    hooks.allocate=[](std::uint32_t) { return false; }; // 확보실패를효과/배포성공으로바꾸지않는다.
    RawBombRegular failed(scene.pool,scene.types,scene.state,hooks);bool badAllocation{};
    try { failed.Handle(Sid{50},0,0,1.0F); }catch (const std::bad_alloc&) { badAllocation=true; }CHECK(badAllocation);
    std::uint32_t releases{};
    hooks.allocate=[](std::uint32_t) { return true; }; // 확보뒤대상검색누락도해제해야한다.
    hooks.release=[&](std::uint32_t bytes) { CHECK(bytes==400);++releases; };
    RawBombRegular missing(scene.pool,scene.types,scene.state,hooks);CHECK(Throws([&] { missing.Handle(Sid{50},0,0,1.0F); }));CHECK(releases==1);
    hooks.targets=[](Sid,std::span<std::uint32_t>) { return 101U; }; // 손상된하위반환이210바이트버퍼를넘지않도록거부한다.
    RawBombRegular overflow(scene.pool,scene.types,scene.state,hooks);CHECK(Throws([&] { overflow.Handle(Sid{50},0,0,1.0F); }));CHECK(releases==2);
}
