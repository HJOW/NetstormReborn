// 원본 맵 메뉴 부모의 독립 기계어 관찰과 실제 Tell 메타 호출 시점을 확인한다.
#include "TestSupport.h"
#include "client/TellMapMenus.h"
#include "client/DialogScript.h"
#include "client/HtmlConditionEffect.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
using namespace netstorm::client;
namespace {
// TSV의 마지막 빈 필드까지 원문 순서로 나눈다.
std::vector<std::string> Fields(std::string_view source) {
    std::vector<std::string> fields;std::size_t start=0;
    for (;;) { // 각 탭을 별도 필드 경계로 읽으며 trace 내부 구분자는 보존한다.
        const auto end=source.find('\t',start);fields.emplace_back(source.substr(start,end==source.npos ? source.npos : end-start));
        if (end==source.npos) return fields;start=end+1;
    }
}
// 검사에서 외부 Refresh/행 장치에 공급하는 자료와 관찰 trace다. 부모 결과는 직접 쓰지 않는다.
struct Devices {
    std::uint32_t menus{},dirty{}; // 같은 Tell의 메뉴 수와 외부 캐시 dirty다.
    std::string key,mutation,trace; // 현재 파일 키, 명시 장치 변경 종류, 외부 호출 기록이다.
    std::int32_t count{}; // Refresh 뒤/각 행 뒤에 부모가 다시 읽는 현재 개수다.
    std::size_t begins{},rows{}; // 첫 외부 콜백 변경을 구별한다.
    // 원본과 같은 장치 경계만 기록하며 dirty/키 전환은 제품 부모에 맡긴다.
    TellMapMenuHooks Hooks() {
        TellMapMenuHooks hooks;
        hooks.beginVertical=[this](auto flags) { Record("begin:"+std::to_string(flags));if (!begins++ && mutation=="begin-menus") menus=5; };
        hooks.invalidate=[this] { dirty=1; };
        hooks.selectFileSpec=[this](bool solo) { key=solo ? "SOLOSpec" : "fortSpec"; };
        hooks.refresh=[this](auto mode) { Record("refresh:"+std::to_string(mode)+":"+key+":"+std::to_string(dirty));dirty=0; };
        hooks.count=[this] { return count; };
        hooks.appendRow=[this](auto index) {
            Record("row:"+std::to_string(index));if (index>=count) return false;
            if (!rows++) {
                if (mutation=="row-count") count=32;
                if (mutation=="row-state") { key="changed!";dirty=7; }
            }return true;
        };return hooks;
    }
    // 외부 효과의 순서를 한 줄의 결정적 표현으로 보관한다.
    void Record(std::string event) { if (!trace.empty()) trace+=';';trace+=event; }
};
// 예외로 중단된 원본 분기를 정상 복구처럼 처리하지 않았는지 확인한다.
template<class Function> bool Rejected(Function function) { try { function(); } catch (const std::logic_error&) { return true; }return false; }
}

// 원본 부모가 만든 최종 메뉴 수/dirty/키와 모든 외부 호출을 그대로 대조한다.
TEST_CASE(TellMapMenus_IndependentNativeRegion) {
    std::ifstream input(NETSTORM_TELL_MAP_MENUS_FIXTURE);CHECK(input.good());std::string line;std::size_t compared=0;
    while (std::getline(input,line)) { // 기계어가 관찰한 기대 trace를 읽으며 C++로 기대 분할을 다시 계산하지 않는다.
        if (line.empty() || line.starts_with('#')) continue;
        const auto row=Fields(line);CHECK(row.size()==13);if (row.size()!=13) continue;
        Devices device;device.menus=std::stoul(row[2]);device.dirty=std::stoul(row[3]);device.key=row[4];device.count=std::stoi(row[5]);device.mutation=row[6];
        const auto handled=AppendTellMapMenus(row[1],device.menus,device.Hooks());
        CHECK(handled==(row[7]=="1"));CHECK(device.menus==std::stoul(row[8]));CHECK(device.dirty==std::stoul(row[9]));CHECK(device.key==row[10]);CHECK(device.trace==row[11]);
        CHECK(row[12]=="0");++compared; // native가 region 끝에서 확인한 시작 ESP 대비 차이다. 완전한 함수 ABI 검사는 아니다.
    }CHECK(compared>=80);
}

