// 실제 공통 초기화만 검사한다. Take 수신/패킷/통신/게임/공중3종은 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/SquidFactory.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;

TEST_CASE(FactoryInitializationCurrent_Native576FullSlotsAndDiagnosticResume) { // 양판본 부모·실제 HP 하위의 전체 슬롯/진단/현재 전역을 대조한다.
    std::size_t rows{};
    for (const auto& row:LoadFixture(NETSTORM_FACTORY_INITIALIZATION_CURRENT_FIXTURE).rows) { // 기계어의 동결 입력/관측만 읽는다.
        CHECK(row.size()==7);const auto input=Split(row[1],',');CHECK(input.size()==6);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=edition==OriginalEdition::Patch1078;
        bool server=Number(input[3])!=0,quarter=false;std::uint32_t priest=158,dais=165;const auto mode=Number(input[4]),profile=Number(input[5]);SidPool pool(edition,32768,true,&server);const auto sid=pool.Allocate(2);
        const auto view=pool.Slot(sid);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(view.data()),view.size()};for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*17+23); // native 초기 payload와 폭만 같게 공급한다.
        raw[10]=static_cast<std::uint8_t>(Number(input[1]));raw[11]=static_cast<std::uint8_t>(Number(input[2]));raw[patch ? 40 : 35]=mode==2 ? 1 : 0;
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);for (const auto number:{3U,74U,75U,76U}) { // 입력 TYPE는 원문 장치와 같은 값만 갖는다.
            types[number].maxHitPoints=number==74 ? 0x12345678 : -37;types[number].flags1=mode==0 ? 16 : 0;types[number].flags2=mode ? 0x400000 : 0;types[number].zOrder=0x123+static_cast<std::int32_t>(number);
        }
        SquidFactory factory(pool,types);factory.BindWeakenedMana(quarter);factory.BindHitPointTypes(&priest,&dais);std::string diagnostics;
        factory.BindDiagnostics({types,[&](auto expression,auto file,auto line) { // 같은 원문 외부 진단의 명시 정상 복귀 변경이다.
            if (!diagnostics.empty()) diagnostics+=';';diagnostics+=std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line)+'|'+Hex(raw);
            if (profile==1) { raw[10]=75;types[75].maxHitPoints=-37;types[75].zOrder=0x345;quarter=true;priest=75; }
            else if (profile==2) { raw[10]=76;server=false;raw[11]&=static_cast<std::uint8_t>(~8U); }
        },{}});
        const auto free=pool.FreeCount();if (input[0]=="create") factory.PostCreate(sid);else { CHECK(input[0]=="take");factory.PostTake(sid); }
        const bool same=Hex(raw)==row[2] && (diagnostics.empty() ? "-" : diagnostics)==row[3] && server==(Number(row[4])!=0) && quarter==(Number(row[5])!=0) && priest==Number(row[6]);
        if (!same) { std::printf("%s %s\nactual %s %s\nnative %s %s\n",row[0].c_str(),row[1].c_str(),Hex(raw).c_str(),diagnostics.c_str(),row[2].c_str(),row[3].c_str());CHECK(same);return; }CHECK(pool.FreeCount()==free);++rows;
    }
    CHECK(rows==576);
}

TEST_CASE(FactoryInitializationCurrent_CdSelectedMissingDiagnosticAndCapturedLifetime) { // CD도 입구 실패/후단 부분 쓰기와 진단 자기 교체 수명을 보존한다.
    SidPool pool(OriginalEdition::Cd1072,32768);std::vector<RiftTypeRecord> types(171);types[74].zOrder=0x123;types[74].flags1=16;types[74].maxHitPoints=0x12345678;SquidFactory factory(pool,types);const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);raw[10]=3;raw[11]=15;raw[32]=9;raw[33]=77;raw[28]=0xcd;
    factory.BindDiagnostics({types,{},{}});const auto before=std::vector(raw.begin(),raw.end());CHECK(Throws([&] { factory.PostCreate(sid); }) && std::equal(before.begin(),before.end(),raw.begin()));
    std::vector<std::uint32_t> lines;factory.BindDiagnostics({types,[&](auto expression,auto file,auto line) { // 실행 중 사본은 연결을 바꿔도 정상 복귀해야 한다.
        CHECK(expression=="!isForm()" && file=="\\ns\\o\\Squid.cpp" && line==0x7a6);lines.push_back(line);raw[10]=74;factory.BindDiagnostics({types,{},{}});
    },{}});
    CHECK(Throws([&] { factory.PostCreate(sid); }));CHECK(lines==std::vector<std::uint32_t>{0x7a6});CHECK(raw[33]==77 && raw[32]==9); // 다음 contained 입구 진단에서 중단한다.
    raw[11]=7;factory.PostCreate(sid);CHECK(raw[33]==0x23 && raw[32]==0 && raw[26]==0x78 && raw[27]==0x56 && raw[28]==0xcd);
    raw[11]=15;raw[33]=77;CHECK(Throws([&] { factory.PostTake(sid); }) && raw[33]==0x23 && raw[11]==15); // PostTake는 zOrder를 쓴 뒤 마지막 진단에서 중단한다.
}
