// 보호 PE의 생산/후처리1639입력을 전체80BYTE와 세 슬롯 및 효과 순서로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawWindAviaryProcess.h"
#include "o/RawSunAviaryProcess.h"
#include <filesystem>
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint16_t,3> kWindIds{50,60,70}; // 부모/임시 플라이어/Form의 원본 순서다.
// 원문과 같은 입력/명시 효과만 준비하며 결과 계산은 실제 제품 Frame에 맡긴다.
struct WindScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false};WindAviaryProcessFields fields; // 실제 PATCH 풀과80BYTE다.
    bool fort{},friendly{},server{true},checking{},boss{true};std::uint32_t editor{},processType{59},flyerType{138},aviaryType{139};double now{100}; // 현재 전역 입력이며 boss는 실제 세션의 bool을 참조한다.
    std::string trace;std::unique_ptr<RawWindAviaryProcess> logic; // 원본 효과 순서 관찰과 제품 몸체다.
    // 독립 원문과 같은 현재 입력만 설치한다.
    WindScene(char kind,const std::vector<std::string>& in) {
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==70) break;if (!sid.value || sid.value>70) throw std::logic_error("Wind SID 입력 준비 실패"); } // 세 실제 슬롯까지 확보한다.
        for (auto id:kWindIds) { auto raw=pool.AllocatedBytes(Sid{id});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x1240f000);Put(raw,4,0,2);Put(raw,6,id==50 ? 70U : 0U,2);Put(raw,8,0,2);raw[10]=id==50 ? 139 : id==60 ? 138 : 59;raw[11]=0;Put(raw,12,0,2);Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(id)+.25f));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(id)+.5f));raw[34]=1;raw[40]=0;Put(raw,36,0); } // 세 슬롯의 전체50BYTE를 원문 입력 그대로 설치한다.
        fields.bytes.fill(0xa5);Put(fields.bytes,0,0x509dfc);fields.bytes[8]=0x10;D(16,100);Put(fields.bytes,24,1);D(32,99);D(40,100);Put(fields.bytes,48,0);Put(fields.bytes,56,0x509ddc);Put(fields.bytes,60,77);Put(fields.bytes,64,50);Put(fields.bytes,68,4);Put(fields.bytes,72,2);
        if (kind=='F') { Put(fields.bytes,24,Number(in[0]));D(40,100-std::stod(in[1]));D(32,std::stod(in[2]));Put(fields.bytes,48,Number(in[3]));server=Number(in[4])!=0;boss=Number(in[5])!=0;pool.AllocatedBytes(Sid{50})[10]=static_cast<std::uint8_t>(Number(in[6]));editor=Number(in[7]);pool.AllocatedBytes(Sid{50})[40]=static_cast<std::uint8_t>(Number(in[8]));Put(pool.AllocatedBytes(Sid{50}),36,static_cast<std::uint32_t>(std::stol(in[9]))); }
        else { server=Number(in[0])!=0;fort=Number(in[1])!=0;friendly=Number(in[2])!=0;fields.bytes.fill(0xa5);Put(pool.AllocatedBytes(Sid{50}),6,0,2); }
        logic=std::make_unique<RawWindAviaryProcess>(pool,WindAviaryGlobals{fort,friendly,server,boss,editor,checking,now,processType,flyerType,aviaryType},Hooks());
    }
    void D(std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,at,static_cast<std::uint32_t>(bits));Put(fields.bytes,at+4,static_cast<std::uint32_t>(bits>>32)); } // 원문 double의 비트 입력이다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 원본 호출 순서를 보존한다.
    WindAviaryHooks Hooks() {
        WindAviaryHooks h;h.findProcess=[this](Sid sid,std::uint32_t type) { return FindAttachedProcess(pool,sid,type); }; // 실제 원문 첫 종속 조회다.
        h.allocate=[this]()->std::optional<WindAviaryProcessFields> { Event("Allocate,80");WindAviaryProcessFields f;f.bytes.fill(0xa5);return f; };
        h.attach=[this](std::optional<WindAviaryProcessFields> f,Sid parent,std::uint32_t type) { Event("Attach,"+std::to_string(type)+','+std::to_string(parent.value)+",8");fields=*f; };
        h.damageable=[this](Sid,std::uint32_t flags) { Event("Base,"+std::to_string(flags)); };h.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line)); };
        h.setFrame=[this](Sid sid,std::int32_t frame) { Event("Frame,"+std::to_string(sid.value)+','+std::to_string(frame));Put(pool.AllocatedBytes(sid),36,std::bit_cast<std::uint32_t>(frame)); };
        h.create=[this](std::uint32_t type) { Event("Create,"+std::to_string(type));return Sid{60}; };
        h.setOwner=[this](Sid sid,std::uint32_t owner) { Event("Owner,"+std::to_string(owner));pool.AllocatedBytes(sid)[34]=static_cast<std::uint8_t>(owner); };
        h.height=[this](Sid,std::int32_t) { Event("Height");return 44.0f; };h.footY=[](Sid) { return 3.0f; };
        h.pop=[this](Sid,float x,float y) { Event("Pop,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))); };
        h.createFlyerProcess=[this](Sid sid,Sid parent) { Event("Allocate,104");Event("Process,"+std::to_string(sid.value)+','+std::to_string(parent.value)+",1082130432,1056964608"); };
        h.bindFlyer=[this](Sid sid,Sid home) { Event("Bind,"+std::to_string(sid.value)+','+std::to_string(home.value)); };
        h.circlePosition=[this](Sid,std::int32_t radius,std::int32_t index) { Event("SpecialHeight");Event("Center");Event("Circle,"+std::to_string(radius)+','+std::to_string(index));return std::array<float,2>{static_cast<float>(61.25-1.399999976158142),static_cast<float>(44.0*.09090909361839294-3.0+62.5)}; };
        h.finish=[this](WindAviaryProcessFields&) { Event("Finish,0"); };return h;
    } // 명시 큰 효과는 원문 입력의 정해진 결과만 돌려준다.
    std::string Objects() const { std::string out;for (auto sid:kWindIds) out+=Hex(pool.Slot(Sid{sid}));return out; } // 세 슬롯 전체의 원본 순서다.
};
// 상태마다 독립 입력을 재생하여 이전 행의 메모리가 섞이지 않게 한다.
void CompareWind(char kind,std::size_t count) {
    auto fixture=LoadFixture(NETSTORM_WIND_AVIARY_FRAME_FIXTURE);std::size_t seen{};
    for (std::size_t i=1;i<fixture.rows.size();++i) { // 해당 범위의 원본 행만 순서대로 재생한다.
        const auto& row=fixture.rows[i];if (row[0]!=std::string(1,kind)) continue;const auto input=Split(row[1],',');WindScene scene(kind,input);++seen;
        if (kind=='F') scene.logic->Frame(scene.fields);else scene.logic->PostPop(Sid{50},Number(input[3]));
        const auto trace=scene.trace.empty() ? "-" : scene.trace;if (Hex(scene.fields.bytes)!=row[2] || scene.Objects()!=row[3] || trace!=row[4]) std::fprintf(stderr,"Wind mismatch %s\n%s\n%s\n",row[1].c_str(),trace.c_str(),row[4].c_str());CHECK(Hex(scene.fields.bytes)==row[2]);CHECK(scene.Objects()==row[3]);CHECK(trace==row[4]);
    }
    CHECK(seen==count);
}
}
TEST_CASE(WindAviary_NativeFrame1607) { CompareWind('F',1607); } // 실제4/6/26초와 전체 상태/가감속/표시 유무다.
TEST_CASE(WindAviary_NativePostPop32) { CompareWind('P',32); } // 실제 편집/친군 사격/서버/최초 비트 조합이다.