// 빈 목록/비맵도 첫 열을 만들며 호출되지 않은 장치는 요구하지 않는다.
TEST_CASE(TellMapMenus_EmptyUnknownAndMissingDevices) {
    std::uint32_t menus=0;TellMapMenuHooks hooks;std::vector<std::uint32_t> flags;
    hooks.beginVertical=[&](auto value) { flags.push_back(value); };
    CHECK(!AppendTellMapMenus("ordinary",menus,hooks));CHECK(menus==1 && flags==std::vector<std::uint32_t>({0x40000}));
    CHECK(!AppendTellMapMenus("*FORTS*",menus,{}));CHECK(menus==1); // 원문에 없는 별칭은 맵으로 확장하지 않는다.
    CHECK(Rejected([&] { AppendTellMapMenus("*SOLOLIST*",menus,hooks); }));
    Devices empty;empty.key="initial!";CHECK(AppendTellMapMenus("*SOLOLIST*",empty.menus,empty.Hooks()));
    CHECK(empty.menus==1 && empty.dirty==1 && empty.key=="fortSpec" && empty.trace=="begin:262144;refresh:2:SOLOSpec:1;row:0");
}

// 실패한 행 추가 뒤 임의 finally로 키/dirty를 복구하지 않는다.
TEST_CASE(TellMapMenus_ExceptionPreservesPartialEffects) {
    Devices device;device.key="initial!";auto hooks=device.Hooks();
    hooks.appendRow=[](auto)->bool { throw std::logic_error("명시 행 장치 실패"); };
    CHECK(Rejected([&] { AppendTellMapMenus("*SOLOLIST*",device.menus,hooks); }));
    CHECK(device.menus==1 && device.key=="SOLOSpec" && device.dirty==0);
}

// 숨김 HTML은 목록을 읽지 않으며 선택된 metadata만 원본 종류 우선순위의 그 시점에 확장한다.
TEST_CASE(TellMapMenus_RuntimeMetadataOrderAndColumns) {
    HtmlTextParserState state;HtmlTextParserGlobals globals;Devices device;device.count=16;device.key="initial!";
    netstorm::o::ConfigInterface config({});HtmlTextParserHooks html;BindHtmlConditionConfig(html,config); // 숨김 판단도 실제 현재Config 조건 처리기를 사용한다.
    std::vector<std::string> resolved;DialogMetadataHooks metadata;
    metadata.resolveAction=[&](const auto& action) { resolved.push_back(action.command+":"+action.argument); }; // sentinel의 placeholder도 목록 장치보다 먼저 조회한다.
    metadata.appendMenu=[&](DialogPage& page,DialogItem item) {
        auto hooks=device.Hooks();hooks.appendRow=[&](auto index) {
            CHECK(resolved==std::vector<std::string>({"Tell:back","DoNothing:0"}));
            if (index>=device.count) return false;
            page.items.push_back({"map"+std::to_string(index),{"ReadFort","map"+std::to_string(index)},true,index==3,index!=5,false,page.menuColumns-1});return true;
        }; // 실제 Row 장치가 생성한 항목을 현재 Tell 열에 추가한다.
        if (!AppendTellMapMenus(item.label,page.menuColumns,hooks)) { item.menuColumn=page.menuColumns-1;page.items.push_back(std::move(item)); }
    };
    const auto page=ParseDialogPage("Body<?0>$Menu=*FORTLIST*,Ignored,hidden</?>$Menu=*SOLOLIST*,DoNothing,0$Button=Back,Tell,back",state,globals,html,netstorm::o::OriginalEdition::Patch1078,metadata);
    CHECK(page.menuColumns==2 && page.items.size()==17);CHECK(page.items[0].label=="Back");
    CHECK(page.items[8].menuColumn==0 && page.items[9].menuColumn==1);CHECK(page.items[4].checked && !page.items[6].enabled);
    CHECK(device.dirty==1 && device.key=="fortSpec");CHECK(resolved.size()==2);
}
