// 보호 PE의 실제 세 인자 선택·짧은 메뉴 입구 관찰을 제품 몸체와 대조한다.
#include "RawSceneSupport.h"
#include "client/RawUserSelection.h"
#include "client/RawUserHighlight.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientUserSelectionCleanup.h"
#include "client/RawSurfaceWorld.h"
#include "DudeSpawnSupport.h"
#include <bit>
#include <algorithm>
#include <stdexcept>
using namespace netstorm::test::rawscene;
using namespace netstorm::client;

namespace {
// 원문 DWORD 입력/출력을 순서대로 읽으며 기대 분기를 계산하지 않는다.
std::vector<std::uint32_t> SelectionNumbers(std::string_view text) {
    std::vector<std::uint32_t> result; // 원본 unsigned 십진수 비트를 보관한다.
    // 음수였던 입력도 상위 비트를 잃지 않고 읽는다.
    for (const auto& field:Split(text,',')) result.push_back(Number(field));return result;
}
// 같은 물리 풀과 실제 강조/선택 모듈이 공유하는 독립 입력 저장소다.
struct SelectionScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 주소 경계가 원본과 같은 실제 물리 풀이다.
    std::array<RiftTypeRecord,188> types{}; // callback이 바꾸는 TYPE도 다음 실제 읽기에서 사용한다.
    std::uint32_t selected{},mouseType{},mode{},enabled{},blocked{}; // 서로 다른 원본 다섯 전역의 현재 값이다.
    std::array<std::uint32_t,2> remembered{}; // 두 float 원문 비트다.
    std::array<std::uint32_t,5> multiple{}; // 현재 다중 선택의 명시 물리 입력이다.
    std::int32_t count{},mouseX{320},mouseY{240}; // signed 개수와 실제 강조가 조회할 마우스 좌표다.
    std::vector<std::uint32_t> row; // 원본31DWORD 입력이며 기대값 열과 분리한다.
    std::string trace; // 실제 외부 경계 호출 순서와 인자다.
    // fixture는 예약1/마지막23999까지 독립 raw 입력을 주입한다. 게임 생성 성공을 가장하지 않는다.
    std::span<std::uint8_t> Raw(std::uint32_t sid) {
        const auto bytes=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()};
    }
    // 물리 필드의 낮은 바이트부터 원본 폭만 쓴다.
    void Write(std::uint32_t sid,std::size_t offset,std::uint32_t value,std::size_t width=4) { Put(Raw(sid),offset,value,width); }
    // 전체DWORD와 문자열을 원본 TSV의 경계 사건 표현으로 보존한다.
    void Event(std::string_view letter,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=letter;
        // 각 인자의 부호 비트까지 unsigned 십진수로 출력한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 원본 입력에 지정된 외부 효과만 변경하고 부모 선택/반환 분기는 제품에 맡긴다.
    void Mutate(std::uint32_t boundary) {
        if (row[27]!=boundary) return;
        switch (boundary) {
        case 1:count=1;mouseType=77;break;
        case 2:selected=99;break;
        case 3:Write(50,10,101,1);break;
        case 4:Write(50,10,102,1);break;
        case 5:Write(50,40,1,1);break;
        case 6:selected=0;break;
        case 7:remembered={row[23],row[24]};break;
        case 8:selected=202;mouseType=77;break;
        case 9:enabled=0;blocked=1;break;
        case 10:selected=99;mouseType=77;break;
        case 11:Write(60,4,65,2);break;
        case 12:count=5;break;
        default:break;
        }
    }
    // 실제 강조 모드 처리기도 선택과 같은 현재 저장소를 읽도록 연결한다.
    UserHighlightHooks HighlightHooks() {
        UserHighlightHooks hooks; // 커서/역투영/이동만 원본 관찰의 명시 경계다.
        hooks.selectCursor=[this](std::uint32_t value) { Event("K",{value}); };
        hooks.project=[this](std::int32_t x,std::int32_t y) { Event("J",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});Mutate(8);return std::array{std::bit_cast<std::uint32_t>(7.5F),std::bit_cast<std::uint32_t>(9.25F)}; };
        hooks.move=[this](std::uint32_t sid,std::uint32_t x,std::uint32_t y,std::uint32_t force) { Event("H",{sid,x,y,force}); };return hooks;
    }
    // 도달한 실제 경계의 인자/반환만 공급한다. 선택/다중 정리/기억 비교/강조를 대신하지 않는다.
    UserSelectionHooks Hooks(const RawUserHighlight& highlight) {
        UserSelectionHooks hooks; // 같은 선택/타입/모드 참조를 사용하는 원본 처리기다.
        hooks.paused=[this] { Event("P");return row[5]; };hooks.enabled=[this] { return enabled; };hooks.blocked=[this] { Event("B");return blocked; };
        hooks.localOwner=[this] { return row[14]; };hooks.editor=[this] { return row[13]; };hooks.dais=[] { return 165U; };hooks.manaProcess=[] { return 50U; };hooks.rangeProcess=[] { return 51U; };
        hooks.selectNotice=[this](Sid sid,std::uint32_t on) { Event("N",{sid.value,on,selected});Mutate(sid.value==55 ? 1U : sid.value==56 ? 12U : sid.value==51 ? 2U : on ? 6U : 0U); };
        hooks.refresh=[this](Sid sid) { Event("R",{sid.value,selected});Mutate(sid.value!=51 ? 7U : 0U); };
        hooks.processNotice=[this](Sid sid,std::uint32_t event,std::uint32_t info,std::uint32_t value) { Event("F",{sid.value});Event("Q",{event,info,value});Mutate(11); };
        hooks.dump=[this](Sid sid) { Event("D",{sid.value});Mutate(3); };hooks.showMana=[this](Sid sid,std::uint32_t flag) { Event("T",{sid.value,flag}); };
        hooks.rangeType=[this](std::uint32_t type) { Event("V",{type});Mutate(4);return row[15]; };hooks.showRange=[this](Sid sid,std::uint32_t flag) { Event("C",{sid.value,flag});Mutate(5); };
        hooks.content=[this](Sid sid) { Event("I",{sid.value,1,0,0});return Sid{static_cast<std::uint16_t>(row[16])}; };hooks.altar=[this](Sid sid) { Event("A",{sid.value});Mutate(9);return Sid{static_cast<std::uint16_t>(row[18])}; };
        hooks.moving=[this](Sid sid) { Event("M",{sid.value});return row[17]; };hooks.movementPreview=[this](Sid sid) { Event("E",{sid.value}); };
        hooks.highlight=[&highlight](std::uint32_t value) { highlight.Highlight(value); };hooks.assertion=[this] { Event("!");trace+=":selectedSid:3841"; };
        hooks.sound=[this](std::uint32_t x,std::uint32_t y,std::string_view name,std::uint32_t flag) { Event("S",{x,y});trace+=':'+std::string(name)+':'+std::to_string(flag);Mutate(10); };return hooks;
    }
    // 새 행은 이전 원본/제품 실행의 변경을 모두 버리고 같은 독립 입력만 설치한다.
    void Prepare(std::vector<std::uint32_t> values) {
        row=std::move(values);CHECK(row.size()==31);trace.clear();selected=row[4];mouseType=row[20];mode=row[19];enabled=row[6];blocked=row[7];remembered={row[21],row[22]};count=std::bit_cast<std::int32_t>(row[25]);
        multiple=row[26] ? std::array<std::uint32_t,5>{0,24000,55,0xffffffffU,56} : std::array<std::uint32_t,5>{55,56,57,58,59};
        // 예약/물리 경계까지 raw입력만 설치하며 할당·생성 검증과 섞지 않는다.
        for (const std::uint32_t sid:{1U,50U,51U,52U,55U,56U,57U,58U,59U,60U,61U,62U,63U,64U,65U,99U,202U,23999U}) {
            auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=100;raw[34]=static_cast<std::uint8_t>(row[12]);Write(sid,14,row[23]);Write(sid,18,row[24]);
        }
        Write(50,11,row[8],1);Write(50,40,row[9],1);if (row[18]) Write(50,10,165,1);Write(51,6,60,2);
        // 원본 ProcessFinder의 자산/내장 Form 필터와 다음 링크 캡처를 실제 제품에서 검사한다.
        for (const auto& [sid,type]:std::array<std::pair<std::uint32_t,std::uint32_t>,6>{{{60,50},{61,51},{62,49},{63,100},{64,6},{65,5}}}) { Write(sid,10,type,1);Write(sid,4,sid<65 ? sid+1 : 0,2); }
        Write(57,11,1,1);Write(58,11,2,1);Write(59,11,8,1);
        // 현재 네 TYPE의 명시 입력은 callback뒤 다시 조회할 수 있다.
        for (const std::uint32_t type:{100U,101U,102U,165U}) { types[type].flags1=type==101 ? 0x10000000U : row[10];types[type].flags2=type==102 ? 0U : row[11]; }
    }
    // 원본 관찰과 같은 순서로 현재 전역과 변경될 수 있는 물리 필드를 반환한다.
    std::vector<std::uint32_t> Current() const {
        const auto raw=pool.Slot(Sid{50}),form=pool.Slot(Sid{60});return {selected,mouseType,remembered[0],remembered[1],std::bit_cast<std::uint32_t>(count),mode,enabled,raw[10],raw[40],std::uint32_t(form[4])|(std::uint32_t(form[5])<<8)};
    }
};
}

