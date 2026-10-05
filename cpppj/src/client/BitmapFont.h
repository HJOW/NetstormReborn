// 원본 Screen.cpp 004a3240·004a3430·004a39f0: 비트맵 글꼴 캐시와 바이트 단위 글자 출력.
#pragma once
#include "client/Screen.h"
#include "client/VFXDraw.h"
#include "o/BaseFile.h"
#include <memory>
#include <string>
#include <string_view>

namespace netstorm::client {
// 원본 문맥의 스타일 번호(004a3ed0). 글꼴 슬롯 2는 원본에서도 비어 있다.
enum class FontStyle { Normal, Italic, Bold, Strikeout, Underline };
struct FontGlyph {
    int advance{}, bearing{}, drawAdvance{};
    std::unique_ptr<ShapeDatabase> shape;
};
class BitmapFont {
public:
    // 원본 .chfnt의 네 정수 표와 글자별 독립 VFX 블록을 읽는다. 손상된 캐시는 거부한다.
    explicit BitmapFont(std::span<const std::uint8_t> bytes);
    // 원본 GDI 생성 경로. 캐시가 없는 PC에서는 메모리에서만 만들며 기존 원본 파일을 쓰지 않는다.
    static std::unique_ptr<BitmapFont> Generate(std::string_view face, int height, int weight, FontStyle style);
    // 004a3df0: Windows-1252 바이트의 전진 폭 합계. UTF-8은 호출자가 먼저 변환한다.
    int Measure(std::string_view bytes) const;
    // 원본 글리프와 그리기 전진 폭을 제공한다.
    const FontGlyph& Glyph(std::uint8_t code) const;
    // 캐시가 저장한 전체 글자 높이.
    int Height() const;
    // 기준선 위쪽의 높이.
    int Ascent() const;
    // 기준선 아래쪽의 높이.
    int Descent() const;
private:
    int height_{}, ascent_{}, descent_{};
    std::array<FontGlyph, 256> glyphs_{};
};
class FontStore {
public:
    // 원본 기본 글꼴 슬롯을 초기화한다. 파일 시스템은 이 객체보다 오래 살아야 한다.
    FontStore(const o::BaseFileSystem& files, std::string face);
    // 슬롯과 스타일의 캐시를 읽고, 없거나 손상됐으면 같은 GDI 글꼴로 만든다.
    const BitmapFont& Get(int slot = 0, FontStyle style = FontStyle::Normal);
private:
    const o::BaseFileSystem& files_;
    std::string face_;
    std::array<std::array<std::unique_ptr<BitmapFont>, 5>, 7> fonts_{};
};
}
