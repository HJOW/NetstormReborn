// 실제 x86 주변음/재예약/바람을 대조하고 실제 Client의 공유 기한/난수/비활성 음향을 확인한다.
#include "RawSceneSupport.h"
#include "o/RawAmbientSound.h"
#include "o/RawAmbientWind.h"
#include "client/ClientMain.h"
#include "client/ClientAudio.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientRandomGlobals.h"
#include "client/ExistingEditorAfterCheckpoint.h"
#include <bit>
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 기대는 독립 CPU TSV만 사용하고 경계 장치는 명시 입력/요청만 기록한다.
const std::vector<std::vector<std::string>>& AmbientRows() { static const auto rows=LoadFixture(NETSTORM_AMBIENT_SOUND_FIXTURE).rows;return rows; } // 원문 입력/출력 전체를 읽는다.
std::uint64_t Wide(const std::string& value) { return std::stoull(value); } // 시각은 double 변환 없이 원래64비트를 읽는다.
// 실제 요청 인자를 변환 없이 관측한다. 구현 결과로 기대 파일을 만들지 않는다.
void SoundTrace(std::string& trace,std::string_view name,std::uint32_t loop,std::int32_t volume,std::int32_t pan,std::uint32_t priority,std::int32_t limit) {
    trace+="S,"+std::string(name)+','+std::to_string(loop)+','+std::to_string(volume)+','+std::to_string(pan)+','+std::to_string(priority)+','+std::to_string(limit)+";";
}
// 각 행의 독립 초기 전역을 만들고 행 안에서는 동일 기한/난수/바람을 유지한다.
std::vector<std::string> ReplayAmbient(const std::vector<std::string>& row) {
    const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;double deadline=std::bit_cast<double>(Wide(row[3]));std::int32_t kind=std::stoi(row[4]),forced=std::stoi(row[5]);std::uint32_t dialog=Number(row[6]);double wall=std::bit_cast<double>(Wide(row[7]));const double increment=std::bit_cast<double>(Wide(row[8]));const auto steps=Number(row[9]),mode=Number(row[10]),action=Number(row[11]);GameRandom random(Number(row[2]));AmbientWindState wind;std::uint32_t calls{};std::string trace;
    AmbientSoundHooks hooks;hooks.wallSeconds=[&] { const double now=wall;wall+=increment;++calls;trace+="W,"+std::to_string(std::bit_cast<std::uint64_t>(now))+";";if ((action&4U) && calls==2) { forced=1;kind=2; }return now; };
    hooks.sound=[&](auto name,auto loop,auto volume,auto pan,auto priority,auto limit) { SoundTrace(trace,name,loop,volume,pan,priority,limit);if (action&1U) dialog=0;if (action&2U) dialog=0x80000000U;if (action&8U) deadline=-4.25; };
    hooks.currentDialog=[&] { return dialog; };hooks.flash=[&](auto brightness) { trace+="F,"+std::to_string(brightness)+";"; };
    if (mode&1U) ResetAmbientSoundDeadline(deadline,hooks.wallSeconds);
    // 원문 Client처럼 소리 부모 바로 뒤에서 같은 난수를 소비하는 바람을 실행한다.
    for (std::uint32_t step=0;step<steps;++step) { StepAmbientSound({deadline,kind,forced},random,hooks);if (mode&2U) StepAmbientWind(wind,random,edition); }
    return {std::to_string(std::bit_cast<std::uint64_t>(deadline)),std::to_string(kind),std::to_string(forced),std::to_string(dialog),std::to_string(random.State()),std::to_string(calls),trace.empty() ? "-" : trace,std::to_string(std::bit_cast<std::uint32_t>(wind.x)),std::to_string(std::bit_cast<std::uint32_t>(wind.y)),std::to_string(std::bit_cast<std::uint32_t>(wind.angle))};
}
}
// 양판본의 시간/난수/종류/강제값/모달/요청 순서와 결합 바람1272행을 직접 비교한다.
TEST_CASE(AmbientSound_ReplaysNativeCpuWithResetAndWindOrder) {
    std::size_t count{};
    // 첫 불일치의 전체 입력 번호와 필드를 기록하고 나머지 행도 대조한다.
    for (const auto& row:AmbientRows()) {
        CHECK(row.size()==22);const auto actual=ReplayAmbient(row);const std::vector<std::string> expected(row.begin()+12,row.end());CHECK(actual==expected);
        if (actual!=expected) { for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("주변음 %s 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),row[1].c_str(),index,expected[index].c_str(),actual[index].c_str()); } // 각 필드의 Native 원시값을 보존한다.
        ++count;
    }
    CHECK(count==1272);
}
// 실제 Client는 noAudio/게임 정지에서도 같은 난수를 소비하고 실제 이름표에 요청을 남긴다.
TEST_CASE(AmbientSound_ActualNoAudioClientUsesSameOwnerAndNameTable) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));client.InitializeNativeAudio();
    auto owner=client.LocalStateGlobals();auto view=owner->AmbientSound();CHECK(&view.deadline==&owner->ActivityEnd() && view.deadline==0 && view.kind==0 && view.forced==0);view.kind=2;view.forced=1;view.deadline=-1;client.RandomGlobals()->Game().SetState(55);client.Pause(true);
    auto& names=client.WorldAudio().Names();const auto before=names.FirstFree();client.UpdateAmbientSound();
    const auto& rows=AmbientRows();const auto native=std::find_if(rows.begin(),rows.end(),[](const auto& row) { return row[0]=="originals" && Number(row[2])==55 && Wide(row[3])==0 && row[4]=="2" && row[5]=="1" && Number(row[6])==0 && Number(row[10])==0 && Number(row[11])==0; });
    CHECK(native!=rows.end());if (native!=rows.end()) CHECK(client.RandomGlobals()->Game().State()==Number((*native)[16]));CHECK(names.Name(before)=="whale.wav" && names.FirstFree()>before && !client.WorldAudio().DeviceReady() && !client.Audio());
    const auto after=names.FirstFree(),seed=client.RandomGlobals()->Game().State();owner->ActivityEnd()=1.0e300;client.UpdateAmbientSound();CHECK(names.FirstFree()==after && client.RandomGlobals()->Game().State()==seed && view.deadline==1.0e300);client.Pause(false);
}
// 기존 Editor/메인 State tail의 재예약이 새 주변음 부모에도 즉시 보이는 같은8BYTE인지 확인한다.
TEST_CASE(AmbientSound_ExistingEditorTailSharesDeadlineAndCurrentClock) {
    ClientLocalStateGlobals owner;auto view=owner.AmbientSound();InputQueue input;owner.State().current[0]=3;ExistingEditorAfterCheckpointEffects tail;
    tail.hud=[](auto) {};tail.wallSeconds=[] { return 100.25; };tail.requestMusic=[](auto) {};tail.fortPalette=[] {};ApplyExistingEditorAfterCheckpoint(owner,input,tail);CHECK(view.deadline==110.25 && &view.deadline==&owner.ActivityEnd());
    GameRandom random(55);std::uint32_t calls{};AmbientSoundHooks hooks;hooks.wallSeconds=[&] { ++calls;return 110.0; };StepAmbientSound(view,random,hooks);CHECK(calls==1 && random.State()==55 && view.deadline==110.25);
    hooks.wallSeconds=[&] { ++calls;return calls==2 ? 110.25 : 110.5; };view.forced=1;view.kind=3;StepAmbientSound(view,random,hooks);CHECK(calls==3 && view.deadline==230.5 && owner.ActivityEnd()==230.5 && random.State()==55);
}
