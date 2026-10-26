// 실제 원문 입력/Guide 부모174행과 메뉴 인자·공유 이름·부분 실패를 대조한다.
#include "RawSceneSupport.h"
#include "client/EditorTestBattle.h"
#include "client/FortSelection.h"
#include "client/RawGameMenus.h"
#include "client/GameMenuCommands.h"
#include <algorithm>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 기존 검사 본문과 분리한 원문 입력 장면이다.
struct TestBattleStop {}; //선택된 큰 하위의 명시 중단을 정상 반환과 구별한다.
struct TestBattleScene { //두 고정 이름/Guide 현재와 실제 명령 부모가 공유할 최소 저장소다.
    std::array<std::array<char,256>,2> names{}; //빌린 주소를 유지하면서 해제 중 이름 길이를 바꿀 수 있다.
    std::uint32_t active{},pointer{},mode{},next{},newMode{7},newDemo{3}; //현재/주소/선택/예약과 별도 공장 객체의 두 필드다.
    std::string guide{"OldGuide"},pending{"old"},trace,mutation,stop; //Guide 이름과 pending은 현재 Fort 이름과 별개다.
    bool reentrant{}; //해제 중 생긴 새 현재를 한 번 더 정리하지 않는 원문 입력이다.
    explicit TestBattleScene(const std::vector<std::string>& row) { //최초14열만 입력으로 해석한다.
        active=Number(row[5]);pointer=Number(row[6]);mode=Number(row[7]);next=Number(row[8]);reentrant=Number(row[11])!=0;mutation=row[12];stop=row[13];
        Name(0,"OriginalFort");Name(1,"AlternateFort");
    }
    void Name(std::size_t index,std::string_view value) { //물리 주소/범위를 유지하고 첫 NUL 앞의 새 내용을 설치한다.
        if (value.size()>=names.at(index).size()) throw std::out_of_range("Test Battle 이름 범위");
        std::copy(value.begin(),value.end(),names[index].begin());names[index][value.size()]='\0';
    }
    void Effect(std::string_view trigger,std::string text) { //명시 변경과 실패는 해당 하위에서 발생한 순서로만 적용한다.
        if (!trace.empty()) trace+=';';trace+=text;
        if (mutation!="-") for (const auto& item:Split(mutation,';')) { //기대 출력으로 현재 상태를 구성하지 않는다.
            const auto fields=Split(item,'/');if (fields.size()!=3) throw std::logic_error("Test Battle 변경 폭");if (fields[0]!=trigger) continue;
            if (fields[1]=="name0" || fields[1]=="name1") Name(fields[1].back()=='0' ? 0U : 1U,fields[2]);
            else if (fields[1]=="pointer") pointer=Number(fields[2]);else if (fields[1]=="mode") mode=Number(fields[2]);
            else if (fields[1]=="next") next=Number(fields[2]);else if (fields[1]=="active") active=Number(fields[2]);
            else throw std::logic_error("Test Battle 미명시 변경 필드");
        }
        if (stop==trigger) throw TestBattleStop{};
    }
    EditorTestBattleHooks Hooks() { //실제 Guide/편집 예약 모듈은 같은 현재 저장소를 직접 사용한다.
        EditorTestBattleHooks hooks;hooks.snapshot=[this] { Effect("save","save"); };
        hooks.currentFortName=[this] { return std::string_view(names.at(pointer).data(),names.at(pointer).size()); }; //길이는 복사 시점의 NUL로 결정하며 여기에서 문자열을 소유 복사하지 않는다.
        hooks.guide.current=[this] { return active!=0; };
        hooks.guide.finish=[this] { Effect("finish","finish:"+std::to_string(active)); };
        hooks.guide.release=[this] { const auto previous=active;active=reentrant ? 2U : 0U;guide.clear();Effect("release","release:"+std::to_string(previous)); };
        hooks.guide.create=[this](auto name) { guide=name;newMode=newDemo=0;Effect("create","create:"+guide);active=3; };
        hooks.guide.mode=[this](auto value) { CHECK(active==3);newMode=value; };
        hooks.pending=[this](auto name) { pending=name.substr(0,name.find('\0'));Effect("pending","pending:"+pending); };
        hooks.requestEdit=[this] { Effect("edit","edit:"+std::to_string(mode));std::uint32_t battle=0,state=14,slot=1,owner=1;RequestFortEdit({mode,next,battle,state,slot,owner},{}); }; //이번 원문 자료의 Battle0도 실제 next1 부모를 사용한다.
        return hooks;
    }
};
}

TEST_CASE(EditorTestBattle_ReplaysNative174LateNamesReentryAndPartialStops) { //기대 열은 실제 모듈 실행 뒤 비교할 때만 읽는다.
    const auto fixture=LoadFixture(NETSTORM_EDITOR_TEST_BATTLE_FIXTURE);std::size_t checked{},stopped{};
    for (const auto& row:fixture.rows) { //두 x87 제어어의 관측도 각각 독립 재생한다.
        if (row[0]=="name") continue;CHECK(row.size()==28);if (row.size()!=28) continue;
        TestBattleScene scene(row);bool partial=false;
        const InputEvent event{Number(row[2]),std::bit_cast<std::int32_t>(Number(row[3])),std::bit_cast<std::int32_t>(Number(row[4]))};
        try {
            if (!(scene.active && Number(row[9])) && !Number(row[10])) CHECK(DispatchEditorTestBattle(event,scene.mode,scene.Hooks())); //Guide/Gump 소비는 상위 입력 관문의 명시 경계다.
        } catch (const TestBattleStop&) { partial=true; }
        CHECK(partial==(Number(row[14])==0));CHECK(scene.active==Number(row[15]) && scene.pointer==Number(row[16]));
        CHECK(std::string(scene.names[0].data())==row[17] && std::string(scene.names[1].data())==row[18]);
        CHECK(scene.guide==row[19] && scene.pending==row[20]);CHECK(scene.newMode==Number(row[21]) && scene.newDemo==Number(row[22]));
        CHECK(scene.mode==Number(row[23]) && scene.next==Number(row[24]));CHECK((scene.trace.empty() ? "-" : scene.trace)==row[25]);
        CHECK(Number(row[27])>0);if (!partial) CHECK(Number(row[26])==0);else ++stopped;++checked;
    }
    CHECK(checked==174 && stopped==12);
}

