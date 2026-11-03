// 원본 Screen.cpp 004a3240·004a3430·004a39f0: 비트맵 글꼴 캐시와 바이트 단위 글자 출력.
#pragma once
#include "client/Screen.h"
#include "client/VFXDraw.h"
#include "o/BaseFile.h"
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <optional>
#include <vector>

namespace netstorm::client {
// 원본 문맥의 스타일 번호(004a3ed0). 글꼴 슬롯 2는 원본에서도 비어 있다.
enum class FontStyle { Normal, Italic, Bold, Strikeout, Underline };
// 한 글자의 이동 폭·기준점·그림을 묶은 글리프이다.
struct FontGlyph {
    // advance: 글자를 그린 뒤 다음 글자 기준점으로 이동할 폭.
    // bearing: 글자 기준점에서 실제 그림까지의 오프셋.
    // drawAdvance: 글자 출력에 사용하는 보정 이동 폭.
    int advance{}, bearing{}, drawAdvance{};
    // shape: 프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료.
    std::unique_ptr<ShapeDatabase> shape;
};
// 원본 글리프와 GDI 글꼴을 같은 폭 계산·출력 인터페이스로 제공하는 글꼴이다.
class BitmapFont {
public:
    // 원본 .chfnt의 네 정수 표와 글자별 독립 VFX 블록을 읽는다. 손상된 캐시는 거부한다.
    explicit BitmapFont(std::span<const std::uint8_t> bytes);
    // 원본 GDI 생성 경로. FontStore가 생성 뒤 같은 바이트를 캐시하며 개발 원본/쓰기불가 설치는 메모리 글꼴을 유지한다.
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
    std::vector<std::uint8_t> CacheBytes() const; // 현재 표와 같은 VFX 블록을 원문 CHFNT 순서로 직렬화한다. 그림을 재생성하지 않는다.
private:
    // height_: 세로 크기.
    // ascent_: 글꼴 기준선 위쪽의 픽셀 높이.
    // descent_: 글꼴 기준선 아래쪽의 픽셀 높이.
    int height_{}, ascent_{}, descent_{};
    // glyphs_: 문자 코드로 찾아 사용할 글리프 표.
    std::array<FontGlyph, 256> glyphs_{};
};
// 원문 EnumFontFamiliesA가 반환한 이름/종류/charset을 현재 순서로 전달하는 동기 경계다.
using FontFaceVisitor=std::function<void(std::string_view,std::uint32_t,std::uint8_t)>;
using FontFaceEnumeration=std::function<void(const FontFaceVisitor&)>; //실제 OS 열거 또는 창 없는 집중 검사의 명시 입력이다.
// 요청한 이름·크기의 글꼴을 읽어 캐시하는 저장소이다.
class FontStore {
public:
    // 원본 기본 글꼴 슬롯을 초기화한다. 파일 시스템은 이 객체보다 오래 살아야 한다.
    FontStore(const o::BaseFileSystem& files,std::string face,FontFaceEnumeration enumerate={});
    std::optional<std::string_view> FaceAt(std::int32_t index) const; //4a2df0의 현재 목록 인덱스다. 음수는 원문 assert 영역의 호스트 오류, 양수 범위 밖은 NULL이다.
    void Rebuild(std::optional<std::string_view> face); //같은 참조 아래 전체 캐시를 비운 뒤 재생성하며 NULL/유효 빈 이름을 구별한다.
    std::string_view CurrentFace() const; //4a2e30의 현재32BYTE 중 첫NUL까지이며 생성 완료 뒤에만 바뀐다.
    // 슬롯과 스타일의 캐시를 읽고, 없거나 손상됐으면 같은 GDI 글꼴로 만든다.
    const BitmapFont& Get(int slot = 0, FontStyle style = FontStyle::Normal);
    // 004a39f0가 별도 전역에 지정한 슬롯 높이다. 캐시 tmHeight와 구분하며 빈 슬롯 2는 0이다.
    static int LogicalHeight(int slot);
    // 10.78 초기화가 실제로 만드는 슬롯/스타일 조합인지 확인한다. 빈 조합은 Gump 측정에서 폭 0이다.
    static bool Available(int slot,FontStyle style);
private:
    // files_: 디스크·보조 디렉터리·아카이브 순서로 게임 파일을 찾는 파일 시스템 (o::BaseFileSystem).
    const o::BaseFileSystem& files_;
    // face_: GDI 글꼴 생성에 요청할 글꼴 이름.
    std::string face_;
    std::vector<std::array<char,32>> faces_; //원문 두 번 열거의 최초 순서/최대31BYTE 이름이며 Session/메뉴 교체로 지우지 않는다.
    std::array<char,32> currentFace_{}; //005accd8의 현재 요청 이름이다. GDI가 대신 고른 물리 글꼴 이름과 구별한다.
    // fonts_: 이름·크기로 찾아 재사용하는 글꼴 저장소.
    std::array<std::array<std::unique_ptr<BitmapFont>, 5>, 7> fonts_{};
};
}