// 기대 반환/효과 순서/현재 전역은 독립 PE 정상 반환의1074입력에서만 읽는다.
TEST_CASE(UserSelection_NativeThreeArgumentsAndCurrentEffects) {
    SelectionScene scene;std::size_t rows{}; // 동일 현재 저장소를 독립 입력마다 초기화한다.
    // 실제 선택과 세 메뉴 입구 및 두 짧은 몸체를 원본 행 순서 그대로 재생한다.
    for (const auto& fields:LoadFixture(NETSTORM_USER_SELECTION_FIXTURE).rows) {
        CHECK(fields.size()==4);if (fields.size()!=4) continue;scene.Prepare(SelectionNumbers(fields[0]));
        RawUserHighlight highlight({scene.mode,scene.selected,scene.mouseType,scene.mouseX,scene.mouseY},scene.HighlightHooks());
        RawUserSelection selection(scene.pool,scene.types,{scene.selected,scene.mouseType,scene.remembered,scene.multiple,scene.count},scene.Hooks(highlight));std::uint32_t result{}; // 실제 두 몸체가 같은 참조를 빌린다.
        const auto operation=scene.row[0];
        if (!operation) result=selection.Select(scene.row[1],scene.row[2],scene.row[3]);
        else if (operation==1) selection.ClearMultiple();
        else if (operation==2) selection.Remember(scene.row[23],scene.row[24],100);
        else {
            const InputEvent event{0x20000,std::bit_cast<std::int32_t>(scene.row[29]),std::bit_cast<std::int32_t>(scene.row[30])};
            const auto common=[&scene](Sid sid,const InputEvent& input) { scene.Event("O",{sid.value,input.code,std::bit_cast<std::uint32_t>(input.x),std::bit_cast<std::uint32_t>(input.y)});return scene.row[28]; }; // 실제 공통 메뉴는 이 관찰의 명시 경계다.
            result=operation==4 ? selection.ArcherMenu(Sid{50},event,common) : selection.CarrierMenu(Sid{50},event,common);
        }
        CHECK(result==Number(fields[1]));CHECK((scene.trace.empty() ? "-" : scene.trace)==fields[2]);CHECK(scene.Current()==SelectionNumbers(fields[3]));++rows;
    }
    CHECK(rows==1074);
}

