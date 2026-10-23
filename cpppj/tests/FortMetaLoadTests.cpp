// 원본 Badges 장부/Technology wrapper와 기존 같은 풀 Contents 생성을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFortMetaLoad.h"
#include "o/ContentFormHost.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// native 입력 열의 고정 순서이며 기대 값은 fixture에만 있다.
enum MetaField : std::size_t { Kind,Type,Count,Profile,Template,Slot,Present,End,Depth,OldConversion,NewConversion,Mutation };
// 패딩까지 명시한 28개220BYTE 입력 표다.
std::vector<std::uint8_t> BadgeInput(std::uint32_t profile) {
    std::vector<std::uint8_t> bytes(6160,0xa5);
    // 첫 중복/마지막 unsigned 타입을 원본 입력과 같게 준비한다.
    for (std::size_t index=0;index<28;++index) {
        auto type=static_cast<std::uint32_t>(70+index);if (profile && (index==1 || index==8)) type=80;
        if (profile && index==27) type=0xffffffffU;Put(bytes,index*220,type);
    }
    return bytes;
}
// 전체 raw 표를 원본 무손실 BYTE run 형식으로 직렬화한다.
std::string BadgeRle(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef"; // 각BYTE의소문자16진숫자다.
    std::string result;
    // 패딩이나 상위 수량BYTE도 하나도 생략하지 않는다.
    for (std::size_t start=0;start<bytes.size();) {
        auto end=start+1;
        // 같은BYTE의최대연속구간을찾는다.
        while (end<bytes.size() && bytes[end]==bytes[start]) ++end;
        if (!result.empty()) result+=',';result+=std::to_string(end-start)+':';result+=digits[bytes[start]>>4];result+=digits[bytes[start]&15];start=end;
    }
    return result;
}
// 한 native 입력의 명시 하위 경계와 현재 장부/깊이를 갖는 장면이다.
struct FortMetaReplayScene {
    const SidPool& pool; // 판본과실제하위소비자풀을공유한다.
    std::vector<std::uint32_t> input; // 원본입력의12개DWORD다.
    std::vector<std::uint8_t> badges,data; // 전체raw장부와실제stream메모리다.
    std::uint32_t depth{}; // 원본현재loadingDepth이며callback뒤재조회한다.
    FortMetaLoadState state; // Template/isMine/현재conversion의명시입력이다.
    FortMetaStream stream; // pointer존재/pos/end를bytes와구별한다.
    std::vector<std::string> events; // 명시하위경계의원본순서를기록한다.
    std::size_t assertions{}; // 보고뒤정상반환한진단횟수다.
    // 기대장부/분기를계산하지않고원본입력만초기화한다.
    FortMetaReplayScene(const SidPool& source,const std::string& fields):pool(source) {
        // 각원본DWORD를순서그대로전달한다.
        for (const auto& field:Split(fields,',')) input.push_back(Number(field));CHECK(input.size()==12);
        badges=BadgeInput(input[Profile]);depth=input[Depth];state.templatePresent=input[Template]!=0;
        state.templateSlot=std::bit_cast<std::int32_t>(input[Slot]);state.localOwner=1;state.hasTypeConversion=input[OldConversion]!=0;
        const auto count=input[Kind]==0 ? input[Count] : 3U;data.push_back(static_cast<std::uint8_t>(count));
        // native 메모리의타입/수량BYTE만공급하며setter결과는계산하지않는다.
        for (std::uint32_t index=0;index<count;++index) { data.push_back(static_cast<std::uint8_t>(index%7==6 ? 255 : 70+index%28));data.push_back(static_cast<std::uint8_t>(index*13+1)); }
        stream={data,19,input[End],input[Present]!=0};
    }
    // TypeNames/하위Contents실행완료를주장하지않는명시관찰경계다.
    FortMetaLoadHooks Hooks() {
        FortMetaLoadHooks hooks;
        hooks.assertion=[this](FortMetaAssertion reason) {
            constexpr std::array<char,4> names{'B','C','T','M'}; // 원본진단line의fixture이름이다.
            events.push_back(std::string("A:")+names[static_cast<std::size_t>(reason)]);++assertions;if (input[Mutation]==3) Put(badges,0,999);
        };
        hooks.loadingDepthChanged=[this](std::uint32_t value) { CHECK(value==depth);events.push_back("L:"+std::to_string(value)); };
        hooks.technologyParent=[this] { events.push_back("P:50");return Sid{50}; };
        hooks.prepareTypeNames=[this] { events.push_back("N:"+std::to_string(state.hasTypeConversion));state.hasTypeConversion=input[NewConversion]!=0; };
        hooks.contents=[this](Sid parent,FortMetaStream& current,std::uint32_t flags,bool relaxedConversion) {
            CHECK(parent==Sid{50} && flags==17 && relaxedConversion);events.push_back("C:50:17:1:"+std::to_string(depth)+":"+std::to_string(current.position));
            current.position=5;if (input[Mutation]==2) state.hasTypeConversion=true;
        };
        hooks.releaseTypeConversion=[this] { CHECK(state.hasTypeConversion);events.push_back("F"); };
        hooks.refreshTechnologyUi=[this] { events.push_back("U");if (input[Mutation]==1) depth+=5U; };return hooks;
    }
    // 이벤트기록만연결하며발생할이벤트를미리계산하지않는다.
    std::string Events() const {
        std::string value;
        // 모든관찰을순서대로저장한다.
        for (const auto& event:events) { if (!value.empty()) value+=';';value+=event; }return value.empty() ? "-" : value;
    }
};
// 선택소비자에서미복원몸체가호출되면기본성공값을반환하지않고드러낸다.
[[noreturn]] void Unsupported() { throw std::logic_error("선택Technology소비자의미복원경계호출"); }
// 실제Contents만실행하는경로의외부필수효과다. 선택되지않은가상메서드는명시거부한다.
FortObjectLoadHooks ContentsOnly(const RawContainedCreate& contained) {
    FortObjectLoadHooks hooks;
    hooks.create=[](std::uint32_t,std::uint32_t)->Sid { Unsupported(); };hooks.notifySurface=[](Sid) { Unsupported(); };
    hooks.setState=hooks.setOwner=[](Sid,std::uint32_t) { Unsupported(); };hooks.hasHitPoints=[](Sid)->bool { Unsupported(); };
    hooks.maxHitPoints=hooks.virtualCount=[](Sid)->std::int32_t { Unsupported(); };hooks.pop=[](Sid,float,float,std::uint32_t) { Unsupported(); };
    hooks.destroy=[](Sid,std::uint32_t) { Unsupported(); };
    hooks.createContent=[&contained](Sid parent,std::uint32_t type,std::uint32_t quantity,std::uint32_t flags,std::uint32_t attach,std::uint32_t quality) { return contained.Create(parent,type,quantity,flags,attach,quality); };
    hooks.chunkRegion=[](std::int32_t,std::int32_t)->std::int8_t { Unsupported(); };hooks.territoryAllowed=[](std::int8_t,std::uint32_t,std::int32_t)->bool { Unsupported(); };
    hooks.loadingDepthChanged=hooks.finishOwner=[](std::uint32_t) { Unsupported(); };return hooks;
}
}

