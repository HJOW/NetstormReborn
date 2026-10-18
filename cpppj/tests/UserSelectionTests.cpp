// 보호 PE의 실제 세 인자 선택·짧은 메뉴 입구 관찰을 제품 몸체와 대조한다.
#include "RawSceneSupport.h"
#include "client/RawUserSelection.h"
#include "client/RawUserHighlight.h"
#include <bit>
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
