// 세 실제 PE의 일반 탐색 커서/동적 raw 변경과 실제 다리 삭제 훅 연결을 검사한다.
#include "TestSupport.h"
#include "o/RawSquidFinder.h"
#include "o/RawBridgeLifecycle.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 독립 기계어 출력의 판본/정밀도/행 수다. 잘린 파일은 실패한다.
constexpr std::size_t kFindRows=1752,kPreRows=12,kPostRows=12;
// 합성 등록 상태의 풀 크기/번호. 실제 월드의 생성·등록 순서는 별도다.
constexpr std::uint32_t kCapacity=24000;
constexpr std::array<Sid,6> kIds{{{50},{60},{61},{62},{63},{64}}};
// 빈 마지막 필드까지 보존하는 fixture 구분자 판독이다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> fields; std::size_t begin=0;
    // 마지막 필드를 추가한 뒤 종료한다.
    for (;;) {
        const auto end=text.find(separator,begin);fields.emplace_back(text.substr(begin,end==std::string_view::npos ? end : end-begin));
        if (end==std::string_view::npos) return fields;
        begin=end+1;
    }
}
// 저장된 원본 기계어 출력만 읽는다. 실행 파일/Ghidra/Python은 테스트에 필요 없다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=[] {
        std::ifstream input(NETSTORM_RAWFINDER_FIXTURE);
        if (!input) throw std::runtime_error("Missing raw finder fixture");
        std::vector<std::vector<std::string>> result;std::string line;
        // 주석만 제외한다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (!line.empty() && line.front()!='#') result.push_back(Split(line,'\t'));
        }
        return result;
    }();return rows;
}
// little endian raw 필드만 쓴다. 호스트에서 원본 vtable을 호출하지 않는다.
void Write(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 필요한 바이트만 갱신한다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 원본 finder +4..+60의 실제 DWORD 순서로 비교한다. 확장 높이 플래그의 원래 값은 4다.
std::string Snapshot(const RawSquidFinder& finder) {
    const auto& s=finder.State();const auto& a=s.area;
    const std::array<int,15> values{a.left,a.top,a.right,a.bottom,s.firstX,s.firstY,s.lastX,s.lastY,
        s.level,s.x,s.y,s.next.value,s.current.value,s.skipSurface ? 1 : 0,s.extendedHeight ? 4 : 0};
    std::string result;
    // 커서뿐 아니라 버킷 범위와 미리 읽은 next도 확인한다.
    for (int value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }
    return result;
}
// 합성 슬롯을 유지하고 장면마다 체인/좌표/타입/상태를 다시 입력한다.
struct Scene {
    SidPool pool;
    SquidHash hash;
    std::array<std::span<std::uint8_t>,65> slots{};
    std::vector<RiftTypeRecord> types{256};
    std::vector<int> heights=std::vector<int>(256);
    // 이미 할당한 슬롯에 free 비트 입력도 가능하게 한다. 풀 자체의 할당 검증과 구분한다.
    explicit Scene(OriginalEdition edition):pool(edition,kCapacity,false) {
        // 번호 50~64까지의 입력 슬롯을 확보한다.
        for (int i=5;i<=64;++i) { const auto sid=pool.Allocate(2);CHECK(sid.value==i);slots[sid.value]=pool.AllocatedBytes(sid); }
    }
    // 복원한 일반 탐색의 extra 오프셋은 판본별로 다르다.
    std::size_t Extra() const { return pool.Edition()==OriginalEdition::Patch1078 ? 40 : 35; }
    // fixture의 입력 객체를 y/x 결과 계산 없이 버킷 머리에 등록한다.
    void Setup(const std::string& description) {
        hash.Reset();std::fill(heights.begin(),heights.end(),0);
        // 각 타입의 발자국과 높이 확장은 fixture 입력이다.
        for (const auto& object:Split(description,';')) {
            const auto fields=Split(object,',');CHECK(fields.size()==10);
            std::array<std::uint32_t,10> v{};
            // 좌표는 float 비트로 보존한다.
            for (std::size_t i=0;i<v.size();++i) v[i]=static_cast<std::uint32_t>(std::stoul(fields[i]));
            auto raw=slots.at(v[0]);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            raw[10]=static_cast<std::uint8_t>(v[1]);raw[11]=static_cast<std::uint8_t>(v[2]);raw[Extra()]=static_cast<std::uint8_t>(v[3]);
            Write(raw,14,v[4],4);Write(raw,18,v[5],4);
            types[v[1]].footX=static_cast<int>(v[6]);types[v[1]].footY=static_cast<int>(v[7]);heights[v[1]]=static_cast<int>(v[8]);
            auto& head=hash.Bucket(static_cast<int>(v[9]),std::bit_cast<float>(v[4]),std::bit_cast<float>(v[5]));
            Write(raw,4,head,2);head=static_cast<std::uint16_t>(v[0]);
        }
    }
    // 기존 실제 삭제 훅이 읽을 링크 참조/소수 좌표/버킷 입력을 두 순서로 준비한다.
    void Lifecycle(bool post,bool reverse) {
        std::string description;auto ids=kIds;if (reverse) std::reverse(ids.begin(),ids.end());
        // 같은 버킷의 역순은 낙하 콜백이 뒤 후보의 타입을 바꾸는 순서도 바꾼다.
        for (Sid sid:ids) {
            int type=(sid.value==60 || sid.value==63) ? 75 : 74;
            if (post) { if (sid.value==60) type=77;else if (sid.value==64) type=76; }
            if (!description.empty()) description+=';';
            description+=std::to_string(sid.value)+','+std::to_string(type)+','+(sid.value==61 ? "2" : "0")+",0,"+
                std::to_string(std::bit_cast<std::uint32_t>(20.75f))+','+std::to_string(std::bit_cast<std::uint32_t>(21.9f))+
                ",1,1,0,"+(sid.value==50 ? "0" : "1");
        }
        Setup(description);types[74].flags2=4;types[75].flags2=0;types[76].flags2=0x10004;types[77].flags2=0x10000;
        Write(slots[60],12,61,2);Write(slots[60],8,62,2);Write(slots[63],12,60,2);Write(slots[63],8,62,2);
    }
    // 기계어의 외부 효과 대체와 같은 변경을 적용하며 호출 순서를 기록한다.
    void Emit(const BridgeLifecycleEvent& event,std::vector<std::string>& events,bool post) {
        const auto id=std::to_string(event.sid.value),flags=std::to_string(event.flags);
        switch (event.effect) {
        case BridgeLifecycleEffect::DestroyLink:events.push_back("D:"+id+':'+flags);slots[event.sid.value][11]|=2;break;
        case BridgeLifecycleEffect::NotifyRemoval:events.push_back("N:"+id);break;
        case BridgeLifecycleEffect::FallSound:events.push_back("S:"+std::to_string(std::bit_cast<std::uint32_t>(event.x))+':'+
            std::to_string(std::bit_cast<std::uint32_t>(event.y)));break;
        case BridgeLifecycleEffect::FallWalker:
            events.push_back("F:"+id);slots[event.sid.value][11]|=2;
            if (post && event.sid.value==60) slots[63][10]=76;
            break;
        case BridgeLifecycleEffect::BasePreDestroy:events.push_back("A:"+id+':'+flags);break;
        case BridgeLifecycleEffect::BasePostDestroy:events.push_back("B:"+id+':'+flags);break;
        }
    }
    // 타입/state의 효과 후 결과를 같은 입력 번호 순서로 읽는다.
    std::string States() const {
        std::string result;
        // 출력은 정렬하지 않고 부모 검사기의 IDS 입력 순서로 맞춘다.
        for (Sid sid:kIds) {
            if (!result.empty()) result+=';';
            const auto raw=pool.Slot(sid);result+=std::to_string(sid.value)+','+std::to_string(raw[10])+','+std::to_string(raw[11]);
        }
        return result;
    }
};
// 원본 효과 목록의 직렬화다. 빈 목록은 - 표식을 쓴다.
std::string Events(const std::vector<std::string>& events) {
    std::string result;
    // 외부 효과의 호출 순서를 바꾸지 않는다.
    for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }
    return result.empty() ? "-" : result;
}
// 예외 보호는 원본의 assert/무한 루프를 실행하는 대신 새 코드에서 확인한다.
template<class F> bool Throws(F action) { try { action();return false; } catch (const std::exception&) { return true; } }
}

