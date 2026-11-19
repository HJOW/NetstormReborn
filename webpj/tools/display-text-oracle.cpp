#include "o/OriginalText.h"
#include <fstream>
#include <iomanip>
#include <string>
#include <utility>
// 코드 포인트 하나를 UTF-8로 바꿔 같은 cpppj 부호화 함수를 실행한다.
static void emit(std::ofstream& out, unsigned cp) {
    std::string text;
    if (cp < 0x80) text.push_back(static_cast<char>(cp));
    else if (cp < 0x800) { text.push_back(static_cast<char>(0xc0 | (cp >> 6))); text.push_back(static_cast<char>(0x80 | (cp & 63))); }
    else { text.push_back(static_cast<char>(0xe0 | (cp >> 12))); text.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63))); text.push_back(static_cast<char>(0x80 | (cp & 63))); }
    const auto bytes = netstorm::o::EncodeDisplayText(text);
    out << std::hex << cp << '\t';
    for (auto byte : bytes) out << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
    out << '\n';
}
// 정적 모든 문자와 최초 사용 순서의 한자를 기록한다.
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ofstream out(argv[1]);
    for (auto [first, last] : {std::pair{0xac00u, 0xd7a3u}, {0x3131u, 0x318eu}, {0x3000u, 0x303fu}, {0xff01u, 0xff5eu}, {0x3040u, 0x30ffu}, {0xff61u, 0xff9fu}})
        for (unsigned cp = first; cp <= last; ++cp) emit(out, cp);
    for (unsigned cp : {0x65e5u, 0x672cu, 0x8a9eu, 0x65e5u}) emit(out, cp);
    return out ? 0 : 1;
}
