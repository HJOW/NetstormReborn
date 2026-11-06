// 시나리오의 원문 바이트와 수정하지 않은 대본을 보존하는 파일 I/O 없는 편집 문서다.
#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client { // 원문 Config와 별도로 사용자 편집 상태만 소유한다.
// 문서가 입력 바이트를 해석한 방식이며 무변경 저장은 이 방식과 무관하게 입력 그대로다.
enum class ScenarioDocumentEncoding { Windows1252, Utf8Bom, EncodedWindows1252, EncodedUtf8Bom };

// 전체 문서를 재작성하지 않고 Config가 읽는 첫 키의 값 구간만 바꾼다.
class ScenarioDocument {
public:
    // 원본 CP1252/UTF-8 BOM/XOR Config를 읽는다. NUL·잘못된 BOM UTF-8·암호 서명은 예외다.
    static ScenarioDocument FromBytes(std::span<const std::uint8_t> bytes);
    // 이미 디코딩한 유효 UTF-8 문서로 시작하며 최초 저장 형태는 UTF-8 BOM이다.
    static ScenarioDocument FromUtf8(std::string text);
    // Config와 같은 첫 일치 키의 따옴표 제거 원시값이다. 치환과 후행 공백 정리는 하지 않는다.
    std::optional<std::string> Raw(std::string_view key) const;
    // 사용자 문자값을 Config의 백틱/따옴표/매크로 문법에서 보호해 첫 값 구간만 바꾼다.
    // 없는 키만 Header 끝에 삽입한다. 기존 중복·주석·브리핑·미지 키는 이동하지 않는다.
    bool SetLiteral(std::string_view key, std::string_view value);
    // 읽기 전용 디코딩 본문이다. 암호 Config의 mQdsT 접두도 실제 소비자처럼 유지한다.
    const std::string& Text() const;
    // 변경이 없으면 입력 바이트 그대로, 변경되었으면 현재 본문에 UTF-8 BOM을 붙인다.
    std::vector<std::uint8_t> Serialize() const;
    // 현재 본문과 최초 본문이 다른지 확인한다. 파일 저장 성공 여부는 호출자 책임이다.
    bool Changed() const;
    // 입력의 인코딩과 암호화 여부를 표시할 때 사용한다.
    ScenarioDocumentEncoding Encoding() const;
    // UTF-8의 최단 표현·서로게이트·최대 코드포인트·NUL을 검사한다. 파일은 쓰지 않는다.
    static void ValidateUtf8(std::string_view text);
private:
    // 외부 생성은 검증한 정적 생성 함수로 제한한다.
    ScenarioDocument(std::vector<std::uint8_t> bytes,std::string text,ScenarioDocumentEncoding encoding);
    std::vector<std::uint8_t> originalBytes_; // 무변경 저장의 바이트 identity다.
    std::string originalText_; // Changed 비교에 사용하는 최초 디코딩 본문이다.
    std::string text_; // 수정한 첫 값 구간을 포함한 현재 UTF-8 본문이다.
    ScenarioDocumentEncoding encoding_; // 입력 형식 안내용이며 출력 변환을 강제하지 않는다.
};

// 새 사용자 맵의 최소 Tutorial 대본을 만든다. 원본 기본값을 주장하거나 파일을 쓰지 않는다.
// 기본 SP 5000/all은 새 편집 기능의 명시값이고 사제·AI·승리 조건은 자동 생성하지 않는다.
ScenarioDocument CreateDefaultEditorScenario(std::string_view mapName);
}
