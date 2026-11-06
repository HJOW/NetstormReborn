// 공통 firstPop은 TYPE/상태/VT와 무관하게extra BYTE만 읽는다는 양판본 원문을 대조한다.
#include "TestSupport.h"
#include "o/SquidFactory.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <string>

namespace { // 명시 물리 입력만 작성하며 실제 생성/Pop/전투는 진행하지 않는다.
using namespace netstorm::o;
struct FirstPopProfile { // 읽지 않는 원문 필드를 독립적으로 오염시키는 여섯 입력이다.
    std::uint16_t sid; // 예약0/1/2와 일반5를 모두 실제 물리 범위 안에서 조회한다.
    std::uint8_t type,state; // 원문이 읽지 않는 TYPE와free/dead/void/contained 상태다.
    std::uint32_t table; // 실제 클래스 선택 뒤 공통 몸체를 직접 부르는 입력의 VT 기록값이다.
};
}

TEST_CASE(FactoryFirstPopCurrent_Native512AllBytesAndRawProfiles) { // 원문512반환값을각각6물리프로필에적용해선행TYPE/상태/VT검사없음을대조한다.
    std::ifstream input(NETSTORM_FIRST_POP_FLAGS_FIXTURE);CHECK(bool(input));std::string line;std::size_t rows{},calls{};
    while (std::getline(input,line)) { // 두 판본의 모든256BYTE를 빠짐없이 읽는다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        std::stringstream values(line);std::string editionText,extraText,resultText;std::getline(values,editionText,'\t');std::getline(values,extraText,'\t');std::getline(values,resultText);const bool patch=editionText=="originals";const auto edition=patch ? OriginalEdition::Patch1078:OriginalEdition::Cd1072;
        SidPool pool(edition,24000,true);std::vector<RiftTypeRecord> types(patch ? 188:171);SquidFactory factory(pool,types);const auto expected=static_cast<std::uint32_t>(std::stoul(resultText));
        const auto base=patch ? 0x501dd0U:0x5058d8U; // 원문공통base VT를쓰는정상입력도이전검사와함께보존한다.
        const std::array<FirstPopProfile,6> profiles{{{5,70,4,base},{0,255,15,0},{1,6,8,base},{2,70,1,base},{5,255,0,base},{5,70,4,0x12345678U}}}; // 임의타입/반납/예약/다른VT는extra조회앞에서거부하지않는다.
        for (const auto& profile:profiles) { // 같은예상값을사용하되실제풀전체의무쓰기까지확인한다.
            const auto read=pool.Slot(Sid{profile.sid});auto* raw=const_cast<std::uint8_t*>(read.data());std::fill(raw,raw+read.size(),std::uint8_t{0x65});raw[10]=profile.type;raw[11]=profile.state;raw[patch ? 40:35]=static_cast<std::uint8_t>(std::stoul(extraText));
            for (std::size_t i=0;i<4;++i) raw[i]=static_cast<std::uint8_t>(profile.table>>(i*8)); // VT는호스트포인터로역참조하지않는원문DWORD기록값이다.
            const auto before=std::vector<std::uint8_t>(pool.Bytes().begin(),pool.Bytes().end());std::uint32_t actual{};bool failed=false;try { actual=factory.FirstPopFlags(Sid{profile.sid}); }catch (const std::exception&) { failed=true; }
            CHECK(!failed && actual==expected);CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));++calls;
        }++rows;
    }CHECK(rows==512 && calls==3072);
}

TEST_CASE(FactoryFirstPopCurrent_PhysicalRangeStillRejectsWithoutMutation) { // 원문범위밖읽기를복제하지않고물리호스트경계를유지한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 판본별다른슬롯폭에도같은범위조건을사용한다.
        SidPool pool(edition,24000,true);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188:171);SquidFactory factory(pool,types);const auto before=std::vector<std::uint8_t>(pool.Bytes().begin(),pool.Bytes().end());bool failed=false;try { static_cast<void>(factory.FirstPopFlags(Sid{24000})); }catch (const std::out_of_range&) { failed=true; }CHECK(failed && std::equal(before.begin(),before.end(),pool.Bytes().begin()));
    }
}
