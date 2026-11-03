// 실제 제단 부모 CPU1596행과 현재 Client owner/선택 장치를 대조한다. 사제 본체는 호출하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawAltarReleaseParent.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 원문 기대값은 읽기 전용이며 입력/장치만 구성한다.
// 같은 제단50BYTE와 두 현재 방향/검색/범위/프레임 입력을 한 호출 동안 유지한다.
struct AltarReleaseScene {
    std::array<std::uint8_t,50> raw{}; // Native와 같은 예약 필드/표시 없는 프레임 저장소다.
    std::uint32_t checking{},found{},capacity{},frameBase{},action{},directions{};std::int32_t first{},second{}; // 원문 장치에 공급한 명시 현재 입력이다.
    std::string events;AltarReleaseParentHooks hooks; // 실제 외부 요청 순서와 선택 장치다.
    explicit AltarReleaseScene(const std::vector<std::string>& row) { // 출력/분기는 계산하지 않고 같은 초기 raw/장치 입력만 복제한다.
        first=std::stoi(row[1]);second=std::stoi(row[2]);checking=Number(row[3]);found=Number(row[4]);capacity=Number(row[5]);frameBase=Number(row[6]);action=Number(row[7]);
        for (std::uint32_t byte=0;byte<raw.size();++byte) raw[byte]=static_cast<std::uint8_t>(byte*7U+19U); // 예약 바이트를 정규화하지 않는다.
        Put(raw,0,0x61f000);raw[10]=164;raw[11]=0;Put(raw,14,0x3fa00000);Put(raw,18,0xc0200000);Put(raw,36,0x55667788);
        hooks.findDais=[this](Sid altar) { CHECK(altar==Sid{50});const auto result=found;events+="F,"+std::to_string(Get(raw,14))+','+std::to_string(Get(raw,18))+",165,"+std::to_string(result)+';';if (action&1U) { checking=0x80000000;Put(raw,14,0x40b00000);Put(raw,18,0x40d00000); }return Sid{static_cast<std::uint16_t>(result)}; };
        hooks.direction=[this](Sid altar) { CHECK(altar==Sid{50});const auto result=directions ? second : first;events+="G,"+std::to_string(directions)+','+std::to_string(result)+';';if (!directions && (action&2U)) checking=0;++directions;return result; };
        hooks.assertion=[this](auto expression,auto file,auto line) { events+="A,"+std::to_string(line)+':'+std::string(expression)+':'+std::string(file)+';';if (action&4U) { checking=0;capacity=2;raw[10]=165;Put(raw,14,0x40700000);Put(raw,18,0x40900000); }if (action&32U) found=found ? 0U : 127U; };
        hooks.valid=[this](Sid dais) { return dais.value>0 && dais.value<capacity; }; // Native의 같은 고정 pool base/유한 capacity 범위에 대응한다.
        hooks.release=[this](Sid dais) { events+="R,"+std::to_string(dais.value)+';';if (action&8U) { raw[10]=166;Put(raw,14,0x40900000);capacity=1;found=0; } }; // 사제 생성/해방/희생 몸체는 관측 경계다.
        hooks.unpop=[this](Sid altar,std::uint32_t flags) { CHECK(altar==Sid{50} && !flags);events+="U,50,0;";if (action&16U) { raw[10]=167;Put(raw,18,0xc0d00000); } };
        hooks.plainP=[this](Sid altar) { CHECK(altar==Sid{50});const auto type=raw[10];const auto frame=frameBase+type*17U;events+="T,"+std::to_string(type)+','+std::to_string(frame)+';';Put(raw,36,frame); }; // TYPE getter의 입력 표와 순수 raw 쓰기만 공급한다.
        hooks.repop=[this](Sid altar,std::uint32_t flags) { CHECK(altar==Sid{50} && !flags);events+="P,50,0;"; };
    }
    std::vector<std::string> Run() { RunAltarReleaseParent(Sid{50},RawCheckingFlag{checking},hooks);return {Hex(raw),std::to_string(checking),std::to_string(found),std::to_string(capacity),std::to_string(directions),events.empty() ? "-" : events}; } // 실제 부모 실행 뒤 메모리/요청만 출력한다.
};
const auto& AltarReleaseRows() { static const auto rows=LoadFixture(NETSTORM_ALTAR_RELEASE_PARENT_FIXTURE).rows;return rows; } // 최초 CPU1596행을 읽기만 한다.
}
// 첫/두 번째 방향·현재 checking/범위·저장 SID·현재 P 프레임과 가상 요청을 실제 CPU와 비교한다.
TEST_CASE(AltarReleaseParent_NativeCheckingAndCurrentEffects) {
    std::size_t count{};
    for (const auto& row:AltarReleaseRows()) { // 첫 불일치의 원시 입력/필드를 유지하고 기대값을 바꾸지 않는다.
        CHECK(row.size()==14);AltarReleaseScene scene(row);const auto actual=scene.Run();const auto expected=std::vector<std::string>(row.begin()+8,row.end());CHECK(actual==expected);
        if (actual!=expected) { /* 불일치한 출력 필드를 원시 기대값과 함께 기록한다. */ for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("제단 부모 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),index,expected[index].c_str(),actual[index].c_str());return; }++count;
    }CHECK(count==1596);
}
// 장치 누락/진단 중단은 이미 일어난 finder/방향/공간/raw 프레임을 되돌리지 않는다.
TEST_CASE(AltarReleaseParent_SelectedDevicesAndPartialEffects) {
    auto row=AltarReleaseRows().front();row[1]="12";row[2]="0";row[3]="0";row[4]="0";row[5]="128";row[6]="7";row[7]="0";
    AltarReleaseScene skipped(row);skipped.hooks.direction={};skipped.hooks.assertion={};skipped.hooks.release={};skipped.Run();CHECK(!skipped.directions && Get(skipped.raw,36)==2795);
    row[3]="2147483648";AltarReleaseScene missing(row);missing.hooks.assertion={};CHECK(Throws([&] { missing.Run(); }));CHECK(missing.directions==1 && Get(missing.raw,36)==0x55667788U && missing.events.find("U,")==std::string::npos);
    AltarReleaseScene stopped(row);stopped.hooks.assertion=[&](auto,auto,auto) { stopped.checking=0;stopped.raw[10]=165;throw std::runtime_error("선택 진단 중단"); };CHECK(Throws([&] { stopped.Run(); }));CHECK(!stopped.checking && stopped.raw[10]==165 && Get(stopped.raw,36)==0x55667788U);
    row[3]="0";AltarReleaseScene noRepop(row);noRepop.hooks.repop={};CHECK(Throws([&] { noRepop.Run(); }));CHECK(Get(noRepop.raw,36)==2795 && noRepop.events.find("U,50,0;")!=std::string::npos && noRepop.events.find("P,50,0;")==std::string::npos);
}
// 같은 Client DWORD를 첫 finder 뒤 읽고 진단의 현재값 변경을 다음 호출에서도 유지한다.
TEST_CASE(AltarReleaseParent_ActualClientCheckingOwner) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));auto row=AltarReleaseRows().front();row[1]="12";row[3]="0";row[4]="0";AltarReleaseScene scene(row);auto find=scene.hooks.findDais;auto diagnostic=scene.hooks.assertion;std::uint32_t calls{};
    scene.hooks.findDais=[&](Sid sid) { client.GameplayGlobals()->AiDebug().Checking()=std::bit_cast<std::int32_t>(0x80000000U);return find(sid); };scene.hooks.assertion=[&](auto expression,auto file,auto line) { ++calls;diagnostic(expression,file,line);client.GameplayGlobals()->AiDebug().Checking()=0; };RunAltarReleaseParent(Sid{50},RawCheckingFlag{client.GameplayGlobals()->AiDebug().Checking()},scene.hooks);CHECK(calls==1 && scene.directions==1 && !client.GameplayGlobals()->AiDebug().Checking());
    scene.hooks.findDais=std::move(find);scene.hooks.direction={};RunAltarReleaseParent(Sid{50},RawCheckingFlag{client.GameplayGlobals()->AiDebug().Checking()},scene.hooks);CHECK(calls==1 && scene.directions==1); // 같은 owner의0에서는 미선택 방향/진단을 호출하지 않는다.
}
