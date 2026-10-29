// 원본 Smackw32 장치가 제공하던 영상/팔레트/PCM을64비트 디코더 DLL에서 빌린다.
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace netstorm::client { // MovieGump의 시간/입력/창 수명은 디코더 외부에 남긴다.
struct SmackerAudioInfo { // 원본 파일에 있는 각 오디오 트랙의 PCM 규격이다.
    bool exists{};std::uint8_t channels{},bitsPerSample{};std::uint32_t sampleRate{}; // 트랙 존재/채널/샘플폭/초당 샘플 수다.
};
struct SmackerVideoInfo { // 실제 SMK 헤더에서 얻은 영상과 일곱 오디오 트랙의 정보다.
    std::uint32_t width{},height{},frameCount{};double microsecondsPerFrame{}; // 원래 팔레트 픽셀 크기/헤더 프레임 수/프레임 간격이다.
    std::uint8_t yScaleMode{};std::array<SmackerAudioInfo,7> audio{}; // 원래 세로 배율 플래그와 트랙별 독립 규격이다.
};
enum class SmackerFrameStatus { More,Last,Done,Error }; // 마지막 프레임도 유효한 출력이며 EOF/오류와 구별한다.
class SmackerDecoder { // DLL과 압축 원문/디코더를 같은 수명으로 보관하는 단일 스트림이다.
public:
    static std::unique_ptr<SmackerDecoder> Open(std::vector<std::uint8_t> encoded,std::filesystem::path decoderDll={}); // 원문을 소유하며 잘못된 파일/누락 DLL은 실패nullptr다.
    static std::unique_ptr<SmackerDecoder> OpenFile(const std::filesystem::path& path,std::filesystem::path decoderDll={}); // Windows10/11 한글 경로를 W 기반 C++ 파일 장치로 읽는다.
    ~SmackerDecoder(); // 실제 smk_close 뒤에만 DLL/압축 원문을 반납한다.
    SmackerDecoder(const SmackerDecoder&)=delete;SmackerDecoder& operator=(const SmackerDecoder&)=delete; // 디코더의 현재 프레임과 소유권을 복제하지 않는다.
    const SmackerVideoInfo& Info() const; // 파일의 원래 정보이며 표시 해상도/재생 속도를 덮어쓰지 않는다.
    SmackerFrameStatus Next(); // 첫 호출은 첫 프레임, 이후는 다음 프레임의 압축을 풀며 마지막 뒤에는 Done을 유지한다.
    std::uint32_t FrameIndex() const; // 마지막으로 디코딩한0기반 실제 프레임 번호다.
    std::span<const std::uint8_t> Pixels() const; // 다음 Next/파괴까지 유효한 width×height 팔레트 인덱스다.
    std::span<const std::uint8_t,768> Palette() const; // 같은 프레임의 RGB256색이며 재양자화하지 않는다.
    std::span<const std::uint8_t> Audio(std::uint32_t track) const; // 같은 프레임의 PCM 청크이며 없는 트랙은 빈 span이다.
private:
    struct Impl;std::unique_ptr<Impl> impl_; // 실제 DLL 함수표/handle과 입력/메타데이터의 유일한 소유자다.
    explicit SmackerDecoder(std::unique_ptr<Impl> impl); // 모든 open/info/enable 정상 반환 후에만 공개 스트림을 만든다.
};
}
