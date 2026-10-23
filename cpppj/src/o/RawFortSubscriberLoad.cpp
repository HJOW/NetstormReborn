// 원문은파일ID와부분문자열을즉시쓰며후속진단/장치실패때rollback하지않는다.
#include "o/RawFortSubscriberLoad.h"
#include <bit>
#include <stdexcept>
#include <string>

namespace netstorm::o {
namespace {
// 선택한실제장치만필수로요구해이미완료한앞선효과를보존한다.
template<class Function> const Function& NeedSubscriber(const Function& function) { if (!function) throw std::logic_error("Fort Subscriber 실제 경계 미연결");return function; }
// 비정렬raw필드/원문BYTE를little endian으로읽으며논리end는검사하지않는다.
std::uint32_t SubscriberRead(std::span<const std::uint8_t> bytes,std::uint32_t offset,std::uint32_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("Fort Subscriber 직접읽기 물리범위");std::uint32_t value{}; //원문이번접근의최대DWORD값이다.
    for (std::uint32_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8);return value; //서명/길이/각복사단위를구별한다.
}
// 각원문DWORD/BYTE쓰기단위만범위를확인해앞선복사prefix를유지한다.
void SubscriberWrite(std::span<std::uint8_t> bytes,std::uint32_t offset,std::uint32_t width,std::uint32_t value) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("Fort Subscriber 직접쓰기 물리범위");
    for (std::uint32_t byte=0;byte<width;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); //원문이번단위의이웃BYTE를보존한다.
}
// 현재pos/end를먼저읽은뒤DWORDpos를쓰고signed진단한다.
void SubscriberAdvance(std::span<std::uint8_t> fields,std::uint32_t length,const FortSubscriberLoadHooks& hooks) {
    const auto next=SubscriberRead(fields,4,4)+length,end=SubscriberRead(fields,8,4);SubscriberWrite(fields,4,4,next); //진단콜백전에현재pos를공개한다.
    if (std::bit_cast<std::int32_t>(next)>std::bit_cast<std::int32_t>(end)) NeedSubscriber(hooks.assertion)(FortSubscriberLoadAssertion::PositionPastEnd);
}
//004bcb60은signedWORD를캡처하고진단뒤현재data/pos를고정해앞에서부터DWORD/잔여BYTE를복사한다.
void SubscriberString(std::span<std::uint8_t> fields,const MemStreamHooks& memory,std::span<std::uint8_t> handle,const FortSubscriberLoadHooks& hooks) {
    const auto initialData=SubscriberRead(fields,0,4),initialPosition=SubscriberRead(fields,4,4); //길이를읽을때의실제주소다.
    const auto word=static_cast<std::uint16_t>(SubscriberRead(NeedSubscriber(memory.buffer)(initialData),initialPosition,2)); //진단전의signed길이WORD다.
    SubscriberAdvance(fields,2,hooks);const auto length=std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))); //음수를양의16비트길이로고치지않는다.
    const auto data=SubscriberRead(fields,0,4),position=SubscriberRead(fields,4,4);const auto source=NeedSubscriber(memory.buffer)(data);std::uint32_t copied{}; //진단뒤의현재복사시작주소이며중간전체사본을만들지않는다.
    for (std::uint32_t chunk=0;chunk<(length>>2);++chunk,copied+=4U) { //원문REP MOVSD의각단위를읽고쓴뒤다음으로간다.
        const auto value=SubscriberRead(source,position+copied,4);SubscriberWrite(handle,copied,4,value);
    }
    for (std::uint32_t byte=0;byte<(length&3U);++byte,++copied) { //잔여BYTE도앞방향으로복사하고앞선DWORD쓰기를보존한다.
        const auto value=SubscriberRead(source,position+copied,1);SubscriberWrite(handle,copied,1,value);
    }
    SubscriberAdvance(fields,length,hooks);SubscriberWrite(handle,length,1,0); //복사뒤현재pos/end진단이끝나야저장길이위치에NUL을쓴다.
}
}
// 등록설정이이미같은-1이면ReadInt는false이므로실제새ID생성과오탈자설정식도필요하다.
void FinishFortSubscriberRegistration(const FortSubscriberLoadGlobals& globals,const FortSubscriberLoadHooks& hooks) {
    if (globals.subscriber!=0xffffffffU) return;
    if (NeedSubscriber(hooks.readRegistered)("registeredSubId",globals.subscriber)) return;
    globals.subscriber=NeedSubscriber(hooks.generate)();const auto expression=std::string("registerdSubId = \"")+std::to_string(globals.subscriber)+"\""; //CRT%u의32비트unsigned십진수와원문키를그대로유지한다.
    NeedSubscriber(hooks.configure)(expression);
}
//실제파일이선택한고정this와선택반환stream을유지하며등록helper는stream빌림반납뒤호출한다.
void LoadFortSubscriber(std::int32_t slot,std::span<std::uint8_t> handle,const FortSubscriberLoadGlobals& globals,const FortSubscriberLoadHooks& hooks) {
    if (!NeedSubscriber(hooks.manager)(slot)) NeedSubscriber(hooks.assertion)(FortSubscriberLoadAssertion::MissingManager);
    const auto manager=NeedSubscriber(hooks.manager)(slot),stream=NeedSubscriber(hooks.select)(manager,"Subscriber",0); //진단복귀뒤현재manager를재조회한다.
    NeedSubscriber(hooks.access)(stream,[&](auto fields,const auto& memory) {
        const auto data=SubscriberRead(fields,0,4),position=SubscriberRead(fields,4,4);globals.subscriber=SubscriberRead(NeedSubscriber(memory.buffer)(data),position,4); //ID를먼저직접써서뒤진단이현재ID를보게한다.
        SubscriberAdvance(fields,4,hooks);SubscriberString(fields,memory,handle,hooks);
    });
    FinishFortSubscriberRegistration(globals,hooks);
}
}
