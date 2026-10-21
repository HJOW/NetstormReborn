// 게임 하위를 명시 경계로 둔 독립 원본216입력의 계산/호출 순서/메시지 원문을 대조한다.
#include "ConstructionSupport.h"
#include "o/RawFactoryResearch.h"
using namespace netstorm::test::construction;
namespace {
// 문자열의 원본 바이트를 관측 TSV와 같은 hex로 나타낸다.
std::string Bytes(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 원본 실행기의 검색/슬롯/생성/송신 경계에 공급한 명시 입력이다.
struct ResearchScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원본과 같은 물리 SID 범위다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 실제 genus 입력이다.
    std::uint32_t loading{},dirty{3},outpost{126};bool boss{true};double now{12.5}; // 현재 원본 전역 입력이다.
    Sid found{};std::int32_t slots{2};std::string trace; // 검색/슬롯의 명시 경계와 관측한 순서다.
    FactoryResearchHooks hooks; // 계산을 대신하지 않는 하위 관측 경계다.
    // 호출 경계의 효과만 기록하며 기대 결과는 만들지 않는다.
    ResearchScene() {
        auto parent=InputRaw(pool,Sid{50});std::fill(parent.begin(),parent.end(),std::uint8_t{});Put(parent,0,0x509148U);parent[10]=85;parent[34]=1;
        auto form=InputRaw(pool,Sid{100});std::fill(form.begin(),form.end(),std::uint8_t{});Put(form,0,0x512208U);Put(form,18,128,2);Put(form,20,16,2);types[85].flags2=0x4000;
        hooks.find=[this](Sid parent,std::uint32_t mask,std::uint32_t,std::uint32_t index) { CHECK(parent.value==50 && mask==16 && index==0);Add("find,"+std::to_string(found.value));return found; };
        hooks.slots=[this](Sid parent) { CHECK(parent.value==50);Add("slots,"+std::to_string(slots));return slots; };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Add("destroy,"+std::to_string(sid.value)+","+std::to_string(flags));found={}; };
        hooks.create=[this](Sid parent,std::uint32_t kind,std::uint32_t quantity,std::uint32_t flags,std::uint32_t attach,std::uint32_t quality) { Add("create,"+std::to_string(parent.value)+","+std::to_string(kind)+","+std::to_string(quantity)+","+std::to_string(flags)+","+std::to_string(attach)+","+std::to_string(quality));return Sid{20000}; };
        hooks.send=[this](std::uint32_t channel,std::span<const std::uint8_t> packet) { Add("send,"+std::to_string(channel)+","+Hex(packet)); };
        hooks.help=[this](std::string_view text,std::uint32_t flags) { Add("help,"+Bytes(text)+","+std::to_string(flags)); };
        hooks.flare=[this](Sid parent,std::int32_t count,std::uint32_t event) { Add("flare,"+std::to_string(parent.value)+","+std::to_string(count)+","+std::to_string(event)); };
        hooks.refresh=[this](std::uint32_t owner) { Add("refresh,"+std::to_string(owner)); };
        hooks.assertion=[this](std::string_view text) { Add("assert,"+Bytes(text)); };
    }
    // 빈 호출 목록은 '-'로 변환할 때만 처리한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 같은 현재 상태 참조를 매 새 몸체에 연결한다.
    FactoryResearchState State() { return {loading,dirty,boss,now,outpost}; }
};
}
// 원본216행의 반환·dirty·전체 호출 순서와16바이트 송신을 모두 대조한다.
TEST_CASE(FactoryResearch_Native216) {
    std::array<std::size_t,4> counts{};
    // 기대값은 독립 원본 TSV만 읽으며 새 몸체로 재계산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_FACTORY_RESEARCH_FIXTURE).rows) {
        ResearchScene scene;std::uint32_t result{};
        if (row[0]=="Learn") {
            scene.found={static_cast<std::uint16_t>(Number(row[1]))};scene.slots=std::stoi(row[2]);RawFactoryResearch research(scene.pool,scene.types,scene.State(),scene.hooks);research.Learn(Sid{50},128,Number(row[3]));++counts[0];
        } else if (row[0]=="Input") {
            scene.found={static_cast<std::uint16_t>(Number(row[2]))};scene.slots=std::stoi(row[3]);RawFactoryResearch research(scene.pool,scene.types,scene.State(),scene.hooks);result=research.Input(Sid{50},0x03000080U|Number(row[4]),(Number(row[1])<<16)|50U);++counts[1];
        } else if (row[0]=="Receive") {
            auto parent=InputRaw(scene.pool,Sid{50});parent[11]=static_cast<std::uint8_t>(Number(row[2]));parent[10]=static_cast<std::uint8_t>(Number(row[4]));scene.types[parent[10]].flags2=Number(row[3]);scene.boss=Number(row[5])!=0;std::array<std::uint8_t,16> packet{};WriteFactoryResearchCommand(Sid{50},Number(row[6]),scene.now,packet);packet[7]=static_cast<std::uint8_t>(Number(row[7]));RawFactoryResearch research(scene.pool,scene.types,scene.State(),scene.hooks);research.Receive(Number(row[1]),packet);++counts[2];
        } else {
            CHECK(row[0]=="Notice");const Sid form{static_cast<std::uint16_t>(Number(row[1]))};Put(InputRaw(scene.pool,form),20,Number(row[3]),2);scene.loading=Number(row[4]);scene.found={static_cast<std::uint16_t>(Number(row[5]))};RawFactoryResearch research(scene.pool,scene.types,scene.State(),scene.hooks);research.Notice(Sid{50},form,Number(row[2]));++counts[3];
        }
        const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (trace!=row.back()) std::printf("연구 원문 %s 불일치\n실제:%s\n원본:%s\n",row[0].c_str(),trace.c_str(),row.back().c_str());
        CHECK(result==Number(row[row.size()-3]));CHECK(scene.dirty==Number(row[row.size()-2]));CHECK(trace==row.back());
    }
    CHECK(counts[0]==32 && counts[1]==72 && counts[2]==16 && counts[3]==96);
}
// 실제 분배기의 등록·길이·sender 전달을 확인하고 미복원 업그레이드를 가짜 성공으로 만들지 않는다.
TEST_CASE(FactoryResearch_PacketDispatchAndMissingEffects) {
    ResearchScene scene;RawFactoryResearch research(scene.pool,scene.types,scene.State(),scene.hooks);
    PacketDispatchHooks hooks;hooks.now=[] { return 19U; };hooks.record=[](auto&,auto,auto) {};hooks.terminate=[](auto&) {};hooks.close=[](auto&) {};hooks.release=[](auto&) {};
    RawPacketDispatch dispatch(OriginalEdition::Patch1078,hooks);RegisterFactoryResearchCommand(dispatch,research);
    const auto& registration=dispatch.Registration(0x58);CHECK(registration.length==16 && registration.version==1 && registration.mismatch==0 && registration.observer);
    std::array<std::uint8_t,18> packet;packet.fill(0xa5);CHECK(WriteFactoryResearchCommand(Sid{50},128,12.5,packet)==16);CHECK(packet[16]==0xa5 && packet[17]==0xa5);
    PacketConnection connection;connection.sender=257;CHECK(dispatch.Dispatch(connection,packet));CHECK(scene.trace.empty());connection.sender=1;CHECK(dispatch.Dispatch(connection,packet));CHECK(scene.trace=="find,0;slots,2;find,0;create,50,128,0,16,0,0");
    packet[2]=15;CHECK(!dispatch.Validate(packet));packet[2]=16;packet[6]=0;packet[7]=1;CHECK(Throws([&] { research.Receive(1,packet); }));
    CHECK(Throws([&] { WriteFactoryResearchCommand(Sid{50},128,12.5,std::span(packet).first(15)); }));CHECK(Throws([&] { research.Receive(1,std::span(packet).first(7)); }));
    CHECK(research.HandlesNotice(Sid{50}));Put(InputRaw(scene.pool,Sid{50}),0,0x509210U);CHECK(research.HandlesNotice(Sid{50}));Put(InputRaw(scene.pool,Sid{50}),0,0x5096a0U);CHECK(!research.HandlesNotice(Sid{50}));
    scene.hooks.flare={};RawFactoryResearch missing(scene.pool,scene.types,scene.State(),scene.hooks);CHECK(Throws([&] { missing.Notice(Sid{50},Sid{100},1); }));CHECK(scene.dirty==3);
}
// 슬롯로그뒤현재시각을읽고로그가슬롯을바꿔도입력의원래양수판단을유지한다.작성·컴파일만한다.
TEST_CASE(FactoryResearch_InputLogBeforeClockAndCachedSlots) {
    ResearchScene scene;std::vector<std::uint8_t> sent;scene.hooks.inputLog=[&](Sid parent,std::int32_t slots) { CHECK(parent.value==50 && slots==2);scene.Add("inputLog");scene.now=33.75;scene.slots=0; };scene.hooks.send=[&](auto channel,std::span<const std::uint8_t> packet) { CHECK(channel==9);scene.Add("send");sent.assign(packet.begin(),packet.end()); };
    RawFactoryResearch research(scene.pool,scene.types,scene.State(),scene.hooks);CHECK(research.Input(Sid{50},0x03000080U,(128U<<16)|50U)==1);CHECK(scene.trace=="slots,2;inputLog;find,0;slots,0;send" && sent.size()==16);if (sent.size()!=16) return;std::uint64_t bits{};
    // 패킷QWORD를독립적으로조립해로그뒤현재시각비트를확인한다.
    for (std::size_t index=0;index<8;++index) bits|=static_cast<std::uint64_t>(sent[8+index])<<(index*8);CHECK(std::bit_cast<double>(bits)==33.75 && sent[6]==128);
}
