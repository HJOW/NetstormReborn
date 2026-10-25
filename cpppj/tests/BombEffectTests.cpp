#include "o/RawGameAuxState.h"
// 독립 원본1676입력의 수신 효과/전체10슬롯/패킷/누적값/현재 전역을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBombEffect.h"
#include <filesystem>
#include <iostream>
#include <new>

using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kEffectVtable=0x502370,kAlternateVtable=0x20000800; // 원본 실행기의 실제 표와 격리 교체 표 토큰이다.
constexpr std::array<std::uint16_t,10> kEffectSlots{0,50,51,52,60,61,62,63,64,65}; // 독립 원문이 전체 바이트를 보존한 물리 슬롯이다.
// 원본의 명시 외부 입력만 공급하며 수신 판단과 누적은 제품이 실행한다.
struct EffectScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 동일 부모/운반체/대상의 물리 저장소다.
    std::vector<BombEffectType> types=std::vector<BombEffectType>(188); // 현재 TYPE 비용/소리/지연 입력이다.
    BombEffectState state; // 외부 효과가 변경할 수 있는 실제 현재 전역이다.
    std::vector<std::uint32_t> input; // 독립 원문의11개DWORD 입력이다.
    std::array<std::uint8_t,22> packet{}; // 효과가 별명을 통해 바꾸는 호출자 소유 버퍼다.
    std::string trace; // 실제 호출 순서와DWORD 인자를 기록한다.
    std::uint32_t allocations{},applied{}; // 명시 외부 효과의 첫 호출을 구별한다.
    // 검증 입력과 선언 효과만 물리 슬롯에 기록한다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 각 효과의 실제 인자를 원문과 같은 표현으로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> args={}) {
        if (!trace.empty()) trace+=';';trace+=name;
        // DWORD 인자의 순서와 폭을 유지한다.
        for (const auto value:args) trace+=':'+std::to_string(value);
    }
    // 입력을 설치하며 예상 순회/선택/누적 결과는 계산하지 않는다.
    void Prepare(std::string_view text) {
        input.clear();trace.clear();allocations=applied=0;
        // 동결 입력11칸을 부호 없는DWORD로 읽는다.
        for (const auto& field:Split(text,',')) input.push_back(Number(field));
        CHECK(input.size()==11);state=BombEffectState{};state.boss=input[4];state.spentCounter=input[9];
        // 원문의10개 물리 입력 슬롯과 상태 패턴만 그대로 공급한다.
        for (const auto number:kEffectSlots) {
            const Sid sid{number};auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kEffectVtable);
            raw[10]=static_cast<std::uint8_t>(number==50 ? input[3] : 133);
            raw[11]=static_cast<std::uint8_t>(number==50 ? input[0] : number==0 || number==51 || number==52 ? input[1] : input[6]==1 && (number%2) ? 1 : input[6]==2 ? 2 : 0);
        }
        Put(Raw(Sid{50}),12,input[2],2);
        // 세 TYPE의 명시 비용/소리/지연 입력이다. 결과 비용 절삭은 제품만 수행한다.
        for (const auto kind:{117U,118U,168U}) types[kind]={kind==input[3] ? std::bit_cast<float>(input[8]) : 33.75F,1000+kind,kind==117 ? 9 : kind==118 ? 11 : 13};
        packet.fill(0);packet[0]=1;packet[1]=0x5f;Put(packet,2,22,2);Put(packet,4,50,2);Put(packet,6,input[5]);
        // 중복/0번 대상은 원문에 제공한 입력 목록이며 거부/선택 결과가 아니다.
        for (std::size_t i=0;i<6;++i) Put(packet,10+2*i,input[6]==3 ? std::array<std::uint16_t,6>{60,60,0,63,64,65}[i] : 60+static_cast<std::uint32_t>(i),2);
    }
    // 제품 제어 흐름과 별개로 원본에 선언한 큰 하위 효과만 재생한다.
    BombEffectHooks Hooks() {
        BombEffectHooks hooks;
        // 시전 종료 뒤 현재TYPE/QA가 바뀌는 명시 입력이다.
        hooks.stopCast=[this](Sid carrier) { Event("stop-cast",{carrier.value});if (input[7]==1) Raw(Sid{50})[10]=118;if (input[7]==11) Put(Raw(Sid{50}),12,52,2); };
        // 두 확보 경계의 순서와 두 번째 확보 실패만 공급한다.
        hooks.allocate=[this](std::uint32_t bytes) { Event("allocate",{bytes});return ++allocations==1 || input[10]!=0; };
        // 매 호출에 제품이 전달한 현재 가상 표를 사용한다.
        hooks.apply=[this](Sid parent,std::uint32_t table,Sid target) {
            CHECK(parent.value==50);CHECK(table==kEffectVtable || table==kAlternateVtable);Event(table==kEffectVtable ? "apply" : "apply-alt",{parent.value,target.value});
            if (++applied!=1) return;
            if (input[7]==2) Put(Raw(parent),0,kAlternateVtable);
            if (input[7]==3) { Raw(Sid{62})[11]=2;Raw(target)[11]=2; }
            if (input[7]==4) Put(packet,6,1);
            if (input[7]==5) Put(packet,6,5);
            if (input[7]==6) state.boss=0;
            if (input[7]==7) Raw(parent)[10]=118;
            if (input[7]==12) state.secondType=168;
        };
        // 지연 생성 경계에서 관측한 인자와 저장 목록을 보존한다.
        hooks.construct=[this](std::int32_t delay,std::uint32_t capacity,std::span<const std::uint32_t> targets,std::uint32_t kind) {
            Event("construct",{std::bit_cast<std::uint32_t>(delay),kind,capacity,static_cast<std::uint32_t>(targets.size())});
            // 중복/0번과 효과 뒤dead 대상도 받은 순서 그대로 기록한다.
            for (const auto target:targets) trace+=':'+std::to_string(target);
            if (input[7]==8) Raw(Sid{50})[10]=168;
            return 1U;
        };
        // 실제 예약 인자 계산은 독립 원문에도 선언한 경계다.
        hooks.argument=[this](std::uint32_t first,std::uint32_t second) { Event("schedule-argument",{first,second});return 777U; };
        // 확보 실패 때 객체0 경계 호출도 기록한다. 실제NULL 객체 부착의 성공을 가정하지 않는다.
        hooks.attach=[this](std::uint32_t process,std::uint32_t kind,std::uint32_t argument,std::uint32_t first,std::uint32_t second) { CHECK(process<=1);Event("attach",{process,kind,argument,first,second}); };
        // 소리 효과가 최종 삭제 가상 표를 바꾸는 입력을 공급한다.
        hooks.sound=[this](std::array<std::uint32_t,6> args) {
            Event("sound");
            // 원본의 여섯 인자를 동일한 폭으로 기록한다.
            for (const auto value:args) trace+=':'+std::to_string(value);
            if (input[7]==9) Put(Raw(Sid{50}),0,kAlternateVtable);
        };
        // 제품이 소리 뒤 읽은 현재 가상 표와 삭제 flags만 사용한다.
        hooks.destroy=[this](Sid parent,std::uint32_t table,std::uint32_t flags) { CHECK(table==kEffectVtable || table==kAlternateVtable);Event(table==kEffectVtable ? "destroy" : "destroy-alt",{parent.value,flags});if (input[7]==10) Raw(parent)[11]=2; };
        // 최초 대상 배열을 해제하는 외부 효과다.
        hooks.release=[this] { Event("release",{1}); };
        return hooks;
    }
    // 독립 원문과 같은10슬롯 전체를 조립하며 상태를 해석하지 않는다.
    std::string Raws() {
        std::string result;
        // 물리0번 슬롯을 포함한 전체 바이트를 원래 순서로 읽는다.
        for (const auto number:kEffectSlots) { if (!result.empty()) result+=',';result+=Hex(Raw(Sid{number})); }
        return result;
    }
};
}
// 두 정밀도의 모든 동결 결과와 실제 수신 결과를 대조한다.
TEST_CASE(BombEffect_Native1676Inputs) {
    EffectScene scene;std::size_t count{},diagnostics{};
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-effect-x86.tsv").string();
    // 제품을 매 독립 입력에서 호출하고 각 효과/상태의 실제 출력만 비교한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==13);if (row.size()!=13) continue;
        scene.Prepare(row[0]);RawBombEffect effect(scene.pool,scene.types,scene.state,scene.Hooks());effect.Receive(scene.packet);
        const std::array<std::string,6> actual{scene.trace.empty() ? "-" : scene.trace,scene.Raws(),Hex(scene.packet),std::to_string(scene.state.spentCounter),std::to_string(scene.state.boss),std::to_string(scene.state.secondType)};
        // 원본의 효과/전체raw/패킷/누적/두 현재 전역을 각각 확인한다.
        for (std::size_t field=0;field<actual.size();++field) {
            if (actual[field]!=row[1+field] && diagnostics++<3) std::cout<<"Bomb효과 입력 "<<row[0]<<" 필드 "<<field<<"\n실제 "<<actual[field]<<"\n원본 "<<row[1+field]<<'\n';
            CHECK(actual[field]==row[1+field]);CHECK(row[1+field]==row[7+field]);
        }
        ++count;
    }
    CHECK(count==1676);
}
// 실제 버퍼 경계와 누락된 하위·확보 실패·예외 해제의 보호 경로를 검사한다.
TEST_CASE(BombEffect_PacketBoundsAndLazyCleanup) {
    EffectScene scene;scene.Prepare("0,4,51,168,0,3,0,0,1065353216,0,1");
    RawBombEffect empty(scene.pool,scene.types,scene.state,{});CHECK(&empty.Pool()==&scene.pool);
    CHECK(Throws([&] { empty.Receive(std::span<const std::uint8_t>(scene.packet).first(5)); }));
    scene.Raw(Sid{50})[11]=2;empty.Receive(std::span<const std::uint8_t>(scene.packet).first(6)); // dead 부모는count를 읽기 전에 반환한다.
    scene.Raw(Sid{50})[11]=0;CHECK(Throws([&] { empty.Receive(std::span<const std::uint8_t>(scene.packet).first(6)); }));
    Put(scene.packet,6,7);CHECK(Throws([&] { empty.Receive(scene.packet); }));
    Put(scene.packet,6,0xffffffffU);CHECK(Throws([&] { empty.Receive(scene.packet); }));
    Put(scene.packet,6,3);
    BombEffectHooks hooks;hooks.allocate=[](std::uint32_t) { return false; }; // 최초 확보 실패는 효과/해제를 성공으로 보고하지 않는다.
    RawBombEffect noMemory(scene.pool,scene.types,scene.state,hooks);bool failed{};
    try { noMemory.Receive(scene.packet); }catch (const std::bad_alloc&) { failed=true; }CHECK(failed);
    std::uint32_t releases{};
    hooks.allocate=[](std::uint32_t) { return true; }; // 확보 뒤 실제 대상 효과 누락도 배열을 해제한다.
    hooks.release=[&] { ++releases; };
    RawBombEffect missing(scene.pool,scene.types,scene.state,hooks);CHECK(Throws([&] { missing.Receive(scene.packet); }));CHECK(releases==1);
    hooks.apply=[&](Sid,std::uint32_t,Sid) { Put(scene.packet,6,7); }; // 효과가 버퍼 밖 대상까지 늘려도 읽기 전에 거부한다.
    RawBombEffect growing(scene.pool,scene.types,scene.state,hooks);CHECK(Throws([&] { growing.Receive(scene.packet); }));CHECK(releases==2);
}


// cast종료 효과 뒤 현재 unsigned bits/TYPE를 다시 읽고 후속 확보실패도 선행 누적을 보존한다.
TEST_CASE(GameAux_BombEffectRequeriesSignedResourceAsUnsignedAfterCast) {
    RawGameAuxState owner;EffectScene scene;scene.Prepare("0,0,51,117,0,0,0,0,1065353216,77,1");scene.state.sharedSpentCounter=&owner.ResourcePool();scene.types[118].cost=7.75F;scene.state.spentCounter=77;owner.ResourcePool()=123;
    BombEffectHooks hooks;hooks.stopCast=[&](Sid carrier) { CHECK(carrier==Sid{51});owner.StoreResourcePoolBits(0xffffffffU);scene.Raw(Sid{50})[10]=118; };hooks.allocate=[](std::uint32_t) { return false; };RawBombEffect effect(scene.pool,scene.types,scene.state,hooks);bool failed{};
    try { effect.Receive(scene.packet); }catch (const std::bad_alloc&) { failed=true; }CHECK(failed && owner.ResourcePoolBits()==6 && scene.state.spentCounter==77); // signed-1의동일DWORD+7이unsigned6으로감기며owner1/성공으로정규화하지않는다.
}
