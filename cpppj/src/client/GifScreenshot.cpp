// 원문 번호 선택/잠금 순서를 보존하며 GIF의 파일 장치와 LZW 출력을 분리한다.
#include "client/GifScreenshot.h"
#include "o/BaseFile.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace netstorm::client { // 입력·설정·GIF 장치는 같은 Client 수명에 연결한다.
namespace { // 선택되지 않은 장치를 요구하지 않는 지역 계약 검사다.
template<class T> const T& Need(const T& hook) { // 실제 도달한 외부 장치만 필수로 검사한다.
    if (!hook) throw std::logic_error("스크린샷 실제 장치 미연결");return hook;
}
}
bool DispatchScreenshotKey(InputEvent event,const std::function<void()>& save,const std::function<void(std::string_view)>& tell) { // 정지 중에도 저장하며 Tell 뒤 정지 여부는 호출자가 다시 읽는다.
    if ((event.code&0x03ffffffU)!=0x760000U || (event.code&(InputCode::kRelease|InputCode::kControl)) || !(event.code&InputCode::kShift)) return false;
    Need(save)();Need(tell)("TellGifWritten");return true;
}
std::string SaveNumberedScreenshot(std::uint32_t& counter,const ScreenshotSaveHooks& hooks) { // 경로의 실제 존재 여부마다 Config 치환을 다시 수행한다.
    std::string path; // 이번 시도에서 치환한 원문 GifOutSpec1이다.
    do { // 이미 있는 파일은 덮어쓰지 않고 다음 signed 번호 이름을 시험한다.
        const auto current=counter++; // 원문 DWORD는 서식 호출 전에 먼저 증가한다.
        Need(hooks.set)("GifOutName","pic"+std::to_string(std::bit_cast<std::int32_t>(current))+".gif");
        if (!Need(hooks.read)("GifOutSpec1",path)) throw std::logic_error("스크린샷 GifOutSpec1 설정 없음");
        if (path.size()>=256) throw std::length_error("스크린샷 원본256BYTE 경로 한도");
    } while (Need(hooks.exists)(path));
    Need(hooks.lock)();
    try { Need(hooks.gif)(path); }
    catch (...) { Need(hooks.unlock)();throw; } // 호환 파일 장치의 예외도 같은 잠금을 반납한다.
    Need(hooks.unlock)();Need(hooks.log)(path);return path;
}
std::filesystem::path ResolveScreenshotDiskPath(const std::filesystem::path& installRoot,std::string_view spec) { // 명시 절대 경로는 유지하고 원문 기본 디스크 이름만 설치 루트와 합친다.
    const std::filesystem::path direct{std::string(spec)};return direct.is_absolute() ? direct : installRoot/o::NormalizeGamePath(spec);
}
std::array<std::uint8_t,768> ScreenshotPrimaryPalette(std::span<const std::uint8_t> primary) { // 현재 출력 팔레트의 flash/날씨를 고정 primary RGB와 혼동하지 않는다.
    if (primary.size()!=256*56) throw std::invalid_argument("스크린샷 primary0x3800BYTE 필요");
    std::array<std::uint8_t,768> rgb{}; // 원래 색 번호 순서의256개 RGB다.
    for (std::size_t color=0;color<256;++color) { // 모든 원문 primary 기록을 번호 순서대로 방문한다.
        for (std::size_t channel=0;channel<3;++channel) { // 원문의R/G/B double 필드만 읽는다.
            double value{};std::memcpy(&value,primary.data()+color*56+24+channel*8,8); // 비정렬 raw를 별칭하지 않는 저장값이다.
            const double scaled=value*255.0; // 기본 x87 precision53의 곱셈 저장 지점이다.
            if (!(scaled>=0.0 && scaled<=255.0)) throw std::out_of_range("스크린샷 primary RGB 범위");
            rgb[color*3+channel]=static_cast<std::uint8_t>(scaled);
        }
    }
    return rgb;
}
std::vector<std::uint8_t> EncodeScreenshotGif(std::uint32_t width,std::uint32_t height,std::uint32_t pitch,std::span<const std::uint8_t> pixels,std::span<const std::uint8_t,768> rgb) { // 외부 양자화 없이 팔레트 인덱스와 행 방향을 그대로 기록한다.
    if (!width || !height || width>65535 || height>65535 || pitch<width || static_cast<std::uint64_t>(pitch)*(height-1)+width>pixels.size()) throw std::invalid_argument("스크린샷 GIF 폭/높이/행 간격 범위");
    std::vector<std::uint8_t> output{'G','I','F','8','7','a'}; // 투명/애니메이션 확장 없는 원본 단일 이미지 형식이다.
    const auto word=[&](std::uint32_t value) { output.push_back(static_cast<std::uint8_t>(value));output.push_back(static_cast<std::uint8_t>(value>>8)); }; // GIF의리틀 엔디언16비트 필드다.
    word(width);word(height);output.insert(output.end(),{0xf7,0,0});output.insert(output.end(),rgb.begin(),rgb.end());
    output.push_back(0x2c);word(0);word(0);word(width);word(height);output.insert(output.end(),{0,8});
    std::vector<std::uint8_t> compressed;std::uint32_t bits{},bitCount{},codeWidth=9,next=258; // LSB 코드 출력과 현재 LZW 사전 번호다.
    std::unordered_map<std::uint32_t,std::uint16_t> dictionary;dictionary.reserve(4096); // 접두 코드×256+다음BYTE의 실제 사전이다.
    const auto emit=[&](std::uint32_t code) { // 현재 폭으로 코드를 쓴 뒤 디코더의 한 항목 늦은 증가에 맞춘다.
        bits|=code<<bitCount;bitCount+=codeWidth;
        while (bitCount>=8) { compressed.push_back(static_cast<std::uint8_t>(bits));bits>>=8;bitCount-=8; } // 완성된 낮은BYTE부터 기록한다.
        if (next==(1U<<codeWidth) && codeWidth<12) ++codeWidth;
    };
    emit(256);std::uint32_t prefix=pixels[0];bool first=true; // clear 뒤 첫 픽셀은 접두로 보관한다.
    for (std::uint32_t y=0;y<height;++y) { // 행 패딩을 건너뛰고 논리 화면의위에서 아래로 방문한다.
        for (std::uint32_t x=0;x<width;++x) { // 현재 행의실제 픽셀만 사전에 공급한다.
            if (first) { first=false;continue; }
            const auto byte=pixels[static_cast<std::size_t>(y)*pitch+x];const auto key=prefix*256U+byte; // 다음 접두/문자열의사전 키다.
            const auto found=dictionary.find(key); // 현재 사전만 조회하며 출력 팔레트를 바꾸지 않는다.
            if (found!=dictionary.end()) { prefix=found->second;continue; }
            emit(prefix);
            if (next<4096) dictionary.emplace(key,static_cast<std::uint16_t>(next++));
            else { emit(256);dictionary.clear();next=258;codeWidth=9; }
            prefix=byte;
        }
    }
    emit(prefix);emit(257);if (bitCount) compressed.push_back(static_cast<std::uint8_t>(bits));
    for (std::size_t offset=0;offset<compressed.size();) { // GIF의최대255BYTE 서브 블록으로 실제 LZW 흐름을 나눈다.
        const auto count=std::min<std::size_t>(255,compressed.size()-offset);output.push_back(static_cast<std::uint8_t>(count));output.insert(output.end(),compressed.begin()+offset,compressed.begin()+offset+count);offset+=count;
    }
    output.insert(output.end(),{0,0x3b});return output;
}
}
