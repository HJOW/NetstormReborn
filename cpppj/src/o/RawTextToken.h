// 원본의80바이트 임시 토큰 읽기와 다음 입력 위치를 복원한다.
#pragma once
#include <cstddef>
#include <string>
#include <string_view>

namespace netstorm::o {
// 출력은 따옴표를 뺀 실제 바이트이며 next는 원본 입력 포인터의 이동량이다.
struct RawTextToken {
    std::string text; // 최대78개 입력 바이트에서 따옴표를 제외한 토큰이다.
    std::size_t next{}; // 연속 구분자를 건너뛴 뒤의 같은 입력 위치다.
};
// PATCH00487620/CD0041a3d0. 앞 space/tab·인용·입력78바이트 제한·뒤 공백의 첫 바이트 보존을 적용한다.
RawTextToken ReadRawTextToken(std::string_view source,std::size_t offset=0,std::string_view delimiters=" \t\n,");
}
