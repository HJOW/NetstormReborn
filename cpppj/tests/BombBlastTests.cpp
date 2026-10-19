// 독립 원본2082+64입력의 폭발 자격/효과/캡처 위치/전체 상태를 비교한다.
#include "RawSceneSupport.h"
#include "o/RawBombBlast.h"
#include <filesystem>
#include <iostream>

using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kBlastVtable=0x502370,kTargetVtable=0x501dd0,kAlternativeVtable=0x20000800; // 원본 실행기의 부모/대상/교체 표 토큰이다.
constexpr std::array<std::uint16_t,4> kBlastSlots{0,50,51,60}; // 원본이 전체 바이트를 동결한 물리 슬롯이다.
// 실제 판단과 좌표 계산은 제품이 실행하며 큰 효과의 명시 입력만 공급한다.
struct BlastScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 현재 부모/대상의 원본 번호를 유지하는 저장소다.
    SquidHash hash; // 좌표 계산기와 같은 월드의 해시이며 이 직접 효과 호출에서는 탐색하지 않는다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 genus/HP 보유/발자국 입력이다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); // 현재 대상의 실제4바이트 코드 입력이다.
    std::vector<float> ranges=std::vector<float>(188); // 현재 부모TYPE+44 입력이다.
    BombTargetState targetState; // 기존 공용 좌표 계산기가 빌리는 상태다.
    RawBombTargets coordinates{pool,hash,types,ranges,targetState,{}}; // 실제 기존41e460/41e3c0 좌표/반경 몸체다.
    BombBlastState state; // 큰 효과가 변경할 수 있는 현재 권한/동맹/종류다.
    std::vector<std::uint32_t> input; // 독립 원문의23개DWORD 입력이다.
    std::string trace; // 실제 큰 효과의 순서와DWORD 인자다.
    // 검증 입력/선언 효과만 물리 슬롯에 기록한다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 원문 형식으로 효과 인자만 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 각 인자의 전체DWORD와 순서를 보존한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 같은 명시 원문 입력을 설치하며 판단/피해 분기를 계산하지 않는다.
    void Prepare(std::string_view text) {
        input.clear();trace.clear();state=BombBlastState{};targetState.boss=1;
        // 실제23DWORD 입력을 부호 없는 원문 폭으로 읽는다.
        for (const auto& field:Split(text,',')) input.push_back(Number(field));
        CHECK(input.size()==23);if (input.size()!=23) throw std::invalid_argument("폭발 원문 입력 개수");
        std::fill(types.begin(),types.end(),RiftTypeRecord{});std::fill(ranges.begin(),ranges.end(),0.0F);std::fill(frames.begin(),frames.end(),RiftTypeFrames({}));
        // 보존된 네 슬롯을 원문과 같이 매 입력 초기화한다.
        for (const auto sid:kBlastSlots) std::fill(Raw(Sid{sid}).begin(),Raw(Sid{sid}).end(),std::uint8_t{});
        auto parent=Raw(Sid{50});Put(parent,0,kBlastVtable);parent[10]=168;parent[34]=static_cast<std::uint8_t>(input[10]);Put(parent,12,input[8],2);Put(parent,14,input[14]);Put(parent,18,input[15]);
        const Sid target{static_cast<std::uint16_t>(input[9])};auto victim=Raw(target);Put(victim,0,kTargetVtable);victim[10]=static_cast<std::uint8_t>(input[1]);victim[11]=static_cast<std::uint8_t>(input[21]);victim[34]=static_cast<std::uint8_t>(input[11]);victim[40]=static_cast<std::uint8_t>(input[4]);Put(victim,14,input[16]);Put(victim,18,input[17]);Put(victim,36,input[5]);Put(victim,26,input[22]);
        auto& type=types[input[1]];type.flags1=input[3];type.flags2=input[2];type.footX=static_cast<std::int32_t>(input[18]);type.footY=static_cast<std::int32_t>(input[19]);ranges[168]=std::bit_cast<float>(input[13]);
        std::vector<FrameCode> codes;
        // 현재 프레임의 보호 비트 선택은 제품이 수행하며 네 코드만 입력한다.
        for (std::uint8_t i=0;i<4;++i) codes.push_back({74,0,static_cast<std::uint8_t>(i+1),static_cast<std::uint8_t>(input[6]^(i*17U))});
        frames[input[1]]=RiftTypeFrames(std::move(codes));state.boss=input[7];state.alliances[input[10]*9+input[11]]=input[12];
    }
    // 독립 실행기의 큰 가상 효과/재진입만 공급한다.
    BombBlastHooks Hooks() {
        BombBlastHooks hooks;
        // 일반 경로의 실제radius는 기존 제품 helper로 계산한다.
        hooks.radius=[this](Sid parent,std::uint32_t table) {
            CHECK(parent.value==50 && table==kBlastVtable);const Sid target{static_cast<std::uint16_t>(input[9])};
            if (input[20]<1 || input[20]>4) return coordinates.DefaultRadius(parent);
            Event("radius-override",{50});
            if (input[20]==1) { Put(Raw(target),14,std::bit_cast<std::uint32_t>(200.0F));Put(Raw(target),18,std::bit_cast<std::uint32_t>(200.0F)); }
            if (input[20]==2) Put(Raw(target),0,kAlternativeVtable);
            if (input[20]==3) state.boss=0;
            if (input[20]==4) { Raw(target)[10]=71;types[70].footX=8; }
            return 3;
        };
        // 현재 가상 표에 해당하는 피해 경계의 실제 인자를 기록한다.
        hooks.damage=[this](Sid target,std::uint32_t table,std::int32_t amount,std::uint8_t owner,Sid parent) {
            CHECK(table==kTargetVtable || table==kAlternativeVtable);Event(table==kTargetVtable ? "damage" : "damage-alt",{target.value,std::bit_cast<std::uint32_t>(amount),owner,parent.value});
            if (input[20]==5) { Put(Raw(target),14,std::bit_cast<std::uint32_t>(200.0F));Put(Raw(target),18,std::bit_cast<std::uint32_t>(200.0F)); }
            if (input[20]==6) Raw(parent)[34]=3;
            if (input[20]==7) Put(Raw(target),0,kAlternativeVtable);
            if (input[20]==8) { Raw(target)[11]=2;Put(Raw(target),26,1); }
        };
        // 현재 삭제 표와 두 종류의 정확한 flags를 관측한다.
        hooks.destroy=[this](Sid target,std::uint32_t table,std::uint32_t flags) {
            CHECK(table==kTargetVtable || table==kAlternativeVtable);Event(table==kTargetVtable ? "destroy" : "destroy-alt",{target.value,flags});
            if (input[20]==9) { Put(Raw(target),14,std::bit_cast<std::uint32_t>(200.0F));Put(Raw(target),18,std::bit_cast<std::uint32_t>(200.0F)); }
            if (input[20]==10) Put(Raw(target),0,kAlternativeVtable);
        };
        // 피해 전 캡처 중심을 받으며 이동 후 좌표로 기대값을 다시 계산하지 않는다.
        hooks.ground=[this](float x,float y) { Event("ground",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});if (input[20]==11) Raw(Sid{static_cast<std::uint16_t>(input[9])})[40]=1; };
        // 실제421f70의 대상/부모 중심 요청만 관측한다.
        hooks.edge=[this](Sid target,float x,float y) { Event("edge",{target.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)}); };
        return hooks;
    }
    // 실제 네 슬롯 전체를 원문 순서로 읽는다.
    std::string Raws() {
        std::string result;
        // 예약0번 슬롯도 제외하지 않고 물리 바이트를 비교한다.
        for (const auto sid:kBlastSlots) { if (!result.empty()) result+=',';result+=Hex(Raw(Sid{sid})); }
        return result;
    }
};
}
// 기존2082입력과 별도64추가 원문을 바꾸지 않고 실제 제품에 대조한다.
TEST_CASE(BombBlast_Native2146Inputs) {
    BlastScene scene;std::size_t count{},diagnostics{};
    // 기존/추가 기대값은 각각 독립 동결 파일에서 읽는다.
    for (const auto name:{"bomb-blast-x86.tsv","bomb-blast-dais-x86.tsv"}) {
        const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures"/name).string();
        // 자격/직접효과/합성 호출을 원문 operation대로 실행한다.
        for (const auto& row:LoadFixture(path.c_str()).rows) {
            CHECK(row.size()==11);if (row.size()!=11) continue;scene.Prepare(row[0]);RawBombBlast blast(scene.pool,scene.types,scene.frames,scene.coordinates,scene.state,scene.Hooks());const Sid target{static_cast<std::uint16_t>(scene.input[9])};std::uint32_t result{};
            if (scene.input[0]==0) result=blast.MayTarget(target);
            else if (scene.input[0]==1) blast.Apply(Sid{50},target);
            else { result=blast.MayTarget(target);if (result) blast.Apply(Sid{50},target); }
            const std::array<std::string,5> actual{std::to_string(result),scene.trace.empty() ? "-" : scene.trace,scene.Raws(),std::to_string(scene.state.boss),std::to_string(scene.types[70].footX)};
            // 두 정밀도의 반환/효과/전체raw/권한/현재발자국과 각각 비교한다.
            for (std::size_t field=0;field<actual.size();++field) {
                if (actual[field]!=row[1+field] && diagnostics++<3) std::cout<<"Bomb폭발 입력 "<<row[0]<<" 필드 "<<field<<"\n실제 "<<actual[field]<<"\n원본 "<<row[1+field]<<'\n';
                CHECK(actual[field]==row[1+field]);CHECK(row[1+field]==row[6+field]);
            }
            ++count;
        }
    }
    CHECK(count==2146);
}
// 미도달 효과/실제 하위 누락과 잘못된 frame/owner/다른 풀의 연결을 확인한다.
TEST_CASE(BombBlast_LazyEffectsAndBounds) {
    BlastScene scene;const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-blast-x86.tsv").string();scene.Prepare(LoadFixture(path.c_str()).rows.front()[0]);
    RawBombBlast blast(scene.pool,scene.types,scene.frames,scene.coordinates,scene.state,{});CHECK(&blast.Pool()==&scene.pool);CHECK(blast.Handles(Sid{50}));CHECK(!blast.Handles(Sid{60}));blast.Apply(Sid{50},Sid{51}); // QA와 같으면 효과 경계를 요구하지 않는다.
    CHECK(Throws([&] { blast.Apply(Sid{50},Sid{60}); }));
    scene.types[70].flags2=4;Put(scene.Raw(Sid{60}),36,9);CHECK(Throws([&] { blast.MayTarget(Sid{60}); }));
    scene.Raw(Sid{60})[10]=158;scene.Raw(Sid{60})[34]=255;CHECK(Throws([&] { blast.Apply(Sid{50},Sid{60}); }));
    SidPool other(OriginalEdition::Patch1078,24000,true);CHECK(Throws([&] { RawBombBlast mismatch(other,scene.types,scene.frames,scene.coordinates,scene.state,{}); }));
}
