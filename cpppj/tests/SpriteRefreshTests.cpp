// 일반 Sprite의 원본 표시/정기와 실제 현재 Form finder 경계를 비MP 명시 입력으로 검증한다.
#include "RawSceneSupport.h"
#include "client/NativeMetaSpriteRefresh.h"
#include "o/RawSpriteRegular.h"
#include "o/SquidDisplay.h"
#include "o/SquidUnpop.h"
#include <bit>
#include <filesystem> //독립 fixture의 실제 소스 기준 절대 경로를 구성한다.

namespace { //독립 원본 입력과 장치 관찰을 이 검사에 한정한다.
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
std::string SpriteEvents(const std::vector<std::string>& events) { //원본 관찰과 같은 세미콜론 연결이며 빈 관찰은-다.
    std::string text;for (const auto& value:events) { if (!text.empty()) text+=';';text+=value; }return text.empty() ? "-":text; //장치 호출 순서를 바꾸거나 정렬하지 않는다.
}
struct SpriteScene { //일반 Sprite만 준비하며 메타/MP 객체를 생성하거나 실행하지 않는다.
    std::array<std::uint8_t,50> object{}; //같은 실제50BYTE 입력이며 dirty 뒤 BYTE35 쓰기를 확인한다.
    std::array<std::array<std::uint8_t,50>,8> forms{}; //60~67의 현재 Form 입력과 선저장 next다.
    ClientChallengeGlobals challenge; //공통 장치 형식의 중립 소유자이며 CS/MP 동작을 실행하지 않는다.
    std::uint32_t excluded{},end{}; //외부 Redraw가 바꾸는 현재 전체DWORD 역할이다.
    bool metamap{},boss{},first{true}; //삭제 부모이면서 checking1/boss0인 Sprite의 원문 허용 경로다.
    int mutate{}; //bounds/dirty/Redraw 시점의 명시 자료 변경 입력이다.
    ScreenRect rect{}; //498ff0 경계가 반환할 입력 사각형이다.
    std::vector<std::string> events; //원문과 비교할 장치 인자/순서만 기록한다.
    explicit SpriteScene(const std::vector<std::string>& row) { //입력만 공급하고 기대 좌표/필터를 계산하지 않는다.
        rect={std::stoi(row[1]),std::stoi(row[2]),std::stoi(row[3]),std::stoi(row[4])};excluded=Number(row[9]);end=Number(row[10]);mutate=std::stoi(row[8]);
        Put(object,0,0x50cc08U);object[10]=161;object[11]=3;Put(object,26,Number(row[5]));Put(object,8,Number(row[6]),2);object[35]=0x77;Put(object,36,Number(row[11]));Put(object,6,Number(row[7]) ? 60:0,2);
        constexpr std::array<std::uint8_t,8> kinds{2,3,6,9,10,46,69,70},states{0,1,2,8,3,255,4,0}; //원본 실행기와 같은 혼합 Form/자산·상태 입력이다.
        for (std::size_t index=0;index<forms.size();++index) { //물리 next와 현재 TYPE/상태를 준비하고 실제 helper가 필터한다.
            auto& raw=forms[index];raw[10]=kinds[index];raw[11]=states[index];Put(raw,4,index<7 ? 61U+static_cast<std::uint32_t>(index):0U,2);Put(raw,18,1060U+static_cast<std::uint32_t>(index));
        }
    }
    NativeMetaSpriteRefreshGlobals Globals() { return {challenge,excluded,end,RawCheckingFlag{true},metamap,boss}; } //현재 역할 주소를 빌리고 복사본으로 초기화하지 않는다.
    NativeMetaSpriteRefreshHooks Hooks() { //큰 장치의 실제 인자와 명시 부수 효과를 원본 관찰과 같은 형식으로 기록한다.
        NativeMetaSpriteRefreshHooks hooks;
        hooks.bounds=[this](auto raw) { CHECK(raw.size()==50 && raw[10]==161);events.push_back("B");if (mutate==1) Put(object,26,2);return rect; };
        hooks.dirty=[this](auto value) { events.push_back("D:"+std::to_string(value.left)+':'+std::to_string(value.top)+':'+std::to_string(value.right)+':'+std::to_string(value.bottom));if (mutate==2) { object[35]=0x89;Put(object,6,61,2); } };
        hooks.raw=[this](auto sid)->std::span<const std::uint8_t> { if (sid<60 || sid>67) throw std::out_of_range("Sprite 검사 Form SID 범위");return forms[sid-60]; };
        hooks.redrawGump=[this](auto id) { events.push_back("Q:"+std::to_string(id));if (id%2) return;events.push_back("G:"+std::to_string(id)+":0");
            if (first && mutate==3) { Put(forms[0],4,0,2);forms[1][10]=69; }if (first && mutate==4) { excluded=3;end=70; }first=false;
        };
        hooks.assertion=[this](auto,auto file,auto line) { CHECK(file=="\\Ns\\O\\Squidfinder.cpp");events.push_back("A:"+std::to_string(line)); };return hooks;
    }
};
std::vector<std::string> SpriteInput() { return {"D","10","20","30","40","1","0","1","0","1","47","0","8","0"}; } //정상 표시와 부분 효과 검사에 쓰는 명시 입력이다.
}