// 조기 관문은 실제 효과를 요구하지 않으며 미연결 하위는 선행 정리 뒤 그 위치에서 오류다.
TEST_CASE(UserSelection_LazyBoundariesAndWrittenPrefix) {
    SelectionScene scene;std::vector<std::uint32_t> input(31);input[1]=50;input[6]=1;input[21]=input[23]=std::bit_cast<std::uint32_t>(12.5F);scene.Prepare(input);
    UserSelectionHooks paused;paused.paused=[] { return 1U; };RawUserSelection denied(scene.pool,scene.types,{scene.selected,scene.mouseType,scene.remembered,scene.multiple,scene.count},std::move(paused));CHECK(denied.Select(50,0,0)==0);
    UserSelectionHooks hooks;hooks.paused=[] { return 0U; };hooks.enabled=[] { return 1U; };hooks.blocked=[] { return 0U; };scene.count=1;std::uint32_t notices{}; // 정리의+7c는 실행되고 미연결+88에서 실패해야 한다.
    hooks.selectNotice=[&](Sid sid,std::uint32_t on) { CHECK(sid.value==55 && on==0);++notices; };RawUserSelection missing(scene.pool,scene.types,{scene.selected,scene.mouseType,scene.remembered,scene.multiple,scene.count},std::move(hooks));bool rejected{};
    try { missing.Select(50,0,0); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && notices==1 && scene.count==1);
}

