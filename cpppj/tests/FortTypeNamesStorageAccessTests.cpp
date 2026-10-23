// 제안모듈을실제Storage의같은fields16/현재버퍼에연결하는headless검사초안이다.
#include "TestSupport.h"
#include "client/CampaignTemplateStorage.h"
#include "o/RawFortTypeNamesLoad.h"
#include <algorithm>
#include <array>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace netstorm::client;
using namespace netstorm::o;
constexpr std::uint32_t kAlphaHash=0x68706CA2,kBetaHash=0x61746542; // 원본signedchar해시의명시Alpha/Beta입력값이다.
// raw필드의실제네BYTE만읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t result{};for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[at+byte])<<(byte*8);return result; // 비정렬필드의little endian값이다.
}
// 명시콜백효과가같은현재필드/버퍼에만DWORD를쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 이웃BYTE를보존한다.
}
// 실제원본부모/빌림계약의선택거부를확인한다.
template<class Function> bool Rejects(Function function) { try { function(); }catch (const std::logic_error&) { return true; }return false; }
// TYPE경계의명시부분중단이며OS예외처리완료를뜻하지않는다.
struct TypeStop:std::runtime_error { TypeStop():std::runtime_error("TYPE2명시중단") {} };
// 변환할당은검사만의명시소유자다.Client전역의실제소유자가연결됐다는증거로사용하지않는다.
struct ConversionOwner {
    std::uint32_t current{},next{0x3001}; // 원본현재pointer와별도실제할당들의검사토큰이다.
    std::map<std::uint32_t,std::array<std::uint8_t,255>> blocks; // 같은토큰은동일물리배열을가리키며버퍼사본을alias로반환하지않는다.
    // 실제요청255BYTE를새비0배열로소유하고현재pointer쓰기는로더에맡긴다.
    std::uint32_t Allocate(std::uint32_t size) { CHECK(size==255);const auto token=next++;blocks[token].fill(0xA7);return token; }
    // 해당실제배열을직접빌리며0/미등록주소는거부한다.
    std::span<std::uint8_t> Bytes(std::uint32_t token) { return blocks.at(token); }
};
// 실제Storage/선택노드와동기범위만공급하고count/hash/매핑기대는제품에주입하지않는다.
struct StorageAccessDevices {
    CampaignTemplateStorage storage;ConversionOwner conversion;std::uint32_t depth{3}; // 검사별독립Storage/명시변환소유자/실제공유깊이다.
    TemplateWriteStream selected;std::vector<std::string> trace; // Select가반환한현재노드수명과실제경계순서다.
    bool active{},changeCurrent{},checkGuards{},zeroEndAndCount{},changeStoredHash{},stopAtBeta{}; // 명시외부효과입력이며제품알고리즘분기가아니다.
    std::span<std::uint8_t> activeFields;const MemStreamHooks* activeMemory{}; // AccessSection의동기범위안에서만유효하며성공/실패양쪽에서비운다.
    const std::uint8_t* observedFieldAddress{};const std::uint8_t* observedDataAddress{}; // 비교만하는주소관측이다.범위밖에서역참조하지않는다.
    const std::array<std::array<std::uint8_t,7>,3> names{{{'U','n','u','s','e','d',0},{'A','l','p','h','a',0,0},{'B','e','t','a',0,0,0}}}; // 실제NUL포함TYPE이름입력이다.

