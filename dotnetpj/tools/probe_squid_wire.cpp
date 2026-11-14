// 현재 C++ 모듈의 패킷·버퍼·누적·조회 순서를 창/게임/네트워크 없이 관측한다.
#include "o/RawSquidTransmission.h"
#include "o/SidPool.h"
#include <array>
#include <bit>
#include <cstdio>
#include <string>
#include <stdexcept>
#include <vector>

namespace { // 고정 입력을 원문 모듈에 전달하고 출력만 직렬화한다.
// 정확한 원문 폭의 명시 입력을 쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 낮은 바이트부터 지정한 폭만 기록한다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
// 관측된 바이트를 소문자16진수로 만들고 빈 출력은 '-'로 기록한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    std::string text;
    // 관측된 바이트의 상위/하위4비트를 그대로 쓴다.
    for (const auto value:bytes) { text+="0123456789abcdef"[value>>4];text+="0123456789abcdef"[value&15]; }
    return text.empty() ? "-":text;
}
}

// 256개 독립 입력의 실제 C++ writer 결과를 TSV로 출력한다. 원본 PE 관측과는 구별한다.
int main() {
    using namespace netstorm::o;
    // 각 입력은 같은 초기 풀만 사용하고 패킷 출력·누적은 새 상태로 시작한다.
    for (std::uint32_t index=0;index<256;++index) {
        SidPool pool(OriginalEdition::Patch1078,24000,false);const Sid sid{15000}; // 실제 물리 server 번호 입력이다.
        auto physical=pool.Slot(sid);auto raw=std::span<std::uint8_t>{const_cast<std::uint8_t*>(physical.data()),physical.size()};
        // 생성 효과를 대신하지 않고 writer가 읽을50바이트 입력만 준비한다.
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*11+index);
        const auto selector=index%17==0 ? -2:-1; // 선택자별 실제+54/+58을 구별한다.
        Put(raw,0,index%19==0 ? 0x5092d0:0x501dd0);raw[10]=74;raw[11]=static_cast<std::uint8_t>((index%8)|((index&32) ? 16:0));raw[34]=static_cast<std::uint8_t>(index%9);raw[40]=static_cast<std::uint8_t>(index*11);
        // 좌표·수량·프레임은 압축 경계의 명시 입력이며 결과 마스크를 계산하지 않는다.
        constexpr std::array<std::uint32_t,4> xs{0x41f00000,0x41f20000,0x437f0000,0x7fc01234},ys{0x42200000,0x42220000,0,0x7f800000};
        constexpr std::array<std::uint32_t,5> quantities{0,1,255,256,65535};
        constexpr std::array<std::uint32_t,4> frames{0,1,256,0x12345678};
        Put(raw,14,xs[index%4]);Put(raw,18,ys[index%4]);Put(raw,12,quantities[(index/4)%5],2);Put(raw,26,(index&32) ? 65535:200,2);Put(raw,36,frames[(index/16)%4]);
        raw[35]=static_cast<std::uint8_t>((index*7)^((index&4) ? 0x80:0));
        std::vector<RiftTypeRecord> types(188);types[74].flags1=(index&8) ? 16:0;types[74].zOrder=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(index*7));
        RawSquidTransmissionState state;
        // 처음 버퍼의 전체 stale 패턴을 넣어 반환 크기 뒤 보존을 함께 관찰한다.
        for (std::size_t i=0;i<state.buffer.size();++i) state.buffer[i]=static_cast<std::uint8_t>(i*17+index);
        state.compress=index%4==0 ? 0:(index%4==1 ? 1:-1);state.sameOwnerEnabled=index&2;state.lastOwner=(index&1) ? raw[34]:0xffffff9d;
        if (index%32==7) state.lastOwner=std::uint32_t(raw[34])+256;
        state.compressedCount=(index&128) ? 254:0;state.compressedBytes=(index&128) ? 8880:0;
        if (index%32==31) { state.compressedCount=0xffffffff;state.compressedBytes=0xfffffffa; }
        if (index%64==63) { state.compressedCount=0x800000fe;state.compressedBytes=0x80000000; }
        state.update={static_cast<std::uint8_t>(0x20+index%16),static_cast<std::uint8_t>(0x81+index%4),static_cast<std::uint16_t>((index&32) ? 65534:index%32)};
        state.compressed={static_cast<std::uint8_t>(0x70+index%16),static_cast<std::uint8_t>(0x81+index%4),(index&64) ? 0x10002U:index%4};
        state.deletion.sid=(index&1) ? sid.value:15001;state.deletion.token=0xa5a51234+index;
        std::size_t sent=0;std::uint32_t typeReads=0,defaultReads=0,hpReads=0;std::string logs;RawSquidTransmissionHooks hooks;
        hooks.type=[&](std::uint8_t number)->const RiftTypeRecord& { ++typeReads;return types.at(number); };
        hooks.defaultType=[&] { ++defaultReads;if (index&128) { auto replacement=types[74];replacement.flags1^=16;types[74]=replacement; }return ((index&16) ? 74U:82U)+((index&64) ? 256U:0U); };
        hooks.maxHp=[&](std::uint8_t) { ++hpReads;if (index&64) ++types[74].zOrder;return (index&64) ? -1:200; };
        hooks.broadcast=[&](auto packet) { sent=packet.size(); };hooks.trace=[&](auto level,auto text) { logs+=std::to_string(level)+":"+std::string(text); };
        hooks.assertion=[](auto,auto,auto) { throw std::runtime_error("프로브 원문 진단"); };
        RawSquidTransmission writer(pool,state,std::move(hooks));writer.Transmit(sid,-1,selector,0,0x12345678U^index);
        const auto head=Hex(std::span<const std::uint8_t>(state.buffer).first(64));
        // 전체 나머지가 입력과 같은지 기록해 헤더/데이터 밖 쓰기를 검출한다.
        bool tail=true;for (std::size_t i=64;i<state.buffer.size();++i) tail=tail && state.buffer[i]==static_cast<std::uint8_t>(i*17+index);
        const auto log=Hex({reinterpret_cast<const std::uint8_t*>(logs.data()),logs.size()});
        std::printf("%u\t%zu\t%u\t%u\t%u\t%u\t%s\t%d\t%s\t%u,%u,%u\n",index,sent,state.compressedCount,state.compressedBytes,state.lastOwner,state.deletion.sid,head.c_str(),tail ? 1:0,log.c_str(),typeReads,defaultReads,hpReads);
    }
}
