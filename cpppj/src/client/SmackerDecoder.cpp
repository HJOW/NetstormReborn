// 원본32비트 Smackw32.dll을 직접 로드할 수 없는 Windows10/11 x64에서 LGPL 디코더 장치를 사용한다.
#include "client/SmackerDecoder.h"
#include "../../third_party/libsmacker/smacker.h"
#ifndef NOMINMAX
#define NOMINMAX // Windows 매크로가 표준 수치 한계 함수 이름을 가리지 않게 한다.
#endif
#include <windows.h>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 재생 정책 없이 실제 압축 프레임/오디오 장치만 제공한다.
namespace { // 상류 DLL의 C ABI와 Windows 수명만 감싼다.
struct SmackerApi { // 디코더가 살아 있는 동안 같은 DLL과 정확한 상류 함수형을 보관한다.
    HMODULE module{}; // 프로세스 전역 기본 검색 대신 명시 실행 파일 폴더에서 연 DLL이다.
    decltype(&::smk_open_memory) open{};decltype(&::smk_close) close{}; // 메모리 스트림 생성과 반납이다.
    decltype(&::smk_info_all) info{};decltype(&::smk_info_video) videoInfo{};decltype(&::smk_info_audio) audioInfo{}; // 현재 프레임/헤더/트랙 정보의 직접 조회다.
    decltype(&::smk_enable_all) enable{};decltype(&::smk_first) first{};decltype(&::smk_next) next{}; // 실제 트랙 활성화와 순차 해제다.
    decltype(&::smk_get_video) video{};decltype(&::smk_get_palette) palette{}; // DLL 소유의 현재 팔레트 인덱스와 RGB다.
    decltype(&::smk_get_audio) audio{};decltype(&::smk_get_audio_size) audioSize{}; // 트랙별 현재 PCM 청크와 실제 길이다.
    ~SmackerApi() { if (module) FreeLibrary(module); } // 마지막 스트림 반납 뒤에만 코드를 해제한다.
};
// GetProcAddress의 실제 C 호출형만 복원하며 함수가 없으면 빈 포인터를 유지한다.
template<class Function> Function Procedure(HMODULE module,const char* name) { return std::bit_cast<Function>(GetProcAddress(module,name)); }
// 원본 ANSI 검색 경로와 달리 한글 설치 경로와64비트 DLL을 지원하는 Windows10/11용 명시 파일 위치다.
std::filesystem::path DecoderPath() {
    std::vector<wchar_t> buffer(32768);const auto size=GetModuleFileNameW(nullptr,buffer.data(),static_cast<DWORD>(buffer.size()));
    if (!size || size>=buffer.size()) throw std::runtime_error("Smacker 실행 파일 경로 조회 실패");return std::filesystem::path(buffer.data()).parent_path()/L"NetstormSmacker.dll";
}
// 누락/다른 버전 DLL은 원본 SmackOpen의 실패처럼 구별하여 호출자에게 돌려준다.
std::shared_ptr<SmackerApi> LoadApi(const std::filesystem::path& path) {
    const auto filename=path.empty()?DecoderPath():std::filesystem::absolute(path);auto api=std::make_shared<SmackerApi>();api->module=LoadLibraryExW(filename.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if (!api->module) return {};
    api->open=Procedure<decltype(api->open)>(api->module,"smk_open_memory");api->close=Procedure<decltype(api->close)>(api->module,"smk_close");
    api->info=Procedure<decltype(api->info)>(api->module,"smk_info_all");api->videoInfo=Procedure<decltype(api->videoInfo)>(api->module,"smk_info_video");api->audioInfo=Procedure<decltype(api->audioInfo)>(api->module,"smk_info_audio");
    api->enable=Procedure<decltype(api->enable)>(api->module,"smk_enable_all");api->first=Procedure<decltype(api->first)>(api->module,"smk_first");api->next=Procedure<decltype(api->next)>(api->module,"smk_next");
    api->video=Procedure<decltype(api->video)>(api->module,"smk_get_video");api->palette=Procedure<decltype(api->palette)>(api->module,"smk_get_palette");api->audio=Procedure<decltype(api->audio)>(api->module,"smk_get_audio");api->audioSize=Procedure<decltype(api->audioSize)>(api->module,"smk_get_audio_size");
    if (!api->open || !api->close || !api->info || !api->videoInfo || !api->audioInfo || !api->enable || !api->first || !api->next || !api->video || !api->palette || !api->audio || !api->audioSize) return {};return api;
}
}
struct SmackerDecoder::Impl { // 원문 버퍼를 유지해 상류 구현의 입력 수명과 독립적으로 안전하게 해제한다.
    std::shared_ptr<SmackerApi> api;std::vector<std::uint8_t> encoded;smk stream{}; // DLL/입력/현재 압축 스트림의 같은 소유 수명이다.
    SmackerVideoInfo info;bool started{},finished{},valid{}; // 메타데이터와 첫 해제/EOF/현재 출력 유효 여부다.
    ~Impl() { if (stream) api->close(stream); } // 멤버 DLL과 원문이 해제되기 전에 실제 스트림을 닫는다.
    void RequireFrame() const { if (!valid) throw std::logic_error("Smacker의 디코딩된 실제 프레임 없음"); } // 초기 미정 버퍼를 영상처럼 반환하지 않는다.
};
// 공개 객체는 초기화가 완료된 단일 구현만 받는다.
SmackerDecoder::SmackerDecoder(std::unique_ptr<Impl> impl):impl_(std::move(impl)) {}
SmackerDecoder::~SmackerDecoder()=default;
// 상류의32비트 입력 길이를 넘는 경우 자르지 않고 실패한다.
std::unique_ptr<SmackerDecoder> SmackerDecoder::Open(std::vector<std::uint8_t> encoded,std::filesystem::path decoderDll) {
    if (encoded.size()<4 || encoded.size()>std::numeric_limits<unsigned long>::max()) return {};
    auto impl=std::make_unique<Impl>();impl->api=LoadApi(decoderDll);if (!impl->api) return {};impl->encoded=std::move(encoded);impl->stream=impl->api->open(impl->encoded.data(),static_cast<unsigned long>(impl->encoded.size()));if (!impl->stream) return {};
    unsigned long width{},height{},frames{};double period{};unsigned char scale{},mask{};std::array<unsigned char,7> channels{},depths{};std::array<unsigned long,7> rates{};
    if (impl->api->info(impl->stream,nullptr,&frames,&period)<0 || impl->api->videoInfo(impl->stream,&width,&height,&scale)<0 || impl->api->audioInfo(impl->stream,&mask,channels.data(),depths.data(),rates.data())<0) return {};
    if (!width || !height || !frames || !std::isfinite(period) || period<=0 || std::size_t(width)>std::numeric_limits<std::size_t>::max()/height) return {};
    impl->info.width=width;impl->info.height=height;impl->info.frameCount=frames;impl->info.microsecondsPerFrame=period;impl->info.yScaleMode=scale;
    for (std::size_t track=0;track<7;++track) impl->info.audio[track]={(mask&(1U<<track))!=0,channels[track],depths[track],rates[track]}; // 각 트랙의 실제 형식을 그대로 유지한다.
    if (impl->api->enable(impl->stream,0xff)<0) return {};return std::unique_ptr<SmackerDecoder>(new SmackerDecoder(std::move(impl)));
}
// 원본 fopen ANSI 대신 filesystem 경로를 받아 Windows10/11의 한글 파일 이름도 같은 바이트로 읽는다.
std::unique_ptr<SmackerDecoder> SmackerDecoder::OpenFile(const std::filesystem::path& path,std::filesystem::path decoderDll) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);if (!file) return {};const auto size=file.tellg();if (size<4 || static_cast<std::uint64_t>(size)>std::numeric_limits<unsigned long>::max()) return {};
    std::vector<std::uint8_t> encoded(static_cast<std::size_t>(size));file.seekg(0);if (!file.read(reinterpret_cast<char*>(encoded.data()),static_cast<std::streamsize>(encoded.size()))) return {};return Open(std::move(encoded),std::move(decoderDll));
}
// 메타데이터 조회는 디코더 시각이나 프레임을 진행시키지 않는다.
const SmackerVideoInfo& SmackerDecoder::Info() const { return impl_->info; }
// 링 프레임이 있는 파일도 한 순회 마지막을 호출자에게 명시하고 자동 반복하지 않는다.
SmackerFrameStatus SmackerDecoder::Next() {
    if (impl_->finished) return SmackerFrameStatus::Done;const auto status=impl_->started ? impl_->api->next(impl_->stream) : impl_->api->first(impl_->stream);
    if (status==SMK_MORE || status==SMK_LAST) { impl_->started=impl_->valid=true;if (status==SMK_LAST) { impl_->finished=true;return SmackerFrameStatus::Last; }return SmackerFrameStatus::More; }
    impl_->finished=true;if (status==SMK_DONE) return SmackerFrameStatus::Done;impl_->valid=false;return SmackerFrameStatus::Error;
}
// 실제 현재 번호를 직접 읽어 헤더의 프레임 수와 구별한다.
std::uint32_t SmackerDecoder::FrameIndex() const { unsigned long frame{};if (impl_->api->info(impl_->stream,&frame,nullptr,nullptr)<0) throw std::runtime_error("Smacker 프레임 번호 조회 실패");return frame; }
// 반환 span의 메모리는 DLL 소유이므로 다음 해제 뒤 보관하려면 호출자가 복사한다.
std::span<const std::uint8_t> SmackerDecoder::Pixels() const { impl_->RequireFrame();return {impl_->api->video(impl_->stream),std::size_t(impl_->info.width)*impl_->info.height}; }
// 디코더의 원래 RGB 확장표 결과를 그대로 빌린다.
std::span<const std::uint8_t,768> SmackerDecoder::Palette() const { impl_->RequireFrame();return std::span<const std::uint8_t,768>(impl_->api->palette(impl_->stream),768); }
// 오디오가 없는 프레임/트랙은 빈 청크이며 이전 청크를 반복하지 않는다.
std::span<const std::uint8_t> SmackerDecoder::Audio(std::uint32_t track) const { impl_->RequireFrame();if (track>=7) throw std::out_of_range("Smacker 오디오 트랙 범위");if (!impl_->info.audio[track].exists) return {};return {impl_->api->audio(impl_->stream,static_cast<unsigned char>(track)),impl_->api->audioSize(impl_->stream,static_cast<unsigned char>(track))}; }
}
