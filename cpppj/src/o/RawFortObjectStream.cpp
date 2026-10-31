// 원문004bdc60/004be160/004be2c0/004be3c0/004be610의 읽기와 즉시 생성 순서를 보존한다.
#include "o/RawFortObjectStream.h"
#include "o/RawFortObjectLoad.h"
#include <bit>
#include <cstdio>
#include <stdexcept>

namespace netstorm::o { // 실제10.78 슬롯과 원본 공유 version/depth/scope를 사용한다.
namespace { // 현재 stream 필드와 직접 읽기의 폭을 관리한다.
template<class F> const F& NeedStream(const F& function) { if (!function) throw std::logic_error("Fort 원시 Objects 실제 장치 미연결");return function; } // 선택한 하위만 요구한다.
std::uint32_t StreamDword(std::span<const std::uint8_t> bytes,std::size_t at) { // 비정렬DWORD의 현재 네BYTE만 읽는다.
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Fort Objects stream DWORD 물리범위");std::uint32_t value{};
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; // 낮은 BYTE부터 합한다.
}
void StreamPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { // pos 또는 실제 슬롯의 한DWORD만 쓴다.
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Fort Objects DWORD 쓰기 물리범위");
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); // 이웃 필드를 보존한다.
}
std::uint32_t StreamByte(std::uint32_t token,const FortObjectStreamHooks& hooks) { // BYTE 직접 읽기는 논리 end 검사를 추가하지 않는다.
    std::uint32_t value{};NeedStream(hooks.access)(token,[&](auto fields,const auto& memory) {
        const auto pos=StreamDword(fields,4);const auto bytes=NeedStream(memory.buffer)(StreamDword(fields,0));if (pos>=bytes.size()) throw std::out_of_range("Fort Objects 직접 BYTE 물리범위");
        value=bytes[pos];StreamPut(fields,4,pos+1U);
    });return value;
}
std::uint32_t StreamWord(std::uint32_t token,const FortObjectStreamHooks& hooks,bool signedValue) { // WORD를 먼저 읽고 pos 증가 뒤 signed end 진단을 수행한다.
    std::uint32_t value{};NeedStream(hooks.access)(token,[&](auto fields,const auto& memory) {
        const auto pos=StreamDword(fields,4);const auto bytes=NeedStream(memory.buffer)(StreamDword(fields,0));if (pos>bytes.size() || bytes.size()-pos<2) throw std::out_of_range("Fort Objects 직접 WORD 물리범위");
        value=std::uint32_t(bytes[pos])|(std::uint32_t(bytes[pos+1])<<8);const auto end=StreamDword(fields,8);StreamPut(fields,4,pos+2U);
        if (std::bit_cast<std::int32_t>(pos+2U)>std::bit_cast<std::int32_t>(end)) NeedStream(hooks.assertion)(0x2e);
    });return signedValue ? std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value)))) : value;
}
std::int32_t StreamAdd(std::int32_t a,std::uint32_t b) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a)+b); } // 원본 signed 좌표의 DWORD 감김 덧셈이다.
std::uint32_t StreamType(std::uint32_t type,const FortObjectStreamHooks& hooks) { // 원문 BYTE 종류의 현재 변환표/검사 모드를 재조회한다.
    if (type>=0xf6U) return type;auto conversion=NeedStream(hooks.conversion)();if (!conversion) return type;
    const auto read=[&](auto pointer) { const auto bytes=NeedStream(hooks.conversionBuffer)(pointer);if (type>=bytes.size()) throw std::out_of_range("Fort Objects 변환표 BYTE 물리범위");return std::uint32_t(bytes[type]); }; // 진단 뒤 바뀐 현재표도 같은 원래 종류로 읽는다.
    if (NeedStream(hooks.checking)() && !read(conversion)) { NeedStream(hooks.assertion)(0x79);conversion=hooks.conversion(); }
    return read(conversion);
}
bool StreamRemaining(std::uint32_t token,const FortObjectStreamHooks& hooks) { // 버전 BYTE를 읽은 뒤 원문 data/end/pos 관문만 판단한다.
    bool present{};NeedStream(hooks.access)(token,[&](auto fields,const auto&) { present=StreamDword(fields,0) && StreamDword(fields,8) && std::bit_cast<std::int32_t>(StreamDword(fields,4))<std::bit_cast<std::int32_t>(StreamDword(fields,8)); });return present;
}
}
Sid RawFortObjectLoad::ObjectStream(std::uint32_t stream,std::int32_t originX,std::int32_t originY,std::uint32_t defaultOwner,const FortObjectStreamHooks& input) const { // 필드를 읽기 전에 객체를 만들고 각 실제 효과 뒤 현재 TYPE/전역을 읽는다.
    if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("Fort 원시 Objects는10.78 전용");
    const auto position=StreamByte(stream,input),stored=StreamByte(stream,input);auto type=stored,owner=defaultOwner;
    if (stored<0xf6U) type=StreamType(stored,input);else { owner=255U-stored;type=static_cast<std::uint8_t>(state_.bridgeType); }
    if (!type) { if (StreamByte(stream,input)) NeedStream(input.assertion)(0x2e8);return {}; }
    if (type==state_.islandType) { NeedStream(input.access)(stream,[](auto fields,const auto&) { StreamPut(fields,4,StreamDword(fields,4)+1U); });return {}; } // island의 추가BYTE는 읽지 않고 pos만 증가한다.
    const float x=static_cast<float>(StreamAdd(originX,position>>4)),y=static_cast<float>(StreamAdd(originY,position&15U));
    const auto sid=hooks_.create(type,0);Type(sid);
    if (Type(sid).flags1&0x20U) Frame(sid,StreamByte(stream,input));
    if (Type(sid).flags1&0x1000U) { const auto byte=StreamByte(stream,input);Word(sid,(std::uint32_t(pool_.Slot(sid)[13])<<8)|byte); }
    if (Type(sid).flags1&0x2000U) Word(sid,StreamWord(stream,input,true));
    if (Type(sid).flags2&4U) Frame(sid,StreamByte(stream,input));
    if (!state_.version) hooks_.setState(sid,StreamByte(stream,input)); // 원문의 legacy 판정004ad0a0은 OR 때문에 항상 참이다.
    if (std::bit_cast<std::int32_t>(state_.version)>1 && (Type(sid).flags2&0x4000U)) hooks_.setState(sid,StreamByte(stream,input));
    if (std::bit_cast<std::int32_t>(state_.version)>0) {
        if (Type(sid).flags2&state_.ownerSavedMask) { owner=StreamByte(stream,input);if (!owner || owner>8) owner=1;if (!state_.special && !state_.mission) owner=defaultOwner; }
    } else owner=defaultOwner;
    if (Type(sid).flags2&state_.ownerClearedMask) owner=0;const auto current=pool_.Slot(sid)[10];
    if (current==state_.islandType || current==state_.noIslandType) owner=0;
    if (current==state_.largeIslandType) NeedStream(input.assertion)(0x334);
    hooks_.setOwner(sid,owner);if (hooks_.hasHitPoints(sid)) StreamPut(pool_.AllocatedBytes(sid),26,static_cast<std::uint32_t>(hooks_.maxHitPoints(sid)));
    if (Type(sid).flags1&0x200000U) if (const auto ground=Terrain(x,y);ground.value) { const auto frame=StreamDword(pool_.Slot(ground),36);Frame(sid,frame==0xffffffffU ? 0U : frame); }
    if (Type(sid).flags1&0x100000U) { const auto frame=RandomFrame(sid);Frame(sid,frame==-1 ? 0U : static_cast<std::uint32_t>(frame)); }
    hooks_.pop(sid,x,y,9);if (Type(sid).flags1&0x20000U) NeedStream(input.contents)(sid,stream);
    if (state_.fort && !state_.FortDestroyBlocked()) {
        if (!(Type(sid).flags2&state_.FortKeepGenus()) || (!state_.special && !state_.mission && (Type(sid).flags2&0x400000U) && hooks_.virtualCount(sid)==1)) hooks_.destroy(sid,0);
    }
    return sid; // 원문은 void이며 삭제된 SID도 호스트 관측에만 반환한다.
}
void RawFortObjectLoad::ChunkStream(std::uint32_t stream,std::int32_t x,std::int32_t y,std::uint32_t owner,const FortObjectStreamHooks& input) const { // 표지/WORD 개수 뒤 각 객체를 읽은 즉시 적용한다.
    const auto originX=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(x)<<4),originY=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(y)<<4);
    if (StreamByte(stream,input)!=99U) NeedStream(input.assertion)(0x37a);const auto count=StreamWord(stream,input,false);
    for (std::uint32_t index=0;index<count;++index) static_cast<void>(ObjectStream(stream,originX,originY,owner,input)); // 뒤 레코드 실패 때 앞선 객체/난수를 유지한다.
}
void RawFortObjectLoad::SectionStream(std::uint32_t stream,std::uint32_t owner,std::int32_t territory,const FortObjectStreamHooks& input) const { // 원문 y/x와 각 현재 지도 크기/정책을 그대로 사용한다.
    state_.scope=territory<0 ? 2U : 1U;
    for (std::int32_t y=0;y<state_.chunksY;y=StreamAdd(y,1)) { // 매 행 끝에서 현재 높이를 다시 비교한다.
        for (std::int32_t x=0;x<state_.chunksX;x=StreamAdd(x,1)) { // 같은 현재 청크와 Terr 허용을 읽은 뒤에만 파일을 소비한다.
            const auto region=hooks_.chunkRegion(x,y);if (territory>=0 && !hooks_.territoryAllowed(region,owner,territory)) continue;ChunkStream(stream,x,y,owner,input);
        }
    }
}
void RawFortObjectLoad::LoadStream(std::int32_t slot,const FortObjectStreamHooks& input,std::int32_t start,std::int32_t end) const { // 깊이/변환표 준비→Terr→Chaff→정상표 반납→후처리이며 예외는 깊이만 반납한다.
    if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("Fort 원시 Objects 부모는10.78 전용");
    ++state_.loadingDepth;hooks_.loadingDepthChanged(state_.loadingDepth);
    try {
        if (!NeedStream(input.manager)(slot)) NeedStream(input.assertion)(0x3f5);if (start==end) NeedStream(input.assertion)(0x3f9);
        NeedStream(input.prepareTypes)(slot);const auto owner=slot>=1 && slot<=8 ? static_cast<std::uint32_t>(slot) : 0U;
        for (auto territory=start;territory<end;territory=StreamAdd(territory,1)) { // 원문과 같은 현재 manager를 각 선택 직전에 다시 읽는다.
            char name[32]{};std::snprintf(name,sizeof(name),"Terr%02d",territory);const auto stream=NeedStream(input.select)(input.manager(slot),name,1);state_.version=StreamByte(stream,input);
            if (StreamRemaining(stream,input)) SectionStream(stream,owner,territory,input);
        }
        if (state_.fort || state_.chaffMode) { const auto stream=NeedStream(input.select)(input.manager(slot),"Chaff",1);state_.version=StreamByte(stream,input);if (StreamRemaining(stream,input)) SectionStream(stream,owner,-1,input); }
        NeedStream(input.releaseConversion)();hooks_.finishOwner(owner);
    }catch (...) { --state_.loadingDepth;hooks_.loadingDepthChanged(state_.loadingDepth);throw; } // 원문004fb840→004bcc50과 같이 표와 이미 만든 월드는 보존한다.
    --state_.loadingDepth;hooks_.loadingDepthChanged(state_.loadingDepth);
}
}
