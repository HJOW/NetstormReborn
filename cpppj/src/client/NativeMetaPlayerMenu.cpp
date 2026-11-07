// 같은 참가자 물리 필드와 실제 메뉴 호출 순서/부분효과를 유지한다.
#include "client/NativeMetaPlayerMenu.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <stdexcept>

namespace netstorm::client { //예외전에이미작성한controlled메뉴를임의취소하지않는다.
namespace { //원문비정렬필드/이름/256BYTE서식의실제폭을구별한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("메타 참가자 메뉴의 실제 선택 장치 미연결");return hook; } //미선택장치는빈성공으로요구하지않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) { if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("메타 참가자 메뉴의 실제 필드 부족");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[offset+byte])<<(8*byte);return value; } //같은원문폭의낮은BYTE부터조립한다.
std::string Name(std::span<const std::uint8_t> record) { const auto bytes=record.subspan(0x26);const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("메타 참가자 메뉴 이름의 실제 NUL 없음");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())}; } //임의길이로이름을절단하지않는다.
}
// 자기메뉴는스택에남긴뒤객체부착하며다른메뉴는Open이꺼낸토큰을직접부착한다.
void OpenMetaParticipantMenu(std::span<const std::uint8_t> record,std::int32_t x,std::int32_t y,const NativeMetaPlayerMenuHooks& h) {
    if (Read(record,4)==Need(h.subscriber)()) {
        Need(h.beginFancy)();if (Read(record,4)) { const auto pattern=Need(h.translate)("Player %s\n"),name=Name(record);std::array<char,256> text;const auto size=std::snprintf(text.data(),text.size(),pattern.c_str(),name.c_str());if (size<0 || static_cast<std::size_t>(size)>=text.size()) throw std::out_of_range("메타 참가자 메뉴의 실제256BYTE 제목 범위");Need(h.title)({text.data(),static_cast<std::size_t>(size)}); }
        if (Read(record,0xa4)) { const auto text=Need(h.translate)("Leave this Battle");const auto command=Need(h.command)(0x542714);Need(h.item)(text,command,0); }
        const auto text=Need(h.translate)("View your Player Info");const auto subscriber=Read(record,4),command=Need(h.command)(0x542960);Need(h.item)(text,command,subscriber);Need(h.attachCurrentObject)(Read(record,0xb8),x,y);return;
    }
    const auto menu=Need(h.playerMenu)(Read(record,4));const auto parent=Need(h.root)();Need(h.attachObject)(menu,Read(record,0xb8),static_cast<float>(x),static_cast<float>(y),parent,0);
}
// WORD분모0/비정상비율은100으로돌리며clamp/반올림을추가하지않는다.
std::uint32_t MetaPlayerReliability(const bool& metamap,std::uint32_t participant,const std::function<std::span<const std::uint8_t>(std::uint32_t)>& record) {
    if (!metamap) return 100;const auto bytes=Need(record)(participant);const auto games=Read(bytes,0x54,2);if (!games) return 100;const auto reliability=Read(bytes,0x56,2)*100U/games;return reliability<101U ? reliability:100U;
}
}
