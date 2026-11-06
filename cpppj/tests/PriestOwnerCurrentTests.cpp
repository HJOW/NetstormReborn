// 사제 소유자 본문의 현재 진단/알림/공통 부모 뒤 부분 효과를 실제 PE 관측과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPriestOwner.h"
#include <utility>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 진단과 알림이 바꾼 현재 모드/WORD 및 부모 중단을 독립144행과 비교한다.
TEST_CASE(PriestOwnerCurrent_IndependentNative144LocalInputs) {
    std::size_t count{}; // 실제로 대조한 최초 관측 행 수다.
    for (const auto& row:LoadFixture(NETSTORM_PRIEST_OWNER_CURRENT_FIXTURE).rows) { // 실제 게임/통신/포획 없이 두 판본의 사제 부모만 실행한다.
        CHECK(row.size()==6);if (row.size()!=6) continue;const bool patch=row[0]=="originals";const auto args=Split(row[1],',');CHECK(args.size()==4);if (args.size()!=4) continue;
        const auto battle=Number(args[0]),loading=Number(args[1]),player=Number(args[2]),profile=Number(args[3]);
        SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768);const auto slot=pool.Slot(Sid{5});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()};
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*29+7); // Native와 같은 입력만 구성하며 기대 결과는 읽기만 한다.
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=158;raw[11]=4;Put(raw,12,0xab83,2);const auto ownerOffset=patch ? 34U : 32U;raw[ownerOffset]=3;
        SquidOwnerMode mode{true,battle!=0,false};PriestOwnerState state{loading,2,3};std::string events,cause="returned";std::uint32_t notices{}; // 명시 입력과 실제 호출 결과다.
        const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 선택된 순서를 그대로 직렬화한다.
        PriestOwnerHooks hooks;
        hooks.assertion=[&](auto expression,auto file,auto line) {
            event("D"+std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line));
            if (profile==1) { mode.battle=true;mode.fort=false;state.loadingDepth=0;state.battlePlayer=0;Put(raw,12,0xcc81,2); }
        }; // 진단이 정상 복귀하며 변경한 현재 상태도 원문 후단이 다시 읽어야 한다.
        hooks.notify=[&](Sid sid) {
            event("N"+std::to_string(sid.value)+','+std::to_string(Get(raw,12,2))+','+std::to_string(raw[ownerOffset]));++notices;
            if (profile==4) throw std::string("notify");
            if (profile==2 && notices==1) { mode.battle=false;mode.fort=false;state.loadingDepth=1;Put(raw,12,0xbb07,2); }
        }; // 무상태 알림의 호출 경계와 명시 변경/중단 장치를 구분한다.
        hooks.base=[&](Sid,std::uint32_t owner) {
            event("B"+std::to_string(owner)+','+std::to_string(Get(raw,12,2))+','+std::to_string(raw[ownerOffset]));if (profile==5) throw std::string("base");raw[ownerOffset]=static_cast<std::uint8_t>(owner);
            if (profile==3) { raw[11]|=1;raw[10]=255;Put(raw,12,0xcc81,2);mode.battle=false;mode.fort=true;state.loadingDepth=0;state.fortPlayer=2; }
        }; // 공통 부모 진입 뒤에 TYPE/free가 바뀌어도 사제 후단이 선행 거부하지 않는지 확인한다.
        RawPriestOwner owner(pool,mode,state,std::move(hooks));try { owner.Set(Sid{5},player); }catch (const std::string& value) { cause=value; }
        const auto current=std::to_string(mode.fort)+','+std::to_string(mode.battle)+','+std::to_string(state.loadingDepth)+','+std::to_string(state.fortPlayer)+','+std::to_string(state.battlePlayer);
        CHECK(cause==row[2] && (events.empty()?"-":events)==row[3] && current==row[4] && Hex(raw)==row[5]);++count;
    }
    CHECK(count==144);
}

// CD의 범위 진단은 원래 소유자 WORD/알림 뒤에 호출되며 중단해도 이미 바뀐 WORD는 남는다.
TEST_CASE(PriestOwnerCurrent_CdCommonDiagnosticKeepsEarlierWord) {
    for (const bool stop:{false,true}) { // challenge0·비전투·비편집의 로컬 공통 진단만 확인한다.
        SidPool pool(OriginalEdition::Cd1072,32768);std::vector<RiftTypeRecord> types(171);const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);Put(raw,0,kCdPriestVtable);raw[10]=158;raw[11]=4;Put(raw,12,0xab83,2);raw[32]=3;
        SquidPostPopState book;SquidOwnerMode mode;SquidOwner base(pool,types,book,mode);std::uint32_t diagnostics{},notices{};PriestOwnerState state{1,2,3}; // 실제 공통 모듈과 사제 몸체가 같은 풀/모드를 읽는다.
        base.BindDiagnostics({types,[&](auto expression,auto file,auto line) { ++diagnostics;CHECK(expression=="newPlayerId == INVALID_PLAYER_ID ||( FIRST_REAL_PLAYER <= newPlayerId && newPlayerId < MAX_PLAYERS)" && file=="\\ns\\o\\Squid.cpp" && line==0x9c1);CHECK(Get(raw,12,2)==0xab89 && notices==1);if (stop) throw std::string("diagnostic"); }});
        RawPriestOwner owner(pool,mode,state,{[&](Sid target,std::uint32_t value) { base.Set(target,value); },[&](Sid) { ++notices; },{}});
        bool stopped=false;try { owner.Set(sid,9); }catch (const std::string&) { stopped=true; }
        CHECK(stopped==stop && diagnostics==1 && notices==1 && Get(raw,12,2)==0xab89 && raw[32]==(stop ? 3 : 9));
    }
}
