// 보호막 생성 전체 원본 관찰과 실제 생성자/소유자/조회·삭제 준비 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestShield.h"

namespace {
using namespace netstorm::test::rawscene;
// 각 PE 독립 관찰 수·새 자산 입력 번호·소리 조회 반환값이다.
constexpr std::size_t kRows=832;
constexpr Sid kShield{100};
constexpr std::uint32_t kSound=0x12345678;
// NaN payload와 부호 있는 0까지 동일한 double 입력을 재생한다.
constexpr std::array<std::uint64_t,4> kClocks{0,0x8000000000000000ULL,0x3ff0000000000000ULL,0x7ff8000000001234ULL};
// 입력 슬롯을 실제 할당/Pop 없이 공급할 때만 쓰는 독립 관찰 어댑터다.
std::span<std::uint8_t> InputRaw(SidPool& pool,Sid sid) {
    const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()};
}
// Python/원본 파일 없이 기대 출력만 한 번 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_PRIESTSHIELD_FIXTURE).rows;return rows; }
struct ShieldScene {
    SidPool pool;
    SquidHash hash;
    std::vector<RiftTypeRecord> types;
    PriestForcefieldState forcefield;
    PriestShieldState state;
    bool patch;
    // 타입과 해시/현재 전역은 행마다 바꾸되 풀 객체 자체는 안정된 주소를 유지한다.
    explicit ShieldScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {}
    // 원본 장면의 raw 자산을 준비하고 지정한 해시 단계에만 넣는다.
    void Node(Sid sid,std::uint8_t type,std::uint8_t status,std::uint8_t extra,float x,float y,std::uint8_t owner,int level,bool registered) {
        auto raw=InputRaw(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,sid==kSource ? (patch ? kPatchPriestVtable : kCdPriestVtable) : kVtable);raw[10]=type;raw[11]=status;
        raw[patch ? 40 : 35]=extra;raw[patch ? 34 : 32]=owner;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));
        types[type].footX=types[type].footY=1;
        if (registered) { auto& head=hash.Bucket(level,x,y);Put(raw,4,head,2);head=sid.value; }
    }
    // 결과를 계산하지 않고 원본 실행기와 같은 등록 순서/입력만 준비한다.
    void Prepare(const std::vector<std::string>& row) {
        const auto index=Number(row[1]),owner=Number(row[2]);const float x=index%2==0 ? 20.75F : 254.9F,y=index%2==0 ? 21.9F : 254.25F;
        const std::array<std::uint8_t,4> status{0,2,4,6},extra{0,1,8,9};hash.Reset();types.assign(types.size(),RiftTypeRecord{});
        Node(kSource,kPriestType,status[index%4],extra[index%4],x,y,static_cast<std::uint8_t>(owner),0,false);
        // 입력 후보는 다른 소유자/타입·매장·다른 위치·해시 순서를 가진다.
        if (index) for (const auto i:index==7 ? std::array<int,2>{1,0} : std::array<int,2>{0,1}) {
            Node(Sid{static_cast<std::uint16_t>(60+i)},index==3 ? 166 : 167,status[(index+static_cast<std::uint32_t>(i))%4],index==4 ? 8 : 0,
                index==6 ? x-1 : x,y,static_cast<std::uint8_t>(index==2 ? (owner+1)%9 : owner),(static_cast<int>(index)+i)%4,true);
        }
        forcefield.type=index==5 ? 168 : 167;state={Number(row[6]),Number(row[5]),std::bit_cast<double>(kClocks[Number(row[4])])};
        auto raw=InputRaw(pool,kShield);std::fill(raw.begin(),raw.end(),std::uint8_t{0xab});
    }
    // 외부 효과가 바꾸는 입력만 제공하며 생성/알림 판단을 구현하지 않는다.
    void Change(const std::vector<std::string>& row,std::uint32_t stage) {
        if (Number(row[7])!=stage) return;auto raw=InputRaw(pool,kSource);
        if (stage==1) { Put(raw,14,0x7fc12345);Put(raw,18,0x80000000);raw[patch ? 34 : 32]=static_cast<std::uint8_t>((Number(row[2])+1)%9); }
        else if (stage==2) { Put(raw,14,std::bit_cast<std::uint32_t>(99.5F));Put(raw,18,std::bit_cast<std::uint32_t>(98.5F)); }
        else if (stage==3) { raw[patch ? 34 : 32]=8;state.localPlayer=8;state.loadingDepth=0;state.noticeClock=0; }
        else if (stage==4) state.noticeClock=std::bit_cast<double>(kClocks[3]);
    }
};
// 조회와 공통 소유자는 실제 모듈이며 미복원 생성/Pop·소리/문구 경계만 관찰한다.
void Replay(std::string_view edition) {
    ShieldScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);CHECK(Fixture().size()==3*kRows);std::size_t count=0;
    // 각 원본 행의 전체 raw/전역/사건 순서를 그대로 대조한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==14);scene.Prepare(row);std::string events;
        // 외부 효과의 사건은 실제 전달 인자로만 기록한다.
        const auto append=[&](const std::string& value) { if (!events.empty()) events+=';';events+=value; };
        SquidPostPopState books;SquidOwnerMode mode;SquidOwner owner(scene.pool,scene.types,books,mode);
        RawPriestForcefield lookup(scene.pool,scene.hash,scene.types,scene.forcefield);
        RawPriestShield shield(scene.pool,lookup,scene.state,{
            [&](std::uint32_t type,std::uint32_t flags) {
                append("C:"+std::to_string(type)+":"+std::to_string(flags));auto raw=InputRaw(scene.pool,kShield);std::fill(raw.begin(),raw.end(),std::uint8_t{});
                Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=4;scene.Change(row,1);return kShield;
            },[&](Sid sid,std::uint32_t value) {
                append("O:"+std::to_string(sid.value)+":"+std::to_string(value));scene.Change(row,2);owner.Set(sid,value);
            },[&](Sid sid,float x,float y,std::uint32_t flags) {
                append("P:"+std::to_string(sid.value)+":"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y))+":"+std::to_string(flags));
                auto raw=InputRaw(scene.pool,sid);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));raw[11]=0;scene.Change(row,3);
            },[&] { append("A:40:"+row[3]);return Number(row[3])!=0; },[&](std::string_view file) {
                append("L:"+std::string(file));scene.Change(row,4);return kSound;
            },[&](Sid sid,std::uint32_t sound,std::uint32_t first,std::uint32_t second) {
                append("S:"+std::to_string(sid.value)+":"+std::to_string(sound)+":"+std::to_string(first)+":"+std::to_string(second));
            },[&](const PriestShieldNotice& notice) {
                std::string event="N:"+std::string(notice.file);
                // 원본 전역 소리 인자를 순서대로 기록한다.
                for (const auto value:notice.arguments) event+=':'+std::to_string(value);append(event);
            },[&](std::string_view key) { append("T:"+std::string(key));return std::string("notice-token"); },
            [&](std::string_view message) { append("D:"+std::string(message)); }});
        shield.Ensure(kSource);if (events.empty()) events="-";
        const bool same=events==row[8] && Hex(scene.pool.Slot(kSource))==row[9] && Hex(scene.pool.Slot(kShield))==row[10] &&
            scene.state.localPlayer==Number(row[11]) && scene.state.loadingDepth==Number(row[12]) && std::bit_cast<std::uint64_t>(scene.state.noticeClock)==std::stoull(row[13]);
        CHECK(same);if (!same) { std::printf("사제 보호막 %s 행 %zu 불일치\n",std::string(edition).c_str(),count);break; }++count;
    }
    CHECK(count==kRows);
}
}