// 두실제PE의고유정상/assert입력을같은자료형으로전수대조한다.
TEST_CASE(FortMetaLoad_ReplaysBothNativeFullBadgeAndWrapper) {
    const auto fixture=LoadFixture(NETSTORM_FORTMETA_FIXTURE);std::size_t checked=0,normal=0,diagnosed=0;
    // 판본별풀은장기객체이며각입력의raw표와stream만새로구성한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,false);
        // 고유행의전체장부/pos/end/depth/변환/효과/진단을native출력과대조한다.
        for (const auto& row:fixture.rows) {
            if (row[0]!=(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")) continue;CHECK(row.size()==10);
            FortMetaReplayScene scene(pool,row[1]);RawFortMetaLoad loader(pool,scene.badges,scene.depth,scene.state,scene.Hooks());std::string result="-";
            if (scene.input[Kind]==0) loader.LoadBadges(scene.stream);
            else if (scene.input[Kind]==1) loader.SetBadge(scene.input[Type],std::bit_cast<std::int32_t>(scene.input[Count]));
            else if (scene.input[Kind]==2) loader.LoadTechnology(scene.stream);
            else result=std::to_string(std::bit_cast<std::uint32_t>(loader.FindBadge(scene.input[Type])));
            const bool same=result==row[2] && std::to_string(scene.stream.position)==row[3] && std::to_string(scene.stream.end)==row[4] &&
                std::to_string(scene.depth)==row[5] && std::to_string(scene.state.hasTypeConversion)==row[6] && scene.Events()==row[7] &&
                BadgeRle(scene.badges)==row[8] && std::to_string(scene.assertions)==row[9];CHECK(same);
            if (!same) { std::fprintf(stderr,"FortMeta %s row%zu input%s\nactual%s\nexpect%s\n",row[0].c_str(),checked,row[1].c_str(),scene.Events().c_str(),row[7].c_str());return; }
            CHECK(&loader.Pool()==&pool);++checked;if (row[9]=="0") ++normal;else ++diagnosed;
        }
    }
    CHECK(checked==fixture.rows.size() && normal>0 && diagnosed>0);
}

// 선택Technology의Contents만기존실제Factory→ContainedCreate→ContentForm으로같은풀에생성한다.
TEST_CASE(FortMetaLoad_ActualSamePoolContentsAndMandatoryBoundaries) {
    // 부모getter/변환준비/해제/UI는명시경계이며HP/Frame/Pop/Owner는이분기에서요청되지않는다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,false);SquidHash hash;std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188U : 171U);
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));types[89].constructorAddress=TypeConstructorAddress(edition,89);
        types[89].footX=types[89].footY=1;types[110].flags1=0x1000;types[110].contentListFlags=0x10;types[111].flags1=0x2000;types[111].contentListFlags=0x20;
        SquidFactory factory(pool,types);const auto parent=factory.Create(89,2);std::vector<Sid> children;std::size_t transmissions=0;
        ContentFormHost forms(pool,types,[&](Sid owner,Sid child,std::uint32_t reason) { CHECK(owner==parent && reason==1);children.push_back(child); },
            [&](Sid,std::uint32_t) { ++transmissions; });
        ContainedCreateState containedState;RawContainedCreate contained(pool,containedState,forms.BindCreate(factory,{{},[](Sid) { Unsupported(); },{}}));
        GameRandom random{12345};FortObjectLoadState objectState;RawFortObjectLoad objects(pool,hash,types,frames,random,objectState,ContentsOnly(contained));
        FortTemplate fort;fort.sections.resize(FortTemplate::kSectionCount);const auto names=FortTemplate::SectionNames();
        const auto index=static_cast<std::size_t>(std::ranges::find(names,"Technology")-names.begin());fort.sections[index]={2,110,9,111,254,255,0xee};fort.technologyTrailingBytes=1;
        FortContent first;first.storedType=110;first.type=110;first.quantity=9;FortContent second;second.storedType=111;second.type=111;second.quantity=-2;fort.technology.items={first,second};
        std::uint32_t depth=3;FortMetaLoadState state;state.hasTypeConversion=true;std::vector<std::string> effects;FortMetaLoadHooks hooks;
        hooks.assertion=[](FortMetaAssertion) { Unsupported(); };hooks.loadingDepthChanged=[&](std::uint32_t value) { CHECK(value==depth);objectState.loadingDepth=value; };
        hooks.technologyParent=[&] { effects.push_back("parent");return parent; };
        hooks.prepareTypeNames=[&] { effects.push_back("prepare-boundary");CHECK(state.hasTypeConversion); };
        hooks.releaseTypeConversion=[&] { effects.push_back("release-boundary");CHECK(state.hasTypeConversion); };hooks.refreshTechnologyUi=[&] { effects.push_back("ui-boundary"); };
        hooks=MakeFortMetaContentsHooks(pool,objects,fort,hooks);auto badges=BadgeInput(0);RawFortMetaLoad meta(pool,badges,depth,state,hooks);
        FortMetaStream stream{fort.Section("Technology"),19,7,true};meta.LoadTechnology(stream);
        CHECK(children.size()==2 && transmissions==0 && stream.position==6 && stream.end==7 && depth==3 && objectState.loadingDepth==3 && !state.hasTypeConversion);
        CHECK(effects==std::vector<std::string>({"parent","prepare-boundary","release-boundary","ui-boundary"}));
        CHECK(SquidForms(pool).Parent(children[0])==parent && SquidForms(pool).Parent(children[1])==parent);
        CHECK(Get(pool.Slot(children[0]),18,2)==110 && Get(pool.Slot(children[1]),18,2)==111);
        CHECK(Get(pool.Slot(children[0]),12,2)==9 && Get(pool.Slot(children[1]),12,2)==65534 && random.State()==12345);
        CHECK(Get(pool.Slot(parent),6,2)==children[0].value && Get(pool.Slot(children[0]),4,2)==children[1].value);
        SidPool foreign(edition,kCapacity,false);CHECK(Throws([&] { MakeFortMetaContentsHooks(foreign,objects,fort,hooks); }));
        auto wrong=stream;wrong.bytes={};CHECK(Throws([&] { hooks.contents(parent,wrong,17,true); }));
    }
}

