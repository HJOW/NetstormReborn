//원문Reset13BYTE의raw보존범위와기존GameClock공개호출열을구별해대조한다.
#include "TestSupport.h"
#include "o/GameClock.h"
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
using namespace netstorm::o;
//고정원시표의각칸을마지막빈칸까지분리한다.
std::vector<std::string> ClockResetFields(std::string_view text,char separator) {
    std::vector<std::string> fields;std::size_t first{}; //아직소비하지않은원시칸의위치다.
    for (;;) { //마지막한칸을보존한뒤반환한다.
        const auto end=text.find(separator,first);fields.emplace_back(text.substr(first,end==text.npos?text.npos:end-first));if (end==text.npos) return fields;first=end+1;
    }
}
//실제double반환비트를Nativelittleendian출력과같은16자리hex로표현한다.
std::string ClockResetDouble(double value) {
    constexpr std::string_view digits="0123456789abcdef";const auto bits=std::bit_cast<std::array<std::uint8_t,8>>(value);std::string result; //호스트GameClock반환값자체의비트다.
    for (const auto byte:bits) { result+=digits[byte>>4];result+=digits[byte&15U]; }return result; //반올림허용오차로차이를감추지않는다.
}
}

//Native공개호출114행은C++와비교하고raw24행은별도원문전역불변증거로만확인한다.
TEST_CASE(GameClockReset_IndependentNative138AndPublic114) {
    static_assert(std::is_same_v<decltype(&GameClock::Reset),void(GameClock::*)()>); //Reset은현재시각이나난수인자를요구하지않는다.
#ifdef NETSTORM_GAME_CLOCK_RESET_FIXTURE
    const std::filesystem::path path=NETSTORM_GAME_CLOCK_RESET_FIXTURE; //공용등록은최초자료의고정절대경로를공급한다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/game-clock-reset-x86.tsv"; //격리사본은별도정의로같은자료를공급한다.
#endif
    std::ifstream input(path);if (!input) throw std::runtime_error("GameClockReset최초fixture없음");std::string line,key;std::unique_ptr<GameClock> clock;std::size_t rows{},rawRows{},publicRows{},resets{};
    while (std::getline(input,line)) { //두정밀도의모든원문행을빠짐없이분류한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=ClockResetFields(line,'\t');if (row.size()!=13) throw std::logic_error("GameClockReset TSV칸수");++rows;
        CHECK(row[0]=="639" || row[0]=="895");CHECK(row[9].size()==192 && row[10]=="efcdab8900ceab8911ceab8922ceab89");
        if (row[1].starts_with("raw")) { //C++객체private바이트를Native배치라고가장하지않는다.
            CHECK(row[6]=="Reset" && row[9]==row[3].substr(0,176)+"0000000000000000" && row[11]=="0");++rawRows;continue;
        }
        const auto current=row[0]+':'+row[1];if (current!=key) { key=current;clock=std::make_unique<GameClock>(static_cast<std::uint32_t>(std::stoul(row[2])));CHECK(row[5]=="0"); }
        const auto now=static_cast<std::uint32_t>(std::stoul(row[7]));std::string result="-"; //원시DWORD시각입력을그대로공급한다.
        if (row[6]=="Game") result=ClockResetDouble(clock->GameSeconds(now));
        else if (row[6]=="Wall") result=ClockResetDouble(clock->WallSeconds(now));
        else if (row[6]=="IsPaused") result=clock->IsPaused()?"1":"0";
        else if (row[6]=="Pause") clock->Pause(now);
        else if (row[6]=="Resume") clock->Resume(now);
        else if (row[6]=="Reset") { clock->Reset();++resets;CHECK(row[11]=="0"); }
        else throw std::logic_error("GameClockReset미해석명령");
        CHECK(result==row[8]);++publicRows;
    }
    CHECK(rows==138 && rawRows==24 && publicRows==114 && resets==22);
}

//누적정지만지워현재Frame의번호/이전game기준과원점을유지하며즉시Capture하지않는다.
TEST_CASE(GameClockReset_PreservesOriginAndCapturedFrameHistory) {
    GameClock clock(1000);const auto first=clock.Capture(2000);CHECK(first.game==1 && first.wall==1 && first.delta==1 && first.number==1);
    clock.Pause(2000);clock.Resume(5000);const auto previous=clock.Capture(6000);CHECK(previous.game==2 && previous.wall==5 && previous.delta==1 && previous.number==2);
    clock.Reset();CHECK(!clock.IsPaused() && clock.GameSeconds(6000)==5 && clock.WallSeconds(6000)==5);
    const auto next=clock.Capture(6000);CHECK(next.game==5 && next.wall==5 && next.delta==3 && next.number==3); //Reset이frame을비우거나미리증가시키면실패한다.
    clock.Reset();const auto repeated=clock.Capture(6000);CHECK(repeated.game==5 && repeated.wall==5 && repeated.delta==0 && repeated.number==4);
}

//중첩정지와저장게임시각은유지하며마지막Resume만원래계산을수행한다.
TEST_CASE(GameClockReset_KeepsNestedPauseAndHeldGameTime) {
    GameClock clock(1000);clock.Pause(2000);clock.Pause(2500);clock.Reset();CHECK(clock.IsPaused() && clock.GameSeconds(8000)==1);
    clock.Resume(9000);CHECK(clock.IsPaused() && clock.GameSeconds(10000)==1);clock.Reset();clock.Resume(11000);
    CHECK(!clock.IsPaused() && clock.GameSeconds(12000)==2 && clock.WallSeconds(12000)==11);
    const auto first=clock.Capture(12000);CHECK(first.game==2 && first.delta==2 && first.number==1);clock.Reset();
    const auto next=clock.Capture(12000);CHECK(next.game==11 && next.delta==9 && next.number==2); //pausedTotal이외저장소의초기화를검출한다.
}
