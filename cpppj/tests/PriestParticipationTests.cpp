// 실제10.78전체함수의독립출력을같은live/Carrier/Contained/typed finder와공유ownerState로대조한다.
#include "RawSceneSupport.h"
#include "o/RawPriestParticipation.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 독립기계어관찰행수와실제세션의풀크기다.사용하지않는꼬리도free로직접순회한다.
constexpr std::size_t kParticipationRows=304;
// 원본배치입력을공급하는검사전용접근이다.제품의할당/반납경계를확장하지않는다.
std::span<std::uint8_t> Input(SidPool& pool,Sid sid) {
    const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()};
}
// 부호있는해시단계-1과DWORD비트를한입력파서로읽는다.
std::vector<std::int64_t> Values(std::string_view text) {
    std::vector<std::int64_t> values;
    // 각필드는입력정수이며반환값을재계산하지않는다.
    for (const auto& value:Split(text,',')) values.push_back(std::stoll(value));
    return values;
}
// 조회전용Carrier몸체의실행하면안되는삭제효과는빈성공이아닌명시예외다.
CarrierPreDestroyHooks QueryOnlyCarrierHooks() {
    return {[](Sid,std::uint32_t) { throw std::logic_error("참여조회는Carrier하위삭제검사를호출하지않는다"); },
        [](Sid,std::uint32_t) { throw std::logic_error("참여조회는Carrier삭제를호출하지않는다"); },
        [] { throw std::logic_error("참여조회는Carrier후처리를호출하지않는다"); }};
}
// 원본9DWORD상태와동일한쉼표직렬화다.
std::string States(const PriestPostPopState& state) {
    std::string text;
    // 칸0도포함해원본이예약칸을건드리지않는지비교한다.
    for (const auto value:state.ownerState) { if (!text.empty()) text+=',';text+=std::to_string(value); }
    return text;
}
// 매행새입력을만들되제품모듈과현재타입참조는같은인스턴스로유지한다.
struct ParticipationScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; // 원본관찰과동일24000슬롯저장소다.
    SquidHash hash; // 현재일반4단계버킷이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 입력의현재genus와발자국표다.
    PriestPostPopState priests; // live목록과실제결과저장소다.
    ContainedFinderState contained; // Carrier/Contained가공유하는현재전역이다.
    RawCarrierPreDestroy carrier{pool,contained,QueryOnlyCarrierHooks()}; // 기존실제종속조회다.
    std::uint32_t priestType=158,altarType=164,daisType=165; // 같은제품참여객체가행마다다시읽을현재타입DWORD다.
    RawPriestParticipation participation{pool,hash,types,priests,carrier,{priestType,altarType,daisType}}; // 실제몸체만조합한다.
    // 슬롯과체인을동일한명시입력으로준비하고결과는제품몸체가계산하게한다.
    void Prepare(const std::vector<std::string>& row) {
        const auto current=Values(row.at(3));CHECK(current.size()==5);priestType=static_cast<std::uint32_t>(current[0]);altarType=static_cast<std::uint32_t>(current[1]);daisType=static_cast<std::uint32_t>(current[2]);contained.containedType=static_cast<std::uint32_t>(current[3]);contained.boss=current[4]!=0;contained.checkingDead=false;
        priests.priests.count=Number(row[1]);priests.priests.entries.clear();
        // 활성개수밖꼬리도동일하게준비해조기반환과signed개수검사를확인한다.
        for (const auto value:Values(row[2])) priests.priests.entries.push_back(static_cast<std::uint32_t>(value));
        // 실제기계어가쓴상태와정확히같은초기9DWORD다.
        for (std::size_t index=0;index<priests.ownerState.size();++index) priests.ownerState[index]=static_cast<std::uint32_t>(0x9000+index);
        const auto bytes=pool.Bytes();std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};std::fill(raw.begin(),raw.end(),std::uint8_t{});hash.Reset();std::fill(types.begin(),types.end(),RiftTypeRecord{});
        // 미사용슬롯전체를free로둔다.풀외곽을짧은가짜범위로대체하지않는다.
        for (std::uint32_t sid=0;sid<pool.Capacity();++sid) raw[sid*pool.Layout().stride+11]=1;
        // 명시입력의배열순서대로머리를삽입한다.포획여부/선택결과는입력하지않는다.
        for (const auto& entry:Split(row[4],';')) {
            const auto node=Values(entry);CHECK(node.size()==16);const auto number=static_cast<std::uint16_t>(node[0]);auto slot=Input(pool,Sid{number});std::fill(slot.begin(),slot.end(),std::uint8_t{});const auto type=static_cast<std::size_t>(node[1]);slot[10]=static_cast<std::uint8_t>(type);slot[11]=static_cast<std::uint8_t>(node[2]);slot[34]=static_cast<std::uint8_t>(node[3]);Put(slot,12,static_cast<std::uint32_t>(node[4]),2);slot[40]=static_cast<std::uint8_t>(node[5]);Put(slot,6,static_cast<std::uint32_t>(node[6]),2);Put(slot,4,static_cast<std::uint32_t>(node[7]),2);Put(slot,14,static_cast<std::uint32_t>(node[10]));Put(slot,18,static_cast<std::uint32_t>(node[11]));
            if (node[8] || node[9] || type==contained.containedType) { Put(slot,18,static_cast<std::uint32_t>(node[8]),2);Put(slot,20,static_cast<std::uint32_t>(node[9]),2); }
            types.at(type).flags2=static_cast<std::uint32_t>(node[15]);types[type].footX=static_cast<int>(node[13]);types[type].footY=static_cast<int>(node[14]);
            if (node[12]>=0) { const auto level=static_cast<int>(node[12]);const auto x=std::bit_cast<float>(static_cast<std::uint32_t>(node[10])),y=std::bit_cast<float>(static_cast<std::uint32_t>(node[11]));auto& head=hash.Bucket(level,x,y);Put(slot,4,head,2);head=number;slot[33]=static_cast<std::uint8_t>(level); }
        }
    }
};
}

