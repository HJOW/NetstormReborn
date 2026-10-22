// 애시드 수신의 원본 필터/콜백 재조회/현재 가상 삭제와11바이트 헤더를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawAcidZap.h"
#include <iostream>

using namespace netstorm::test::rawscene;
namespace {
// 원본의 raw/패킷을 기대값 재계산 없이 읽는다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr char kDigits[]="0123456789abcdef"; //원문과 같은 소문자16진수다.
    std::string result;
    for (const auto byte:bytes) { result+=kDigits[byte>>4];result+=kDigits[byte&15]; } //모든 실제 바이트를 보존한다.
    return result;
}
// 입력의 signed/unsigned DWORD를 원본 하위 비트로 읽는다.
std::vector<std::uint32_t> Numbers(std::string_view text) {
    std::vector<std::uint32_t> result;
    for (const auto& word:Split(text,',')) result.push_back(static_cast<std::uint32_t>(std::stoll(word))); //음수 방향도 비트로 보존한다.
    return result;
}
// 원본 다섯 몸체의 고유 입력을 코드가 만드는 기대값과 섞지 않는다.
void Replay(bool headers) {
    SidPool pool(OriginalEdition::Patch1078,32768,false);
    for (std::uint16_t value=5;value<=63;++value) CHECK(pool.Allocate(2)==Sid{value}); //같은 여섯 raw 슬롯을 확보한다.
    const auto writable=[&](Sid sid) { const auto raw=pool.Slot(sid);return std::span<std::uint8_t>(const_cast<std::uint8_t*>(raw.data()),50); }; //free 상태도 포함한 독립 원문 입력을 같은 예약 슬롯에 다시 설치한다.
    std::size_t count{},index{};
    for (const auto& row:LoadFixture(NETSTORM_ACID_ZAP_FIXTURE).rows) { //1017행의 최초 원문이다.
        const auto line=index++;const auto c=Numbers(row[0]);if ((c[0]!=0)!=headers) continue;CHECK(row.size()==4 && c.size()==15);
        for (const auto sid:{50,51,60,61,62,63}) { //현재 대상 두 개와 폴 네 개를 같은 원본 입력으로 설치한다.
            auto raw=writable(Sid{static_cast<std::uint16_t>(sid)});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(sid<60 ? c[3] : 145);raw[11]=static_cast<std::uint8_t>(sid==50 ? c[1] : sid==60 ? c[2] : 0);raw[34]=static_cast<std::uint8_t>(sid==60 ? c[7] : 3);Put(raw,14,std::bit_cast<std::uint32_t>(20.75F));Put(raw,18,std::bit_cast<std::uint32_t>(21.5F));
        }
        std::vector<RiftTypeRecord> types(188);types[c[3]].flags2=c[4];AcidZapState state{c[5],c[6]};FormPacketHeader header{static_cast<std::uint8_t>(c[13]),static_cast<std::uint8_t>(c[12]),static_cast<std::uint16_t>(c[14])};
        std::array<std::uint8_t,11> packet{static_cast<std::uint8_t>(c[12]&0x7f),static_cast<std::uint8_t>(c[13])};Put(packet,2,c[14],2);Put(packet,4,50,2);Put(packet,6,60,2);Put(packet,8,62,2);packet[10]=static_cast<std::uint8_t>(c[8]);
        std::string effects;const auto event=[&](const char* kind,std::initializer_list<std::uint32_t> values) { if (!effects.empty()) effects+=';';effects+=kind;for (const auto value:values) effects+=':'+std::to_string(value); }; //원문 요청 인수를DWORD로 출력한다.
        const auto mutate=[&](bool sound) {
            if (sound) {
                if (c[11]==1) writable(Sid{60})[34]=7;
                if (c[11]==2) Put(packet,6,61,2);
                if (c[11]==3) Put(packet,4,51,2);
                if (c[11]==4) { Put(packet,8,63,2);packet[10]=255; }
                if (c[11]==5) Put(writable(Sid{50}),0,0x12200300);
            } else {
                if (c[11]==6) writable(Sid{60})[34]=9;
                if (c[11]==7) Put(writable(Sid{50}),0,0x12200300);
                if (c[11]==8) writable(Sid{50})[11]=3;
                if (c[11]==9) Put(packet,6,61,2);
                if (c[11]==10) { writable(Sid{60})[34]=0;Put(writable(Sid{50}),0,0x12200300); }
            }
        }; //콜백이 공급한 현재 raw/패킷 변경만 수행한다.
        AcidZapHooks hooks;
        hooks.sound=[&](float x,float y,std::string_view name,std::uint32_t loop) { event("S",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});effects+=':'+std::string(name)+':'+std::to_string(loop);mutate(true); };
        hooks.dissolve=[&](Sid victim,Sid first,Sid second,std::int32_t orientation) { event("F",{victim.value,first.value,second.value,std::bit_cast<std::uint32_t>(orientation)});mutate(false); };
        hooks.destroy=[&](Sid victim,std::uint32_t flags) { event(Get(pool.Slot(victim),0)==0x12200000 ? "D" : "d",{victim.value,flags}); };
        RawAcidZap zap(pool,types,state,header,std::move(hooks));
        if (headers) packet=zap.Build(Sid{50},Sid{60},Sid{62},std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(c[8])));else zap.Receive(c[9],c[10],packet);
        std::string raws;
        for (const auto sid:{50,51,60,61,62,63}) { if (!raws.empty()) raws+=',';raws+=Hex(pool.Slot(Sid{static_cast<std::uint16_t>(sid)})); } //실제 콜백 이후의 여섯 슬롯이다.
        if ((effects.empty() ? "-" : effects)!=row[1] || raws!=row[2] || Hex(packet)!=row[3]) { std::cout<<"애시드 원문 행"<<line<<"\n실제:"<<effects<<"\n원본:"<<row[1]<<'\n';CHECK(false); }
        ++count;
    }
    CHECK(count==(headers ? 90 : 927));
}
}
// free/dead·두 제외TYPE/사제·입력 무시·효과 뒤 현재 owner/가상 표를 비교한다.
TEST_CASE(AcidZap_NativeReceiveAndCurrentCallbacks) { Replay(false); }
// 현재 프로토콜 상위비트/메시지/보정하지 않는 선언길이와 모든 페이로드를 비교한다.
TEST_CASE(AcidZap_NativePacketHeader) { Replay(true); }
