// 실제 두 PE의60바이트 결과/네 getter/객체 메뉴DWORD를 독립 원문과 비교한다.
#include "RawSceneSupport.h"
#include "o/RawPlayerIdentity.h"
#include "client/RawObjectMenu.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// fixture13개 독립 입력 열이다.
enum Input { Modes,Table,Local,Subscriber,Address,Name,QueryOwner,QuerySub,QueryAddress,QueryName,Argument,RawOwner,InitMode };
// 원본 바이트hex를 문자 인코딩 변환 없이 읽는다.
std::string Decode(std::string_view text) {
    std::string result;if (text=="-") return result;CHECK(text.size()%2==0);
    // 바이트 단위로 원문의 순서를 보존한다.
    for (std::size_t index=0;index<text.size();index+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16));return result;
}
// 문자열을 원문 효과와 같은hex로 기록한다.
std::string TextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 세 공유 기록표의 독립 입력 한 행이다. 검색 결과/예상 우선순위는 포함하지 않는다.
struct Entry {
    std::uint32_t bit,index,owner,subscriber; // 입력 mask·물리행·owner·subscriber다.
    std::string_view name; // 원본 NUL 바이트 문자열이다.
    std::uint32_t active; // metamap에만 사용하는 독립 활성 flags다.
};
// 같은 현재 전역/Player/meta/chat과 getter를 조립한다. 반환값은 실제 몸체가 계산한다.
struct IdentityScene {
    OriginalEdition edition; // 현재 원본 판본이다.
    std::size_t stride,metaStride; // 실제 두 기록의 판본별 크기다.
    std::vector<std::uint8_t> players,meta; // 조회기가 직접 빌리는 실제 기록표다.
    std::array<std::uint8_t,4800> chat{}; // 실제100×48 채팅 원문이다.
    std::array<std::uint32_t,13> input{}; // 문자열을 제외한 독립DWORD 입력이다.
    std::string localName,queryName; // 현재 로컬/요청 이름의 원본 바이트다.
    PlayerIdentityReads reads; // 같은 현재 전역의 실제 getter다.
    PlayerIdentityRecord record; // 초기 표식과 원문 필드를 보존하는60바이트다.
    std::string trace; // 실제 객체 메뉴가 보낸 문자열/정수 효과다.
    // fixture의 독립 입력을 실제 저장소에 쓰며 예상 결과는 계산하지 않는다.
    IdentityScene(OriginalEdition source,const std::vector<std::string>& row):edition(source),stride(source==OriginalEdition::Patch1078 ? 180 : 172),metaStride(source==OriginalEdition::Patch1078 ? 460 : 296),players(9*stride),meta(40*metaStride) {
        // 각 숫자 열은 상위DWORD비트도 그대로 보존한다.
        for (std::size_t index=0;index<input.size();++index) if (index!=Name && index!=QueryName) input[index]=static_cast<std::uint32_t>(std::stoull(row[index+2]));
        localName=Decode(row[Name+2]);queryName=Decode(row[QueryName+2]);
        constexpr std::array<Entry,5> playerEntries{{{1,2,2,0x80000002,"Alpha",0},{2,8,8,0xfffffff0,"LAST",0},{4,3,3,0x80000002,"Duplicate",0},{8,4,4,0,"OnlyName",0},{2048,2,2,0x80000055,"localuser",0}}}; // 원문과 같은 독립 Player 입력이다.
        // 실제 Player 이름/Subscriber 필드만 쓴다.
        for (const auto& entry:playerEntries) if (input[Table]&entry.bit) { auto bytes=std::span(players).subspan(entry.index*stride,stride);Put(bytes,44,entry.subscriber);Text(bytes,12,entry.name); }
        constexpr std::array<Entry,3> metaEntries{{{16,0,6,0x80000002,"MetaFirst",1},{32,39,7,0xfffffff1,"MetaLast",1},{64,1,5,0x80000100,"Inactive",2}}}; // 두 판본에 같은 의미의 독립 metadata 입력이다.
        // 활성 flag의 물리 위치/이름 offset만 판본에 맞춘다.
        for (const auto& entry:metaEntries) if (input[Table]&entry.bit) { auto bytes=std::span(meta).subspan(entry.index*metaStride,metaStride);Put(bytes,0,entry.owner);Put(bytes,4,entry.subscriber);Text(bytes,source==OriginalEdition::Patch1078 ? 38 : 8,entry.name);Put(bytes,source==OriginalEdition::Patch1078 ? 100 : 72,entry.active); }
        constexpr std::array<Entry,4> chatEntries{{{128,0,0,0x80000002,"ChatFirst",0},{256,99,0,0xfffffff2,"ChatLast",0},{512,1,0,0,"ZeroSub",0},{1024,2,0,0x80000002,"ChatSecond",0}}}; // 처음/마지막/0/중복 Subscriber 입력이다.
        // 기존100×48 원문의 이름/Subscriber만 채운다.
        for (const auto& entry:chatEntries) if (input[Table]&entry.bit) { auto bytes=std::span(chat).subspan(entry.index*48,48);Text(bytes,0,entry.name);Put(bytes,44,entry.subscriber); }
        reads.localOwner=[this] { return input[Local]; };reads.subscriber=[this] { return input[Subscriber]; };reads.address=[this] { return input[Address]; };reads.name=[this] { return std::string_view(localName); };
        reads.battle=[this] { return (input[Modes]&1)!=0; };reads.editor=[this] { return (input[Modes]&2)!=0; };reads.metamap=[this] { return (input[Modes]&4)!=0; };reads.unknownName=[] { return std::string_view("SOMEONE"); };
        record.fill(0xa5);Put(record,0,input[QueryOwner]);Put(record,4,input[QuerySub]);Text(record,8,queryName);Put(record,48,input[QueryAddress]);Put(record,52,0xdeadbeef);
    }
    // NUL까지만 쓰고 이름 뒤의 원문 표식을 지우지 않는다.
    static void Text(std::span<std::uint8_t> bytes,std::size_t offset,std::string_view text) { CHECK(offset+text.size()<bytes.size());std::copy(text.begin(),text.end(),bytes.begin()+offset);bytes[offset+text.size()]=0; }
    // 저장소/전역 사본 없이 실제 조회기를 생성한다.
    RawPlayerIdentity Identity() { return RawPlayerIdentity(edition,players,meta,chat,reads); }
    // 실제로 도달한 GUI 효과만 원문과 같은 형식으로 연결한다.
    void Add(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 실제 getter를 기본 객체 메뉴에 연결하고 최종 Player DWORD를 기록한다.
    void ObjectMenu() {
        SidPool pool(edition,24000,true);
        // 현재 객체50의 실제 클라이언트 슬롯을 확보한다.
        for (std::uint32_t number=5;number<=50;++number) CHECK(pool.Allocate(2).value==number);
        auto raw=pool.AllocatedBytes(Sid{50});raw[10]=158;raw[edition==OriginalEdition::Patch1078 ? 34 : 32]=static_cast<std::uint8_t>(input[RawOwner]);
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);ObjectMenuState common;common.localOwner=input[Local];common.editor=(input[Modes]&2)!=0;common.helpWord="AboutType";ObjectMenuHooks hooks;auto identity=Identity();BindObjectMenuIdentity(hooks,identity);
        hooks.format=[this](std::string_view key,std::span<const PriestMenuArgument> arguments) { CHECK(arguments.empty());Add("T,"+TextHex(key));return std::string(key); };
        hooks.item=[this](std::string_view label,std::uint32_t command,const ObjectMenuParameter& parameter) { const auto value=std::holds_alternative<std::uint32_t>(parameter) ? "N:"+std::to_string(std::get<std::uint32_t>(parameter)) : "S:"+TextHex(std::get<std::string>(parameter));Add("I,"+TextHex(label)+','+std::to_string(command)+','+value); };
        hooks.playerMenu=[this](std::uint32_t subscriber,bool first,bool second) { Add("P,"+std::to_string(subscriber)+','+std::to_string(first)+','+std::to_string(second));return 123U; };
        hooks.submenu=[this](std::string_view label,std::uint32_t token) { Add("S,"+TextHex(label)+','+std::to_string(token)); };
        RawObjectMenu menu(pool,types,common,hooks);menu.Open(Sid{50});
    }
};
}
// 생성/Resolve의 전체60바이트와 정상 반환366입력을 두 판본 원문과 비교한다.
TEST_CASE(PlayerIdentity_NativeRecords366) {
    std::size_t count=0;
    // 원본 실제 결과는 미작성 이름 뒤와 마지막DWORD 표식까지 포함한다.
    for (const auto& row:LoadFixture(NETSTORM_PLAYER_IDENTITY_FIXTURE).rows) {
        if (row[1]!="ctor" && row[1]!="resolve") continue;CHECK(row.size()==18);IdentityScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,row);auto identity=scene.Identity();std::uint32_t result=0;
        if (row[1]=="ctor") RawPlayerIdentity::Initialize(scene.record,scene.input[InitMode]);else result=identity.Resolve(scene.record);
        if (Hex(scene.record)!=row[16]) std::fprintf(stderr,"식별자 원문 차이 %s %s modes%u table%u owner%u sub%u\n",row[0].c_str(),row[1].c_str(),scene.input[Modes],scene.input[Table],scene.input[QueryOwner],scene.input[QuerySub]);
        CHECK(std::to_string(result)==row[15]);CHECK(Hex(scene.record)==row[16]);++count;
    }CHECK(count==366);
}
// 네 getter는 숫자 세 종류/실제 이름과 실패 대체 이름을 별도로 대조한다.
TEST_CASE(PlayerIdentity_NativeGetters1288) {
    std::size_t count=0;
    // 실제 네 wrapper의 관찰 결과이며 Resolve 성공을 추가 관문으로 쓰지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_PLAYER_IDENTITY_FIXTURE).rows) {
        const auto& kind=row[1];if (kind!="subscriber" && kind!="owner" && kind!="address" && kind!="name") continue;IdentityScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,row);auto identity=scene.Identity();std::string result;
        if (kind=="subscriber") result=std::to_string(identity.SubscriberForOwner(scene.input[Argument]));else if (kind=="owner") result=std::to_string(identity.OwnerForSubscriber(scene.input[Argument]));else if (kind=="address") result=std::to_string(identity.AddressForSubscriber(scene.input[Argument]));else result=TextHex(identity.NameForSubscriber(scene.input[Argument]));
        CHECK(result==row[15]);++count;
    }CHECK(count==1288);
}
// 공통 객체→실제 subscriber getter→Player 메뉴의 상위32비트까지 원문과 비교한다.
TEST_CASE(PlayerIdentity_NativeObjectMenuDword48) {
    std::size_t count=0;
    // Player 메뉴 몸체만 명시 경계이며 이름 문자열로 치환하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_PLAYER_IDENTITY_FIXTURE).rows) {
        if (row[1]!="menu") continue;IdentityScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,row);scene.ObjectMenu();CHECK(scene.trace==row[17]);++count;
    }CHECK(count==48);
}
// 동일 조회기가 현재 원문 변경을 읽으며 부분완료와 누락 모드를 실제 계약대로 구분한다.
TEST_CASE(PlayerIdentity_CurrentStorageAndRequiredMeta) {
    const auto rows=LoadFixture(NETSTORM_PLAYER_IDENTITY_FIXTURE).rows;IdentityScene scene(OriginalEdition::Patch1078,rows.at(1));auto identity=scene.Identity();CHECK(identity.SubscriberForOwner(2)==0x80000002U);Put(scene.players,2*180+44,0xabcd9911);CHECK(identity.SubscriberForOwner(2)==0xabcd9911U);
    scene.input[Modes]=4;scene.record.fill(0);Put(scene.record,4,0x80000002U);RawPlayerIdentity missing(OriginalEdition::Patch1078,scene.players,{},scene.chat,scene.reads);CHECK(Throws([&] { missing.Resolve(scene.record); }));
    scene.input[Modes]=0;scene.record.fill(0);Put(scene.record,4,0x1234);IdentityScene::Text(scene.record,8,"Unlisted");CHECK(identity.Resolve(scene.record));CHECK(Get(scene.record,0)==0);
}
