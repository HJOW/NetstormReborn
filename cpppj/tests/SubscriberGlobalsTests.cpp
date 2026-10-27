// 실제 두 Subscriber 소유자와 기존 값형/명시 참조형의 수명 계약을 독립 검사한다.
#include "TestSupport.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/TutorialInit.h"
#include "o/RawCampaignWorldLoad.h"
#include "o/RawChatManagerPrefix.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using namespace netstorm::client;
using netstorm::o::CampaignWorldGlobals;
// Native 칼럼의 little endian DWORD만 읽으며 제품 값으로 기대를 만들지 않는다.
std::uint32_t SubscriberDword(std::string_view hex) {
    if (hex.size()!=8) throw std::logic_error("Subscriber 원시 DWORD 폭");
    std::uint32_t value{}; // 원문 현재 DWORD의 비트다.
    for (std::size_t byte=0;byte<4;++byte) value|=static_cast<std::uint32_t>(std::stoul(std::string(hex.substr(byte*2,2)),nullptr,16))<<(byte*8); // 낮은 BYTE부터 합친다.
    return value;
}
// 원시 TSV의 빈 칼럼도 유지하며 모든 관측행을 분류한다.
std::vector<std::string> SubscriberFields(std::string_view line) {
    std::vector<std::string> fields;std::size_t first{}; // 분리 결과와 현재 시작 위치다.
    for (;;) { // 마지막 칼럼까지 원문 순서대로 읽는다.
        const auto next=line.find('\t',first);fields.emplace_back(line.substr(first,next==line.npos?line.npos:next-first));
        if (next==line.npos) return fields;
        first=next+1;
    }
}
}

// PE 초기 두 DWORD만 새 Client 소유자가 구성하며 같은 const/nonconst 주소를 제공한다.
TEST_CASE(SubscriberGlobals_OriginalInitialValuesAndStableReferences) {
    static_assert(sizeof(ClientSubscriberGlobals)==8); // editor/post를 숨은 공유 상태로 추가하지 않는다.
    static_assert(!std::is_copy_constructible_v<ClientSubscriberGlobals> && !std::is_move_constructible_v<ClientSubscriberGlobals>); // 빌린 주소를 이동하지 않는다.
    ClientSubscriberGlobals owner;const auto& readOnly=owner; // 실제 새 owner와 같은 읽기 view다.
    CHECK(owner.MySubscriber()==0xffffffffU && owner.LoginSubscriber()==1);
    CHECK(&owner.MySubscriber()==&readOnly.MySubscriber() && &owner.LoginSubscriber()==&readOnly.LoginSubscriber());
    owner.MySubscriber()=0x80000000U;owner.LoginSubscriber()=0xabcdef01U;
    CHECK(readOnly.MySubscriber()==0x80000000U && readOnly.LoginSubscriber()==0xabcdef01U);
}

// 기존 독립 기본/부분 중괄호 입력은 자기 저장소를 소유하는 값 생성으로 보존한다.
TEST_CASE(SubscriberGlobals_IndependentDefaultsAndValueConstruction) {
    CampaignWorldGlobals empty,one{7},two{8,9},four{10,11,12,false}; // 기존 호출 문법과 네 값의 기본값을 함께 확인한다.
    CHECK(empty.mySubscriber==0 && empty.loginSubscriber==0 && empty.editor==0 && empty.missionPost);
    CHECK(one.mySubscriber==7 && one.loginSubscriber==0 && one.editor==0 && one.missionPost);
    CHECK(two.mySubscriber==8 && two.loginSubscriber==9 && four.mySubscriber==10 && four.loginSubscriber==11 && four.editor==12 && !four.missionPost);
    CHECK(&empty.mySubscriber!=&one.mySubscriber && &empty.loginSubscriber!=&two.loginSubscriber);
}