    // 원본파일없이실제manager/노드/버퍼를만들고같은manager를8번헤더에서도빌린다.
    explicit StorageAccessDevices(bool empty=false) {
        FortTemplate fort;fort.sections.resize(FortTemplate::kSectionCount);storage.Install(1,std::move(fort),"types-access.fort");
        if (!empty) {
            const auto target=storage.CreateSection(1,"TypeNames");std::array<std::uint8_t,9> bytes{};bytes[0]=2;Put(bytes,1,kAlphaHash);Put(bytes,5,kBetaHash);storage.WriteSection(target,bytes);
            const auto scratch=storage.CreateSection(1,"Scratch");const std::array<std::uint8_t,1> extra{99};storage.WriteSection(scratch,extra); // 다음노드생성으로TypeNames실제tail도압축한다.
        }
        storage.Borrow(8,1);
    }
    // 동기본문만빌림을보유하고예외를포함한모든반환전에필드/장치참조를비운다.
    void Leave() { activeFields={};activeMemory=nullptr;active=false; }
    // 실제현재두DWORD를로더/cleanup에빌린다.
    FortTypeNamesLoadGlobals Globals() { return {depth,conversion.current}; }
    // 기존Storage.AccessSection의실제guard를사용하며legacygetter는호출되면즉시실패한다.
    FortTypeNamesLoadHooks Hooks() {
        FortTypeNamesLoadHooks hooks; // 모든진입은실제고정슬롯/선택/동기범위를따른다.
        hooks.manager=[this](std::int32_t slot) { CHECK(slot==8 && depth==4 && !active);trace.push_back("manager");return Read(storage.Headers(),static_cast<std::size_t>(slot)*20); };
        hooks.select=[this](std::uint32_t manager,std::string_view name,std::int32_t writable) {
            CHECK(manager==Read(storage.Headers(),20) && name=="TypeNames" && writable==1 && depth==4 && !active);trace.push_back("select");selected=storage.SelectSection(8,name,writable);return selected.section;
        };
        hooks.stream=[](auto)->std::span<std::uint8_t> { throw std::logic_error("access경로의legacy stream호출"); };
        hooks.buffer=[](auto)->std::span<const std::uint8_t> { throw std::logic_error("access경로의legacy buffer호출"); };
        hooks.access=[this](std::uint32_t token,const auto& operation) {
            CHECK(token==selected.section && depth==4 && !active && storage.CurrentSection(1)=="TypeNames");trace.push_back("access+");
            try {
                storage.AccessSection(selected,[&](auto fields,const MemStreamHooks& memory) {
                    active=true;activeFields=fields;activeMemory=&memory;observedFieldAddress=fields.data();
                    if (const auto pointer=Read(fields,0)) { observedDataAddress=memory.buffer(pointer).data();CHECK(observedDataAddress==storage.Template(1).Section("TypeNames").data()); }
                    try { operation(fields,memory); }catch (...) { Leave();throw; }
                    CHECK(fields.data()==observedFieldAddress);Leave();
                });
            }catch (...) { Leave();trace.push_back("access-throw");throw; }
            trace.push_back("access-");CHECK(!active && activeFields.empty() && !activeMemory && depth==4);
        };
        hooks.allocate=[this](std::uint32_t size) {
            CHECK(active && activeMemory && Read(activeFields,4)==1);trace.push_back("allocate");
            if (zeroEndAndCount) { Put(activeFields,8,0);activeMemory->buffer(Read(activeFields,0))[0]=0; } // count는이미캡처됐으며동일필드/버퍼의실제현재값만변경한다.
            return conversion.Allocate(size);
        };
        hooks.conversionBuffer=[this](auto token) { CHECK(active);return conversion.Bytes(token); };hooks.typeCount=[this] { CHECK(active);return 3; };
        hooks.typeName=[this](std::int32_t type)->std::span<const std::uint8_t> {
            CHECK(active && activeMemory && activeFields.data()==observedFieldAddress && depth==4);trace.push_back("type"+std::to_string(type));
            if (type==1 && changeCurrent) storage.FindSection(1,"Scratch"); // current교체가캡처선택노드/fields를바꾸지않는다.
            if (type==1 && checkGuards) {
                CHECK(Rejects([&] { storage.AccessSection(selected,[](auto,const auto&) {}); }));
                CHECK(Rejects([&] { storage.CreateSection(1,"TypeNames"); }));CHECK(Rejects([&] { storage.RenameSection(1,"Renamed","TypeNames"); }));
            }
            if (type==1 && changeStoredHash) Put(activeMemory->buffer(Read(activeFields,0)),1,kBetaHash); // 캡처data주소의실제현재hash를바꾸며사본은사용하지않는다.
            if (type==2 && stopAtBeta) throw TypeStop{};
            return names.at(static_cast<std::size_t>(type));
        };
        hooks.assertion=[](auto) { throw std::logic_error("실제Storage검사의예상밖기존conversion진단"); };
        hooks.loadingDepthChanged=[this](std::uint32_t value) { CHECK(!active && value==depth);trace.push_back("depth"+std::to_string(value)); };return hooks;
    }
    // 새로운동기guard안에서숫자/주소만관측하고span을반환하지않는다.
    std::array<std::uint32_t,3> Observe() {
        std::array<std::uint32_t,3> values{};storage.AccessSection(selected,[&](auto fields,const auto&) { CHECK(fields.data()==observedFieldAddress);values={Read(fields,0),Read(fields,4),Read(fields,8)}; });return values;
    }
};
}