// 패치의 실제 lookup/owner 및 번역 안내를 전체 원본 관찰로 검사한다.
TEST_CASE(PriestShield_ReplaysOriginals) { Replay("originals"); }
// CD의 안내 소리만 있는 흐름을 별도로 대조한다.
TEST_CASE(PriestShield_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE도 독립 원본 관찰에 대조한다.
TEST_CASE(PriestShield_ReplaysExtra1037) { Replay("original1037"); }

// 실제 factory/소유자·조회·삭제 준비와 연결하고 중복 생성/소리 확보 실패를 검사한다.
TEST_CASE(PriestShield_CreatesOnceAndCanBeFoundByPriestDeletion) {
    // 두 판본에서 클라이언트 SID 할당·원본 생성자/공통 초기화를 적용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        types[167].constructorAddress=TypeConstructorAddress(edition,167);types[167].footX=types[167].footY=1;
        SquidFactory factory(pool,types);const Sid priest=pool.Allocate(2);auto raw=pool.AllocatedBytes(priest);
        raw[10]=kPriestType;Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[patch ? 34 : 32]=1;
        Put(raw,14,std::bit_cast<std::uint32_t>(20.75F));Put(raw,18,std::bit_cast<std::uint32_t>(21.9F));raw[11]=0;
        SquidHash hash;PriestForcefieldState forcefield;RawPriestForcefield lookup(pool,hash,types,forcefield);SquidPostPopState books;SquidOwnerMode mode;
        SquidOwner owner(pool,types,books,mode);PriestShieldState state{1,0,0};std::uint32_t pops=0,sounds=0,notices=0,tells=0,reservations=0;Sid created;
        auto hooks=MakePriestShieldCreationHooks(pool,factory,owner,{{},{},[&](Sid sid,float x,float y,std::uint32_t flags) {
            CHECK(flags==0 && x==20.75F && y==21.9F);created=sid;++pops;auto bytes=pool.AllocatedBytes(sid);
            // 미복원 Pop 경계가 등록 입력만 공급하며 전체 가상 Pop의 완료를 뜻하지 않는다.
            Put(bytes,14,std::bit_cast<std::uint32_t>(x));Put(bytes,18,std::bit_cast<std::uint32_t>(y));bytes[11]=0;
            auto& head=hash.Bucket(0,x,y);Put(bytes,4,head,2);head=sid.value;
        },[&] { ++reservations;return reservations>1; },[](std::string_view file) { CHECK(file=="priestForceField.wav");return kSound; },
            [&](Sid sid,std::uint32_t sound,std::uint32_t first,std::uint32_t second) { CHECK(sid==created && sound==kSound && first==0 && second==1);++sounds; },
            [&](const PriestShieldNotice& notice) { CHECK(notice.file=="ourPriestImmobile.wav");++notices; },
            [](std::string_view key) { CHECK(key=="PriestImmobile");return std::string("사제 이동 불가"); },[&](std::string_view text) { CHECK(text=="사제 이동 불가");++tells; }});
        RawPriestShield shield(pool,lookup,state,std::move(hooks));CHECK(lookup.Find(priest)==Sid{});shield.Ensure(priest);
        CHECK(lookup.Find(priest)==created && pool.Slot(created)[patch ? 34 : 32]==1 && pops==1 && sounds==0 && notices==1);
        const auto freeCount=pool.FreeCount();shield.Ensure(priest);CHECK(pool.FreeCount()==freeCount && pops==1 && notices==1 && reservations==1);
        PriestPostPopState priests;priests.priests={{priest.value},1};std::uint32_t destroyed=0,carrier=0;
        RawPriestPreDestroy pre(pool,priests,MakePriestForcefieldHooks(pool,lookup,{{},[&](Sid sid,std::uint32_t flags) {
            CHECK(sid==created && flags==0);++destroyed;
            // 미복원 가상 destroy 경계의 공간 해제/반납 입력을 제공한다.
            hash.Bucket(0,20.75F,21.9F)=0;pool.AllocatedBytes(sid)[11]=4;pool.Release(sid);
        },[](Sid,std::uint32_t) {},[&](Sid sid,std::uint32_t) { CHECK(sid==priest);++carrier; }}));
        pre.PreDestroy(priest,0);CHECK(destroyed==1 && carrier==1 && priests.priests.count==0 && lookup.Find(priest)==Sid{});
        shield.Ensure(priest);CHECK(lookup.Find(priest)==created && pops==2 && sounds==1 && notices==2 && tells==(patch ? 2U : 0U));
    }
}

