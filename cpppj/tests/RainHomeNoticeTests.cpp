// 맨 오 워 사망 통지의 실제48바이트/난수/현재 갱신 요청을 원문과 비교한다.
#include "RawSceneSupport.h"
#include "o/RawRainAviaryProcess.h"
#include "o/SpStore.h"

using namespace netstorm::test::rawscene;
namespace {
// 독립 원문 hex를 입력/기대 바이트로만 읽는다.
std::vector<std::uint8_t> Decode(std::string_view value) {
    std::vector<std::uint8_t> bytes;
    // 두 문자마다 바이트 하나를 복원한다.
    for (std::size_t index=0;index<value.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(value.substr(index,2)),nullptr,16)));
    return bytes;
}
}
// finder/Host lookup 뒤 선택된 실제 프로세스의 stage/time/dirty와 정수 지연을 비교한다.
TEST_CASE(RainAviaryHomeNotice_NativeStateDelayAndRefresh) {
    SidPool pool(OriginalEdition::Patch1078,24000,false);bool fort=false,friendly=false,server=false,checking=false;std::uint32_t editor=0,processType=60,flyerType=144;double now=0;
    RawRainAviaryProcess logic(pool,{fort,friendly,server,editor,checking,now,processType,flyerType},{});std::size_t count=0;
    //36개 실제 입력에서 전체 필드와 두 정밀도 공통 원문을 대조한다.
    for (const auto& row:LoadFixture(NETSTORM_RAIN_HOME_NOTICE_FIXTURE).rows) {
        if (row.size()!=8) throw std::runtime_error("맨 오 워 홈 통지 fixture 열 수 오류");server=Number(row[0])!=0;GameRandom random(Number(row[1]));now=std::bit_cast<double>(std::stoull(row[2]));const auto before=Decode(row[4]);if (before.size()!=48) throw std::runtime_error("맨 오 워 홈 통지 raw 폭 오류");
        RainAviaryProcessFields fields;std::copy(before.begin(),before.end(),fields.bytes.begin());std::size_t refreshed=0;std::string event;
        logic.FlyerDied(fields,random,[&](RainAviaryProcessFields& current) { CHECK(&current==&fields && current.Form().value==80 && current.Parent().value==50);event="R:"+Hex(current.bytes);++refreshed; });
        CHECK(std::equal(fields.bytes.begin(),fields.bytes.end(),Decode(row[5]).begin()));CHECK(random.State()==Number(row[6]));CHECK(refreshed==(server ? 1U : 0U));
        if (server) { const auto offset=row[7].find(";R:");CHECK(offset!=std::string::npos);if (offset!=std::string::npos) CHECK(event==row[7].substr(offset+1)); }else CHECK(row[7]=="-" && event.empty());++count;
    }
    CHECK(count==36);
}
