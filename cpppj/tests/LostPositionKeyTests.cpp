// 최초 원문 U/u 관측을 실제 제품 helper와 대조하며 공통 부모의 외부 모달 관문을 구별한다.
#include "RawSceneSupport.h"
#include "client/LostPositionKey.h"
#include <filesystem>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

// 원문 부모의 모든 행을 읽고 U/u는 호출 순서·raw 좌표·부분 중단까지 같은 제품 함수로 확인한다.
TEST_CASE(LostPositionKey_IndependentNative1022Inputs) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/lost-position-key-x86.tsv").string().c_str());std::size_t count{},modalRows{},otherRows{},outsideRows{};
    for (const auto& row:fixture.rows) { //선행 뗌/모달·현재 정지·두 축 경계·수정키와 다른 사건을 모두 방문한다.
        CHECK(row.size()==18);if (row.size()!=18) continue;const auto event=Number(row[0]),key=event&0x03ffffffU;float x=std::bit_cast<float>(Number(row[1])),y=std::bit_cast<float>(Number(row[2]));auto cells=std::bit_cast<std::int32_t>(Number(row[3]));std::string trace;bool completed=true;LostPositionKeyHooks hooks;
        hooks.paused=[&] { trace="P:"+row[4];if (Number(row[6])) { x=std::bit_cast<float>(Number(row[7]));y=std::bit_cast<float>(Number(row[8])); }return Number(row[4])!=0; };
        hooks.center=[&](float first,float second) { trace+=";C:"+std::to_string(std::bit_cast<std::uint32_t>(first))+':'+std::to_string(std::bit_cast<std::uint32_t>(second));if (Number(row[9])) throw std::runtime_error("원문 Center 명시 중단 입력"); };
        if (!(event&InputCode::kRelease) && Number(row[5])) { //모달 거부는 Uber의 외부 선행 관문이며 U helper의 기능으로 합산하지 않는다.
            CHECK(row[17]=="modal" && row[12]=="-" && Number(row[14])==0);++modalRows;
        } else if (!(event&InputCode::kRelease) && key!='U' && key!='u') { //전체 키 부모는 다른 키도 정지/strchr를 거치지만 U 전용 helper는 실제 장치를 조회하지 않는다.
            CHECK(!DispatchLostPositionKey({event,0,0},x,y,cells,{}));CHECK(row[17]=="key" && Number(row[14])==0 && row[12].find("C:")==std::string::npos);++otherRows;
        } else {
            bool accepted=false;
            try { accepted=DispatchLostPositionKey({event,0,0},x,y,cells,hooks); }catch (const std::runtime_error&) { completed=false; }
            if (completed) CHECK(accepted==!(event&InputCode::kRelease));
            CHECK((trace.empty() ? "-" : trace)==row[12]);
        }
        CHECK(std::bit_cast<std::uint32_t>(x)==Number(row[10]) && std::bit_cast<std::uint32_t>(y)==Number(row[11]) && completed==(Number(row[13])!=0));
        if (event&InputCode::kOutside) ++outsideRows;++count;
    }
    CHECK(count==1022 && count==fixture.rows.size() && modalRows && otherRows && outsideRows);
}

//정지 callback가 바꾼 같은 현재 좌표/세계 크기를 읽으며 무효점에서는 Center 장치를 요구하지 않는다.
TEST_CASE(LostPositionKey_CurrentReferencesAndLazyBoundaries) {
    float x=-10000,y=-10000;std::int32_t cells=1;LostPositionKeyHooks hooks;
    for (const auto event:std::array{InputCode::kRelease|std::uint32_t('U'),InputCode::kCommand|std::uint32_t('U'),0x550000U,std::uint32_t('V')}) CHECK(!DispatchLostPositionKey({event,0,0},x,y,cells,hooks)); //뗌/일반 명령/가상키와 다른 글자는 실제 정지/카메라 장치가 없어도 거부된다.
    bool threw=false;try { static_cast<void>(DispatchLostPositionKey({'U',0,0},x,y,cells,hooks)); }catch (const std::logic_error&) { threw=true; }CHECK(threw);
    hooks.paused=[&] { x=111.25F;y=121.5F;cells=256;return false; };std::uint32_t calls{};hooks.center=[&](float first,float second) { CHECK(first==111.25F && second==121.5F);++calls; };
    CHECK(DispatchLostPositionKey({InputCode::kOutside|InputCode::kShift|InputCode::kControl|InputCode::kAlt|std::uint32_t('u'),0,0},x,y,cells,hooks) && calls==1); //outside는 키 마스크로 제거되며 수정키는 새 거부 조건이 아니다.
    hooks.paused=[&] { cells=100;return false; };hooks.center={};CHECK(DispatchLostPositionKey({'U',0,0},x,y,cells,hooks)); //같은 float가 새 세계 상한 밖이면 Center 미연결도 선택하지 않는다.
    hooks.paused=[&] { cells=256;return false; };threw=false;try { static_cast<void>(DispatchLostPositionKey({'u',0,0},x,y,cells,hooks)); }catch (const std::logic_error&) { threw=true; }CHECK(threw && x==111.25F && y==121.5F);
    hooks.paused=[] { return true; };CHECK(DispatchLostPositionKey({'U',0,0},x,y,cells,hooks)); //정지는 유효점이어도 카메라 장치를 요구하지 않는다.
}

//즉시 카메라가 실패한 뒤 원문 좌표를 지우거나 성공으로 합성하지 않고 부분 효과를 보존한다.
TEST_CASE(LostPositionKey_CenterPartialFailurePreservesCurrentData) {
    float x=41.25F,y=73.5F;std::int32_t cells=256;LostPositionKeyHooks hooks;hooks.paused=[] { return false; };std::string trace;
    hooks.center=[&](float first,float second) { CHECK(first==41.25F && second==73.5F);trace="center";x=111.25F;y=121.5F;throw std::runtime_error("실제 카메라 중단 경계"); };bool threw=false;
    try { static_cast<void>(DispatchLostPositionKey({'U',0,0},x,y,cells,hooks)); }catch (const std::runtime_error&) { threw=true; }
    CHECK(threw && trace=="center" && x==111.25F && y==121.5F && cells==256);
}
