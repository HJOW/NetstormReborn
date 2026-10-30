// 원문 마나의 출력/현재 SHP 읽기/출력 후 raw·TYPE 변경 관측을 그대로 재생한다.
#include "ConstructionSupport.h"
#include "o/RawManaDraw.h"
#include <map>
#include <sstream>

namespace { // 기대 좌표/출력을 계산하지 않고 완성 PE fixture를 읽는다.
using namespace netstorm::test::construction; // 기존 숫자/fixture/바이트 도구다.
std::vector<std::uint8_t> Bytes(const std::string& text) { // 원본50BYTE raw의 메모리 순서다.
    std::vector<std::uint8_t> result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(index,2),nullptr,16))); // 두16진수 글자가 한BYTE다.
    return result;
}
template<class... Values> std::string Numbers(Values... values) { // 원문 관측과 같은 부호 없는 DWORD 인수열이다.
    std::ostringstream stream;bool first=true;const auto append=[&](auto value) { if (!first) stream<<',';first=false;stream<<static_cast<std::uint32_t>(value); };(append(values),...);return stream.str();
}
std::map<std::uint32_t,std::uint32_t> Shapes(const std::string& text) { // 명시 입력의 두 TYPE/SHP 매핑을 읽는다.
    std::map<std::uint32_t,std::uint32_t> result;
    for (const auto& field:Split(text,';')) { const auto values=Split(field,'=');result[Number(values[0])]=Number(values[1]); } // 실제 번호/토큰을 재계산하지 않는다.
    return result;
}
std::string Encoded(const std::map<std::uint32_t,std::uint32_t>& shapes) { // 실제 현재 SHP 표를 주소순으로 직렬화한다.
    std::string result;
    for (const auto& [number,shape]:shapes) { if (!result.empty()) result+=';';result+=std::to_string(number)+'='+std::to_string(shape); } // 기대 표와 같은 원문 순서다.
    return result;
}
}

TEST_CASE(ManaAdditional_Original756DrawsCurrentFramesAndShapeReads) { // WORD/좌표 감김/현재 raw·TYPE 변경을 독립 원문과 대조한다.
    const auto fixture=LoadFixture(NETSTORM_MANA_ADDITIONAL_FIXTURE).rows;std::size_t count{};
    for (const auto& row:fixture) { // 각 사례는 독립 raw/TYPE/출력 저장소로 실행한다.
        CHECK(row.size()==13);if (row.size()!=13) continue;auto raw=Bytes(row[1]);const auto selected=raw[10];auto shapes=Shapes(row[4]);std::string draws,reads;
        netstorm::o::ManaAdditionalHooks hooks;hooks.shape=[&](auto type) { const auto shape=shapes.at(type);if (!reads.empty()) reads+=';';reads+=Numbers(type,shape);return shape; };
        hooks.draw=[&](const auto& request) {
            if (!draws.empty()) draws+=';';draws+=Numbers(request.shadow,request.shape,request.frame,request.x,request.y);
            if (request.shadow && Number(row[5])) {
                Put(raw,36,Number(row[7]));raw[10]=static_cast<std::uint8_t>(Number(row[6]));
                if (Number(row[5])==2) { shapes[selected]=Number(row[8]);raw[12]=raw[13]=0; }
            }
        };
        netstorm::o::DrawManaAdditional(raw,std::bit_cast<std::int32_t>(Number(row[2])),std::bit_cast<std::int32_t>(Number(row[3])),hooks);
        const bool same=draws==row[9] && reads==row[10] && raw==Bytes(row[11]) && Encoded(shapes)==row[12];CHECK(same);
        if (!same) { std::fprintf(stderr,"Mana Additional %s: %s != %s; shape %s != %s\n",row[0].c_str(),draws.c_str(),row[9].c_str(),reads.c_str(),row[10].c_str());break; }++count;
    }
    CHECK(count==756);
}
