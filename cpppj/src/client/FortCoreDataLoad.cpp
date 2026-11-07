// 같은 현재 stream과 실제 Client Core/중첩 깊이로 원문 읽기/부분효과를 공유한다.
#include "client/FortCoreDataLoad.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client { //Session/메타 모두 같은 물리 저장소를 빌리는 읽기 계층이다.
namespace { //정렬 가정 없이 실제 stream DWORD를 읽고 쓴다.
// 선택된 장치가 없으면 해당 원문 위치에서만 중단한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("CoreData 읽기의 실제 장치 미연결");return function; }
// Native stream의 한 DWORD를 실제 물리 범위 안에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("CoreData 읽기 stream DWORD 범위");std::uint32_t value{};
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte); //요청한 네BYTE만 낮은 자리부터 합친다.
    return value;
}
// 현재 stream cursor DWORD만 덮어쓰며 인접 end/capacity를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("CoreData 읽기 cursor DWORD 범위");
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); //같은 실제 cursor 네BYTE만 갱신한다.
}
}
// 자연 관문은 depth/stream/Core getter보다 먼저며 예외에서도 중첩 깊이만 반납한다.
void LoadFortCoreData(CampaignTemplateStorage& storage,std::uint32_t slot,bool minimum,FortCoreDataLoadContext c) {
        if (c.missionPost || c.editor) return;
        ++c.loadingDepth;if (c.depthChanged) c.depthChanged(c.loadingDepth);
        try {
            if (slot>=9) throw std::out_of_range("Fort CoreData의 실제9슬롯 범위");
            if (!Read(storage.Headers(),slot*20)) Need(c.assertion)("tm","\\Ns\\O\\Template.cpp",0x47f);
            const auto stream=storage.SelectSection(slot,"CoreData",1); // 같은 노드의 writable 선택과 pos0을 원문 시점에 수행한다.
            storage.AccessSection(stream,[&c,slot,minimum](auto fields,const auto& memory) {
                if (!Read(fields,0) || !Read(fields,8)) return;
                if ((slot>=1 && slot<=8 ? slot : 0U)!=c.player.battleOwner) Need(c.assertion)("getPlayerId() == myPlayerId()","\\Ns\\O\\Template.cpp",0x484);
                c.core.Active()=1;
                const auto currentCore=[&c] { const std::uint32_t metamap=c.metamap ? 1U : 0U;return c.core.Read(c.player,metamap,{[&c](auto line) { Need(c.assertion)("_myCoreData == &lastKnownCoreData","\\Ns\\O\\Player.cpp",line); }}); }; // 읽기·무결성 실패 위치마다 같은 현재 포인터를 다시 조회한다.
                auto target=currentCore();const auto source=memory.buffer(Read(fields,0));const auto position=Read(fields,4);
                if (target.size()<10 || position>source.size() || 10>source.size()-position) throw std::out_of_range("Fort CoreData10BYTE의 실제 물리 범위");
                std::copy_n(source.begin()+position,10,target.begin());Put(fields,4,Read(fields,4)+10U);
                if (std::bit_cast<std::int32_t>(Read(fields,4))>std::bit_cast<std::int32_t>(Read(fields,8))) Need(c.assertion)("pos<=end","c:\\ns\\o\\memstream.h",0x2e);
                if (minimum) {
                    const auto readValue=[](std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width) { // 현재 Core의 unsigned BYTE/WORD만 읽으며 호스트 물리 범위 밖 입력은 거부한다.
                        if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("CoreData 최소 병합의 현재 필드 범위");return std::uint32_t(bytes[at])|(width==2 ? std::uint32_t(bytes[at+1])<<8 : 0U);
                    };
                    const auto mergeMinimum=[&](std::size_t at,std::size_t width) { // 원문 비교/선택/저장마다 현재 포인터를 재조회하고 unsigned BYTE/WORD만 대조한다.
                        const auto cached=readValue(c.core.Cached(),at,width);const auto current=readValue(currentCore(),at,width);
                        const auto selected=current<cached ? readValue(currentCore(),at,width) : readValue(c.core.Cached(),at,width);auto destination=currentCore();
                        if (at>destination.size() || width>destination.size()-at) throw std::out_of_range("CoreData 최소 병합의 현재 쓰기 범위");destination[at]=static_cast<std::uint8_t>(selected);if (width==2) destination[at+1]=static_cast<std::uint8_t>(selected>>8);
                    };
                    mergeMinimum(4,1);mergeMinimum(0,2);mergeMinimum(2,2); // 원문 랭크 BYTE→승리 WORD→패배 WORD 순서를 보존한다.
                }
                Need(c.refreshRank)();
                // 원문 인라인 BYTE/WORD/DWORD 읽기는 저장 후 WORD/DWORD만 논리 end를 진단한다.
                const auto readWord=[&](std::uint32_t width) {
                    const auto bytes=memory.buffer(Read(fields,0));const auto at=Read(fields,4);
                    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Fort CoreData 인라인 읽기 물리 범위");
                    std::uint32_t value{};for (std::uint32_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8); // 선택한 실제 폭만 합성한다.
                    Put(fields,4,Read(fields,4)+width);
                    if (width>1 && std::bit_cast<std::int32_t>(Read(fields,4))>std::bit_cast<std::int32_t>(Read(fields,8))) Need(c.assertion)("pos<=end","c:\\ns\\o\\memstream.h",0x2e);return value;
                };
                if (std::bit_cast<std::int32_t>(Read(fields,4))<std::bit_cast<std::int32_t>(Read(fields,8))) {
                    const auto word=readWord(2);
                    auto& seedValue=Need(c.seed)();seedValue=(seedValue&0xffff0000U)|word; // 원문의 lowWORD 대입은 상위WORD를 보존한다.
                }
                for (std::uint32_t group=0;group<3;++group) { // 남아 있는 세 무결성 블록을 랭크 BYTE·승리 WORD·패배 WORD 순서로 확인한다.
                    if (std::bit_cast<std::int32_t>(Read(fields,4))>=std::bit_cast<std::int32_t>(Read(fields,8))) break;
                    const auto data=currentCore();if (data.size()<5) throw std::out_of_range("Fort CoreData 무결성 현재 포인터 폭");
                    const auto expected=group==0 ? std::uint32_t(data[4]) : std::uint32_t(data[(group-1)*2])|(std::uint32_t(data[(group-1)*2+1])<<8);
                    for (std::uint32_t bit=0;bit<32;++bit) { // 원문004bea70은 앞8 BYTE·다음8 WORD·나머지16 DWORD의 해당 비트만 대조한다.
                        const auto encoded=readWord(bit<8 ? 1U : bit<16 ? 2U : 4U);const auto mask=1U<<bit;
                        if ((encoded&mask)!=(expected&mask)) { auto current=currentCore();if (current.size()<5) throw std::out_of_range("Fort CoreData 불일치 현재 포인터 폭");current[4]=1;Need(c.invalidData)(); } // 로컬 저장 자료의 원문 효과이며 외부 서버 검증을 실행하지 않는다.
                    }
                }
            });
        }catch (...) { --c.loadingDepth;if (c.depthChanged) c.depthChanged(c.loadingDepth);throw; }
        --c.loadingDepth;if (c.depthChanged) c.depthChanged(c.loadingDepth);

}
}
