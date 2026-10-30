// 독립 PE의 점멸/Idle/표시 순서·전체 노드와 기본 dirty 영역을 함께 재생한다.
#include "GumpRawSupport.h"
#include "client/InventoryGump.h"
#include "client/GumpLifetime.h"
#include "client/GumpRedraw.h"
#include <iomanip>
#include <sstream>

namespace { // fixture의 원문 비트를 읽고 실제 공통 처리기에만 전달한다.
using namespace netstorm::test::gumpraw; // 전체 기본 노드의 원문 offset 직렬화를 빌린다.
constexpr std::uint32_t kRoot=0x15000000,kItem=kRoot+128; // 원문 관측의 루트/항목 토큰이며 호스트 주소가 아니다.
std::string TimeBits(double value) { // NaN을 포함한 원문 double 비트의16자리 표기다.
    std::ostringstream stream;stream<<std::hex<<std::setfill('0')<<std::setw(16)<<std::bit_cast<std::uint64_t>(value);return stream.str();
}
InventoryItemNode LoadItem(const std::vector<std::uint8_t>& raw) { // 원문0xc0 필드를 호스트 파생 노드로 읽으며 패딩을 추론하지 않는다.
    InventoryItemNode n;static_cast<GumpCreateNode&>(n)=Decode(raw);n.content=Get(raw,160);n.reservedA4=Get(raw,164);
    n.blinkUntil=std::bit_cast<double>(std::uint64_t(Get(raw,168))|(std::uint64_t(Get(raw,172))<<32));n.blink=raw[176];n.decoration=raw[177];n.reservedB2=static_cast<std::uint16_t>(Get(raw,178,2));
    n.requireImageHit=Get(raw,180);n.additional=Get(raw,184);n.iconScale=Float(Get(raw,188));return n;
}
std::string EncodeItem(const InventoryItemNode& n) { // 변경하지 않아야 할 파생 필드도 원문 전체192바이트에 포함한다.
    auto raw=Bytes(Encode(n));raw.resize(192);Put(raw,160,n.content);Put(raw,164,n.reservedA4);const auto bits=std::bit_cast<std::uint64_t>(n.blinkUntil);
    Put(raw,168,static_cast<std::uint32_t>(bits));Put(raw,172,static_cast<std::uint32_t>(bits>>32));raw[176]=n.blink;raw[177]=n.decoration;Put(raw,178,n.reservedB2,2);Put(raw,180,n.requireImageHit);Put(raw,184,n.additional);Put(raw,188,Bits(n.iconScale));return Hex(raw);
}
}

TEST_CASE(InventoryBlink_Original110StateOrderAndDirtyRegions) { // 실제 기본 가시성/Position/Idle표와 모든 시각·요청을 원문과 비교한다.
    const auto rows=LoadFixture(NETSTORM_INVENTORY_BLINK_FIXTURE).rows;std::size_t count{};
    for (const auto& row:rows) { // 입력마다 UI 노드와 수명/Pool을 초기화해 이전 점멸 단계가 섞이지 않게 한다.
        CHECK(row.size()==11);if (row.size()!=11) continue;auto rootBytes=Bytes(row[1]);rootBytes.resize(160);auto root=Decode(rootBytes);auto item=LoadItem(Bytes(row[2]));
        const auto base=[&](auto token)->GumpCreateNode* { return token==kRoot ? &root : token==kItem ? &item : nullptr; };GumpCreateState create;create.collection.root=kRoot;
        const GumpCreate factory(create,{base,{}});GumpLifetimeState lifetimeState;if (item.flags&0x80U) lifetimeState.idle[0]=kItem;GumpLifetime lifetime(create,lifetimeState,{base});
        GumpCollection collection(create.collection,{base});std::string trace,requests;std::size_t timeCalls{};const auto times=Split(row[3],',');
        const auto event=[&](std::string_view name,std::string value) { if (!trace.empty()) trace+=';';trace+=std::string(name)+','+std::to_string(item.flags)+','+value; };
        GumpRedraw redraw(collection,{base,[] { return ScreenRect{}; },[&](auto rect,auto flags) { if (!requests.empty()) requests+=';';requests+=std::to_string(static_cast<std::uint32_t>(rect.left))+','+std::to_string(static_cast<std::uint32_t>(rect.top))+','+std::to_string(static_cast<std::uint32_t>(rect.right))+','+std::to_string(static_cast<std::uint32_t>(rect.bottom))+','+std::to_string(flags); }});
        SidPool pool(OriginalEdition::Patch1078,24000,true);auto content=InputRaw(pool,Sid{1});std::fill(content.begin(),content.end(),std::uint8_t{0});content[21]=255;const std::vector<std::uint8_t> initial(content.begin(),content.end());
        InventoryGumpHooks hooks;hooks.item=[&](auto token) { return token==kItem ? &item : nullptr; };hooks.idle=[&](auto token,bool enabled) { event("idle",enabled ? "1" : "0");lifetime.SetIdle(token,enabled); };
        hooks.now=[&] { if (!timeCalls && row[10]!="-") item.blinkUntil=std::bit_cast<double>(static_cast<std::uint64_t>(std::stoull(row[10],nullptr,16)));const auto value=std::bit_cast<double>(static_cast<std::uint64_t>(std::stoull(times.at(timeCalls++),nullptr,16)));event("clock",TimeBits(value));return value; };
        hooks.redraw=[&](auto token,bool erase) { event("redraw",erase ? "1" : "0");redraw.Whole(token,erase ? 1U : 0U); };hooks.clearBlink=[&](auto sid) { CHECK(sid==1);InputRaw(pool,{static_cast<std::uint16_t>(sid)})[21]&=0xfd; }; // 원문 fixture는 할당 없는 입력 SID를 사용하므로 같은 입력 바이트만 쓴다.
        ContainedFinderState finder;ScreenPoint point{};std::uint32_t cursor{};InventoryGump handler(create,factory,pool,finder,{point,cursor},std::move(hooks));handler.IdleItem(kItem);
        if (trace.empty()) trace="-";if (requests.empty()) requests="-";const auto encoded=EncodeItem(item);const auto expectedIdle=Inputs(row[6]);bool sameIdle=expectedIdle.size()==lifetimeState.idle.size();
        for (std::size_t index=0;sameIdle && index<expectedIdle.size();++index) sameIdle=expectedIdle[index]==lifetimeState.idle[index]; // 끝500칸까지 원본 Idle표와 비교한다.
        const bool same=encoded==row[4] && trace==row[5] && sameIdle && content[21]==Number(row[7]) && requests==row[8] && timeCalls==Number(row[9]);CHECK(same);
        if (!same) { std::fprintf(stderr,"Blink %s node%d trace%d idle%d pool%d requests%d time%d\n%s != %s\n",row[0].c_str(),encoded==row[4],trace==row[5],sameIdle,content[21]==Number(row[7]),requests==row[8],timeCalls==Number(row[9]),trace.c_str(),row[5].c_str());break; }
        content[21]=initial[21];CHECK(std::equal(content.begin(),content.end(),initial.begin()));CHECK(Encode(root)==Hex(rootBytes));++count;
    }
    CHECK(count==110);
}
