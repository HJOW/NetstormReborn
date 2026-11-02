// 원문 순수 진단 fixture와 실제 checking owner의 수명/재조회만 검사하며 게임 행동은 실행하지 않는다.
#include "TestSupport.h"
#include "o/RawCheckingDiagnostics.h"
#include "o/RawGunProcess.h"
#include "o/RawFactoryUpgradeCommand.h"
#include "o/RawAiCarrierActions.h"
#include "o/RawContainedFinder.h"
#include "client/ClientGameplayGlobals.h"
#include <filesystem>
#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

using namespace netstorm::o; // 실제 raw 진단/Pool/TYPE를 사용하며 별도 행동 모형을 만들지 않는다.
namespace { // 동결 관측의 입력 해독과 새 합성 진단 슬롯만 소유한다.
std::vector<std::string> CheckingColumns(const std::string& line,char delimiter) { // 빈값이 없는 TSV/입력 목록의 필드를 원래 순서로 읽는다.
    std::stringstream stream(line);std::string item;std::vector<std::string> result;
    while (std::getline(stream,item,delimiter)) result.push_back(item);return result; // 실제 필드 수만 순회한다.
}
std::vector<std::uint32_t> CheckingNumbers(const std::string& line) { // fixture의 원시 uint32 입력을 signed로 축소하지 않는다.
    std::vector<std::uint32_t> result;for (const auto& item:CheckingColumns(line,',')) result.push_back(static_cast<std::uint32_t>(std::stoull(item)));return result; // 입력 원래 순서다.
}
std::ifstream CheckingFixture(const char* name) { return std::ifstream(std::filesystem::path(__FILE__).parent_path()/"fixtures"/name); } // 새 fixture만 읽으며 기존 파일에는 쓰지 않는다.
void CheckingPut(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) { // 합성 진단 슬롯의 선택한 BYTE만 쓴다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(8*index)); // 원문 little endian 폭이다.
}
template<class F> bool CheckingFails(F operation) { try { operation(); }catch (const std::exception&) { return true; }return false; } // 호스트 물리 범위/순환은 가짜 성공 대신 명시 실패한다.
}

TEST_CASE(CheckingConsumers_CurrentFullDwordViews) {
    std::int32_t owner{};RawCheckingFlag first;first.Bind(owner);const auto copy=first;
    bool enabled{};RawCheckingFlag oldBoolean{enabled};std::uint32_t word=29;RawCheckingFlag native{word};
    for (const auto value:std::array<std::uint32_t,5>{0,1,29,99,0xffffffffU}) { // 전체 비트와 비영 관문의 실시간 변경을 함께 확인한다.
        owner=std::bit_cast<std::int32_t>(value);CHECK(first.Read()==value && copy.Read()==value);CHECK(bool(first)==(value!=0));
    }
    enabled=true;word=99;CHECK(oldBoolean && native.Read()==99);enabled=false;word=0;CHECK(!oldBoolean && !native);
}

TEST_CASE(CheckingConsumers_TemporaryAndParentViewLifetime) {
    RawCheckingFlag literal{true},signedLiteral{std::int32_t{-1}},wordLiteral{std::uint32_t{29}};
    CHECK(literal.Read()==1 && signedLiteral.Read()==0xffffffffU && wordLiteral.Read()==29);
    const bool constBoolean=true;const std::int32_t constSigned=-1;const std::uint32_t constWord=99; // const rvalue 경로도 임시 주소를 보관하면 안 된다.
    const RawCheckingFlag movedBoolean{std::move(constBoolean)},movedSigned{std::move(constSigned)},movedWord{std::move(constWord)};CHECK(movedBoolean.Read()==1 && movedSigned.Read()==0xffffffffU && movedWord.Read()==99);
    std::int32_t owner{};GunProcessState parent;parent.checking.Bind(owner);bool boss=true;double now{};
    const FactoryUpgradeCommandState child{boss,parent.checking,now}; // 임시 bool 참조 대신 원래 장기 owner를 공유한다.
    owner=99;CHECK(child.checking.Read()==99);owner=0;CHECK(!child.checking);
    const auto stable=[](const std::int32_t& value) { RawCheckingFlag temporary;temporary.Bind(value);return temporary; }(owner); // 반환한 뷰가 지역 wrapper를 빌리지 않는지 확인한다.
    owner=-1;CHECK(stable.Read()==0xffffffffU);
}

TEST_CASE(CheckingConsumers_ClientOwnerAndFinderReborrow) {
    netstorm::client::ClientGameplayGlobals first,second;auto* identity=&first.AiDebug().Checking();
    first.WriteDebugWord(0x5e4794,99);CHECK(first.finder.checkingDead.Read()==99 && !second.finder.checkingDead);
    { auto use=first.Acquire();CHECK(&first.AiDebug().Checking()==identity && first.finder.checkingDead.Read()==99); }
    first.WriteDebugWord(0x5e4794,0xffffffffU);CHECK(first.finder.checkingDead.Read()==0xffffffffU);
    { auto use=first.AcquirePrepared();CHECK(first.finder.checkingDead.Read()==0xffffffffU); }
    CHECK(&first.AiDebug().Checking()==identity && first.ReadDebugWord(0x5e4794)==0xffffffffU && !second.AiDebug().Checking());
}

