// 패치004afe50의 진단 복귀/현재 TYPE와 저장 genus·지도 byte를 구별한다.
#include "o/RawSquidUnpopParent.h"
#include "o/Squid.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
namespace netstorm::o { // CD 기존 몸체는 별도 caller에 남긴다.
namespace { // 원문 byte 접근과 선택 장치 검사를 내부에서 공유한다.
constexpr std::uint32_t kBuildingMask=0x50444200; // 직접 대입이 없는 PE005424b8의 초기 건물 부류다.
constexpr std::string_view kSource="\\Ns\\O\\Squid.cpp"; // 실제 공통 해제의 원문 진단 파일이다.
template<class Function> const Function& NeedUnpop(const Function& function) { if (!function) throw std::logic_error("공통 Unpop의 선택된 실제 장치 미연결");return function; } // 앞선 부분 변경은 되돌리지 않는다.
std::span<std::uint8_t> UnpopRaw(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // free 진단 복귀도 같은 물리 슬롯에서 진행한다.
std::uint16_t UnpopWord(std::span<const std::uint8_t> raw,std::size_t offset) { return static_cast<std::uint16_t>(raw[offset]|(std::uint16_t(raw[offset+1])<<8)); } // 원문 next WORD를 읽는다.
void UnpopPutWord(std::span<std::uint8_t> raw,std::size_t offset,std::uint16_t value) { raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); } // 이웃 payload를 보존한다.
float UnpopFloat(std::span<const std::uint8_t> raw,std::size_t offset) { std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);return std::bit_cast<float>(value); } // 비정렬 위치의 정확 DWORD를 읽는다.
int UnpopCell(float value) { if (!std::isfinite(value) || value<0 || value>=256) throw std::out_of_range("Unpop 지도 좌표");return static_cast<int>(value); } // 원문의 양수 지도 절삭을 소비 시점에 검증한다.
void UnpopAssert(const SquidUnpopParentHooks& hooks,std::string_view expression,std::uint32_t line) { NeedUnpop(hooks.assertion)(expression,kSource,line); } // 돌아온 뒤 원문 순서로 계속한다.
}
void RunSquidUnpopParent(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,Sid sid,std::uint32_t flags,RawCheckingFlag checking,const SquidUnpopParentHooks& hooks) {
    auto raw=UnpopRaw(pool,sid);if (raw[10]<70) UnpopAssert(hooks,"!isForm()",0x5d8);
    if (raw[11]&8U) UnpopAssert(hooks,"!isContained()",0x5d9);
    if (!sid.value || sid.value>=pool.Capacity()) UnpopAssert(hooks,"isValid()",0x5e0);
    if (raw[11]&5U) return;
    raw[11]|=4;const auto& initial=NeedUnpop(hooks.type)(raw[10]);const auto genus=initial.flags2;
    if ((genus&6U) && !(raw[40]&8U)) NeedUnpop(hooks.miniMap)(sid,genus);
    if (((genus&(kBuildingMask|0x400ffU)) || (NeedUnpop(hooks.type)(raw[10]).flags1&0x800U)) && !(raw[40]&8U) && !(raw[11]&8U)) {
        const int right=UnpopCell(UnpopFloat(raw,14)),bottom=UnpopCell(UnpopFloat(raw,18));
        const auto left=static_cast<std::int64_t>(right)-initial.footX+1,top=static_cast<std::int64_t>(bottom)-initial.footY+1;
        if (left<0 || top<0 || initial.footX<0 || initial.footY<0 || initial.footX>256 || initial.footY>256) throw std::out_of_range("Unpop 물리 발자국");
        for (int y=static_cast<int>(top);y<=bottom;++y) { // 원문 y/x 순서이며 저장한 범위를 진단 뒤 다시 만들지 않는다.
            for (int x=static_cast<int>(left);x<=right;++x) { // 각 현재 TYPE/위치를 새로 조회한다.
                const auto& current=NeedUnpop(hooks.type)(raw[10]);const auto effective=static_cast<std::uint8_t>(Squid::EffectiveGenus(current.flags2,UnpopFloat(raw,14),UnpopFloat(raw,18),current.footX,current.footY,x,y));
                if (effective) { const auto saved=spots[y*256+x];if (checking && (saved&effective)!=effective) UnpopAssert(hooks,"(flags & effectiveGenus) == ( unsigned char)effectiveGenus",0x617);spots[y*256+x]=static_cast<std::uint8_t>(saved&~effective); }
                if ((genus&kBuildingMask) && (x==left || x==right) && y!=top && y!=bottom) {
                    const auto surface=Sid{hash.Cell(0,x,y)};if (surface.value && surface.value<pool.Capacity()) NeedUnpop(hooks.side)(x,y);
                }
                if ((NeedUnpop(hooks.type)(raw[10]).flags1&0x800U) && checking && hash.Cell(0,x,y)!=sid.value) UnpopAssert(hooks,"getGridSid( (float)fx, (float)fy) == getSid()",0x629);
            }
        }
    }
    if (raw[11]&8U) { NeedUnpop(hooks.contained)(sid);return; }
    bool found{};
    for (int level=0;level<4 && !found;++level) { // 현재 좌표로 각 단계의 실제 버킷을 고른다.
        auto& head=hash.Bucket(level,UnpopFloat(raw,14),UnpopFloat(raw,18));Sid previous{},current{head};std::unordered_set<std::uint16_t> seen;
        while (current.value) { // 물리 범위/무한 순환만 소비 시점에서 거부하며 앞선 쓰기는 남긴다.
            if (!seen.insert(current.value).second) throw std::logic_error("Unpop 순환 체인");const auto entry=UnpopRaw(pool,current);const auto next=UnpopWord(entry,4);
            if (current==sid) { if (!previous.value) head=next;else UnpopPutWord(UnpopRaw(pool,previous),4,next);found=true;break; }previous=current;current={next};
        }
    }
    if (!found) UnpopAssert(hooks,"found",0x65d);NeedUnpop(hooks.update)(sid,flags);
}
}
