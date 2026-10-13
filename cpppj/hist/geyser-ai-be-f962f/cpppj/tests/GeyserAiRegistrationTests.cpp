// null AI 자연 반환과 첫 명시 하위 거부를 원본 두 PE 관찰 및 공유 Player 현재바이트로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGeyserAiRegistration.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 새 독립 기계어 fixture만 읽으며 첫 AI 번호나 완료 여부를 C++ 기대값으로 만들지 않는다.
const std::vector<std::vector<std::string>>& GeyserAiRows() {
    static const auto rows=LoadFixture(NETSTORM_GEYSER_AI_REGISTRATION_FIXTURE).rows;return rows;
}
// 같은 저장소를 참조하는 등록 객체에 현재 입력 AI 포인터들을 배치한다.
void PrepareGeyserPlayers(std::span<std::uint8_t> players,std::size_t stride,std::size_t ai,
    const std::vector<std::uint32_t>& input) {
    std::fill(players.begin(),players.end(),0xa5);
    // 원본 Player0..8의 실제 AI 필드만 초기화하며 나머지 padding은 A5 그대로 둔다.
    for (std::size_t owner=0;owner<9;++owner) Put(players,owner*stride+ai,0);
    Put(players,ai,input[2]);
    if (input[3]) {
        Put(players,input[3]*stride+ai,input[4]);
        if (input[5]) Put(players,(input[3]==3 ? 7U : 2U)*stride+ai,0xfedcba98);
    }
}
// 양판본 각 정상84/하위거부218을 같은 전체 Player 저장소에서 실행하고 원본 출력을 읽는다.
void ReplayGeyserAi(const char* edition) {
    const bool patch=std::string_view(edition)=="originals";const std::size_t stride=patch ? 0xb4U : 0xacU,ai=patch ? 0xb0U : 0xa8U;
    std::vector<std::uint8_t> players(9*stride);RawGeyserAiRegistration registration(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,players);
    std::size_t normal=0,rejected=0;
    // fixture 모든 입력의 분류·첫 요청 DWORD·전체 바이트를 대조한다.
    for (const auto& row:GeyserAiRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==6);std::vector<std::uint32_t> input;
        // 원본 실행기로 전달한 DWORD를 부호 없이 그대로 읽는다.
        for (const auto& value:Split(row[1],',')) input.push_back(Number(value));
        CHECK(input.size()==6);PrepareGeyserPlayers(players,stride,ai,input);const auto before=players;
        std::string kind="normal",request="-";
        try { registration.Register(input[0],input[1]); }
        catch (const GeyserAiRegistrationBoundary& error) {
            kind="boundary";request=std::to_string(error.owner)+','+std::to_string(error.aiPointer)+','+
                std::to_string(error.sid)+','+std::to_string(error.state);
        }
        const bool same=kind==row[2] && request==row[4] && players==before && Adler(players)==Number(row[5]);
        if (!same) std::fprintf(stderr,"GeyserAi %s input=%s kind=%s request=%s expected=%s/%s\n",edition,row[1].c_str(),kind.c_str(),request.c_str(),row[2].c_str(),row[4].c_str());
        CHECK(same);normal+=kind=="normal";rejected+=kind=="boundary";
    }
    CHECK(normal==84 && rejected==218);
}
}
// 10.78의 실제 Player+b0 조회 및 첫 callback 경계를 전수 대조한다.
TEST_CASE(GeyserAiRegistration_ReplaysPatch) { ReplayGeyserAi("originals"); }
// CD10.72의 실제 Player+a8 조회 및 inlined 첫 하위 경계를 전수 대조한다.
TEST_CASE(GeyserAiRegistration_ReplaysCd) { ReplayGeyserAi("originalCD"); }
// 등록 객체를 다시 만들지 않아도 같은 현재 Player AI 변경을 읽고 Player0은 무시한다.
TEST_CASE(GeyserAiRegistration_ReadsSharedCurrentPlayersAndRejectsMissingStorage) {
    // 판본별 실제 9기록을 장기 소유하는 입력이며 별도 AI 힙을 생성하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const std::size_t stride=edition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU,ai=stride-4;
        std::vector<std::uint8_t> players(9*stride);RawGeyserAiRegistration registration(edition,players);Put(players,ai,123);registration.Register(0xffffffff,0xffffffff);
        Put(players,8*stride+ai,0x12345678);bool rejected=false;
        try { registration.Register(50,1); }
        catch (const GeyserAiRegistrationBoundary& error) { rejected=error.owner==8 && error.aiPointer==0x12345678 && error.sid==50 && error.state==1; }
        CHECK(rejected);Put(players,8*stride+ai,0);registration.Register(50,1);Put(players,stride+ai,77);rejected=false;
        try { registration.Register(50,0); }
        catch (const GeyserAiRegistrationBoundary& error) { rejected=error.owner==1 && error.aiPointer==77 && error.sid==50 && error.state==0; }
        CHECK(rejected);bool missing=false;
        try { RawGeyserAiRegistration invalid(edition,std::span<const std::uint8_t>(players).first(8*stride)); }
        catch (const std::invalid_argument&) { missing=true; }
        CHECK(missing);
    }
}
