// 원본 소유자 색 변경의 표 전체/풀 상태/프레임 요청을 독립 관찰 결과와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPlayerColor.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 9소유자·4색·전투2조건·3배치의216입력을 같은24000슬롯 풀에서 검사한다.
TEST_CASE(PlayerColor_Native216) {
    const auto fixture=LoadFixture(NETSTORM_PLAYER_COLOR_FIXTURE);CHECK(fixture.rows.size()==217 && fixture.rows.front()[0]=="input");
    for (std::size_t rowIndex=1;rowIndex<fixture.rows.size();++rowIndex) { // 원본 출력으로 다음 입력을 만들지 않고 매번 새 풀을 소유한다.
        const auto& row=fixture.rows[rowIndex];CHECK(row.size()==4);const auto input=Split(row[0],',');CHECK(input.size()==4);
        const auto owner=Number(input[0]),color=Number(input[1]),layout=Number(input[3]);SidPool pool{OriginalEdition::Patch1078,24000,false};BridgeConnectState state;state.islandType=94;state.stalagType=95;state.battle=Number(input[2])!=0;std::string events; // 독립 입력과 실제 변경/호출 기록이다.
        for (std::uint16_t index=5;index<=20;++index) { // 모든 입력 슬롯을 먼저 확보하고 원본 관찰기와 같은 바이트를 채운다.
            const auto sid=pool.Allocate(2);CHECK(sid==Sid{index});auto raw=pool.AllocatedBytes(sid);
            for (std::size_t byte=0;byte<raw.size();++byte) raw[byte]=static_cast<std::uint8_t>(index*13+byte*29+layout*17); // 프레임 외 필드의 보존도 함께 확인한다.
            raw[10]=std::array<std::uint8_t,4>{94,95,82,96}[(index+layout)%4];raw[11]=std::array<std::uint8_t,6>{0,1,2,4,128,34}[(index+layout)%6];raw[34]=static_cast<std::uint8_t>((index*3+layout)%9);
        }
        ChangePlayerColor(pool,state,owner,color,[&](Sid sid,std::int32_t frame,std::uint32_t flags) { if (!events.empty()) events+=';';events+=std::to_string(sid.value)+','+std::to_string(frame)+','+std::to_string(flags);Put(pool.AllocatedBytes(sid),36,std::bit_cast<std::uint32_t>(frame)); });
        CHECK(Hex({reinterpret_cast<const std::uint8_t*>(state.ownerColors.data()),36})==row[1]);CHECK(Hex(pool.Bytes().subspan(250,800))==row[2]);CHECK((events.empty() ? "-" : events)==row[3]);
    }
}
// 선택된 프레임 소비자의 실패는 원본처럼 먼저 쓴 색 표를 되돌리지 않는다.
TEST_CASE(PlayerColor_FrameFailureKeepsColorWrite) {
    SidPool pool{OriginalEdition::Patch1078,24000,false};const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);raw[10]=94;raw[34]=2;BridgeConnectState state;state.islandType=94;state.stalagType=95;
    CHECK(Throws([&] { ChangePlayerColor(pool,state,2,8,{}); }));CHECK(state.ownerColors[2]==8);
    CHECK(Throws([&] { ChangePlayerColor(pool,state,9,1,{}); }));
}