// 두x87정밀도로각각완료한Find/Refresh네번의원본관찰304행을실제공유상태와대조한다.
TEST_CASE(PriestParticipation_ReplaysCompleteNativeBodies) {
    const auto fixture=LoadFixture(NETSTORM_PRIEST_PARTICIPATION_FIXTURE);CHECK(fixture.rows.size()==kParticipationRows);ParticipationScene scene;std::size_t checked=0;
    // 각행에서전체풀/목록/버킷을다시준비해앞행출력에의존하지않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==9);scene.Prepare(row);const auto player=Number(row[0]);const auto before=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());const auto entries=scene.priests.priests.entries;const auto initial=scene.priests.ownerState;std::array<std::vector<std::uint16_t>,4> heads;
        // 모든실제해시배열을보존해조회가등록체인을변경하지않는지검사한다.
        for (int level=0;level<4;++level) { const auto source=scene.hash.Entries(level);heads[static_cast<std::size_t>(level)]=std::vector<std::uint16_t>(source.begin(),source.end()); }
        const auto result=scene.participation.Find(player);CHECK(result==Number(row[5]));CHECK(scene.priests.ownerState==initial);scene.participation.Refresh(player);CHECK(States(scene.priests)==row[6]);CHECK(std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));CHECK(Adler(scene.pool.Bytes())==Number(row[7]));CHECK(scene.priests.priests.count==Number(row[1]) && scene.priests.priests.entries==entries);
        std::array<std::uint8_t,16> list{};
        // 개수밖꼬리까지원본목록checksum과비교한다.
        for (std::size_t index=0;index<entries.size();++index) Put(list,index*4,entries[index]);
        CHECK(Adler(list)==Number(row[8]));
        // 각단계의버킷머리도모두읽기전용이다.
        for (int level=0;level<4;++level) CHECK(std::equal(heads[static_cast<std::size_t>(level)].begin(),heads[static_cast<std::size_t>(level)].end(),scene.hash.Entries(level).begin()));
        CHECK(&scene.participation.Pool()==&scene.pool && &scene.participation.Priests()==&scene.priests);++checked;
    }
    CHECK(checked==kParticipationRows);
}

// live의이른성공뒤손상된후속목록/타입/다이스전역을읽지않고전체DWORD소유자폭을유지한다.
TEST_CASE(PriestParticipation_PreservesLazyReadsAndDwordOwners) {
    ParticipationScene scene;scene.priests.priests.entries={50,0x10032};scene.priests.priests.count=2;auto live=Input(scene.pool,Sid{50});std::fill(live.begin(),live.end(),std::uint8_t{});live[10]=255;live[11]=9;live[34]=1;scene.daisType=0xffffffff;
    CHECK(scene.participation.Find(1)==50);scene.participation.Refresh(1);CHECK(scene.priests.ownerState[1]==1);CHECK(Throws([&] { scene.participation.Find(257); }));
    scene.priests.priests.count=0;live[11]=0;CHECK(Throws([&] { scene.participation.Find(1); }));
    live[10]=82;live[11]=1;scene.priests.ownerState.fill(77);scene.participation.Refresh(0);scene.participation.Refresh(9);scene.participation.Refresh(0xffffffff);
    CHECK(std::all_of(scene.priests.ownerState.begin(),scene.priests.ownerState.end(),[](std::uint32_t value) { return value==77; }));
}

// same풀Carrier연결만허용하고실제접근한종속순환을조용한실패로돌리지않는다.
TEST_CASE(PriestParticipation_RejectsDifferentPoolAndBrokenContainedChain) {
    ParticipationScene scene;SidPool other(OriginalEdition::Patch1078,kCapacity,false);RawCarrierPreDestroy foreign(other,scene.contained,QueryOnlyCarrierHooks());
    CHECK(Throws([&] { RawPriestParticipation wrong(scene.pool,scene.hash,scene.types,scene.priests,foreign,{scene.priestType,scene.altarType,scene.daisType}); }));
    auto parent=Input(scene.pool,Sid{5});std::fill(parent.begin(),parent.end(),std::uint8_t{});parent[10]=82;Put(parent,6,10,2);scene.types[82].flags2=0x10000;auto contained=Input(scene.pool,Sid{10});std::fill(contained.begin(),contained.end(),std::uint8_t{});contained[10]=6;Put(contained,4,10,2);Put(contained,18,157,2);Put(contained,20,0x20,2);
    CHECK(Throws([&] { scene.participation.Find(1); }));
}
