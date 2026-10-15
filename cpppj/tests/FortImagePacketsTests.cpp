// 실제 DataManager 이미지와 sendFort의 예약·선언·전송 전체 BYTE를 독립 PE 관찰과 비교한다.
#include "DudeSpawnSupport.h"
#include "o/RawFortImage.h"
#include "o/RawMissionLoginPackets.h"
#include <array>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// Early Missions 여섯 개와 캠페인1-1/1-2의 실제 loadFort 파일명이다.
constexpr std::array<const char*,8> kFortNames{{"BridgeTheGap","SecretWorkshop","CaptureThePriest","TacticalCombat","SubtleDefense","RawPower","TheWarBegins","MasterOfWhirligigs"}};
constexpr std::size_t kFortNativeCases=219; // 원본 Save116/send93/observer10의 중복 없는 입력 개수다.
// 원본에 준 현재 섹션 입력만 만들며 이미지의 기대 헤더·길이는 계산하지 않는다.
FortTemplate FortInput(std::uint32_t profile,std::uint32_t seed,std::uint32_t count,std::uint32_t flag) {
    FortTemplate fort;
    if (profile>=100) {
        const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
        fort=FortTemplate::Parse(assets.files.Read(std::string("d/")+kFortNames.at(profile-100)+".fort"),assets.game.TypeTable());
        if (seed&1U) { if (fort.sections.size()<13) fort.sections.resize(13);fort.sections[12]={0xa0,static_cast<std::uint8_t>(profile),static_cast<std::uint8_t>(seed),0,0xfe}; }
    } else {
        fort.sections.resize(count);
        // 합성 payload는 원본 입력과 같은 패턴이며 저장 순서·길이 기대값을 만들지 않는다.
        for (std::uint32_t index=0;index<count;++index) {
            const std::size_t size=profile==0 ? 0 : profile==1 ? (index*7+seed)%17 : profile==2 && index==34 ? 32696 : profile==3 && index==34 ? 32695 : 0;
            fort.sections[index].resize(size);
            // BYTE 패턴은 각 현재 stream의 독립 입력이다.
            for (std::size_t byte=0;byte<size;++byte) fort.sections[index][byte]=static_cast<std::uint8_t>(byte*29+index*13+seed);
        }
    }
    fort.flag=static_cast<std::uint8_t>(flag);return fort;
}
// 빈 관찰은 fixture의 단일 '-'로 표시한다.
std::string FortHex(std::span<const std::uint8_t> bytes) { return bytes.empty() ? "-" : Hex(bytes); }
// 원본의 논리 이미지 뒤 전체 고정 할당이0으로 남았는지 검사한다.
void CheckFortAllocation(const FortImageData& image) {
    CHECK(image.storage.size()==RawFortImage::kCapacity);CHECK(image.end<=image.storage.size());
    CHECK(std::all_of(image.storage.begin()+static_cast<std::ptrdiff_t>(image.end),image.storage.end(),[](std::uint8_t byte) { return byte==0; }));
}
}

