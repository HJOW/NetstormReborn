// 실제cf570의 본문/호출순서/현재 serial 필드를 독립 원문과 비교한다.
#include "RawSceneSupport.h"
#include "client/MissionLoadingMessage.h"
#include "client/DialogScript.h"
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kFirstParent=0x12200000,kSecondParent=0x12204000; // 독립 PE 관측의 두 명시 HTML 노드 토큰이다.
// 원문 hex의 실제 바이트만 읽으며 UTF-8 재해석/번역은 하지 않는다.
std::string LoadingBytes(std::string_view hex) {
    if (hex=="-") return {};
    std::string result; // 첫NUL도 포함한 명시 입력이며 C문자열 처리는 실제 제품이 소유한다.
    for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)); // 기대본문을 만들지 않는 바이트 복구다.
    return result;
}
// 관찰한 본문 바이트를 원문과 같은 hex로 기록한다.
std::string LoadingHex(std::string_view bytes) { return bytes.empty() ? "-" : Hex({reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()}); }
// callback이 실제 현재 부모 필드를 바꾸는 명시 노드다.
struct LoadingParentFields {
    std::uint32_t depth{},serial{}; // 원본+21bc와+2c를 공유하는 현재 값이다.
};
}
// 확장printf/임시버퍼 경계 뒤 원문1540입력의 조립/하위효과/최종 현재필드를 전행 비교한다.
TEST_CASE(MissionLoadingMessage_Native1540) {
    std::size_t count{}; // 독립 fixture의 비교 완료 행 수다.
    for (const auto& row:LoadFixture(NETSTORM_MISSION_LOADING_MESSAGE_FIXTURE).rows) { // 기대파일은 수정하지 않는다.
        CHECK(row.size()==15);if (row.size()!=15) return;
        const auto message=LoadingBytes(row[0]),cancel=LoadingBytes(row[1]);const auto profile=Number(row[2]),depth=Number(row[3]),mutation=Number(row[5]);
        MissionLoadingMessageState state{17,73};std::map<std::uint32_t,LoadingParentFields> parents{{kFirstParent,{depth,17}},{kSecondParent,{depth,profile==1 ? 41U : 57U}}};
        std::string trace,observedBody; // 원문 외부 효과 순서와 실제 하위 처리기에 전달된 바이트다.
        const auto record=[&](std::string item) { if (!trace.empty()) trace+=';';trace+=item; }; // 호출 순서만 보관한다.
        const auto mutate=[&](std::uint32_t token,char where) { // 같은 fixture의 메타/본문 외부효과 입력이다.
            auto& node=parents.at(token);
            if (mutation==1 && where=='M') { node.depth=100;node.serial=99; }
            else if (mutation==2 && where=='B') { node.depth=0;node.serial=77; }
            else if (mutation==3 && where=='M') { state.transientSerial=888;state.createdSerial=666; }
        };
        MissionLoadingMessageHooks hooks; // 실제 본체의 외부 경계만 명시 입력으로 공급한다.
        hooks.cancelText=[&] { record("T,Cancel");return cancel; };
        hooks.findSerial=[&](std::uint32_t serial) {
            const auto found=profile==0 && serial==17 ? kFirstParent : ((profile==1 && serial==41) || (profile==4 && serial==57)) ? kSecondParent : 0U;
            record("L,"+std::to_string(serial)+','+std::to_string(found));return found;
        };
        hooks.updateDepth=[&](std::uint32_t token)->std::uint32_t& { return parents.at(token).depth; };
        hooks.serial=[&](std::uint32_t token) { return parents.at(token).serial; };
        hooks.metadata=[&](std::uint32_t token,std::string& source) { observedBody=source;record("M,"+std::to_string(token)+','+std::to_string(parents.at(token).depth)+','+LoadingHex(source));mutate(token,'M');if (mutation==4) source="AfterMetadata\n"; };
        hooks.body=[&](std::uint32_t token,std::string_view source) { observedBody=source;record("B,"+std::to_string(token)+','+std::to_string(parents.at(token).depth)+','+LoadingHex(source));mutate(token,'B'); };
        hooks.create=[&](std::string_view source,std::uint32_t mode,std::int32_t width,std::int32_t height) {
            observedBody=source;record("C,"+std::to_string(mode)+','+std::to_string(width)+','+std::to_string(height)+','+LoadingHex(source));
            state.createdSerial=profile==1 ? 41U : profile==4 ? 57U : 0U;return profile==2 ? 0U : 41U;
        };
        const auto result=ShowMissionLoadingMessage(state,message,hooks);
        CHECK(LoadingHex(observedBody)==row[6] && result==Number(row[7]));CHECK(state.transientSerial==Number(row[8]) && state.createdSerial==Number(row[9]));
        CHECK(parents.at(kFirstParent).depth==Number(row[10]) && parents.at(kFirstParent).serial==Number(row[11]));
        CHECK(parents.at(kSecondParent).depth==Number(row[12]) && parents.at(kSecondParent).serial==Number(row[13]));CHECK(trace==row[14]);++count;
    }
    CHECK(count==1540);
}
// 실제 현재 DialogPage 파서가 원본본문에서 instant Cancel/LeaveBattle3만 추출하는지 확인한다.
TEST_CASE(MissionLoadingMessage_StartingMissionDialogMetadata) {
    const auto source=BuildMissionLoadingMessageBody("Starting Mission...",[] { return "Cancel"; });
    CHECK(source=="Starting Mission...\n$ButtonInstant=Cancel,LeaveBattle,3");
    HtmlTextParserState state;HtmlTextParserGlobals globals;std::vector<DialogAction> actions;DialogMetadataHooks metadata;
    metadata.resolveAction=[&](const DialogAction& action) { actions.push_back(action); }; // 현재 기존Trim 계약을 같은 실제 파서에서 관측한다.
    const auto page=ParseDialogPage(source,state,globals,{},netstorm::o::OriginalEdition::Patch1078,metadata);
    CHECK(page.hasContent && page.items.size()==1 && actions.size()==1);if (page.items.size()!=1 || actions.size()!=1) return;
    CHECK(page.items[0].label=="Cancel" && page.items[0].instant && !page.items[0].menu && page.items[0].enabled);
    CHECK(page.items[0].action.command=="LeaveBattle" && page.items[0].action.argument=="3");CHECK(actions[0].command=="LeaveBattle" && actions[0].argument=="3");
    CHECK(page.background.empty() && page.timeoutSeconds==0 && page.onExit.command.empty());
}
// 원문이 조회하지 않는 Cancel/생성 경계는 요구하지 않고 버퍼 밖 NUL 쓰기만 명시적으로 거부한다.
TEST_CASE(MissionLoadingMessage_ConditionalCallbacksAndBounds) {
    CHECK(BuildMissionLoadingMessageBody("~.Body",{})=="~.Body\n");CHECK(BuildMissionLoadingMessageBody("Body$Buttonish",{})=="Body$Buttonish\n");
    bool missing{};try { static_cast<void>(BuildMissionLoadingMessageBody("Body",{})); } catch (const std::logic_error&) { missing=true; }CHECK(missing);
    CHECK(BuildMissionLoadingMessageBody(std::string("~.")+std::string(1020,'A')+'\n',{}).size()==1023);
    bool oversized{};try { static_cast<void>(BuildMissionLoadingMessageBody(std::string(1023,'A'),{})); } catch (const std::length_error&) { oversized=true; }CHECK(oversized);
    MissionLoadingMessageState state;MissionLoadingMessageHooks hooks;hooks.cancelText=[] { return "Cancel"; };missing=false;
    try { static_cast<void>(ShowMissionLoadingMessage(state,"Body",hooks)); } catch (const std::logic_error&) { missing=true; }CHECK(missing && !state.transientSerial && !state.createdSerial);
}
