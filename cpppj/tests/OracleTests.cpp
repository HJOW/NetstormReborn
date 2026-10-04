// 두 판본 원본 x86 기계어에서 생성한 기대값으로 C++ 복원 함수를 검사한다.
#include "TestSupport.h"
#include "o/Config.h"
#include "o/RiftType.h"
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
// TSV의 16진수 바이트를 손실 없이 문자열로 바꾼다. '-'는 빈 버퍼다.
std::string Unhex(const std::string& text) {
    if (text == "-") return {};
    if (text.size() % 2 != 0) throw std::runtime_error("Invalid fixture hex");
    std::string result;
    // 두 자리씩 1바이트로 변환한다.
    for (std::size_t pos = 0; pos < text.size(); pos += 2) result.push_back(static_cast<char>(std::stoul(text.substr(pos,2), nullptr,16)));
    return result;
}
}

// Python 모델이 아니라 원본 기계어의 기대값으로 순서·부호 확장·원시 파서를 검증한다.
TEST_CASE(RecoveredFunctions_MatchBothOriginalX86Editions) {
    std::ifstream file(NETSTORM_ORACLE_FIXTURE);
    CHECK(file.is_open());
    std::string line;
    std::size_t count = 0;
    // TSV 한 줄이 두 판본에서 같은 결과를 낸 하나의 입력이다.
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream stream(line);
        std::string kind, dataHex;
        stream >> kind >> dataHex;
        if (kind == "Config") {
            std::string keyHex, expectedValue;
            std::int64_t expectedOffset = 0;
            stream >> keyHex >> expectedOffset >> expectedValue;
            const netstorm::o::ConfigText config(Unhex(dataHex));
            const auto offset = config.FindKey(Unhex(keyHex));
            const auto value = config.GetRaw(Unhex(keyHex));
            CHECK((offset ? static_cast<std::int64_t>(*offset) : -1) == expectedOffset);
            if (expectedValue == "missing") CHECK(!value);
            else { CHECK(value.has_value()); CHECK(value == Unhex(expectedValue.substr(4))); }
        } else {
            const auto data = Unhex(dataHex);
            std::vector<netstorm::o::FrameCode> frames;
            // 원본 4바이트 코드 순서를 그대로 가져온다.
            for (std::size_t pos = 0; pos < data.size(); pos += 4)
                frames.push_back({static_cast<std::uint8_t>(data[pos]), static_cast<std::uint8_t>(data[pos+1]),
                                  static_cast<std::uint8_t>(data[pos+2]), static_cast<std::uint8_t>(data[pos+3])});
            const netstorm::o::RiftTypeFrames table(std::move(frames));
            unsigned int side = 0, variant = 0, number = 0;
            int expected = 0, actual = 0;
            if (kind == "FrameFindFlags") {
                std::int32_t flags = 0;
                stream >> side >> variant >> flags >> expected;
                actual = table.FindFlags(static_cast<std::uint8_t>(side), static_cast<std::uint8_t>(variant), flags);
            } else if (kind == "FrameFindMasked") {
                std::uint32_t mask = 0;
                stream >> side >> variant >> number >> mask >> expected;
                actual = table.FindMasked(static_cast<std::uint8_t>(side), static_cast<std::uint8_t>(variant), static_cast<std::uint8_t>(number), mask);
            } else if (kind == "FrameFindNumber") {
                stream >> side >> variant >> number >> expected;
                actual = table.FindNumber(static_cast<std::uint8_t>(side), static_cast<std::uint8_t>(variant), static_cast<std::uint8_t>(number));
            } else throw std::runtime_error("Unknown oracle fixture kind");
            if (actual != expected) std::fprintf(stderr, "Oracle row %zu: %s\n", count+1, line.c_str());
            CHECK(actual == expected);
        }
        CHECK(!stream.fail());
        ++count;
    }
    CHECK(count == 1806);
}
