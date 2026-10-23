// 실제CRT관측의정적타입/ASCII구성과count만지우는reset을같은원본장부에적용한다.
#include "o/RawFortBadgesGlobals.h"
#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace netstorm::o {
namespace {
// 원본CRT가각typed기록에배치하는고정타입과앞일곱행이다.
struct BadgeStaticRecord {
    std::uint32_t type; // 원본00541174등PE상수에서읽은typeDWORD다.
    std::array<std::string_view,7> rows; // 각15BYTE영역의일곱ASCII문자이며나머지는원문0이다.
};
constexpr std::array<BadgeStaticRecord,27> kBadgeInitialRecords{{ // 독립PE의BSS0→CRT결과와정적대입을서로대조한27개기록이다.
    {71U,{""," sss   ","   s   ","  ss   ","   s   ","       ","       "}},
    {72U,{"       ","       ","       ","   s   "," s   s "," ss ss ","   s   "}},
    {73U,{"       ","       ","       ","   s   "," s   s "," ss ss ","   s   "}},
    {78U,{"       ","       ","   s   ","  s s  "," ss ss ","       ","       "}},
    {128U,{"       ","       "," ssss  ","  s s  ","       ","  s    ","       "}},
    {111U,{"       ","  s s  ","  s s  ","   s   ","   s   ","   s   ","       "}},
    {136U,{"       "," s   s ","       ","   s   ","       "," s   s ","       "}},
    {133U,{"       ","     s ","    s  ","   s   ","  s    "," s     ","       "}},
    {140U,{"       "," w  w  ","w    w ","w ww w "," w  w  ","  ww   ","       "}},
    {139U,{"       "," wwww  ","  www  ","   ww  ","    w  "," wwww  ","       "}},
    {138U,{"       "," wwww  ","  www  ","   ww  ","    w  "," wwww  ","       "}},
    {75U,{"       ","   w   ","  w w  ","   w   ","   w   ","   ww  ","       "}},
    {142U,{"       "," wwwww ","   w   ","  w w  "," ww ww ","       ","       "}},
    {141U,{"       ","       ","wwwwww ","   w   "," ww ww ","  w w  ","       "}},
    {137U,{"       ","   w   ","   ww  "," w w w "," wwwww ","       ","       "}},
    {148U,{"       ","       "," t   t ","  t t  "," t t t ","       ","       "}},
    {77U,{"       ","  t    ","  t t  ","  t t  ","  t t  ","   t   ","       "}},
    {79U,{"       "," tt tt "," tt tt "," tt tt "," tt tt "," tt tt ","       "}},
    {131U,{"       ","   t   ","  ttt  ","   t   ","  ttt  ","  ttt  ","       "}},
    {135U,{"       ","    tt "," ttttt "," tttt  "," tttt  "," t  t  ","       "}},
    {143U,{"       ","  rrr  "," r   r ","   r   ","   r   ","       ","       "}},
    {144U,{"       ","  rrr  "," r   r ","   r   ","   r   ","       ","       "}},
    {76U,{"       ","  rr   ","   rr  ","   r   ","  rr   ","   r   ","       "}},
    {147U,{"       ","   r   ","  rrr  ","   r   ","  r r  "," rr rr ","       "}},
    {129U,{"       ","    rr "," rr    ","    rr "," rr    ","    rr ","       "}},
    {146U,{"       "," r r r ","       ","   r   ","       "," r   r ","       "}},
    {145U,{"       "," rr rr "," r   r "," rr rr ","  r r  ","   r   ","       "}},
}};
// 원본물리장부밖의접근은임의초기화로대신하지않고효과전거부한다.
void BadgePhysical(std::span<std::uint8_t> records) {
    if (records.size()!=kFortBadgesGlobalsBytes) throw std::invalid_argument("Badges 실제6160BYTE장부누락");
}
// 비정렬DWORD네바이트만little endian으로쓴다.
void BadgePut(std::span<std::uint8_t> records,std::size_t at,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) records[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 인접한원본자료는보존한다.
}
}
// CRT에는외부효과/분기가없으며지정영역최종구성과유일한미작성8BYTE를보존한다.
void InitializeFortBadgesRecords(std::span<std::uint8_t> records) {
    BadgePhysical(records);
    for (std::size_t index=0;index<kFortBadgesRecordCount;++index) { // 마지막28번째기록은원본55DWORD전체0구성을적용한다.
        const auto at=index*kFortBadgesRecordBytes;
        for (std::size_t byte=0;byte<kFortBadgesRecordBytes;++byte) { // 원본이쓰지않는첫기록+4..11만입력그대로둔다.
            if (index || byte<4 || byte>=12) records[at+byte]=0;
        }
        if (index==kBadgeInitialRecords.size()) continue;const auto& source=kBadgeInitialRecords[index];BadgePut(records,at,source.type);
        for (std::size_t row=0;row<source.rows.size();++row) { // NUL/뒤일곱행은앞선원본0구성으로남긴다.
            std::copy(source.rows[row].begin(),source.rows[row].end(),records.begin()+static_cast<std::ptrdiff_t>(at+4+row*15));
        }
    }
}
// 현재수량만지우며CRT자료/타입/첫레코드의미정바이트를다시구성하지않는다.
void ResetFortBadgeCounts(std::span<std::uint8_t> records) {
    BadgePhysical(records);
    for (std::size_t index=0;index<kFortBadgesRecordCount;++index) BadgePut(records,index*kFortBadgesRecordBytes+kFortBadgesCountOffset,0); // 원본28개의count주소만지운다.
}
// 같은Client의새단일배열에서최초BSS0과CRT를분리해재현한다.
RawFortBadgesGlobals::RawFortBadgesGlobals() { InitializeFortBadgesRecords(records_); }
// 로더/현재수량setter가복제없이빌리는같은배열이다.
std::span<std::uint8_t,kFortBadgesGlobalsBytes> RawFortBadgesGlobals::Records() { return records_; }
// getter/writer도같은주소의전체원본기록을읽는다.
std::span<const std::uint8_t,kFortBadgesGlobalsBytes> RawFortBadgesGlobals::Records() const { return records_; }
// 시작시점명시reset만호출하고소유자의CRT를반복실행하지않는다.
void RawFortBadgesGlobals::ResetCounts() { ResetFortBadgeCounts(records_); }
}