// 물리 크기가 다른 CD와 누락 TYPE는 실행 전 거부한다. CD 대응 주소는 문서에 별도 기록한다.
TEST_CASE(UserSelection_EditionAndTypeContract) {
    SelectionScene scene;bool rejected{};try { RawUserSelection invalid(scene.pool,std::span<const RiftTypeRecord>(scene.types).first(187),{scene.selected,scene.mouseType,scene.remembered,scene.multiple,scene.count},{}); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    SidPool cd(OriginalEdition::Cd1072,15000,true);rejected=false;try { RawUserSelection invalid(cd,scene.types,{scene.selected,scene.mouseType,scene.remembered,scene.multiple,scene.count},{}); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}

#include <filesystem>

// 실제004d5c60의87개 입력과 현재 전역/가상 효과 순서를 기존 선택 몸체와 함께 대조한다.
TEST_CASE(UserSelection_ClearNativeEnabledAndCurrentEffects) {
    SelectionScene scene;std::size_t rows{}; // 기존 물리 풀과 TYPE·선택/강조 저장소를 사용한다.
    const auto path=(std::filesystem::path(NETSTORM_USER_SELECTION_FIXTURE).parent_path()/"user-selection-clear-x86.tsv").string(); // 새 원본 관측만 읽는다.
    // 반환이 없는 Clear는 EAX를 성공값으로 해석하지 않고 원본 최종 전역/가상 효과를 비교한다.
    for (const auto& fields:LoadFixture(path.c_str()).rows) {
        CHECK(fields.size()==4);if (fields.size()!=4) continue;scene.Prepare(SelectionNumbers(fields[0]));
        RawUserHighlight highlight({scene.mode,scene.selected,scene.mouseType,scene.mouseX,scene.mouseY},scene.HighlightHooks());
        RawUserSelection selection(scene.pool,scene.types,{scene.selected,scene.mouseType,scene.remembered,scene.multiple,scene.count},scene.Hooks(highlight));selection.Clear();
        CHECK(Number(fields[1])==0);CHECK((scene.trace.empty() ? "-" : scene.trace)==fields[2]);CHECK(scene.Current()==SelectionNumbers(fields[3]));++rows;
    }
    CHECK(rows==87);
}
// enabled0은 후속 경계를 요구하지 않으며 강조 누락의 오류도 선행 실제 쓰기를 보존한다.
TEST_CASE(UserSelection_ClearLazyBoundaryAndWrittenPrefix) {
    SelectionScene scene;scene.selected=51;scene.count=2;UserSelectionHooks disabled;disabled.enabled=[] { return 0U; }; // 금지된 선택 해제의 명시 입력이다.
    RawUserSelection denied(scene.pool,scene.types,{scene.selected,scene.mouseType,scene.remembered,scene.multiple,scene.count},std::move(disabled));denied.Clear();CHECK(scene.selected==51 && scene.count==2);
    scene.count=0;UserSelectionHooks enabled;enabled.enabled=[] { return 1U; };RawUserSelection missing(scene.pool,scene.types,{scene.selected,scene.mouseType,scene.remembered,scene.multiple,scene.count},std::move(enabled));bool rejected{}; // 강조 경계를 의도적으로 비운다.
    try { missing.Clear(); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && !scene.selected && !scene.count);
}

namespace { // 공유 owner의 입력/출력을 실제 아홉 DWORD 비트로만 비교한다.
// signed 좌표/count와 나머지 uint32를 같은 순서의 독립 값 사본으로 읽는다.
std::array<std::uint32_t,9> SelectionOwnerDwords(const ClientUserSelectionGlobals& owner) {
    using Field=ClientUserSelectionGlobals::Field;const std::array fields{Field::Enabled,Field::Mode,Field::Selected,Field::MouseType,Field::MouseX,Field::MouseY,Field::RememberedX,Field::RememberedY,Field::MultipleCount};
    std::array<std::uint32_t,9> values{}; // 현재 owner와 별개인 검사용 출력 snapshot이다.
    for (std::size_t index=0;index<fields.size();++index) values[index]=owner.ReadDword(fields[index]); // 각 DWORD를 현재 필드에서 읽는다.
    return values;
}
}

// bool/SID16 재해석 없이 아홉 DWORD의 비트를 쓰며 원시 mouse와 Cursor 보정 좌표를 합치지 않는다.
TEST_CASE(UserSelectionGlobals_NativeDwordBitsAndDistinctRawMouse) {
    ClientGameplayGlobals gameplay;auto& owner=gameplay.selection;gameplay.cursor.screenX=37;gameplay.cursor.screenY=41;
    using Field=ClientUserSelectionGlobals::Field;const std::array fields{Field::Enabled,Field::Mode,Field::Selected,Field::MouseType,Field::MouseX,Field::MouseY,Field::RememberedX,Field::RememberedY,Field::MultipleCount};
    const std::array<std::uint32_t,9> input{0x80000000U,0xffffffffU,0xfedcba98U,0x12345678U,0xfffffffeU,0x80000000U,0x7fc00001U,0xffffffffU,0xfffffff9U}; // 전체 상위비트·NaN비트·음수count의 명시 입력이다.
    for (std::size_t index=0;index<fields.size();++index) owner.WriteDword(fields[index],input[index]); // 형식의 의미를 바꾸지 않는 원시 DWORD store다.
    CHECK(SelectionOwnerDwords(owner)==input && owner.mouse.x==-2 && owner.multipleCount==-7 && gameplay.cursor.screenX==37 && gameplay.cursor.screenY==41);
    auto selection=owner.SelectionState();auto highlight=owner.HighlightState();CHECK(&selection.selected==&owner.selected && &highlight.selectedSid==&owner.selected && &highlight.mouseX==&owner.mouse.x);
    owner.selected=0x87654321U;CHECK(selection.selected==0x87654321U && highlight.selectedSid==0x87654321U); // DWORD를 저장할 때 SID 소비용 WORD로 잘리지 않는다.
}

// 첫 메뉴는 Session 없이 실제 raw Clear를 실행하고 같은 mode0이면 mouseType와 나머지 전역을 보존한다.
TEST_CASE(UserSelectionGlobals_FirstMenuClearKeepsMouseTypeAndNegativeCountTail) {
    ClientGameplayGlobals gameplay;const auto surface=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game,gameplay.Authority(),gameplay.GameAux());auto& owner=gameplay.selection;
    CHECK(owner.enabled==1 && owner.mode==0 && owner.selected==0 && owner.multipleCount==0 && !gameplay.Borrowed());
    owner.enabled=0x80000000U;owner.selected=0xfedcba98U;owner.mouseType=0x89abcdefU;owner.multipleCount=-7;owner.multiple={0x10037U,0xffffffffU};owner.mouse={-2,73};owner.remembered={0x7fc00001U,0x80000000U};
    const auto multiple=owner.multiple;ClearClientUserSelection(gameplay,*surface); // mode0이므로 하드웨어 커서 함수도 필요 없다.
    CHECK(owner.enabled==0x80000000U && owner.selected==0 && owner.multipleCount==0 && owner.mode==0 && owner.mouseType==0x89abcdefU && owner.multiple==multiple);
    CHECK(owner.mouse.x==-2 && owner.mouse.y==73 && owner.remembered[0]==0x7fc00001U && owner.remembered[1]==0x80000000U && !gameplay.Borrowed());
}

// enabled0은 count/선택/강조까지 보존하며 배열 getter·게임·커서 효과를 호출하지 않는다.
TEST_CASE(UserSelectionGlobals_DisabledClearPreservesAllCurrentDwords) {
    ClientGameplayGlobals gameplay;const auto surface=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game,gameplay.Authority(),gameplay.GameAux());auto& owner=gameplay.selection;
    owner.enabled=0;owner.mode=5;owner.selected=0x12345678U;owner.mouseType=71;owner.multipleCount=-3;owner.mouse={-33,65};owner.remembered={1,2};owner.multiple={55,56};const auto before=SelectionOwnerDwords(owner);const auto multiple=owner.multiple;
    ClearClientUserSelection(gameplay,*surface,[](auto) { throw std::logic_error("비활성 선택의 커서 효과 도달"); });CHECK(SelectionOwnerDwords(owner)==before && owner.multiple==multiple);
}

