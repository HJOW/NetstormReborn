// 실제 좌표/finder/타입/소유자 조회와 사제 삭제 준비의 독립 원본 결과를 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestForcefield.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 PE별 조회/삭제 준비 행 수와 합성 raw 입력 번호다.
constexpr std::size_t kRowsPerEdition=2976;
constexpr std::array<Sid,7> kIds{{{50},{60},{61},{62},{63},{64},{65}}};
// Python/원본 없이 저장된 독립 출력만 읽는다.
const auto& Fixture() { static const auto data=LoadFixture(NETSTORM_PRIESTFORCEFIELD_FIXTURE);return data; }
struct ForcefieldScene {
    SidPool pool;
    SquidHash hash;
    std::vector<RiftTypeRecord> types;
    std::array<std::span<std::uint8_t>,66> slots{};
    PriestForcefieldState forcefield;
    PriestPostPopState priests;
    std::string events;
    bool patch;
    // 모든 입력 슬롯을 미리 확보하고 free 상태 표본은 이 원래 메모리 범위에서 준비한다.
    explicit ForcefieldScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // 입력 번호를 client SID로 확보한다.
        for (std::uint16_t id=5;id<=65;++id) { CHECK(pool.Allocate(2)==Sid{id});slots[id]=pool.AllocatedBytes(Sid{id}); }
    }
    // node 필드: sid/type/state/extra/x/y/footX/footY/owner/level. 판단 결과는 계산하지 않는다.
    void PutNode(const std::vector<std::string>& fields,bool registered) {
        CHECK(fields.size()==10);const Sid sid{static_cast<std::uint16_t>(Number(fields[0]))};auto raw=slots.at(sid.value);
        std::fill(raw.begin(),raw.end(),std::uint8_t{});const auto number=Number(fields[1]);
        Put(raw,0,sid==kSource ? (patch ? kPatchPriestVtable : kCdPriestVtable) : kVtable);
        raw[10]=static_cast<std::uint8_t>(number);raw[11]=static_cast<std::uint8_t>(Number(fields[2]));
        raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(fields[3]));raw[patch ? 34 : 32]=static_cast<std::uint8_t>(Number(fields[8]));
        Put(raw,14,Number(fields[4]));Put(raw,18,Number(fields[5]));types.at(number).footX=std::stoi(fields[6]);types.at(number).footY=std::stoi(fields[7]);
        if (registered) {
            auto& head=hash.Bucket(std::stoi(fields[9]),std::bit_cast<float>(Number(fields[4])),std::bit_cast<float>(Number(fields[5])));
            Put(raw,4,head,2);head=sid.value;
        }
    }
    // 장면 입력만 재현한다. 보호막 검색 결과와 목록 압축은 여기서 만들지 않는다.
    void Prepare(const std::vector<std::string>& scene) {
        CHECK(scene.size()==8);hash.Reset();events.clear();
        // 앞 관찰의 raw/발자국을 남기지 않는다.
        for (auto sid:kIds) std::fill(slots[sid.value].begin(),slots[sid.value].end(),std::uint8_t{});
        const std::vector<std::string> root{"50","158",scene[3],scene[4],scene[0],scene[1],"1","1",scene[2],"0"};
        PutNode(root,Number(scene[6])!=0);
        // 등록 순서만 복원하고 first SID는 실제 finder가 결정하게 둔다.
        for (const auto& node:Split(scene[7],';')) PutNode(Split(node,','),true);
        forcefield.type=Number(scene[5]);priests.priests={{50,60,50,61,50,62},6};
    }
    // 미복원 파생 삭제 효과만 원본 실행기와 같은 인자로 기록한다.
    PriestPreDestroyHooks Hooks() {
        return {{},[this](Sid sid,std::uint32_t flags) { Record('D',sid,flags); },[](Sid,std::uint32_t) {},
            [this](Sid sid,std::uint32_t flags) { Record('C',sid,flags); }};
    }
    // 외부 효과의 순서와 전달 인자를 기록한다.
    void Record(char marker,Sid sid,std::uint32_t flags) {
        if (!events.empty()) events+=';';events+=std::string(1,marker)+':'+std::to_string(sid.value)+':'+std::to_string(flags);
    }
    // 비활성 꼬리도 원본에 남으므로 목록 저장소 전체를 검사한다.
    std::string Entries() const {
        std::string result;
        // 입력 저장소 순서를 유지한다.
        for (auto value:priests.priests.entries) { if (!result.empty()) result+=',';result+=std::to_string(value); }
        return result;
    }
    // 후보를 포함한 모든 입력 슬롯의 필드가 조회에서 변하지 않는지 검사한다.
    std::string Slots() const {
        std::string result;
        // 원본 관찰의 IDS 순서대로 raw 슬롯 전체를 잇는다.
        for (auto sid:kIds) { if (!result.empty()) result+=';';result+=std::to_string(sid.value)+':'+Hex(pool.Slot(sid)); }
        return result;
    }
};
// 두 raw 배치에서 세 PE의 독립 조회 및 preDestroy 결과를 전부 재생한다.
void Replay(std::string_view edition) {
    ForcefieldScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().rows.size()==3*kRowsPerEdition);
    // 각 행을 독립적으로 초기화한다. 조회 객체에는 입력 타입 발자국을 복사한다.
    for (const auto& row:Fixture().rows) {
        if (row[1]!=edition) continue;CHECK(row.size()==9);scene.Prepare(Fixture().scenes.at(row[2]));
        RawPriestForcefield lookup(scene.pool,scene.hash,scene.types,scene.forcefield);bool same=true;
        if (row[0]=="Find") same=lookup.Find(kSource).value==Number(row[4]);
        else { RawPriestPreDestroy pre(scene.pool,scene.priests,MakePriestForcefieldHooks(scene.pool,lookup,scene.Hooks()));pre.PreDestroy(kSource,Number(row[3])); }
        same=same && scene.priests.priests.count==Number(row[5]) && scene.Entries()==row[6] && (scene.events.empty() ? "-" : scene.events)==row[7] && scene.Slots()==row[8];
        CHECK(same);
        if (!same) { std::printf("보호막 %s %s 장면 %s: 기대 %s / 실제 %u, %s\n",std::string(edition).c_str(),row[0].c_str(),row[2].c_str(),row[4].c_str(),lookup.Find(kSource).value,scene.events.c_str());break; }
        ++count;
    }
    CHECK(count==kRowsPerEdition);
}
}
// 직접 조회는 대체 없이 실제 patch 공간 탐색 결과를 비교한다.
TEST_CASE(priest_forcefield_patch_x86_lookup_and_predestroy) { Replay("originals"); }
// CD의 별도 좌표/CRT/타입 발자국 getter를 포함한 관찰이다.
TEST_CASE(priest_forcefield_cd_x86_lookup_and_predestroy) { Replay("originalCD"); }
// 추가 10.37 실제 PE는 CD 배치로 별도 재생한다.
TEST_CASE(priest_forcefield_1037_x86_lookup_and_predestroy) { Replay("original1037"); }

