// 배율/고정 추상색의 실제 PE 출력과 SHP 조회·조기 반환을 C++로 재생한다.
#include "ConstructionSupport.h"
#include "o/RawAuxiliaryDraw.h"
#include <sstream>

namespace { // 기대 배율/좌표는 fixture의 실제 관측에서만 읽는다.
using namespace netstorm::test::construction; // 기존 UTF-8 fixture/숫자 도구다.
std::vector<std::uint8_t> Bytes(const std::string& text) { // 완성50BYTE 객체를 메모리 순서로 읽는다.
    std::vector<std::uint8_t> result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(index,2),nullptr,16))); // 두16진수 글자가 한BYTE다.
    return result;
}
template<class... Values> std::string Numbers(Values... values) { // 원문 DWORD 인수와 같은 unsigned 표기다.
    std::ostringstream stream;bool first=true;const auto append=[&](auto value) { if (!first) stream<<',';first=false;stream<<static_cast<std::uint32_t>(value); };(append(values),...);return stream.str();
}
}

TEST_CASE(AuxiliaryDraw_Original1728ScaledAndAbstractRequests) { // hidden/분수/고정 배율과 고정 치환표 요청을 대조한다.
    const auto fixture=LoadFixture(NETSTORM_AUXILIARY_DRAW_FIXTURE).rows;std::size_t count{},scaled{},abstract{};
    for (const auto& row:fixture) { // 각 사례를 독립 raw/호출 관측 저장소로 실행한다.
        CHECK(row.size()==8);if (row.size()!=8) continue;auto raw=Bytes(row[2]);const auto original=raw;std::string draws,reads;
        netstorm::o::AuxiliaryDrawHooks hooks;hooks.shape=[&](auto type) { if (!reads.empty()) reads+=';';reads+=Numbers(type,Number(row[5]));return Number(row[5]); };
        hooks.draw=[&](const auto& value) { if (!draws.empty()) draws+=';';draws+=Numbers(value.abstract,value.shape,value.frame,value.x,value.y,value.scale,value.scale); };
        const auto x=std::bit_cast<std::int32_t>(Number(row[3])),y=std::bit_cast<std::int32_t>(Number(row[4]));
        if (row[1]=="Scaled") { netstorm::o::DrawScaledMetadisplay(raw,x,y,hooks);++scaled; }else { CHECK(row[1]=="Abstract");netstorm::o::DrawAbstractAdditional(raw,x,y,hooks);++abstract; }
        if (draws.empty()) draws="-";if (reads.empty()) reads="-";const bool same=draws==row[6] && reads==row[7] && raw==original;CHECK(same);
        if (!same) { std::fprintf(stderr,"Auxiliary Draw %s/%s: %s != %s; reads %s != %s\n",row[1].c_str(),row[0].c_str(),draws.c_str(),row[6].c_str(),reads.c_str(),row[7].c_str());break; }++count;
    }
    CHECK(count==1728 && scaled==864 && abstract==864);
    std::array<std::uint8_t,30> hidden{};hidden[27]=0x40;netstorm::o::DrawScaledMetadisplay(hidden,0,0,{}); // 원문 조기 반환은 프레임/SHP 장치 없이도 성립한다.
}