// 가상 통지 중 재할당된 실제 DWORD 배열과 signed count를 다음 반복에서 모두 다시 읽는다.
TEST_CASE(UserSelectionGlobals_CurrentMultipleGetterSurvivesReallocationAndCountChanges) {
    SelectionScene scene;ClientGameplayGlobals gameplay;auto& owner=gameplay.selection;owner.selected=0xfedcba98U;owner.multiple={55,56};owner.multipleCount=2;
    for (const std::uint32_t number:{55U,56U,57U}) { auto raw=scene.Raw(number);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=100; } // 실제 raw 후보만 준비한다.
    RawUserHighlight highlight(owner.HighlightState(),{});UserSelectionHooks hooks;std::vector<std::uint32_t> notices,refreshes;const auto* first=owner.multiple.data(); // 이전 pointer는 역참조하지 않고 재할당 여부만 대조한다.
    hooks.enabled=[&owner] { return owner.enabled; };hooks.highlight=[&highlight](auto mode) { highlight.Highlight(mode); };
    hooks.selectNotice=[&](Sid sid,std::uint32_t on) {
        CHECK(on==0 && owner.selected==0xfedcba98U);notices.push_back(sid.value);
        if (sid.value==55) { std::vector<std::uint32_t> next(4096,0);next[0]=55;next[1]=57;owner.multiple.swap(next);owner.multipleCount=3; } // 이전 배열을 반납하고 다음 항목을 바꾼다.
    };
    hooks.refresh=[&](Sid sid) { refreshes.push_back(sid.value);if (sid.value==55) owner.enabled=0;else owner.multipleCount=2; }; // enabled는 최초 한 번만, count는 반복마다 읽는다.
    RawUserSelection selection(scene.pool,scene.types,owner.SelectionState(),std::move(hooks));selection.Clear();
    CHECK(owner.multiple.data()!=first && (notices==std::vector<std::uint32_t>{55,57}) && refreshes==notices && owner.multipleCount==0 && owner.selected==0 && owner.enabled==0);
}