TEST_CASE(RawFinder_OriginalMachineCursorAndDynamicInputs) {
    Scene patch(OriginalEdition::Patch1078),cd(OriginalEdition::Cd1072);std::size_t count=0;
    // 저장된 기계어 출력의 모든 행을 두 판본에 대응시킨다. 추가 10.37도 CD 배치로 검사한다.
    for (const auto& row:Fixture()) {
        if (row[0]!="Find") continue;
        CHECK(row.size()==8);++count;
        auto& scene=row[1]=="originals" ? patch : cd;scene.Setup(row[6]);
        const auto area=Split(row[4],',');RawSquidFinder finder(scene.pool,scene.hash,scene.types,scene.heights);
        const auto flags=static_cast<std::uint32_t>(std::stoul(row[3]));
        auto current=finder.Begin({std::stoi(area[0]),std::stoi(area[1]),std::stoi(area[2]),std::stoi(area[3])},flags);
        std::string output=Snapshot(finder);int index=0;
        // 원본과 같은 시점에 입력만 변경하고 후속 커서를 비교한다.
        while (current.value!=0 && index<32) {
            if (index==0) {
                const int mutation=std::stoi(row[5]);
                if (mutation==1) Write(scene.slots[current.value],4,0,2);
                else if (mutation==2) scene.slots[63][scene.Extra()]=8;
                else if (mutation==3) { Write(scene.slots[63],14,std::bit_cast<std::uint32_t>(100.25f),4);Write(scene.slots[63],18,std::bit_cast<std::uint32_t>(100.75f),4); }
                else if (mutation==4) scene.slots[63][10]=static_cast<std::uint8_t>(std::stoul(Split(Split(row[6],';')[0],',')[1]));
            }
            current=finder.Next();output+=';'+Snapshot(finder);++index;
        }
        CHECK(index<32);CHECK(output==row[7]);
    }
    CHECK(count==kFindRows);
}