TEST_CASE(SpriteRefresh_IndependentPe225Inputs) { //78개 표시/147개 정기 원본 관찰과 현재 장치·IDIV·반환 비트를 대조한다.
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/sprite-refresh-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { //각 행은 새 물리 입력에서 시작하여 앞선 부수 효과가 남지 않는다.
        if (row[0]=="D") { SpriteScene f(row);RefreshNativeSprite(f.object,f.Globals(),f.Hooks());CHECK(row[14]=="0" && f.object[35]==Number(row[15]) && Get(f.object,36)==Number(row[16]) && SpriteEvents(f.events)==row[18]); }
        else {
            SidPool pool(OriginalEdition::Patch1078,24000);const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=161;raw[35]=0x77;Put(raw,36,Number(row[11]));std::vector<RiftTypeRecord> types(188);types[161].frameCount=std::stoi(row[12]);types[161].flags2=4;
            std::vector<std::string> events;std::uint32_t updates{};SquidFrameHooks hooks;
            hooks.frameSize=[](auto,auto)->std::array<float,2> { throw std::logic_error("Sprite 명시 Frame.Set 경계의 프레임 크기 호출"); };hooks.unpop=[](auto,auto) { throw std::logic_error("Sprite 명시 Frame.Set 경계의 Unpop 호출"); };hooks.pop=[](auto,float,float,auto) { throw std::logic_error("Sprite 명시 Frame.Set 경계의 Pop 호출"); };
            hooks.update=[&](auto parent,auto flags) { CHECK(parent==sid && flags==0);if (++updates==2) events.push_back("S:"+std::to_string(std::bit_cast<std::int32_t>(Get(raw,36)))); }; //실제 지정기의 옛/새 두 표시를 원문 전체 Set 경계 한 번으로 정규화한다.
            SquidFrame setter(pool,types,std::move(hooks));bool error{};float result{};try { result=RunSpriteRegular(pool,types,setter,sid,std::bit_cast<float>(Number(row[13]))); }catch (const std::domain_error&) { error=true; }
            CHECK(error==(Number(row[14])!=0) && raw[35]==Number(row[15]) && Get(raw,36)==Number(row[16]) && SpriteEvents(events)==row[18]);if (!error) CHECK(std::bit_cast<std::uint32_t>(result)==Number(row[17]));
        }++count;
    }
    CHECK(count==225);
}
TEST_CASE(SpriteRefresh_DestroyingParentSkipsMetaDiagnostic) { //일반 Sprite의yesIknow=1은 dead/checking/boss0이어도 진단을 호출하지 않는다.
    SpriteScene f(SpriteInput());auto hooks=f.Hooks();hooks.assertion=[](auto,auto,auto) { throw std::logic_error("일반 Sprite에 메타 삭제 진단 혼입"); };RefreshNativeSprite(f.object,f.Globals(),hooks);CHECK(f.object[35]==20);
}
TEST_CASE(SpriteRefresh_MissingDirtyKeepsByte35AndStopsForms) { //선택 장치가 없으면 bounds 뒤에 실패하고 원문 이후 쓰기/이름표 효과를 앞당기지 않는다.
    SpriteScene f(SpriteInput());auto hooks=f.Hooks();hooks.dirty={};bool rejected{};try { RefreshNativeSprite(f.object,f.Globals(),hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && f.object[35]==0x77 && SpriteEvents(f.events)=="B");
}
TEST_CASE(SpriteRefresh_MissingFormAccessKeepsDirtyAndByte35) { //Form 장치 누락을 빈 성공으로 바꾸지 않고 앞선 dirty/쓰기 효과는 보존한다.
    SpriteScene f(SpriteInput());auto hooks=f.Hooks();hooks.raw={};bool rejected{};try { RefreshNativeSprite(f.object,f.Globals(),hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && f.object[35]==20 && f.events.size()==2 && f.events[0]=="B");
}
TEST_CASE(SpriteRefresh_DisplayDispatchUsesActualDerivedBeforeFrameChecks) { //일반 Sprite의88/8c는 표시 억제/공통 FrameCheck 없이 실제 파생 장치만 선택한다.
    struct Sink final:SquidDisplaySink { //원문에 없는 일반 표시/억제 호출을 거부하는 명시 파생 장치다.
        std::uint32_t calls{}; //선택한 실제 파생 호출 횟수다.
        bool Suppressed() const override { throw std::logic_error("Sprite에 일반 표시 억제 혼입"); } //원문4791b0은 이 관문을 읽지 않는다.
        void Invalidate(SquidDisplayRect,std::uint32_t) override { throw std::logic_error("Sprite에 일반 사각형 표시 혼입"); } //전체 파생은 별도 현재 장치를 사용한다.
        void MetaRefresh(std::span<const std::uint8_t> raw) override { CHECK(Get(raw,0)==0x50cc08U);++calls; } //기존 어댑터가 전달한 현재 VT만 확인한다.
    } sink;
    std::array<std::uint8_t,50> raw{};Put(raw,0,0x50cc08U);raw[10]=255;Put(raw,36,0xffffffffU);SquidDisplay display(OriginalEdition::Patch1078,{}, {},sink,{});
    for (const auto flags:{0U,0x2000U}) { display.Validate(OriginalEdition::Patch1078,raw,flags);display.Update(raw,flags);CHECK(SquidUnpop::SupportsDisplay(OriginalEdition::Patch1078,0x50cc08U,flags)); } //8c thunk와88은 동일한 실제 파생을 사용한다.
    CHECK(sink.calls==2);
}
