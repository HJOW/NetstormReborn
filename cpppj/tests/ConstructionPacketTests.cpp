// 원본의전체512바이트와송신직전가변길이쓰기를독립fixture에대조한다.
#include "TestSupport.h"
#include "o/RawConstructionPacket.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
using namespace netstorm::o;
namespace {
// 마지막빈필드도보존하며선택한한구분자로만나눈다.
std::vector<std::string> PacketFields(std::string_view line,char delimiter) {
    std::vector<std::string> result;std::size_t begin=0;
    // 끝구분자가없어도남은마지막값을한번보관한다.
    for (;;) { const auto end=line.find(delimiter,begin);result.emplace_back(line.substr(begin,end==line.npos ? line.npos : end-begin));if (end==line.npos) return result;begin=end+1; }
}
// fixture의전체raw바이트를문자치환없이복원한다.
std::vector<std::uint8_t> PacketBytes(std::string_view value) {
    std::vector<std::uint8_t> result;
    // 각두자리16진수를실제BYTE하나로읽는다.
    for (std::size_t i=0;i<value.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(value.substr(i,2)),nullptr,16)));return result;
}
// 원본입력의DWORD비트를보존한다.
std::uint32_t PacketNumber(const std::string& value) { return static_cast<std::uint32_t>(std::stoul(value)); }
}
// 실제Header+Confirm전체출력과최종화의진단후계속쓰기까지비교한다.
TEST_CASE(ConstructionPacket_IndependentPe86) {
    std::ifstream input(NETSTORM_CONSTRUCTION_PACKET_FIXTURE);CHECK(input.good());std::string line;std::size_t count=0;
    // C++에서기대헤더/길이/필드값을계산하지않고원본전체512바이트를읽는다.
    while (std::getline(input,line)) {
        if (line.empty() || line.starts_with("operation\t")) continue;const auto row=PacketFields(line,'\t');CHECK(row.size()==6);
        std::vector<std::uint8_t> bytes(517,0xcc);
        if (row[0]=="Write") {
            const auto values=PacketFields(row[1],','),header=PacketFields(row[2],',');CHECK(values.size()==10 && header.size()==3);
            ConstructionNotice notice;notice.type=static_cast<std::uint8_t>(PacketNumber(values[0]));notice.x=std::bit_cast<float>(PacketNumber(values[1]));notice.y=std::bit_cast<float>(PacketNumber(values[2]));
            notice.argument=static_cast<std::uint8_t>(PacketNumber(values[3]));notice.direction=static_cast<std::uint8_t>(PacketNumber(values[4]));notice.player=static_cast<std::uint8_t>(PacketNumber(values[5]));notice.flags=static_cast<std::uint8_t>(PacketNumber(values[6]));notice.quality=static_cast<std::uint8_t>(PacketNumber(values[7]));notice.timeLow=PacketNumber(values[8]);notice.timeHigh=PacketNumber(values[9]);
            // 실제Factory가원본실행에반환한SID순서만재생한다.
            if (row[3]!="-") for (const auto& sid:PacketFields(row[3],',')) notice.sids.push_back(static_cast<std::uint16_t>(PacketNumber(sid)));
            CHECK(WriteConstructionPacket(notice,{static_cast<std::uint8_t>(PacketNumber(header[0])),static_cast<std::uint8_t>(PacketNumber(header[1])),static_cast<std::uint16_t>(PacketNumber(header[2]))},bytes)==512);
        } else if (row[0]=="Finalize") {
            const auto before=PacketBytes(row[3]);CHECK(before.size()==512);std::copy(before.begin(),before.end(),bytes.begin());std::size_t assertions=0;
            FinalizeConstructionPacket(bytes,PacketNumber(row[1])!=0,[&] { ++assertions; });CHECK(assertions==(row[5]=="Assert" ? 1U : 0U));
        } else CHECK(false);
        const auto expected=PacketBytes(row[4]);CHECK(expected.size()==512);CHECK(std::equal(expected.begin(),expected.end(),bytes.begin()));
        CHECK(std::all_of(bytes.begin()+512,bytes.end(),[](auto byte) { return byte==0xcc; }));++count;
    }
    CHECK(count==86);
}
// 잘못된물리버퍼/과도한SID를효과전에거부하고선언길이를전송크기로오인하지않는다.
TEST_CASE(ConstructionPacket_ContractsBeforeWrite) {
    std::vector<std::uint8_t> bytes(511,0x11);ConstructionNotice notice;bool rejected=false;
    try { WriteConstructionPacket(notice,{},bytes); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
    CHECK(std::all_of(bytes.begin(),bytes.end(),[](auto byte) { return byte==0x11; }));
    bytes.resize(512,0x11);notice.sids.resize(20);rejected=false;
    try { WriteConstructionPacket(notice,{},bytes); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
    FinalizeConstructionPacket({},false);rejected=false;
    try { FinalizeConstructionPacket({},true); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
}