// 원문 DWORD 후보가 Pool 밖이면 자연 생략하며 하위WORD55로 잘라 실제 객체를 통지하지 않는다.
TEST_CASE(UserSelectionGlobals_WideDwordCandidatesAreNotTruncatedToSid) {
    SelectionScene scene;ClientUserSelectionGlobals owner;owner.selected=0xffffffffU;owner.multiple={0x10037U,0xffffffffU};owner.multipleCount=2;const auto input=owner.multiple;
    RawUserHighlight highlight(owner.HighlightState(),{});UserSelectionHooks hooks;hooks.enabled=[&] { return owner.enabled; };hooks.highlight=[&](auto mode) { highlight.Highlight(mode); };
    hooks.selectNotice=[](Sid,std::uint32_t) { throw std::logic_error("범위 밖 DWORD의 잘린 SID 통지"); };RawUserSelection selection(scene.pool,scene.types,owner.SelectionState(),std::move(hooks));selection.Clear();
    CHECK(owner.multiple==input && owner.multipleCount==0 && owner.selected==0);
}

// 후반 배열 경계 실패는 앞선 통지를 보존하고 아직 실행하지 않은 count0/selected0 tail을 만들지 않는다.
TEST_CASE(UserSelectionGlobals_LateArrayFailureKeepsWrittenPrefix) {
    SelectionScene scene;ClientUserSelectionGlobals owner;owner.selected=0x87654321U;owner.multiple={55};owner.multipleCount=2;auto raw=scene.Raw(55);std::fill(raw.begin(),raw.end(),std::uint8_t{});
    std::uint32_t notices{},refreshes{};UserSelectionHooks hooks;hooks.enabled=[&] { return owner.enabled; };hooks.selectNotice=[&](Sid sid,auto) { CHECK(sid.value==55);++notices; };hooks.refresh=[&](Sid) { ++refreshes; };
    RawUserSelection selection(scene.pool,scene.types,owner.SelectionState(),std::move(hooks));bool rejected{};try { selection.Clear(); }catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && notices==1 && refreshes==1 && owner.multipleCount==2 && owner.selected==0x87654321U);
}