// 실제두헤더의동일manager/현재노드교체/중첩guard와같은pos쓰기영속을관측한다.
TEST_CASE(FortTypeNamesStorageAccess_SamePhysicalFieldsAndSelection) {
    StorageAccessDevices devices;devices.changeCurrent=devices.checkGuards=true;auto hooks=devices.Hooks();LoadFortTypeNames(8,devices.Globals(),hooks);
    CHECK(devices.depth==3 && devices.storage.CurrentSection(1)=="Scratch" && devices.storage.CurrentSection(8)=="Scratch");const auto values=devices.Observe();
    CHECK(values[0]!=0 && values[1]==1 && values[2]==9);const auto mapped=devices.conversion.Bytes(devices.conversion.current);CHECK(mapped[0]==1 && mapped[1]==2 && mapped[2]==0);
    CHECK(devices.trace==std::vector<std::string>({"depth4","manager","select","access+","allocate","type0","type1","type2","access-","depth3"}));
    CHECK(devices.storage.Template(1).Section("TypeNames").data()==devices.observedDataAddress && !devices.active && devices.activeFields.empty() && !devices.activeMemory);
}

// 할당콜백의실제end0/count0뒤본문은이미캡처한count2를유지하고같은필드end를1로확장한다.
TEST_CASE(FortTypeNamesStorageAccess_CurrentEndAndCapturedCount) {
    StorageAccessDevices devices;devices.zeroEndAndCount=true;auto hooks=devices.Hooks();LoadFortTypeNames(8,devices.Globals(),hooks);const auto values=devices.Observe();
    CHECK(devices.depth==3 && values[1]==1 && values[2]==1 && devices.storage.Template(1).Section("TypeNames").size()==1);
    CHECK(devices.storage.Template(1).Section("TypeNames")[0]==0);const auto mapped=devices.conversion.Bytes(devices.conversion.current);CHECK(mapped[0]==1 && mapped[1]==2);
}

// TYPE효과가바꾼실제raw hash를현재buffer에서다시읽으며payload사본을alias로사용하지않는다.
TEST_CASE(FortTypeNamesStorageAccess_CurrentRawBufferDuringTypeCallback) {
    StorageAccessDevices devices;devices.changeStoredHash=true;auto hooks=devices.Hooks();LoadFortTypeNames(8,devices.Globals(),hooks);
    const auto mapped=devices.conversion.Bytes(devices.conversion.current);CHECK(mapped[0]==2 && mapped[1]==0 && devices.depth==3);
    CHECK(Read(devices.storage.Template(1).Section("TypeNames"),1)==kBetaHash && devices.Observe()[1]==1);
}

// 본문실패는증가깊이/실제pos/앞매핑을보존하고guard해제뒤별도cleanup한번만적용한다.
TEST_CASE(FortTypeNamesStorageAccess_PartialFailureAndSeparateUnwind) {
    StorageAccessDevices devices;devices.stopAtBeta=true;auto hooks=devices.Hooks();bool stopped=false;try { LoadFortTypeNames(8,devices.Globals(),hooks); }catch (const TypeStop&) { stopped=true; }
    CHECK(stopped && devices.depth==4 && !devices.active && devices.activeFields.empty() && !devices.activeMemory);CHECK(devices.Observe()[1]==1);
    CHECK(devices.conversion.Bytes(devices.conversion.current)[0]==1 && devices.conversion.Bytes(devices.conversion.current)[1]==0);
    const auto extra=devices.storage.CreateSection(1,"AfterFailure");CHECK(extra.section!=0); // 예외반환뒤실제guard가해제됐으므로구조변경이가능하다.
    UnwindFortTypeNamesLoad(devices.Globals(),hooks);CHECK(devices.depth==3 && devices.trace.back()=="depth3");
    CHECK(std::find(devices.trace.begin(),devices.trace.end(),"access-throw")!=devices.trace.end() && std::find(devices.trace.begin(),devices.trace.end(),"access-")==devices.trace.end());
}

// 실제빈TypeNames도Select/Access를거치지만기존pointer는free없이0으로덮고미도달경계를요구하지않는다.
TEST_CASE(FortTypeNamesStorageAccess_EmptySectionDoesNotFreePreviousConversion) {
    StorageAccessDevices devices(true);devices.conversion.current=11;devices.conversion.blocks[11].fill(0x9A);auto hooks=devices.Hooks();hooks.allocate={};hooks.conversionBuffer={};hooks.typeCount={};hooks.typeName={};hooks.assertion={};
    LoadFortTypeNames(8,devices.Globals(),hooks);CHECK(devices.depth==3 && devices.conversion.current==0 && devices.conversion.blocks.size()==1);
    CHECK(std::all_of(devices.conversion.blocks[11].begin(),devices.conversion.blocks[11].end(),[](auto value) { return value==0x9A; }));CHECK(devices.Observe()[1]==0);
    CHECK(devices.trace==std::vector<std::string>({"depth4","manager","select","access+","access-","depth3"}));
}
