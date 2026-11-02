// 원문459c60의 부품 순서·바이트 작성과10칸 출력 수명을 복원한다.
#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace netstorm::o { // OS 경로나 Unicode로 다시 해석하지 않는 원본 바이트 작성기다.
struct FileNamePart {
    std::int32_t kind{}; // -1 루트/-2 디렉터리/-3 파일/-4 확장자이며0은 가변 인자의 종료다.
    std::string_view text; // 각 처리 시점에 현재 NUL까지 복사하며 이전 출력 고리의 별칭도 허용한다.
};
class RawFileName {
public:
    RawFileName()=default; // 같은 Client의 최초 BSS0 고리와 번호를 만든다.
    RawFileName(const RawFileName&)=delete;RawFileName& operator=(const RawFileName&)=delete; // 반환한 뷰의 실제 주소를 복제하지 않는다.
    // 실제 kind 순서 오류는 부분 출력/번호를 보존한 채 invalid_argument다. 원문256BYTE 밖 쓰기는 length_error로 거부하며 절단하지 않는다.
    // separator는00532728의 현재 비교/치환 BYTE다. 원문의 별도 고정 접두 .\와 연결문자 \는 그대로 유지한다.
    std::string_view Format(std::span<const FileNamePart> parts,char separator='\\');
    std::string_view Device(std::string_view fileSpec,char separator='\\'); //459900은 같은 고리를 선증가/clear한 뒤 이미 작성된 FileSpec의 장치 접두를 반환한다. NUL 뒤 바이트도 보존하며256BYTE 밖은 거부한다.
    std::uint32_t Index() const; // 원문 선증가/10 나머지 뒤의 현재 물리 칸 번호다.
    std::span<const std::uint8_t,256> CurrentBytes() const; // NUL 뒤 남은 바이트까지 같은 실제256BYTE를 빌린다.
private:
    std::array<std::array<std::uint8_t,256>,10> buffers_{}; // 모든 반환 뷰는 열 번째 후속 호출 때 같은 칸에 덮인다.
    std::uint32_t index_{}; // 최초0이며 일반 호출에서는0..9를 순환한다.
};
}