// 누락 경계와 다른 풀을 생성 전에 거부하고 실패한 생성 SID를 묵시적으로 사용하지 않는다.
TEST_CASE(PriestShield_RejectsMissingEffectsAndForeignPools) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,false),other(OriginalEdition::Patch1078,kCapacity,false);SquidHash hash;
    std::vector<RiftTypeRecord> types(188);PriestForcefieldState forcefield;RawPriestForcefield lookup(pool,hash,types,forcefield);PriestShieldState state;std::uint32_t created=0;
    PriestShieldHooks hooks{[&](std::uint32_t,std::uint32_t) { ++created;return Sid{}; },[](Sid,std::uint32_t) {},[](Sid,float,float,std::uint32_t) {},
        [] { return false; },[](std::string_view) { return 0U; },[](Sid,std::uint32_t,std::uint32_t,std::uint32_t) {},[](const PriestShieldNotice&) {},
        [](std::string_view) { return std::string{}; },[](std::string_view) {}};
    auto missing=hooks;missing.pop={};CHECK(Throws([&] { RawPriestShield bad(pool,lookup,state,missing); }));
    missing=hooks;missing.tell={};CHECK(Throws([&] { RawPriestShield bad(pool,lookup,state,missing); }));
    CHECK(Throws([&] { RawPriestShield bad(other,lookup,state,hooks); }) && created==0);
    SquidFactory factory(pool,types),foreignFactory(other,types);SquidPostPopState books;SquidOwnerMode mode;SquidOwner owner(pool,types,books,mode),foreignOwner(other,types,books,mode);
    CHECK(Throws([&] { static_cast<void>(MakePriestShieldCreationHooks(pool,foreignFactory,owner,hooks)); }));
    CHECK(Throws([&] { static_cast<void>(MakePriestShieldCreationHooks(pool,factory,foreignOwner,hooks)); }));
    auto raw=InputRaw(pool,kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=kPriestType;Put(raw,0,kPatchPriestVtable);
    RawPriestShield shield(pool,lookup,state,hooks);CHECK(Throws([&] { shield.Ensure(kSource); }) && created==1);
    raw[11]=1;CHECK(Throws([&] { shield.Ensure(kSource); }) && created==1);
}