// 모든 독립 입력의 현재 이미지/송신 예약/길이 observer/wire/효과 읽기 순서를 대조한다.
TEST_CASE(FortImagePackets_Native) {
    std::size_t count=0;
    // 각 행의 현재 Template·Player·효과 변경은 다음 행에 누적하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_FORT_IMAGE_PACKETS_FIXTURE).rows) {
        if (row[0]=="kind") continue;CHECK(row.size()==7);const auto fields=Split(row[1],',');CHECK(fields.size()==13);
        std::array<std::uint32_t,13> input{};
        // fixture의 DWORD 비트를 보존한 명시 입력을 읽는다.
        for (std::size_t index=0;index<input.size();++index) input[index]=Number(fields[index]);
        auto first=FortInput(input[0],input[1],input[2],input[3]);auto second=FortInput(input[0],input[1]+3,input[2],input[3]^0xfeU);
        std::array<std::uint8_t,1620> players{};players[input[6]*180+0x58]=static_cast<std::uint8_t>(input[9]);
        const FortTemplate* current=input[11] ? &first : nullptr;std::string trace;
        const auto event=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; // 실제 외부 읽기/효과만 기록한다.
        FortImageHooks imageHooks;
        imageHooks.typeCount=[&] { event("G,"+std::to_string(input[4]));return input[4]; };
        imageHooks.assertion=[&](FortImageAssertion kind) { event(kind==FortImageAssertion::TypeCount ? "A,types" : kind==FortImageAssertion::SectionCapacity ? "A,section" : "A,stream"); };
        std::string imageText="-",before="-",after="-",wire="-";
        if (row[0]=="I") { const auto image=RawFortImage(OriginalEdition::Patch1078,first,imageHooks).Save();CheckFortAllocation(image);imageText=FortHex(image.Bytes()); }
        else if (row[0]=="F") {
            std::array<std::uint8_t,20> packet;packet.fill(0xa5);Put(packet,12,input[8]);
            FinalizeMissionFortPacket(packet,input[12]!=0,[&] { event("A,packet"); });before=Hex(packet);
        } else {
            MissionFortPacketReads reads;
            reads.current=[&](std::uint32_t owner) { CHECK(owner==input[6]);event(current ? "C,1" : "C,0");return current; };
            reads.initialLength=[&] { event("H,"+std::to_string(input[10]));if (input[12]==3) players[input[6]*180+0x58]=0x7f;return static_cast<std::uint16_t>(input[10]); };
            MissionFortPacketHooks hooks;
            hooks.log=[&](std::uint32_t owner,std::int32_t target) { CHECK(owner==input[6] && std::bit_cast<std::uint32_t>(target)==input[7]);event("L");if (input[12]==1) current=&second;else if (input[12]==2) current=nullptr; };
            hooks.missingTemplate=[&] { event("A,template");CHECK(input[12]==2);current=&second; };
            hooks.send=[&](std::int32_t target,std::span<const std::uint8_t> bytes) {
                event("S,"+std::to_string(std::bit_cast<std::uint32_t>(target)));wire=Hex(bytes);const auto imageSize=Get(bytes,12);CHECK(bytes.size()==imageSize+17);
                const auto image=bytes.subspan(17,imageSize);imageText=FortHex(image);
                // 실제 송신 출력에서 같은 예약 helper를 별도 재생해 ff stale tail까지 비교한다.
                auto packet=MakeMissionFortPacket(input[6],input[8],bytes[16],image,static_cast<std::uint16_t>(input[10]));before=Hex(packet.storage);
                FinalizeMissionFortPacket(packet.storage,true);after=Hex(packet.storage);CHECK(Hex(packet.Bytes())==wire);
            };
            RawMissionLoginPackets packets(OriginalEdition::Patch1078,players,std::move(reads),imageHooks,std::move(hooks));CHECK(packets.SendFort(input[6],std::bit_cast<std::int32_t>(input[7]),input[8])==(input[11]!=0));
        }
        if (imageText!=row[2] || before!=row[3] || after!=row[4] || wire!=row[5] || (trace.empty() ? "-" : trace)!=row[6])
            throw std::runtime_error("Fort native 행 "+std::to_string(count)+" ("+row[0]+","+row[1]+") 현재 trace "+trace+" 기대 "+row[6]);
        ++count;
    }
    CHECK(count==kFortNativeCases);
}

// 실제 파일의 현재 raw 섹션 변경이 반영되며 파일 캐시/해석 Money 필드로 되돌리지 않는다.
TEST_CASE(FortImage_CurrentProtectedSections) {
    // 목표8개 보호 fort의 실제 현재 섹션을 같은 writer로 저장한다.
    for (std::uint32_t profile=100;profile<108;++profile) {
        auto fort=FortInput(profile,0,35,0);FortImageHooks hooks;hooks.typeCount=[] { return 188U; };const auto original=RawFortImage(OriginalEdition::Patch1078,fort,hooks).Save();
        fort.money=999999.0F;const auto unchanged=RawFortImage(OriginalEdition::Patch1078,fort,hooks).Save();CHECK(original.storage==unchanged.storage && original.end==unchanged.end);
        fort.sections[12]={0xa0,static_cast<std::uint8_t>(profile),1,0,0xfe};const auto changed=RawFortImage(OriginalEdition::Patch1078,fort,hooks).Save();CHECK(changed.storage!=original.storage);CheckFortAllocation(changed);
        const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto parsed=FortTemplate::Parse(changed.Bytes(),assets.game.TypeTable());CHECK(parsed.sections==fort.sections);
    }
}

