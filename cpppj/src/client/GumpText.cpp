// 원본의 DWORD 폭 감김·signed 길이·슬롯 높이와 그림자 출력 순서를 보존한다.
#include "client/GumpText.h"
#include "client/StoneButtonGump.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본의 빈 캐시 표는 읽기 전용 폭 0이다. 지원하지 않는 조합에 GDI 글꼴을 새로 만들지 않는다.
constexpr std::array<std::int32_t,256> kEmptyWidths{};
// 원본 x86 덧셈처럼 하위 32비트만 반환한다. signed overflow를 발생시키지 않는다.
std::int32_t Add(std::int32_t a,std::int32_t b) {
    return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b));
}
}
// 문자열 범위 계약을 먼저 확인한 뒤 실제 원본과 같은 순서로 출력 두 필드에 쓴다.
void MeasureGumpText(std::span<const char> bytes,std::span<const std::int32_t,256> widths,
    std::int32_t fontHeight,const GumpTextContext& context,std::int32_t& width,std::int32_t& height,
    std::int32_t length,o::OriginalEdition edition) {
    std::size_t count=0;
    if (length==-1) {
        const auto end=std::find(bytes.begin(),bytes.end(),'\0');
        if (end==bytes.end()) throw std::invalid_argument("Gump 글자 범위에 NUL 종료 필요");
        count=static_cast<std::size_t>(end-bytes.begin());
    } else if (length>0) {
        count=static_cast<std::size_t>(length);
        if (count>bytes.size()) throw std::invalid_argument("Gump 글자 측정 길이 범위 초과");
    }
    std::int32_t total=0;
    // Windows-1252 바이트를 부호 없이 색인하며 NUL의 포함 여부는 위 길이 계약을 따른다.
    for (std::size_t i=0;i<count;++i) total=Add(total,widths[static_cast<unsigned char>(bytes[i])]);
    width=total;height=fontHeight;
    if (context.flags&2U) {
        if (edition==o::OriginalEdition::Patch1078) { width=Add(width,context.shadowX);height=Add(height,context.shadowY); }
        else { width=Add(total,context.shadowX);height=Add(fontHeight,context.shadowY); }
    }
}
// 물리 구조체 재해석 없이 원본 DWORD 배열의 의미를 옮긴다.
GumpTextContext DecodeGumpTextContext(const std::array<std::uint32_t,8>& words) {
    return {words[0],words[1],words[2],words[3],words[4],words[5],
        std::bit_cast<std::int32_t>(words[6]),std::bit_cast<std::int32_t>(words[7])};
}
// 원본 글꼴 생성/캐시의 소유권은 기존 FontStore에 남긴다.
GumpText::GumpText(FontStore& fonts,o::OriginalEdition edition):fonts_(fonts),edition_(edition) {}
// 스타일 fallback의 폭 슬롯과 높이 슬롯은 서로 다를 수 있다. 외곽선 플래그 4는 크기에 더하지 않는다.
void GumpText::Measure(std::span<const char> bytes,const GumpTextContext& context,std::int32_t& width,
    std::int32_t& height,std::int32_t length) const {
    const auto heightSlot=std::bit_cast<std::int32_t>(context.font);
    const auto logicalHeight=FontStore::LogicalHeight(heightSlot);
    const auto maxStyle=edition_==o::OriginalEdition::Patch1078 ? 4U : 2U;
    const auto slot=context.style<=maxStyle ? heightSlot : 0;
    const auto style=context.style<=maxStyle ? static_cast<FontStyle>(context.style) : FontStyle::Normal;
    std::array<std::int32_t,256> widths=kEmptyWidths;
    const bool cdEmpty=edition_!=o::OriginalEdition::Patch1078 && slot==6 && style==FontStyle::Bold;
    if (FontStore::Available(slot,style) && !cdEmpty) {
        const auto& font=fonts_.Get(slot,style);
        // 측정은 첫 폭 표만 쓰며 실제 그리기 전진 폭/ABC A와 캐시의 높이를 섞지 않는다.
        for (std::size_t i=0;i<widths.size();++i) widths[i]=font.Glyph(static_cast<std::uint8_t>(i)).advance;
    }
    MeasureGumpText(bytes,widths,logicalHeight,context,width,height,length,edition_);
}
// 원본 캐시 메트릭은 첫 폭 표 측정의 논리 높이와 다르다. 불법 스타일의 기본 슬롯도 독립 선택한다.
std::array<std::int32_t,3> GumpText::Metrics(const GumpTextContext& context) const {
    const auto maximum=edition_==o::OriginalEdition::Patch1078 ? 4U : 2U;
    const bool legal=context.style<=maximum;
    const auto slot=legal ? std::bit_cast<std::int32_t>(context.font) : 0;
    FontStore::LogicalHeight(slot);
    const auto style=legal ? static_cast<FontStyle>(context.style) : FontStyle::Normal;
    const bool cdEmpty=edition_!=o::OriginalEdition::Patch1078 && slot==6 && style==FontStyle::Bold;
    if (!FontStore::Available(slot,style) || cdEmpty) return {};
    const auto& font=fonts_.Get(slot,style);
    return {font.Height(),font.Ascent(),font.Descent()};
}
// 기본 문맥을 명시 입력으로 전달하므로 저장된 정상/눌림/비활성 Draw 문맥과 혼동하지 않는다.
void BindStoneButtonText(StoneButtonGumpHooks& hooks,const GumpText& text) {
    hooks.measure=[&text](std::uint32_t,const std::array<char,64>& label,const std::array<std::uint32_t,8>& context,
        std::int32_t& width,std::int32_t& height,std::uint32_t length) {
        text.Measure(label,DecodeGumpTextContext(context),width,height,std::bit_cast<std::int32_t>(length));
    };
}
}