TEST_CASE(CheckingConsumers_NativeFormsDiagnostic) {
    auto input=CheckingFixture("checking-forms-x86.tsv");CHECK(bool(input));std::string line;std::size_t rows{};
    SidPool pool(OriginalEdition::Patch1078,24000);const auto parent=pool.Allocate(),child=pool.Allocate(),last=pool.Allocate();
    auto p=pool.AllocatedBytes(parent),c=pool.AllocatedBytes(child),d=pool.AllocatedBytes(last); // 일반 raw 슬롯만 할당하며 TYPE factory/Pop/게임 Frame은 실행하지 않는다.
    while (std::getline(input,line)) { // 최초 원문 관측의 모든 정상 유한 체인을 검사한다.
        if (line.empty() || line[0]=='#') continue;const auto fields=CheckingColumns(line,'\t');const auto v=CheckingNumbers(fields.at(0));CHECK(v.size()==7);++rows;
        p[10]=static_cast<std::uint8_t>(v[0]);p[11]=static_cast<std::uint8_t>(v[1]);CheckingPut(p,6,v[2] ? child.value : 0,2);
        c[10]=static_cast<std::uint8_t>(v[3]);c[11]=static_cast<std::uint8_t>(v[4]);CheckingPut(c,4,v[2]==2 ? last.value : 0,2);d[10]=static_cast<std::uint8_t>(v[5]);d[11]=static_cast<std::uint8_t>(v[6]);CheckingPut(d,4,0,2);
        const auto before=std::vector<std::uint8_t>(pool.Bytes().begin(),pool.Bytes().end());CHECK(CheckRawObjectForms(pool,parent)==(fields.at(1)=="1"));CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));
    }
    CHECK(rows==288);
}

TEST_CASE(CheckingConsumers_FormsHostPhysicalBoundaries) {
    SidPool pool(OriginalEdition::Patch1078,24000);const auto parent=pool.Allocate(),child=pool.Allocate();auto p=pool.AllocatedBytes(parent),c=pool.AllocatedBytes(child);
    p[10]=70;p[11]=0;CheckingPut(p,6,child.value,2);c[10]=6;c[11]=2;CheckingPut(c,4,0,2);CHECK(CheckRawObjectForms(pool,parent)); // dead2의 Form은 원문이 허용한다.
    CheckingPut(c,4,child.value,2);CHECK(CheckingFails([&] { static_cast<void>(CheckRawObjectForms(pool,parent)); }));
    CheckingPut(c,4,65534,2);CHECK(CheckingFails([&] { static_cast<void>(CheckRawObjectForms(pool,parent)); }));
}

TEST_CASE(CheckingConsumers_NativeIslandOwnerDiagnosticPrefix) {
    auto input=CheckingFixture("checking-island-owner-x86.tsv");CHECK(bool(input));std::string line;std::size_t rows{};
    SidPool pool(OriginalEdition::Patch1078,24000);const auto parent=pool.Allocate(),first=pool.Allocate(),second=pool.Allocate(),last=pool.Allocate();
    auto p=pool.AllocatedBytes(parent),a=pool.AllocatedBytes(first),b=pool.AllocatedBytes(second),c=pool.AllocatedBytes(last);SquidHash hash;std::vector<RiftTypeRecord> types(188);
    for (const auto sid:{first,second,last}) { // 모두 같은 finder 영역의 합성 표면이며 게임 가상 생성/Pop은 호출하지 않는다.
        auto raw=pool.AllocatedBytes(sid);raw[11]=0;raw[40]=0;CheckingPut(raw,14,std::bit_cast<std::uint32_t>(20.5F));CheckingPut(raw,18,std::bit_cast<std::uint32_t>(20.5F));
    }
    types[98].footX=types[98].footY=types[99].footX=types[99].footY=1;hash.Bucket(0,20.5F,20.5F)=first.value;CheckingPut(a,4,second.value,2);CheckingPut(b,4,last.value,2);CheckingPut(c,4,0,2);
    while (std::getline(input,line)) { // 독립 원문 접두의 비영 DWORD·조건 재조회·callback 뒤 변화를 현재 finder에서 검사한다.
        if (line.empty() || line[0]=='#') continue;const auto fields=CheckingColumns(line,'\t');const auto v=CheckingNumbers(fields.at(0));CHECK(v.size()==5 && fields.size()==6);++rows;
        std::uint32_t checking=v[0],noIsland=v[2],calls{};p[34]=static_cast<std::uint8_t>(v[1]);a[10]=b[10]=99;c[10]=98;a[34]=b[34]=2;c[34]=3;
        const float upper=v[4]==0 ? 21.0F : v[4]==1 ? 21.000002F : 21.00002F;const SquidFootprint footprint{20.25F,20.25F,upper,upper};
        DiagnoseRawGunIslandOwners(pool,hash,types,parent,footprint,RawCheckingFlag{checking},noIsland,[&](auto number) {
            CHECK(number==0xad);++calls;if (calls==1) { if (v[3]==1) checking=0;if (v[3]==2) p[34]=2;if (v[3]==3) noIsland=98; }
        });
        CHECK(calls==std::stoul(fields[1]) && checking==std::stoull(fields[2]) && noIsland==std::stoull(fields[3]) && p[34]==std::stoul(fields[4]));
        if (checking || fields[5]!="-") { const auto area=CheckingNumbers(fields[5]);CHECK(area.size()==4);CHECK(checking_detail::StoredInteger(upper,true)==static_cast<std::int32_t>(area.at(2))); }
    }
    CHECK(rows==192);
}
