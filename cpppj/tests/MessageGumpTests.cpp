// 제품 구현 전에 동결한10.78 Pause 생성 경로의192바이트와 실제 경계 순서를 대조한다.
#include "GumpRawSupport.h"
#include "RawSceneSupport.h"
#include "client/MessageGump.h"
#include <bit>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::gumpraw;
namespace {
constexpr std::uint32_t kNode=0x12401000U,kCanary=0xa5a5a5a5U; // 원문 실행기의 격리 노드와 미작성 필드 입력이다.
// 실제 경계 입력/순서만 재생하고 생성의 기대값은 fixture에서 읽는다.
struct MessageScene {
    GumpCreateState create;MessageGumpNode node; // 같은 실제 기본/파생 생성 저장소다.
    std::uint32_t challenge{},translation{},mutation{}; // 원문 판단을 포함하지 않는 외부 입력이다.
    std::string trace; // 실제 경계 호출의 원문 순서다.
    // 기본 생성자가 쓰지 않는 모든 위치를 원문과 같은 canary로 시작한다.
    MessageScene() {
        static_cast<GumpCreateNode&>(node)=Decode(std::vector<std::uint8_t>(0xa0,0xa5));
        node.head=node.reserved=node.trailing=kCanary;
        node.kind=node.player=node.parameter=std::bit_cast<std::int32_t>(kCanary);
        node.deadline=std::bit_cast<double>(std::uint64_t{0xa5a5a5a5a5a5a5a5});
    }
    // 현재 호출 효과에만 구분자를 붙인다.
    void Event(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 문자열을 Windows-1252 원문 바이트 그대로 비교한다.
    static std::string TextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
    // 실제 외부 경계가 변경한 현재 필드를 그대로 남긴다.
    void Mutate(std::uint32_t phase) {
        if (mutation==phase) {
            if (phase==1) { node.player=3;node.deadline=42.5; }
            else { node.player=8;node.parameter=123; }
        }
    }
    // 기존 Text/이름/번역/StyleText/측정만 실제 입력 경계로 공급한다.
    MessageGumpHooks Hooks() {
        MessageGumpHooks hooks;
        hooks.node=[this](auto token) { CHECK(token==kNode);return &node; };
        hooks.constructText=[this](auto token) {
            CHECK(token==kNode);Event("Text");node.vtable=0x5144d4;node.flags=0x400001;node.command=45;node.serial=41;node.head=0;
            node.textContext={0,0,0,0,0,0,1,1};
        };
        hooks.challenge=[this] { return challenge; };
        hooks.playerName=[this](auto player) { Event("PlayerName,"+std::to_string(std::bit_cast<std::uint32_t>(player)));Mutate(1); };
        hooks.challengePlayerName=[this](auto player) { Event("ChallengeName,"+std::to_string(std::bit_cast<std::uint32_t>(player)));Mutate(1); };
        hooks.translate=[this](std::string_view source) {
            Event("Translate,"+TextHex(source));Mutate(2);
            const std::array<std::string,3> texts{"~E~wGame Paused!","",std::string("~E~yPause")+std::bit_cast<char>(std::uint8_t{0xff})+'!'}; // 명시 번역 반환 바이트를 signed char 상수 절삭 없이 보존한다.
            return texts.at(translation);
        };
        hooks.style=[this](auto token,std::string_view text) { CHECK(token==kNode);Event("Style,"+TextHex(text));node.head=0x12408000; };
        hooks.fit=[this](auto token,auto x,auto y) { CHECK(token==kNode);Event("Fit,"+std::to_string(x)+','+std::to_string(y));node.baseWidth=node.width=51.25f;node.baseHeight=node.height=13.5f; };
        return hooks;
    }
    // 호스트 멤버 배치와 무관하게 같은 원본 offset 순서로192바이트를 직렬화한다.
    std::string Snapshot() const {
        auto result=Bytes(Encode(node));const auto start=result.size();result.resize(0xc0);
        CHECK(start==0xa0);
        Put(result,0xa0,node.head);Put(result,0xa4,std::bit_cast<std::uint32_t>(node.kind));Put(result,0xa8,std::bit_cast<std::uint32_t>(node.player));Put(result,0xac,node.reserved);
        const auto time=std::bit_cast<std::array<std::uint8_t,8>>(node.deadline); // double 기한의 실제64비트다.
        std::copy(time.begin(),time.end(),result.begin()+0xb0);Put(result,0xb8,std::bit_cast<std::uint32_t>(node.parameter));Put(result,0xbc,node.trailing);
        return Hex(result);
    }
};
}
// 두 x87 정밀도에서 동일했던 전체324개 입력의 필드/효과 순서를 비교한다.
TEST_CASE(MessageGump_PauseNative324) {
    std::size_t count{}; // 실제 비교한 독립 원문 행 수다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_MESSAGE_GUMP_FIXTURE).rows) { // 기대 fixture를 실행 중 갱신하지 않는다.
        CHECK(row.size()==3);if (row.size()!=3) return;
        const auto input=Split(row[0],',');CHECK(input.size()==5);if (input.size()!=5) return;
        MessageScene scene;scene.challenge=Number(input[0]);scene.translation=Number(input[3]);scene.mutation=Number(input[4]);
        MessageGump message(scene.create,scene.Hooks());
        CHECK(message.Construct(kNode,11,std::bit_cast<std::int32_t>(Number(input[1])),std::bit_cast<std::int32_t>(Number(input[2])))==kNode);
        CHECK(scene.Snapshot()==row[1]);CHECK(scene.trace==row[2]);++count;
    }
    CHECK(count==324);
}
// 지원하지 않는 종류와 선택된 Challenge 조회 누락을 성공으로 대체하지 않는다.
TEST_CASE(MessageGump_MissingSelectedBranchesStayExplicit) {
    MessageScene scene;MessageGump message(scene.create,scene.Hooks());bool unsupported{};
    try { static_cast<void>(message.Construct(kNode,18,0,0)); } catch (const std::logic_error&) { unsupported=true; }
    CHECK(unsupported && scene.trace.empty());
    auto hooks=scene.Hooks();hooks.challengePlayerName={};scene.challenge=1;MessageGump challenge(scene.create,std::move(hooks));bool missing{};
    try { static_cast<void>(challenge.Construct(kNode,11,1,0)); } catch (const std::logic_error&) { missing=true; }
    CHECK(missing && scene.trace=="Text");
}