// 명시 binding은 외부 DWORD에 쓰지 않으며 Editor/post는 각 Session view에 남긴다.
TEST_CASE(SubscriberGlobals_BoundConstructionDoesNotResetOrShareModes) {
    ClientSubscriberGlobals owner;owner.MySubscriber()=0x12345678U;owner.LoginSubscriber()=0xfedcba98U; // 이미 Init/이전 Session이 남긴 현재 입력이다.
    auto first=CampaignWorldGlobals::BorrowSubscribers(owner.MySubscriber(),owner.LoginSubscriber());
    CampaignWorldGlobals second(CampaignWorldGlobals::SubscriberBinding{&owner.MySubscriber(),&owner.LoginSubscriber()}); // 실제 Session과 같은 직접 구성으로 두 컴파일러에서 주소를 보존한다.
    CHECK(owner.MySubscriber()==0x12345678U && owner.LoginSubscriber()==0xfedcba98U);
    CHECK(&first.mySubscriber==&owner.MySubscriber() && &second.mySubscriber==&owner.MySubscriber() && &second.loginSubscriber==&owner.LoginSubscriber());
    first.editor=1;first.missionPost=false;first.mySubscriber=25;
    CHECK(second.editor==0 && second.missionPost && second.mySubscriber==25 && owner.MySubscriber()==25);
}

// owner 생략과 일부 주소 생략은 각 view의 독립0 저장소를 사용하며 직접 구성 뒤 값 복사의 독립성도 유지한다.
TEST_CASE(SubscriberGlobals_DirectBindingFallbackAndValueCopy) {
    ClientSubscriberGlobals owner;owner.MySubscriber()=17;owner.LoginSubscriber()=29; // 외부 저장소의 현재값은 구성으로 재설정하지 않는다.
    CampaignWorldGlobals empty(CampaignWorldGlobals::SubscriberBinding{}),other(CampaignWorldGlobals::SubscriberBinding{});
    CampaignWorldGlobals myOnly(CampaignWorldGlobals::SubscriberBinding{&owner.MySubscriber(),nullptr});
    CampaignWorldGlobals loginOnly(CampaignWorldGlobals::SubscriberBinding{nullptr,&owner.LoginSubscriber()});
    CHECK(empty.mySubscriber==0 && empty.loginSubscriber==0 && empty.editor==0 && empty.missionPost);
    empty.mySubscriber=71;empty.loginSubscriber=81;
    CHECK(other.mySubscriber==0 && other.loginSubscriber==0 && owner.MySubscriber()==17 && owner.LoginSubscriber()==29);
    CHECK(&myOnly.mySubscriber==&owner.MySubscriber() && myOnly.loginSubscriber==0);
    CHECK(&loginOnly.loginSubscriber==&owner.LoginSubscriber() && loginOnly.mySubscriber==0);
    myOnly.loginSubscriber=91;loginOnly.mySubscriber=101;
    CampaignWorldGlobals copy(myOnly),moved(std::move(loginOnly)); // 값 복사/이동은 직접 binding 구성 뒤에도 외부 주소를 암묵 전파하지 않는다.
    owner.MySubscriber()=37;owner.LoginSubscriber()=49;
    CHECK(copy.mySubscriber==17 && copy.loginSubscriber==91 && moved.mySubscriber==101 && moved.loginSubscriber==29);
    CHECK(myOnly.mySubscriber==37 && loginOnly.loginSubscriber==49 && other.mySubscriber==0 && other.loginSubscriber==0);
}

// 값 복사/이동은 alias를 암묵 전파하지 않고 대입은 목적지의 기존 binding을 유지한다.
TEST_CASE(SubscriberGlobals_CopyMoveAndCrossAliasAssignment) {
    ClientSubscriberGlobals owner;owner.MySubscriber()=17;owner.LoginSubscriber()=29;
    auto bound=CampaignWorldGlobals::BorrowSubscribers(owner.MySubscriber(),owner.LoginSubscriber());bound.editor=1;bound.missionPost=false;
    CampaignWorldGlobals copy(bound),moved(std::move(bound)); // source의 현재 네 값을 각자 독립 fallback에 복사한다.
    CHECK(&copy.mySubscriber!=&owner.MySubscriber() && &moved.loginSubscriber!=&owner.LoginSubscriber());
    copy.mySubscriber=99;CHECK(owner.MySubscriber()==17 && moved.mySubscriber==17 && bound.mySubscriber==17);
    CHECK(moved.editor==1 && !moved.missionPost && bound.editor==1 && !bound.missionPost);
    auto reversed=CampaignWorldGlobals::BorrowSubscribers(owner.LoginSubscriber(),owner.MySubscriber()); // 교차 alias에서 순차 쓰기 전 source 캡처가 필요하다.
    reversed=bound;CHECK(owner.MySubscriber()==29 && owner.LoginSubscriber()==17);
    CHECK(&reversed.mySubscriber==&owner.LoginSubscriber() && &reversed.loginSubscriber==&owner.MySubscriber());
    CampaignWorldGlobals values{51,61,0,true};bound=std::move(values);bound=bound;
    CHECK(owner.MySubscriber()==51 && owner.LoginSubscriber()==61 && bound.editor==0 && bound.missionPost);
    CHECK(&bound.mySubscriber==&owner.MySubscriber() && values.mySubscriber==51 && values.loginSubscriber==61);
}

