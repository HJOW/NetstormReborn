// 보호 PE의602행 원문으로 생성·누적 읽기·writer·수신·부모 선택을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBombTimerPacket.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;

namespace {
// 실행기와 같은 명시 DWORD 목록이며 WORD 절삭은 제품 writer가 수행한다.
std::vector<std::uint32_t> Members(std::size_t count) {
    constexpr std::array<std::uint32_t,6> values{60,127,60,0,65535,0x12345678}; // 중복·0·상위 비트 입력이다.
    std::vector<std::uint32_t> result;
    for (std::size_t i=0;i<count;++i) result.push_back(values[i%values.size()]); // 원문 입력 순서만 반복한다.
    return result;
}
// 조회 경계에서만 원본처럼 free 슬롯의 물리 상태/TYPE를 입력한다.
std::span<std::uint8_t> Physical(SidPool& pool,Sid sid) {
    const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()};
}
}

TEST_CASE(BombTimerPacket_IndependentNative602) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-timer-packet-x86.tsv").string(); // C++ 결과와 분리한 원본 자료다.
    std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 모든 입력을 같은 물리 풀/명시 패킷에서 실행한다.
        CHECK(row.size()==5);if (row.size()!=5) continue;
        const auto parts=Split(row[0],',');const auto mode=parts[0];std::vector<std::int64_t> a;
        for (std::size_t i=1;i<parts.size();++i) a.push_back(std::stoll(parts[i])); // signed 지연과 현재비트를 보존한다.
        SidPool pool(OriginalEdition::Patch1078,24000);std::vector<RiftTypeRecord> types(188);types[131].flags2=0x40000000U;
        BombEffectTimerFields fields;fields.endTime=std::bit_cast<double>(std::uint64_t{0x7ff8000000001234ULL});fields.flags=0x80000003U;
        std::array<std::uint8_t,256> packet;packet.fill(0xa5);Put(packet,4,50,2);Put(packet,6,60,2);Put(packet,16,3,2);Put(packet,18,2);
        const auto timeBits=std::bit_cast<std::uint64_t>(123.25);Put(packet,8,static_cast<std::uint32_t>(timeBits));Put(packet,12,static_cast<std::uint32_t>(timeBits>>32));
        const auto input=Members(105);std::string effects,result="-";
        for (std::size_t i=0;i<input.size();++i) Put(packet,22+2*i,input[i],2); // 원문 입력의 WORD 패킷을 별도로 준비한다.
        const auto event=[&](std::string value) { if (!effects.empty()) effects+=';';effects+=value; }; // 큰 경계의 호출 순서만 기록한다.
        if (mode=="C") {
            const double now=std::array<double,3>{0.,100.25,1e12}.at(static_cast<std::size_t>(a[2]));
            RawBombEffectTimer logic(pool,types,now,{});fields=logic.Construct(static_cast<std::int32_t>(a[1]),std::span<const std::uint32_t>(input).first(static_cast<std::size_t>(a[0])),static_cast<std::uint32_t>(a[3]));
            CHECK(row[2]=="N:400"); // 원문 allocator의400BYTE 경계이며 C++ allocator 호출 토큰과 혼동하지 않는다.
        } else if (mode=="W") {
            fields.members=Members(static_cast<std::size_t>(a[0]));result=std::to_string(WriteBombTimerPacket(fields,{50},{60},packet,static_cast<std::uint16_t>(a[1]),static_cast<std::uint8_t>(a[2])));
        } else if (mode=="R") {
            fields.capacity=static_cast<std::uint32_t>(a[0]);Put(packet,16,static_cast<std::uint32_t>(a[2]),2);
            for (std::int64_t i=0;i<a[1];++i) fields.members.push_back(static_cast<std::uint32_t>(900+i)); // 기존 목록을 제품 읽기 전에 입력한다.
            if (a[4]) { Put(packet,8,0x4321);Put(packet,12,0x7ff80000); }
            for (std::int64_t i=0;i<=a[3];++i) ReadBombTimerPacket(fields,packet); // 반복 수신으로 포화 뒤의 동작도 비교한다.
        } else if (mode=="A") {
            auto raw=Physical(pool,{50});raw[11]=static_cast<std::uint8_t>(a[0]);std::uint32_t boss=static_cast<std::uint32_t>(a[1]),kind=57;BombTimerReceiveHooks hooks;
            hooks.assertion=[&](std::uint32_t line) { event("D:"+std::to_string(line));if (line==0xcc && a[2]==1) raw[11]=1; };
            hooks.create=[&](std::uint32_t type,std::uint32_t x,std::uint32_t y) { event("T:"+std::to_string(type));event("C:"+std::to_string(x)+':'+std::to_string(y));fields.capacity=100;fields.members.clear();if (a[2]==2) kind=58;return 1U; };
            hooks.read=[&](std::uint32_t token,std::span<const std::uint8_t> current) { CHECK(token==1);if (a[2]==3) { Put(packet,4,51,2);Put(packet,6,70,2); }ReadBombTimerPacket(fields,current); };
            hooks.attach=[&](std::uint32_t token,std::uint32_t type,Sid parent,Sid form,std::uint32_t flags) { CHECK(token==1);event("A:"+std::to_string(type)+':'+std::to_string(parent.value)+':'+std::to_string(form.value)+':'+std::to_string(flags)); };
            ReceiveBombTimerPacket(pool,boss,kind,packet,hooks);
        } else if (mode=="P") {
            const auto current=std::array<std::vector<std::uint32_t>,4>{{{},{60},{60,127,1},{1,127,60}}}.at(static_cast<std::size_t>(a[0]));
            for (const auto [sid,kind]:std::array<std::array<std::uint32_t,2>,3>{{{60,130},{127,131},{1,132}}}) { auto raw=Physical(pool,{static_cast<std::uint16_t>(sid)});raw[10]=static_cast<std::uint8_t>(kind);raw[11]=static_cast<std::uint8_t>(a[2]); } // 후보의 상태값도 추가 필터 없이 입력한다.
            if (a[1]==1) Physical(pool,{60})[10]=131;if (a[1]==2) Physical(pool,{127})[10]=130;
            result=std::to_string(BombTimerParent(pool,types,current).value);
        } else CHECK(false);
        std::string packed=std::to_string(fields.capacity)+','+std::to_string(std::bit_cast<std::uint64_t>(fields.endTime))+','+std::to_string(fields.flags);
        for (const auto member:fields.members) packed+=','+std::to_string(member); // 실제 최종 목록 전체를 비교하며 중복을 줄이지 않는다.
        if (effects.empty()) effects="-";
        if (result!=row[1] || (mode!="C" && effects!=row[2]) || packed!=row[3] || Hex(packet)!=row[4]) {
            std::cout<<"타이머 패킷 원문행"<<count<<':'<<row[0]<<" 실제"<<result<<'/'<<effects<<'/'<<packed<<'\n';CHECK(false);
        }
        ++count;
    }
    CHECK(count==602);std::cout<<"타이머 생성/패킷/부모 원문"<<count<<"행 대조\n";
}

TEST_CASE(BombTimerPacket_InvalidBoundaryRejects) {
    BombEffectTimerFields fields;fields.members={60};std::array<std::uint8_t,23> shortPacket{};
    CHECK(Throws([&] { WriteBombTimerPacket(fields,{50},{60},shortPacket,22); }));
    Put(shortPacket,16,1,2);CHECK(Throws([&] { ReadBombTimerPacket(fields,shortPacket); }));
    SidPool pool(OriginalEdition::Patch1078,24000);std::vector<RiftTypeRecord> types(188);const std::array<std::uint32_t,1> invalid{0x80000000U};
    CHECK(Throws([&] { BombTimerParent(pool,types,invalid); }));
}