// 부족한장부/필수효과/손상물리메모리를기본성공값으로대체하지않는다.
TEST_CASE(FortMetaLoad_RejectsMissingRecordsEffectsAndTruncatedMemory) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,false);FortMetaReplayScene scene(pool,"0,80,1,0,1,1,1,6,0,0,1,0");
    auto hooks=scene.Hooks();hooks.prepareTypeNames={};CHECK(Throws([&] { RawFortMetaLoad bad(pool,scene.badges,scene.depth,scene.state,hooks); }));
    CHECK(Throws([&] { RawFortMetaLoad bad(pool,std::span(scene.badges).first(6159),scene.depth,scene.state,scene.Hooks()); }));
    RawFortMetaLoad loader(pool,scene.badges,scene.depth,scene.state,scene.Hooks());scene.stream.bytes={};CHECK(Throws([&] { loader.LoadBadges(scene.stream); }));
}

// 두 판본의 깊이/Template 진단과 기술 부모가 현재 섹션을 바꾼 뒤에만 실제 getter를 실행한다.
TEST_CASE(FortMetaLoad_SelectsAfterDiagnosticsAndTechnologyParent) {
    // 두 판본은 Badges의 즉시 깊이 증감만 다르며 Technology 선택 시점은 같다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,false);auto badges=BadgeInput(0);std::uint32_t depth=3;FortMetaLoadState state;state.templatePresent=false;state.templateSlot=8;state.localOwner=1;
        std::vector<std::uint8_t> bytes{0};FortMetaStream first{bytes,11,1,true},second{bytes,19,1,true};auto* current=&first;std::vector<std::string> effects;FortMetaLoadHooks hooks;
        hooks.assertion=[&](auto reason) { effects.push_back("A"+std::to_string(static_cast<unsigned>(reason)));if (reason==FortMetaAssertion::MissingBadgesTemplate) current=&second; };
        hooks.loadingDepthChanged=[&](auto value) { CHECK(value==depth);effects.push_back("L"+std::to_string(value)); };
        hooks.technologyParent=[&] { CHECK(depth==4);effects.push_back("P");current=&second;return Sid{50}; };
        hooks.prepareTypeNames=[&] { effects.push_back("N");state.hasTypeConversion=true; };
        hooks.contents=[&](Sid parent,FortMetaStream& stream,auto flags,bool relaxed) { CHECK(parent==Sid{50} && &stream==&second && flags==17 && relaxed && !stream.position && depth==4);effects.push_back("C");stream.position=1; };
        hooks.releaseTypeConversion=[&] { effects.push_back("F"); };hooks.refreshTechnologyUi=[&] { effects.push_back("U"); };
        RawFortMetaLoad meta(pool,badges,depth,state,hooks);
        meta.LoadBadgesSelected([&]() -> FortMetaStream& { CHECK(depth==3 && current==&second);effects.push_back("S3");current->position=0;return *current; });
        const auto badgeEffects=edition==OriginalEdition::Patch1078 ? std::vector<std::string>{"A0","S3"} : std::vector<std::string>{"L4","L3","A0","S3"};
        CHECK(effects==badgeEffects && first.position==11 && second.position==1 && depth==3);effects.clear();current=&first;
        meta.LoadTechnologySelected([&]() -> FortMetaStream& { CHECK(depth==4 && current==&second && effects.back()=="P");effects.push_back("S4");current->position=0;return *current; });
        CHECK(effects==std::vector<std::string>({"L4","A2","A3","P","S4","N","C","F","U","L3"}) && first.position==11 && second.position==1 && !state.hasTypeConversion && depth==3);
    }
}

