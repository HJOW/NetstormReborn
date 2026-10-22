// 원본의334입력을현재C++ 본체/실제일반finder와전체104BYTE로대조한다.
#include "RawSceneSupport.h"
#include "o/RawWindFlyer.h"
#include <filesystem>
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본실행기와동일한입력만설치하고결과를계산하지않는다.
struct WindFlyerScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false};SquidHash hash; // 실제하나의풀과공간해시다.
    std::vector<RiftTypeRecord> types{188}; // 현재후보genus와실제발자국자료다.
    std::array<std::uint32_t,188> stamps{},sounds{}; // 타입별현재소리장부와이름표토큰이다.
    FlyerProcessFields fields; // 실제원문104BYTE와호스트저장소다.
    double now{100.25};bool boss{true};std::uint32_t frame{77}; // 실제현재전역의명시입력이다.
    std::int32_t runCount{22};std::uint32_t mask{3};std::string trace; // 글자개수·대상허용경계와효과순서다.
    std::unique_ptr<RawWindFlyer> logic; // 위동일자료를빌리는제품몸체다.
    WindFlyerScene(char kind,const std::vector<std::string>& in) { // 각행은이전입력의상태와독립이다.
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==70) break;if (!sid.value || sid.value>70) throw std::logic_error("WindFlyer SID 입력준비실패"); } // 실제슬롯을확보한다.
        for (const auto sid:{50,60,61,62}) { // 원본과동일50BYTE공격자·다리·후보를설치한다.
            auto raw=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(sid)});std::fill(raw.begin(),raw.end(),std::uint8_t{});
            Put(raw,0,sid==50 ? 0x50a050U : 0x501ea0U);raw[10]=sid==50 ? 138 : sid==60 ? 82 : sid==61 ? 78 : 79;
            Put(raw,14,std::bit_cast<std::uint32_t>(sid==50 ? 50.25f : 51.0f));Put(raw,18,std::bit_cast<std::uint32_t>(sid==50 ? 50.5f : 51.0f));raw[34]=sid==50 ? 1 : 2;Put(raw,36,sid==50 ? 13U : 0U);
        }
        types[82].flags2=4;sounds[138]=881;fields.bytes.fill(0xa5);Put(fields.bytes,0,0x509f98);Put(fields.bytes,48,0x509f78);Put(fields.bytes,52,70);Put(fields.bytes,56,50);D(96,100);
        hash.Cell(0,51,51)=60;hash.Cell(1,25,25)=61;Put(pool.AllocatedBytes(Sid{61}),4,62,2); // 실제finder가표면→두일반후보순서로반환하는현재배치다.
        if (kind=='C' || kind=='I') { now=std::stod(in[0]);fields.bytes.fill(static_cast<std::uint8_t>(Number(in[1]))); }
        else if (kind=='A') { now=std::stod(in[0]);D(96,std::stod(in[1]));Put(pool.AllocatedBytes(Sid{50}),36,static_cast<std::uint32_t>(std::stol(in[2])));pool.AllocatedBytes(Sid{50})[40]=static_cast<std::uint8_t>(Number(in[3]));runCount=static_cast<std::int32_t>(Number(in[4])); }
        else { boss=Number(in[1])!=0;stamps[138]=Number(in[3]);mask=Number(in[4]);pool.AllocatedBytes(Sid{61})[11]=static_cast<std::uint8_t>(Number(in[5]));types[79].flags2=Number(in[6]); }
        WindFlyerHooks hooks;hooks.runCount=[this](Sid) { Event("Count");return runCount; };
        hooks.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line)); };
        hooks.destroy=[this](Sid,std::uint32_t flags) { Event("Destroy,"+std::to_string(flags)); };
        hooks.sound=[this](float,float,std::uint32_t token) { Event("Sound,"+std::to_string(token)); };
        hooks.mayTarget=[this](Sid,Sid target) { Event("May,"+std::to_string(target.value));return target.value>=61 && target.value<=62 && ((mask>>(target.value-61))&1U)!=0; };
        hooks.weaken=[this](Sid sid) { Event("Weaken,"+std::to_string(sid.value)); };
        hooks.damageRate=[](Sid,Sid) { return 41.5f; };
        hooks.damage=[this](Sid sid,std::int32_t amount,std::uint8_t owner,Sid source) { Event("Damage,"+std::to_string(sid.value)+','+std::to_string(amount)+','+std::to_string(owner)+','+std::to_string(source.value)); };
        logic=std::make_unique<RawWindFlyer>(pool,hash,types,stamps,sounds,now,boss,frame,std::move(hooks));
    }
    void D(std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,at,static_cast<std::uint32_t>(bits));Put(fields.bytes,at+4,static_cast<std::uint32_t>(bits>>32)); } // raw double 입력을비트로설치한다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 실제효과순서만기록한다.
};
// 해당원문범위의모든행을재생하고반환비트·전체원문·stamp·효과를비교한다.
void CompareWindFlyer(char kind,std::size_t expected) {
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/wind-flyer-x86.tsv";
    const auto fixture=LoadFixture(path.string().c_str());std::size_t seen{};
    for (std::size_t i=1;i<fixture.rows.size();++i) { // 다른몸체의관측은별도검사에맡긴다.
        const auto& row=fixture.rows[i];if (row[0]!=std::string(1,kind)) continue;
        const auto in=Split(row[1],',');WindFlyerScene scene(kind,in);std::uint32_t result{};++seen;
        if (kind=='C') {
            bool server=true;std::uint32_t type=31;FlyerProcessHooks hooks;
            hooks.attach=[&](FlyerProcessFields& fields,Sid parent,std::uint32_t processType,std::uint32_t flags) { scene.Event("Attach,"+std::to_string(processType)+','+std::to_string(parent.value)+','+std::to_string(flags));Put(fields.bytes,52,70);Put(fields.bytes,56,50); };
            RawFlyerProcess base(scene.pool,scene.types,{server,scene.now,type},std::move(hooks));base.Construct(scene.fields,Sid{50},Sid{17},std::bit_cast<std::uint32_t>(4.0f),0.5f);RawWindFlyer::Specialize(scene.fields);
        } else if (kind=='I') scene.logic->Initialize(scene.fields);
        else if (kind=='A') result=std::bit_cast<std::uint32_t>(scene.logic->Animation(scene.fields));
        else if (kind=='R') result=std::bit_cast<std::uint32_t>(scene.logic->Handle(Sid{50},0,Number(in[0]),std::stof(in[2])));
        else scene.logic->Attack(Sid{50},std::stof(in[2]));
        const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (Hex(std::span(scene.fields.bytes).first(104))!=row[2] || result!=Number(row[3]) || trace!=row[5]) std::fprintf(stderr,"WindFlyer mismatch %c %s\n%s\n%s\n",kind,row[1].c_str(),trace.c_str(),row[5].c_str());
        CHECK(Hex(std::span(scene.fields.bytes).first(104))==row[2]);CHECK(result==Number(row[3]));CHECK(scene.stamps[138]==Number(row[4]));CHECK(trace==row[5]);
    }
    CHECK(seen==expected);
}
}
TEST_CASE(WindFlyer_NativeConstructor9) { CompareWindFlyer('C',9); } // 실제기본부착뒤전용표쓰기다.
TEST_CASE(WindFlyer_NativeInitialize9) { CompareWindFlyer('I',9); } // 최초시각과20초대기다.
TEST_CASE(WindFlyer_NativeAnimation180) { CompareWindFlyer('A',180); } // 기한/정지/signed프레임경계다.
TEST_CASE(WindFlyer_NativeRegular72) { CompareWindFlyer('R',72); } // 공격횟수·수명·소리장부다.
TEST_CASE(WindFlyer_NativeAttack64) { CompareWindFlyer('D',64); } // 실제해시순회/후보필터·다리약화·피해다.
