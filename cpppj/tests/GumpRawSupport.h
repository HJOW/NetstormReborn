// Gump 생성/트리 검사의 전체 40 DWORD raw 바이트 직렬화를 공유한다. 기대 상태는 계산하지 않는다.
#pragma once
#include "ConstructionSupport.h"
#include "client/GumpCreate.h"

namespace netstorm::test::gumpraw {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 원본 입력/출력의 16진수를 바이트로 바꾼다.
inline std::vector<std::uint8_t> Bytes(const std::string& hex) {
    std::vector<std::uint8_t> result;
    // 두 글자가 한 실제 raw 바이트다.
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
    return result;
}
// 원본 0xa0바이트를 의미별 호스트 필드로 읽는다. 호스트 메모리 배치는 원본 배치와 별개다.
inline GumpCreateNode Decode(std::span<const std::uint8_t> raw) {
    GumpCreateNode n;n.vtable=Get(raw,0);n.flags=Get(raw,4);n.alignFlags=Get(raw,8);n.next=Get(raw,12);n.previous=Get(raw,16);
    n.parent=Get(raw,20);n.child=Get(raw,24);n.id=Get(raw,28);n.command=Get(raw,32);n.object=Get(raw,36);n.form=Get(raw,40);n.serial=Get(raw,44);
    n.depth=Get(raw,48);n.secondaryDepth=Get(raw,52);n.scale=Float(Get(raw,56));n.baseWidth=Float(Get(raw,60));n.baseHeight=Float(Get(raw,64));
    n.width=Float(Get(raw,68));n.height=Float(Get(raw,72));n.x=Float(Get(raw,76));n.y=Float(Get(raw,80));
    // 위치/속도 여섯 필드는 원본 DWORD 비트로 유지한다.
    for (std::size_t i=0;i<n.positioning.size();++i) n.positioning[i]=Get(raw,84+i*4);
    n.cached={std::bit_cast<std::int32_t>(Get(raw,108)),std::bit_cast<std::int32_t>(Get(raw,112))};n.untouched=Get(raw,116);
    n.textPointer=Get(raw,120);n.alternateTextPointer=Get(raw,124);
    // 글자 문맥 여덟 필드도 생성 전 임의 입력을 그대로 공급한다.
    for (std::size_t i=0;i<n.textContext.size();++i) n.textContext[i]=Get(raw,128+i*4);
    return n;
}
// 현재 필드를 원본 offset으로만 직렬화한다. 기대 값/생성 분기 계산은 하지 않는다.
inline std::string Encode(const GumpCreateNode& n) {
    std::array<std::uint8_t,kGumpBaseSize> raw{};
    Put(raw,0,n.vtable);Put(raw,4,n.flags);Put(raw,8,n.alignFlags);Put(raw,12,n.next);Put(raw,16,n.previous);Put(raw,20,n.parent);
    Put(raw,24,n.child);Put(raw,28,n.id);Put(raw,32,n.command);Put(raw,36,n.object);Put(raw,40,n.form);Put(raw,44,n.serial);Put(raw,48,n.depth);Put(raw,52,n.secondaryDepth);
    Put(raw,56,Bits(n.scale));Put(raw,60,Bits(n.baseWidth));Put(raw,64,Bits(n.baseHeight));Put(raw,68,Bits(n.width));Put(raw,72,Bits(n.height));Put(raw,76,Bits(n.x));Put(raw,80,Bits(n.y));
    // 실제 현재 위치/속도 필드를 원래 여섯 위치로 쓴다.
    for (std::size_t i=0;i<n.positioning.size();++i) Put(raw,84+i*4,n.positioning[i]);
    Put(raw,108,std::bit_cast<std::uint32_t>(n.cached.x));Put(raw,112,std::bit_cast<std::uint32_t>(n.cached.y));Put(raw,116,n.untouched);Put(raw,120,n.textPointer);Put(raw,124,n.alternateTextPointer);
    // 실제 현재 문맥을 원본 바이트 순서로 쓴다.
    for (std::size_t i=0;i<n.textContext.size();++i) Put(raw,128+i*4,n.textContext[i]);
    return Hex(raw);
}
}