// Chat이 빌리는 const 참조는 복제 값이 아니며 실제 공유 owner를 유지하는 소비자보다 먼저 소멸하지 않는다.
TEST_CASE(SubscriberGlobals_ChatConstReferenceAndSharedOwnerLifetime) {
    auto owner=std::make_shared<ClientSubscriberGlobals>();const auto retained=owner;std::weak_ptr<ClientSubscriberGlobals> lifetime=owner; // 실제 장기 owner의 소유/참조 수명을 구별한다.
    std::uint32_t manager{},active{}; // Chat helper의 별도 현재 DWORD 입력이다.
    netstorm::o::ChatManagerPrefixGlobals chat{manager,owner->MySubscriber(),active}; // 실제 constref API에 직접 연결한다.
    owner.reset();retained->MySubscriber()=71;
    CHECK(!lifetime.expired() && &chat.subscriber==&retained->MySubscriber() && chat.subscriber==71);
}

// 실제 Tutorial Init이 fileName callback 뒤 현재 login을 복사하는 10개 원문 checkpoint와 대조한다.
TEST_CASE(SubscriberGlobals_TutorialInitUsesCurrentLoginAfterConfiguration) {
#ifdef NETSTORM_SUBSCRIBER_LIFETIME_EDGES_FIXTURE
    const std::filesystem::path path=NETSTORM_SUBSCRIBER_LIFETIME_EDGES_FIXTURE; // 제품 전 동결된 실제 x86 관측 경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/subscriber-lifetime-edges-v2-x86.tsv";
#endif
    std::ifstream file(path);if (!file) throw std::runtime_error("Subscriber 원문 fixture 누락");std::string line;std::size_t rows{},selected{}; // 모든120행과 실제 선택10행을 구별한다.
    while (std::getline(file,line)) { // 부모 전체가 아닌 선택 store checkpoint 관측만 사용한다.
        if (line.empty() || line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto fields=SubscriberFields(line);if (fields.size()!=10) throw std::logic_error("Subscriber 원시 칼럼 수");++rows;
        if (fields[0]!="004c335c") continue;
        CHECK(fields[2]=="currentLoginLoad");ClientSubscriberGlobals owner;owner.MySubscriber()=SubscriberDword(std::string_view(fields[5]).substr(0,8)); // Native 진입 my만 명시 입력한다.
        std::uint32_t tutorial{},mission{},seen{},flags{},demo{};TutorialInitState state{"Tutorial1"}; // 별도 Init 상태이며 두 Subscriber를 소유하지 않는다.
        TutorialInitGlobals globals{tutorial,mission,seen,owner.MySubscriber(),owner.LoginSubscriber()};TutorialInitHooks hooks;std::vector<std::string> trace; // 실제 helper에 큰 효과 장치만 제공한다.
        hooks.clearTemplates=[&] { trace.emplace_back("clear"); };hooks.resetOptions=[&] { trace.emplace_back("reset"); };
        hooks.get=[](std::string_view key,std::optional<std::string_view> fallback) { return key=="demo"?std::string{}:std::string(fallback.value_or("0")); };
        hooks.set=[&](std::string_view key,std::string_view value) { CHECK(key=="fileName" && value=="Tutorial1");CHECK(owner.MySubscriber()==SubscriberDword(std::string_view(fields[5]).substr(0,8)));owner.LoginSubscriber()=static_cast<std::uint32_t>(std::stoull(fields[3]));trace.emplace_back("set"); };
        hooks.requestFort=[&](std::string_view name) { CHECK(name=="Tutorial1" && owner.MySubscriber()==SubscriberDword(std::string_view(fields[6]).substr(0,8)));trace.emplace_back("request"); };
        hooks.header=[&] { CHECK(owner.LoginSubscriber()==SubscriberDword(std::string_view(fields[6]).substr(8,8)));trace.emplace_back("header"); };
        InitializeTutorialMission(state,flags,demo,globals,hooks);
        CHECK(flags==1 && trace==std::vector<std::string>({"clear","reset","set","request","header"}));++selected;
    }
    CHECK(rows==120 && selected==10);
}
