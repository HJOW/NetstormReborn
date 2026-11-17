// 외부 TTF의 단색 EBDT/EBLC 글리프를 읽는다. GDI가 등록하지 못하는 비트맵 전용 글꼴도 픽셀 원형을 유지한다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace netstorm::client { // 게임의 8비트 글리프 생성에 사용하는 OpenType 비트맵 판독기다.
// 기준선 기준 위치와 단색 픽셀을 보관한다. 픽셀은 행 우선이며 0은 투명, 1은 글자다.
struct EmbeddedBitmapGlyph {
    int width{}, height{}; // 원래 비트맵의 가로·세로 픽셀 수다.
    int bearingX{}, bearingY{}; // 펜 기준 왼쪽 거리와 기준선 위쪽 거리다.
    int advance{}, ppem{}; // 다음 글자까지의 폭과 원래 strike의 정수 확대 기준 크기다.
    std::vector<std::uint8_t> pixels; // width*height개의 투명/불투명 값이다.
};
// Unicode cmap4와 수평 1비트 strike를 읽는 변경 불가능한 글꼴이다. 지원 밖 형식·손상은 명시적으로 거부한다.
class EmbeddedBitmapFont {
public:
    // SFNT 테이블 경계를 확인하고 첫 수평 단색 strike를 선택한다. bytes는 내부에 복사하므로 호출 뒤 반납할 수 있다.
    explicit EmbeddedBitmapFont(std::span<const std::uint8_t> bytes);
    // 내부 span의 backing을 옮기거나 복제하지 않도록 값 복사를 금지한다. 소유권은 shared_ptr로 공유한다.
    EmbeddedBitmapFont(const EmbeddedBitmapFont&) = delete;
    EmbeddedBitmapFont& operator=(const EmbeddedBitmapFont&) = delete; // 같은 backing의 주소 불변 계약이다.
    // 외부 파일을 프로세스에서 한 번 읽어 공유한다. 비트맵 테이블이 없는 글꼴은 빈 포인터로 GDI에 넘긴다.
    static std::shared_ptr<const EmbeddedBitmapFont> Load(const std::filesystem::path& file);
    // BMP 문자 하나의 원본 픽셀·치수를 반환한다. cmap/strike에 없는 문자는 빈 값이며 다른 시스템 글꼴로 대체하지 않는다.
    std::optional<EmbeddedBitmapGlyph> Glyph(std::uint32_t codePoint) const;
private:
    std::vector<std::uint8_t> bytes_; // 모든 테이블 span보다 오래 유지하는 외부 TTF 바이트다.
    std::span<const std::uint8_t> cmap_, locations_, images_; // 길이가 검증된 cmap4·EBLC·EBDT 테이블 뷰다.
    std::size_t strike_{}; // EBLC 안에서 선택한 48바이트 BitmapSize 레코드 위치다.
};
}