// prepared 대여/중복 거부/반납과 raw 꼬리 실패는 같은 owner의 현재 값을 암묵 reset하거나 rollback하지 않는다.
TEST_CASE(UserSelectionGlobals_PreparedLifetimeAndLateHighlightFailureKeepCurrentOwner) {
    ClientGameplayGlobals gameplay;const auto surface=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game,gameplay.Authority(),gameplay.GameAux());auto& owner=gameplay.selection;
    owner.selected=0xfedcba98U;owner.mode=4;owner.mouseType=71;owner.multipleCount=-1;owner.multiple={0x12345678};owner.mouse={-33,65};owner.remembered={17,19};const auto before=SelectionOwnerDwords(owner);
    { auto use=gameplay.AcquirePrepared();CHECK(SelectionOwnerDwords(owner)==before && gameplay.Borrowed());CHECK(Throws([&] { gameplay.Acquire(); }) && Throws([&] { gameplay.AcquirePrepared(); }) && SelectionOwnerDwords(owner)==before);
      bool rejected{};try { ClearClientUserSelection(gameplay,*surface,[](auto cursor) { CHECK(cursor==1);throw std::runtime_error("실제 커서 장치 후반 실패 입력"); }); }catch (const std::runtime_error&) { rejected=true; }
      CHECK(rejected && owner.multipleCount==0 && owner.selected==0 && owner.mode==0 && owner.mouseType==0 && owner.mouse.x==-33 && owner.remembered[0]==17 && owner.multiple[0]==0x12345678); }
    CHECK(!gameplay.Borrowed() && owner.mode==0 && owner.mouseType==0 && owner.mouse.y==65 && owner.remembered[1]==19 && owner.multiple[0]==0x12345678);
    { auto use=gameplay.Acquire();CHECK(owner.enabled==1 && owner.mode==0 && owner.selected==0 && owner.mouseType==0 && owner.multiple.size()==30 && std::all_of(owner.multiple.begin(),owner.multiple.end(),[](auto sid) { return sid==0; }) && owner.multipleCount==0); } // 일반 fresh는 Native Clear와 구별한 기존 호스트 입력 준비다.
}