// 손상 사제/좌표/타입 및 다른 풀은 원본 assert 실행 대신 C++ 보호 예외로 확인한다.
TEST_CASE(priest_forcefield_guards_and_read_only_lookup) {
    // 서로 다른 raw 배치의 같은 조회 계약을 확인한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        ForcefieldScene scene(edition);scene.Prepare(Fixture().scenes.begin()->second);
        RawPriestForcefield lookup(scene.pool,scene.hash,scene.types,scene.forcefield);const auto before=scene.Slots();const auto list=scene.Entries();
        CHECK(Throws([&] { lookup.Find({0}); }));CHECK(Throws([&] { lookup.Find({24000}); }));CHECK(scene.Slots()==before && scene.Entries()==list);
        scene.slots[kSource.value][11]=1;CHECK(Throws([&] { lookup.Find(kSource); }));scene.slots[kSource.value][11]=8;CHECK(Throws([&] { lookup.Find(kSource); }));
        scene.slots[kSource.value][11]=0;Put(scene.slots[kSource.value],0,kVtable);CHECK(Throws([&] { lookup.Find(kSource); }));
        Put(scene.slots[kSource.value],0,scene.patch ? kPatchPriestVtable : kCdPriestVtable);
        const auto old=Get(scene.slots[kSource.value],14);
        // 유한한 signed int 범위를 넘는 입력은 임의 포인터/정수 감김으로 복제하지 않는다.
        for (float coordinate:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),2147483648.0f}) {
            Put(scene.slots[kSource.value],14,std::bit_cast<std::uint32_t>(coordinate));CHECK(Throws([&] { lookup.Find(kSource); }));
        }
        Put(scene.slots[kSource.value],14,old);SidPool other(edition,kCapacity,false);
        CHECK(Throws([&] { MakePriestForcefieldHooks(other,lookup,scene.Hooks()); }));
        CHECK(Throws([&] { RawPriestForcefield invalid(scene.pool,scene.hash,std::span(scene.types).first(100),scene.forcefield); }));
        scene.forcefield.type=256;CHECK(lookup.Find(kSource)==Sid{} && scene.events.empty());
    }
}