TEST_CASE(EditorTestBattle_RequiresOnlySelectedEffectsAtOriginalFailurePosition) { //없는 하위를 빈 성공으로 처리하지 않으며 앞선 쓰기를 보존한다.
    std::uint32_t mode=73;CHECK(!DispatchEditorTestBattle({0x020000c9U,0,0},mode,{}));CHECK(mode==73);
    CHECK(DispatchEditorTestBattle({kEditorTestBattleCommand|InputCode::kRelease,0,1},mode,{}));CHECK(mode==73);
    EditorTestBattleHooks hooks;std::string trace;hooks.guide.current=[] { return false; };hooks.currentFortName=[] { return std::string_view("Fort"); };
    hooks.snapshot=[&] { trace+='S'; };hooks.guide.create=[&](auto name) { CHECK(name=="Fort");trace+='C'; };
    bool missing=false;try { static_cast<void>(DispatchEditorTestBattle({kEditorTestBattleCommand,1,0},mode,hooks)); }catch (const std::logic_error&) { missing=true; }
    CHECK(missing && trace=="SC" && mode==73); //새 공장 뒤 +64 writer 누락은 앞선 생성 효과를 되돌리지 않는다.
    trace.clear();missing=false;try { static_cast<void>(DispatchEditorTestBattle({kEditorTestBattleCommand,0,1},mode,hooks)); }catch (const std::logic_error&) { missing=true; }
    CHECK(missing && trace.empty() && mode==73); //pending 미연결 때는 아직 선택 모드2를 쓰지 않는다.
    hooks.pending=[&](auto name) { CHECK(name=="Fort");trace+='P'; };missing=false;
    try { static_cast<void>(DispatchEditorTestBattle({kEditorTestBattleCommand,0,-1},mode,hooks)); }catch (const std::logic_error&) { missing=true; }
    CHECK(missing && trace=="P" && mode==2); //편집 예약 미연결 때는 이미 정상 반환한 pending과 모드2가 남는다.
    CloseGuideMission({[] { return false; }}); //빈 현재에는 release/finish 장치를 요구하지 않는다.
}

TEST_CASE(EditorTestBattle_UsesActualGameMenuNumbersAndInputQueue) { //기존 실제 메뉴가 쓰는 0/1 인자를 같은 명령 부모에 전달한다.
    for (const bool editing:{true,false}) { //Editor의 Test Battle과 +64 미션의 Return to Editing을 각각 작성한다.
        InputQueue queue;GameMenusHooks hooks;std::size_t selected{};
        hooks.global=[&](auto address) { if (address==0x5c85a4U) return editing ? 1U : 0U;if (address==0x594fa4U) return editing ? 0U : 1U;if (address==0x59507cU) return 1U;const auto value=GameMenuCommandValue(address);if (!value) throw std::logic_error("Test Battle 메뉴의 미명시 전역");return *value; };
        hooks.translate=[](auto text) { return std::string(text); };hooks.vertical=[] {};hooks.separator=[] {};hooks.take=[] { return 99U; };
        hooks.editingMission=[] { return 1U; };hooks.demo=[] { return 0U; };
        hooks.item=[&](auto label,auto code,const GameMenuParameter& parameter,auto include,auto enabled) {
            if (code!=kEditorTestBattleCommand) return;CHECK(label==(editing ? "Test Battle" : "Return to Editing"));CHECK(include==1 && enabled==1);
            const auto value=std::get<std::uint32_t>(parameter);CHECK(value==(editing ? 0U : 1U));queue.PushCommand(code,0,std::bit_cast<std::int32_t>(value));++selected;
        }; //다른 실제 메뉴 행도 작성하되 이번 선택 인자는 c2에서만 큐에 넣는다.
        CHECK(RawGameMenus(std::move(hooks)).Game()==99 && selected==1);const auto event=queue.Pop(false);CHECK(queue.Empty());
        std::uint32_t mode=91;EditorTestBattleHooks effects;std::string trace;
        effects.snapshot=[&] { trace+='S'; };effects.currentFortName=[] { return std::string_view("Fort"); };effects.guide.current=[] { return false; };
        effects.guide.create=[&](auto name) { CHECK(name=="Fort");trace+='C'; };effects.guide.mode=[&](auto value) { CHECK(value==1);trace+='M'; };
        effects.pending=[&](auto name) { CHECK(name=="Fort");trace+='P'; };effects.requestEdit=[&] { CHECK(mode==2);trace+='E'; };
        CHECK(DispatchEditorTestBattle(event,mode,effects));CHECK(trace==(editing ? "SCM" : "PE"));CHECK(mode==(editing ? 91U : 2U));
    }
}