// CD 004c8b40의 Pause는 CD 가상표/무강조 문구를 쓰고 기한·음향·Idle을 새로 만들지 않는다.
TEST_CASE(MessageGump_CdPauseUsesCdTextVtableAndPermanentDeadline) {
    for (const auto challenge:{0U,1U}) { // 같은 CD 공통 생성자는 일반/Challenge 이름 조회를 현재 모드에 따라 선택한다.
        MessageScene scene;scene.create.collection.edition=netstorm::o::OriginalEdition::Cd1072;scene.challenge=challenge;
        auto hooks=scene.Hooks();
        hooks.translate=[&scene](std::string_view value) { CHECK(value=="~wGame Paused!");scene.Event("TranslateCd");return std::string("translated CD pause"); }; // 원문 번역 키와 실제 번역 반환을 따로 검증한다.
        hooks.warningSound=[&scene] { scene.Event("UnexpectedSound"); };
        hooks.wallSeconds=[&scene] { scene.Event("UnexpectedClock");return 100.0; };
        hooks.idle=[&scene](auto,bool) { scene.Event("UnexpectedIdle"); };
        MessageGump message(scene.create,std::move(hooks));
        CHECK(message.Construct(kNode,11,0,9)==kNode);
        CHECK(scene.node.vtable==0x506658U && scene.node.kind==11 && scene.node.player==0 && scene.node.parameter==9);
        CHECK(scene.node.deadline==0.0 && scene.node.reserved==kCanary && scene.node.trailing==kCanary);
        CHECK(scene.trace==std::string("Text;")+(challenge ? "ChallengeName,0" : "PlayerName,0")+";TranslateCd;Style,"+MessageScene::TextHex("translated CD pause")+";Fit,0,0");
    }
}

// CD에 없는 PATCH 종류는 TextGump 생성/노드 쓰기/번역을 시작하기 전에 명시적으로 거부한다.
TEST_CASE(MessageGump_CdUnsupportedKindsPreserveUnconstructedNode) {
    for (const auto kind:{0,12,16,17,18}) { // CD switch1~11 밖인 입력과 현재 PATCH 확장 종류를 함께 확인한다.
        MessageScene scene;scene.create.collection.edition=netstorm::o::OriginalEdition::Cd1072;
        const auto before=scene.Snapshot(); // 실패 이전의 전체0xc0 물리 상태다.
        MessageGump message(scene.create,scene.Hooks());bool rejected{}; // 판본 선택만으로 조기 TextGump 생성을 실행하지 않는다.
        try { static_cast<void>(message.Construct(kNode,kind,0,0,"unsupported")); }catch (const std::logic_error&) { rejected=true; }
        CHECK(rejected && scene.trace.empty() && scene.Snapshot()==before);
    }
}

// T/t 게임 시간의 최초 원문 관측과 실제 메시지 생성자를 함께 검사한다(754).
#include "GameTimeMessageTests.inc"

#include "WarningMessageTests.inc" //새kind17의원문192BYTE/텍스트ABI/부분효과를기존공통검사와함께확인한다.
