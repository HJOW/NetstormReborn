// 실제 PE의 다리 삭제 훅/낙하 좌표 기대값을 읽고 raw 풀·동적 콜백·호출 순서를 검사한다.
#include "TestSupport.h"
#include "o/RawBridgeLifecycle.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 독립 기록과 같은 입력 수다. 잘린 fixture나 빠진 판본/정밀도는 실패한다.
constexpr std::size_t kLinkRows=882,kPreRows=2520,kPostRows=1800,kDelayRows=726,kPackRows=242;
// 분석 도구와 같은 합성 풀 경계/입력 번호. 실제 게임 월드 번호 배치를 뜻하지 않는다.
constexpr std::uint32_t kCapacity=24000;
constexpr Sid kBridge{50},kLink{60},kFirst{61},kSecond{62},kFollow{63},kOther{64};
// 주석/빈 행을 제외하되 마지막 빈 필드도 보존하는 구분자 판독기다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;
    std::size_t begin=0;
    // 남은 마지막 필드까지 추가한다.
    for (;;) {
        const auto end=text.find(separator,begin);
        result.emplace_back(text.substr(begin,end==std::string_view::npos ? end : end-begin));
        if (end==std::string_view::npos) return result;
        begin=end+1;
    }
}
// Git에 저장된 실제 기계어 기대값만 읽는다. Python/원본 PE는 테스트에 필요 없다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=[] {
        std::ifstream input(NETSTORM_BRIDGEEFFECTS_FIXTURE);
        if (!input) throw std::runtime_error("Missing bridge effects fixture");
        std::vector<std::vector<std::string>> result;
        std::string line;
        // 설명 주석만 건너뛴다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (!line.empty() && line.front()!='#') result.push_back(Split(line,'\t'));
        }
        return result;
    }();
    return rows;
}
// fixture의 부호 없는 DWORD 필드를 읽는다.
std::uint32_t Number(const std::string& value) { return static_cast<std::uint32_t>(std::stoul(value)); }
// 원본 raw 형식으로 필드 바이트를 쓴다. 기존 내용의 나머지는 보존한다.
void Write(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t size) {
    // little endian 순서.
    for (std::size_t i=0;i<size;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
// 같은 풀의 할당 슬롯을 보존한다. 참조 대상의 free 비트 조합도 원본처럼 읽을 수 있게 한다.
struct Scene {
    SidPool pool;
    std::array<std::span<std::uint8_t>,65> slots{};
    std::vector<RiftTypeRecord> types{256};
    std::vector<Sid> selected;
    std::size_t cursor{};
    std::vector<std::string> events;
    bool dynamic{};
    // 입력 슬롯은 이미 할당되어 있고 파생 vtable은 실행하지 않는다.
    explicit Scene(OriginalEdition edition):pool(edition,kCapacity,false) {
        // 클라이언트 영역의 앞 슬롯들을 할당해 합성 SID 50~64를 준비한다.
        for (int i=5;i<=64;++i) {
            const auto sid=pool.Allocate(2);
            CHECK(sid.value==i);
            slots[sid.value]=pool.AllocatedBytes(sid);
        }
        types[74].flags2=4; types[75].flags2=0; types[76].flags2=0x10004; types[77].flags2=0x10000;
    }
    // 실제 원본 입력과 동일한 슬롯/링크/탐색 목록으로 되돌린다.
    void Prepare(std::uint8_t extra,std::uint32_t x,std::uint32_t y,int scenario,bool post) {
        events.clear(); cursor=0; dynamic=false;
        // 각 객체의 raw 슬롯을 초기화한다. 이전 행의 dead/free 상태를 넘기지 않는다.
        for (Sid sid:{kBridge,kLink,kFirst,kSecond,kFollow,kOther}) {
            auto raw=slots[sid.value];
            std::fill(raw.begin(),raw.end(),std::uint8_t{});
            raw[10]=sid==kLink || sid==kFollow ? 75 : 74;
            raw[11]=sid==kFirst ? 2 : 0;
            raw[Extra()]=sid==kBridge ? extra : 0;
            Write(raw,14,x,4); Write(raw,18,y,4);
        }
        Write(slots[kLink.value],12,kFirst.value,2); Write(slots[kLink.value],8,kSecond.value,2);
        Write(slots[kFollow.value],12,kLink.value,2); Write(slots[kFollow.value],8,kSecond.value,2);
        const std::array<std::vector<Sid>,7> plans{{{}, {kLink,kFollow,kOther},{kFollow,kLink,kOther},
            {kLink,kLink,kFollow},{kOther},{kLink,kFollow},{kFollow,kLink}}};
        selected=plans.at(static_cast<std::size_t>(scenario));
        if (scenario==5) slots[kFirst.value][Extra()]=9;
        if (scenario==6) slots[kSecond.value][11]=2;
        if (post) {
            const std::array<std::vector<Sid>,5> postPlans{{{}, {kLink,kFollow,kOther},{kLink,kFollow,kOther},
                {kLink,kLink,kOther},{kFollow}}};
            selected=postPlans.at(static_cast<std::size_t>(scenario));
            slots[kLink.value][10]=77; slots[kFollow.value][10]=75; slots[kOther.value][10]=76;
            dynamic=scenario==2;
        }
    }
    // 판본별 extra 오프셋을 읽는다.
    std::size_t Extra() const { return pool.Edition()==OriginalEdition::Patch1078 ? 40 : 35; }
    // 실제 외부 효과 경계를 기록하고 같은 풀의 dead/type 변경을 적용한다.
    BridgeLifecycleHooks Hooks() {
        return {
            // 두 판본의 실제 절삭/확장으로 만든 탐색 범위와 순서를 기록한다.
            [this](BridgeLifecycleSearch range) {
                events.push_back("Q:"+std::to_string(range.left)+":"+std::to_string(range.top)+":"+
                    std::to_string(range.right)+":"+std::to_string(range.bottom));
                cursor=0; return selected.empty() ? Sid{} : selected[0];
            },
            // 고정 탐색 목록의 다음 항목. 목록은 원본 일반 탐색기를 대체한 입력 계약이다.
            [this] { ++cursor; return cursor<selected.size() ? selected[cursor] : Sid{}; },
            // 가상 destroy/fall은 이 독립 검사의 동일한 대체 효과(dead/type)만 수행한다.
            [this](const BridgeLifecycleEvent& event) {
                const auto id=std::to_string(event.sid.value),flags=std::to_string(event.flags);
                switch (event.effect) {
                case BridgeLifecycleEffect::DestroyLink:
                    events.push_back("D:"+id+":"+flags); slots[event.sid.value][11]|=2; break;
                case BridgeLifecycleEffect::NotifyRemoval: events.push_back("N:"+id); break;
                case BridgeLifecycleEffect::FallSound:
                    events.push_back("S:"+std::to_string(std::bit_cast<std::uint32_t>(event.x))+":"+
                        std::to_string(std::bit_cast<std::uint32_t>(event.y))); break;
                case BridgeLifecycleEffect::FallWalker:
                    events.push_back("F:"+id); slots[event.sid.value][11]|=2;
                    if (dynamic && event.sid==kLink) slots[kFollow.value][10]=76;
                    break;
                case BridgeLifecycleEffect::BasePreDestroy: events.push_back("A:"+id+":"+flags); break;
                case BridgeLifecycleEffect::BasePostDestroy: events.push_back("B:"+id+":"+flags); break;
                }
            }
        };
    }
    // 효과 순서와 현재 슬롯 상태를 fixture의 정확한 표현으로 기록한다.
    std::string Events() const {
        std::string result;
        // 빈 목록은 '-'이며 나머지는 호출 순서다.
        for (const auto& event:events) { if (!result.empty()) result+=';'; result+=event; }
        return result.empty() ? "-" : result;
    }
    // 전체 풀에서 예상한 타입/state 두 필드 외에는 쓰기가 없는지도 검사한다.
    void CheckState(const std::string& expected,std::vector<std::uint8_t> before) const {
        // 각 객체의 독립 기대값을 읽어 풀 사본의 해당 두 바이트만 갱신한다.
        for (const auto& tuple:Split(expected,';')) {
            const auto values=Split(tuple,',');
            const auto offset=static_cast<std::size_t>(Number(values[0]))*pool.Layout().stride;
            before[offset+10]=static_cast<std::uint8_t>(Number(values[1]));
            before[offset+11]=static_cast<std::uint8_t>(Number(values[2]));
        }
        CHECK(std::ranges::equal(before,pool.Bytes()));
    }
};
// 동일한 코드 배치의 CD/추가 10.37을 각각 독립 fixture로 대조한다.
OriginalEdition Edition(const std::string& name) { return name=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072; }
}

TEST_CASE(BridgeLifecycle_X86_LinkReferencesAndAsymmetricExtra) {
    Scene scene(OriginalEdition::Patch1078);
    RawBridgeLifecycle lifecycle(scene.pool,scene.types);
    std::size_t count=0;
    // 패치 helper의 모든 경계/state/extra 조합.
    for (const auto& row:Fixture()) {
        if (row[0]!="Link") continue;
        scene.Prepare(0,0,0,0,false);
        Write(scene.slots[kLink.value],12,Number(row[3]),2); Write(scene.slots[kLink.value],8,Number(row[4]),2);
        scene.slots[kFirst.value][11]=static_cast<std::uint8_t>(Number(row[5]));
        scene.slots[kSecond.value][11]=static_cast<std::uint8_t>(Number(row[6]));
        scene.slots[kFirst.value][scene.Extra()]=static_cast<std::uint8_t>(Number(row[7]));
        scene.slots[kSecond.value][scene.Extra()]=static_cast<std::uint8_t>(Number(row[8]));
        CHECK(lifecycle.LinkNeedsDestroy(kLink)==(row[9]=="D:60:0"));
        ++count;
    }
    CHECK(count==kLinkRows);
}

// 실제 pre/postDestroy 상위 호출의 모든 효과·탐색 범위·슬롯 바이트를 검사한다.
void ReplayLifecycle(std::string_view kind,std::size_t expectedCount) {
    std::size_t count=0;
    // 동일 배치라도 세 PE의 저장된 결과를 각각 읽어 검사한다.
    for (const std::string name:{"originals","originalCD","original1037"}) {
        Scene scene(Edition(name));
        RawBridgeLifecycle lifecycle(scene.pool,scene.types);
        // 해당 판본의 입력 행만 재생한다.
        for (const auto& row:Fixture()) {
            if (row[0]!=kind || row[1]!=name) continue;
            scene.Prepare(static_cast<std::uint8_t>(Number(row[3])),Number(row[5]),Number(row[6]),
                static_cast<int>(Number(row[7])),kind=="Post");
            const std::vector<std::uint8_t> before(scene.pool.Bytes().begin(),scene.pool.Bytes().end());
            const auto hooks=scene.Hooks();
            if (kind=="Pre") lifecycle.PreDestroy(kBridge,Number(row[4]),75,hooks);
            else lifecycle.PostDestroy(kBridge,Number(row[4]),hooks);
            CHECK(scene.Events()==row[8]);
            scene.CheckState(row[9],before);
            ++count;
        }
    }
    CHECK(count==expectedCount);
}

TEST_CASE(BridgeLifecycle_X86_PreDestroyOrderAndDynamicReferences) { ReplayLifecycle("Pre",kPreRows); }
TEST_CASE(BridgeLifecycle_X86_PostDestroyWalkerAndBaseOrder) { ReplayLifecycle("Post",kPostRows); }

TEST_CASE(BridgeLifecycle_X86_DelayedFallPayloadIsNumericFloat) {
    std::size_t delay=0,pack=0;
    // 비유한 값·음수·바이트 경계·float 정밀도 경계도 실제 _ftol 결과와 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!="Delay" && row[0]!="Pack") continue;
        const auto actual=RawBridgeLifecycle::DelayedFallPayload(std::bit_cast<float>(Number(row[3])),std::bit_cast<float>(Number(row[4])));
        CHECK(std::bit_cast<std::uint32_t>(actual)==Number(row[5]));
        row[0]=="Delay" ? ++delay : ++pack;
    }
    CHECK(delay==kDelayRows && pack==kPackRows);
    CHECK(kBridgeFallEvent==0x2692);
}

TEST_CASE(BridgeLifecycle_InvalidHooksAndSIDDoNotStartEffects) {
    Scene scene(OriginalEdition::Patch1078);
    scene.Prepare(0,0,0,0,false);
    RawBridgeLifecycle lifecycle(scene.pool,scene.types);
    auto hooks=scene.Hooks(); hooks.next={};
    bool rejected=false;
    try { lifecycle.PostDestroy(kBridge,0,hooks); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && scene.events.empty());
    rejected=false;
    try { lifecycle.PreDestroy(Sid{},0,75,scene.Hooks()); } catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && scene.events.empty());
}
