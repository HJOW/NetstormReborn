// 독립 원본630입력의 signed 연산·선택된 현재 가상 효과와 반환을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDecorationRegular.h"
#include <bit>
#include <iostream>
using namespace netstorm::test::rawscene; //원본 fixture 입력/바이트 서식만 공유한다.
namespace { //합성 raw 입력과 장치 관찰만 검사 지역에 둔다.
struct DecorationScene { //원본127BYTE의 분기는 제품 본문만 실행한다.
    SidPool pool{OriginalEdition::Patch1078,24000,true}; //현재 부모50의 같은 물리 슬롯이다.
    bool boss{}; //현재 서버 권한의 명시 입력이다.
    std::vector<std::uint32_t> row; //원본 입력7DWORD이며 기대값을 계산하지 않는다.
    std::string trace; //실제 선택한 효과 순서다.
    std::span<std::uint8_t> Raw() { const auto slot=pool.Slot(Sid{50});return {const_cast<std::uint8_t*>(slot.data()),slot.size()}; } //fixture가 원래 물리 입력50BYTE를 직접 채운다.
    void Prepare(const std::string& input) { row.clear();for (const auto& field:Split(input,',')) row.push_back(Number(field));std::fill(Raw().begin(),Raw().end(),std::uint8_t{0xa5});Put(Raw(),0,0x11010000U);Raw()[10]=100;Put(Raw(),26,row.at(5));boss=row.at(3)!=0;trace.clear(); } //원본과 같은 입력 바이트만 재배치한다.
    void Event(const std::string& value) { if (!trace.empty()) trace+=';';trace+=value; } //현재 효과 문자열을 원문 관찰 형식으로 보존한다.
    DecorationRegularHooks Hooks() { //원본과 같은 큰 경계만 제공하며 HP 감소 판단은 대체하지 않는다.
        DecorationRegularHooks hooks;
        hooks.maxHitPoints=[this](auto sid) { CHECK(sid.value==50);Event("M:50");if (row[6]) { Put(Raw(),0,0x11020000U);Put(Raw(),26,77); }return std::bit_cast<std::int32_t>(row[4]); };
        hooks.hitPoints=[this](auto sid,auto amount,auto owner,auto attacker) { CHECK(sid.value==50);Event(std::string(Get(Raw(),0)==0x11020000U ? "h:":"H:")+std::to_string(std::bit_cast<std::uint32_t>(amount))+":"+std::to_string(owner)+":"+std::to_string(attacker)); };
        hooks.destroy=[this](auto sid,auto flags) { CHECK(sid.value==50);Event(std::string(Get(Raw(),0)==0x11020000U ? "d:":"D:")+std::to_string(flags)); };
        hooks.damageable=[this](auto sid,auto event,auto count,auto payload) { CHECK(sid.value==50);Event("B:"+std::to_string(event)+":"+std::to_string(count)+":"+std::to_string(std::bit_cast<std::uint32_t>(payload)));return 0.375F; };
        return hooks;
    }
};
}
TEST_CASE(DecorationRegular_Original630Inputs) { //기대 반환/현재 raw/현재VT 효과는 독립 원본 실행 결과만 읽는다.
    DecorationScene scene;std::size_t count{},diagnostics{};
    for (const auto& fields:LoadFixture(NETSTORM_DECORATION_REGULAR_FIXTURE).rows) { //모든 독립 입력의 원문 효과와 최종50BYTE를 비교한다.
        CHECK(fields.size()==4);if (fields.size()!=4) continue;scene.Prepare(fields[0]);const auto result=RunDecorationRegular(scene.pool,scene.boss,Sid{50},scene.row[0],scene.row[1],std::bit_cast<float>(scene.row[2]),scene.Hooks());const auto trace=scene.trace.empty() ? "-":scene.trace;
        if ((trace!=fields[2] || Hex(scene.Raw())!=fields[3]) && diagnostics++<5) std::cout<<fields[0]<<"\nactual="<<trace<<"\noriginal="<<fields[2]<<'\n';
        CHECK(std::bit_cast<std::uint32_t>(result)==Number(fields[1]));CHECK(trace==fields[2]);CHECK(Hex(scene.Raw())==fields[3]);++count;
    }CHECK(count==630);
}
TEST_CASE(DecorationRegular_LazySelectedEffectsAndEdition) { //권한 없는 종료와 비0 사건은 최대HP/삭제/피격을 앞당기지 않는다.
    SidPool pool(OriginalEdition::Patch1078,24000,true);bool boss=false;DecorationRegularHooks hooks;CHECK(RunDecorationRegular(pool,boss,Sid{},0,3,9.0F,hooks)==-1.0F);hooks.damageable=[](auto,auto,auto,auto) { return 0.125F; };CHECK(RunDecorationRegular(pool,boss,Sid{},1,0,9.0F,hooks)==0.125F);
    bool failed{};boss=true;try { RunDecorationRegular(pool,boss,Sid{},0,3,9.0F,hooks); }catch (const std::logic_error&) { failed=true; }CHECK(failed);
    SidPool cd(OriginalEdition::Cd1072,15000,true);failed=false;try { RunDecorationRegular(cd,boss,Sid{},0,3,9.0F,hooks); }catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
}