// 기술 selector 실패도 이미 수행한 부모 조회는 유지하면서 원래 깊이를 복구하고 빠진 callback은 쓰기 전에 거부한다.
TEST_CASE(FortMetaLoad_SelectionFailureRestoresDepth) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,false);auto badges=BadgeInput(0);std::uint32_t depth=3,parents{};FortMetaLoadState state;FortMetaLoadHooks hooks;
    hooks.assertion=[](auto) { throw std::logic_error("정상 Template의 예상 밖 진단"); };hooks.loadingDepthChanged=[&](auto value) { CHECK(value==depth); };
    hooks.technologyParent=[&] { CHECK(depth==4);++parents;return Sid{}; };hooks.prepareTypeNames=hooks.releaseTypeConversion=hooks.refreshTechnologyUi=[] { throw std::logic_error("선택 실패 뒤 추가 처리 없음"); };
    hooks.contents=[](Sid,FortMetaStream&,std::uint32_t,bool) { throw std::logic_error("선택 실패 뒤 내용물 없음"); };RawFortMetaLoad meta(pool,badges,depth,state,hooks);
    CHECK(Throws([&] { meta.LoadTechnologySelected([&]() -> FortMetaStream& { CHECK(depth==4 && parents==1);throw std::runtime_error("기술 파일 선택 실패 입력"); }); }));
    CHECK(depth==3 && parents==1 && Throws([&] { meta.LoadTechnologySelected({}); }) && Throws([&] { meta.LoadBadgesSelected({}); }) && depth==3 && parents==1);
}