TEST_CASE(RawFinder_ActualPreDestroyTraversal) {
    std::size_t count=0;
    // 일반 탐색 결과를 별도 합성 목록으로 대신하지 않고 다리 삭제 전 훅에 연결한다.
    for (const auto& row:Fixture()) {
        if (row[0]!="Pre") continue;
        ++count;Scene scene(row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
        scene.Lifecycle(false,std::stoi(row[3])!=0);RawSquidFinder finder(scene.pool,scene.hash,scene.types);
        RawBridgeLifecycle lifecycle(scene.pool,scene.types);std::vector<std::string> events;
        auto hooks=MakeBridgeLifecycleHooks(finder,[&](const BridgeLifecycleEvent& e) { scene.Emit(e,events,false); });
        lifecycle.PreDestroy({50},0x12345678,75,hooks);
        CHECK(Events(events)==row[4]);CHECK(scene.States()==row[5]);
    }
    CHECK(count==kPreRows);
}

TEST_CASE(RawFinder_ActualPostDestroyDynamicWalkerTraversal) {
    std::size_t count=0;
    // 같은 버킷의 반대 순서에서도 이전 낙하 콜백의 타입 변경을 다음 판단에 반영한다.
    for (const auto& row:Fixture()) {
        if (row[0]!="Post") continue;
        ++count;Scene scene(row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
        scene.Lifecycle(true,std::stoi(row[3])!=0);RawSquidFinder finder(scene.pool,scene.hash,scene.types);
        RawBridgeLifecycle lifecycle(scene.pool,scene.types);std::vector<std::string> events;
        auto hooks=MakeBridgeLifecycleHooks(finder,[&](const BridgeLifecycleEvent& e) { scene.Emit(e,events,true); });
        lifecycle.PostDestroy({50},0x12345678,hooks);
        CHECK(Events(events)==row[4]);CHECK(scene.States()==row[5]);
    }
    CHECK(count==kPostRows);
}

TEST_CASE(RawFinder_RejectsUnstartedBrokenChainAndUnsupportedFlags) {
    Scene scene(OriginalEdition::Patch1078);scene.Lifecycle(false,false);
    RawSquidFinder finder(scene.pool,scene.hash,scene.types);
    CHECK(Throws([&] { finder.Next(); }));CHECK(Throws([&] { finder.Begin({},8); }));CHECK(Throws([&] { finder.Begin({},4); }));
    // 자기 순환은 반환 다음 호출에서 거부한다. 다른 버킷의 중복 번호는 이 보호와 별개다.
    Write(scene.slots[50],4,50,2);CHECK(finder.Begin({20,21,20,21}).value==50);CHECK(Throws([&] { finder.Next(); }));
    scene.hash.Cell(0,20,21)=65535;CHECK(Throws([&] { finder.Begin({20,21,20,21}); }));
}

TEST_CASE(RawFinder_FilterUsesCachedNextAndLiveCandidates) {
    Scene scene(OriginalEdition::Patch1078);scene.Lifecycle(false,false);
    // 다음 번호는 필터보다 먼저 저장되므로 필터가 현재 next를 지워도 뒤 후보를 방문한다.
    scene.hash.Cell(0,20,21)=60;Write(scene.slots[60],4,63,2);Write(scene.slots[63],4,0,2);
    RawSquidFinder finder(scene.pool,scene.hash,scene.types);std::vector<std::uint16_t> visited;
    const auto result=finder.Begin({20,21,20,21},0,[&](Sid sid) {
        visited.push_back(sid.value);
        if (sid.value==60) { Write(scene.slots[60],4,0,2);return false; }
        return true;
    });
    CHECK(result.value==63);CHECK((visited==std::vector<std::uint16_t>{60,63}));
    // 아직 읽지 않은 버킷 머리는 다음 호출에서 새 값을 사용한다.
    scene.hash.Cell(0,21,21)=62;CHECK(finder.Next().value==62);
}

TEST_CASE(RawFinder_CoordinateEditionAndReadOnlyPoolContract) {
    Scene patch(OriginalEdition::Patch1078),cd(OriginalEdition::Cd1072);
    // 정상 조회는 raw 풀과 버킷을 바꾸지 않는다. C# 게임/실제 GUI와 무관한 계약이다.
    for (Scene* scene:{&patch,&cd}) {
        scene->Lifecycle(false,false);const auto before=std::vector<std::uint8_t>(scene->pool.Bytes().begin(),scene->pool.Bytes().end());
        std::array<std::vector<std::uint16_t>,4> heads;
        // 조회 전 네 단계의 버킷 머리를 모두 보존한다.
        for (int level=0;level<4;++level) {
            const auto entries=scene->hash.Entries(level);heads[level].assign(entries.begin(),entries.end());
        }
        RawSquidFinder finder(scene->pool,scene->hash,scene->types);
        for (Sid current=finder.Begin({20,21,20,21});current.value!=0;current=finder.Next()) { /* 읽기 전용 탐색을 끝낸다. */ }
        CHECK(std::equal(before.begin(),before.end(),scene->pool.Bytes().begin()));
        // 풀과 마찬가지로 모든 해시 머리도 그대로여야 한다.
        for (int level=0;level<4;++level) CHECK(std::equal(heads[level].begin(),heads[level].end(),scene->hash.Entries(level).begin()));
    }
    // 패치는 0 좌표를 건너뛰고 CD는 원본 assert 대신 예외로 거부한다.
    Write(patch.slots[50],14,0,4);Write(cd.slots[50],14,0,4);
    RawSquidFinder p(patch.pool,patch.hash,patch.types),c(cd.pool,cd.hash,cd.types);
    CHECK(p.Begin({20,21,20,21}).value!=50);CHECK(Throws([&] { c.Begin({20,21,20,21}); }));
}