// 원본의 엄격한 '<' 진단은 정확히32768인 정상 물리 쓰기를 막지 않고 초과만 명시 거부한다.
TEST_CASE(FortImage_PhysicalCapacityAndRequiredConsumers) {
    auto fort=FortInput(2,0,35,0);std::vector<FortImageAssertion> assertions;FortImageHooks hooks;hooks.typeCount=[] { return 188U; };hooks.assertion=[&](FortImageAssertion kind) { assertions.push_back(kind); };
    const auto exact=RawFortImage(OriginalEdition::Patch1078,fort,hooks).Save();CHECK(exact.end==32768 && assertions==std::vector<FortImageAssertion>{FortImageAssertion::SectionCapacity});
    fort.sections[34].push_back(1);CHECK(Throws([&] { RawFortImage(OriginalEdition::Patch1078,fort,hooks).Save(); }));
    fort.sections.clear();hooks.typeCount=[] { return 256U; };hooks.assertion={};CHECK(Throws([&] { RawFortImage(OriginalEdition::Patch1078,fort,hooks).Save(); }));
    CHECK(Throws([&] { RawFortImage(OriginalEdition::Cd1072,fort,hooks); }));hooks.typeCount={};CHECK(Throws([&] { RawFortImage(OriginalEdition::Patch1078,fort,hooks); }));
    hooks.typeCount=[] { return 188U; };fort.sections.resize(36);CHECK(Throws([&] { RawFortImage(OriginalEdition::Patch1078,fort,hooks); }));
    FortImageData invalid;invalid.end=1;CHECK(Throws([&] { invalid.Bytes(); }));
}

// 현재 Template 없음은 자연 반환이고 선택된 실제 소비자 누락은 그 원문 위치에서 거부한다.
TEST_CASE(FortImagePackets_SelectedBoundaries) {
    auto fort=FortInput(0,0,35,0);std::array<std::uint8_t,1620> players{};const FortTemplate* current=nullptr;std::size_t reads=0;MissionFortPacketReads input;
    input.current=[&](std::uint32_t) { ++reads;return current; };input.initialLength=[] { return std::uint16_t{}; };FortImageHooks image;image.typeCount=[] { return 188U; };MissionFortPacketHooks hooks;
    RawMissionLoginPackets absent(OriginalEdition::Patch1078,players,input,image,hooks);CHECK(!absent.SendFort(1,9,0) && reads==1);current=&fort;CHECK(Throws([&] { absent.SendFort(1,9,0); }));
    hooks.log=[](std::uint32_t,std::int32_t) {};RawMissionLoginPackets noSend(OriginalEdition::Patch1078,players,input,image,hooks);CHECK(Throws([&] { noSend.SendFort(1,9,0); }));CHECK(Throws([&] { noSend.SendFort(9,9,0); }));
    input.current={};CHECK(Throws([&] { RawMissionLoginPackets(OriginalEdition::Patch1078,players,input,image,hooks); }));
    FinalizeMissionFortPacket({},false);CHECK(Throws([&] { FinalizeMissionFortPacket({},true); }));
    std::array<std::uint8_t,17> packet{};Put(packet,12,0xffffffffU);CHECK(Throws([&] { FinalizeMissionFortPacket(packet,true); }));
    MissionFortPacket malformed;CHECK(Throws([&] { malformed.Bytes(); }));malformed.storage.resize(4);CHECK(Throws([&] { malformed.Bytes(); }));
    std::vector<std::uint8_t> oversized(32769);CHECK(Throws([&] { MakeMissionFortPacket(1,0,0,oversized,0); }));
}
