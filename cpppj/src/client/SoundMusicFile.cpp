// 원본 음악의 WAVE 헤더 처리와 열린 파일 수명을 실제 Winmm 경계에 연결한다.
#include "client/SoundMusicFile.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <climits>
#include <cstring>
#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 닫힌 토큰은 재사용하지 않아 원본 오류 경로의 남은 File 값이 다른 호스트 파일을 닫지 않게 한다.
struct MusicFileStore::Impl {
    // 파일 표와 토큰/같은 Winmm 위치를 한 IO 호출 동안 보호한다. Open의 실패 정리는 Close로 재진입한다.
    mutable std::recursive_mutex mutex;
    std::map<std::uint32_t,HMMIO> files;std::uint32_t next=1;SoundAssertReport report;
    // 파일을 열지 않고 진단 콜백만 보관한다.
    explicit Impl(SoundAssertReport diagnostic):report(std::move(diagnostic)) {}
    // 알려진 토큰의 핸들 또는 null을 조회한다. 만료된 토큰은 IO 실패로 처리한다.
    HMMIO Get(std::uint32_t token) const { const auto found=files.find(token);return found==files.end() ? nullptr : found->second; }
};
// 소유자와 선택 진단 경계를 만든다.
MusicFileStore::MusicFileStore(SoundAssertReport report):impl_(std::make_unique<Impl>(std::move(report))) {}
// 정상 종료/예외 정리에서 남은 열린 파일을 반납한다.
MusicFileStore::~MusicFileStore() {
    std::lock_guard guard(impl_->mutex);
    // 각 실제 핸들은 표에 한 번만 등록되어 있다.
    for (const auto& [token,handle]:impl_->files) { static_cast<void>(token);mmioClose(handle,0); }
}
// 헤더 출력은 원본처럼 성공한 단계까지 변경한다. 실패/진단 예외는 파일 소유권을 반납한다.
bool MusicFileStore::Open(const std::filesystem::path& path,MusicFileHeader& header) {
    std::lock_guard guard(impl_->mutex);
    auto filename=path.wstring();const auto handle=mmioOpenW(filename.data(),nullptr,MMIO_ALLOCBUF|MMIO_READ);
    header.file=0;if (!handle) return false;
    if (impl_->next==0) { mmioClose(handle,0);throw std::length_error("음악 파일 토큰이 소진됐습니다"); }
    header.file=impl_->next++;
    try { impl_->files.emplace(header.file,handle); } catch (...) { mmioClose(handle,0);throw; }
    // 헤더 실패와 예외에서 닫되 성공하면 열린 파일을 채널에 넘기는 범위 소유자다.
    struct Opening {
        MusicFileStore& files;std::uint32_t token;bool keep=false;
        // 성공 표시가 없는 반환/예외는 닫는다. raw 토큰 값은 지우지 않는다.
        ~Opening() { if (!keep) files.Close(token); }
    } opening{*this,header.file};
    MMCKINFO riff{};riff.fccType=mmioFOURCC('W','A','V','E');
    if (mmioDescend(handle,&riff,nullptr,MMIO_FINDRIFF)!=0) return false;
    MMCKINFO chunk{};chunk.ckid=mmioFOURCC('f','m','t',' ');
    if (mmioDescend(handle,&chunk,&riff,MMIO_FINDCHUNK)!=0 || chunk.cksize<16) return false;
    const auto size=std::min<DWORD>(chunk.cksize,18);
    if (mmioRead(handle,reinterpret_cast<HPSTR>(header.format.data()),static_cast<LONG>(size))!=static_cast<LONG>(size)) return false;
    WAVEFORMATEX format{};std::memcpy(&format,header.format.data(),18);
    if (size<18) {
        if (format.wFormatTag!=WAVE_FORMAT_PCM && impl_->report) impl_->report("wfFormat.wFormatTag == WAVE_FORMAT_PCM",0xeb);
        const auto denominator=static_cast<std::uint32_t>(format.nChannels)*format.nSamplesPerSec;
        if (!denominator) return false;
        format.cbSize=0;format.wBitsPerSample=static_cast<WORD>((format.nAvgBytesPerSec<<3)/denominator);
        std::memcpy(header.format.data(),&format,18);
    }
    // 원본 버퍼가 추가 코덱 자료를 소유하지 않으므로 비PCM 확장/0 나눗셈은 호스트 경계에서 거부한다.
    if (!format.nChannels || !format.nSamplesPerSec || !format.wBitsPerSample || (format.wFormatTag!=WAVE_FORMAT_PCM && format.cbSize!=0)) return false;
    if (mmioAscend(handle,&chunk,0)!=0) return false;
    chunk={};chunk.ckid=mmioFOURCC('d','a','t','a');
    if (mmioDescend(handle,&chunk,&riff,MMIO_FINDCHUNK)!=0 || !chunk.cksize || chunk.cksize>LONG_MAX || chunk.dwDataOffset>LONG_MAX) return false;
    header.length=chunk.cksize;header.dataOffset=chunk.dwDataOffset;opening.keep=true;return true;
}
// Winmm의 signed 절대 seek 범위를 넘기는 입력은 실행하지 않는다.
std::int32_t MusicFileStore::Seek(std::uint32_t token,std::uint32_t absolute) {
    std::lock_guard guard(impl_->mutex);
    const auto handle=impl_->Get(token);return !handle || absolute>LONG_MAX ? -1 : mmioSeek(handle,static_cast<LONG>(absolute),SEEK_SET);
}
// 요청 크기만 실제 파일에서 읽는다. 원본 채널은 짧은 반환을 닫기/정지로 해석한다.
std::int32_t MusicFileStore::Read(std::uint32_t token,std::span<std::uint8_t> target) {
    std::lock_guard guard(impl_->mutex);
    const auto handle=impl_->Get(token);if (!handle || target.size()>LONG_MAX) return -1;
    if (target.empty()) return 0;
    return mmioRead(handle,reinterpret_cast<HPSTR>(target.data()),static_cast<LONG>(target.size()));
}
// 표에서 제거한 뒤 실제 핸들을 닫는다. 재닫기는 다른 파일을 건드리지 않는다.
void MusicFileStore::Close(std::uint32_t token) {
    std::lock_guard guard(impl_->mutex);
    const auto found=impl_->files.find(token);if (found==impl_->files.end()) return;
    const auto handle=found->second;impl_->files.erase(found);mmioClose(handle,0);
}
// 실제 현재 위치를 조회한다.
std::int32_t MusicFileStore::Position(std::uint32_t token) const { std::lock_guard guard(impl_->mutex);const auto handle=impl_->Get(token);return handle ? mmioSeek(handle,0,SEEK_CUR) : -1; }
// 이미 구성된 COM/잠금/기록 경계에 파일 소유자의 수명을 묶는다.
MusicChannelHooks MusicFileStore::FileHooks(MusicChannelHooks hooks) {
    hooks.seek=[this](std::uint32_t token,std::uint32_t offset) { return Seek(token,offset); }; // 절대 되감기 경계다.
    hooks.read=[this](std::uint32_t token,std::span<std::uint8_t> target) { return Read(token,target); }; // 요청만큼 부분 읽는다.
    hooks.close=[this](std::uint32_t token) { Close(token); };return hooks; // 만료 토큰의 안전한 재닫기다.
}
// 실제 경로 조회는 효과음 resolver의 단일 조회만 재사용한다. 음악의 두 검색 문자열은 MusicChannel::Open이 만든다.
MusicOpenHooks MusicFileStore::OpenHooks(SoundFileResolver resolver,std::function<void(std::string_view)> fallback) {
    if (!resolver) throw std::invalid_argument("음악 파일 resolver가 없습니다");
    MusicOpenHooks hooks;
    hooks.find=[resolver=std::move(resolver)](std::string_view pattern)->std::optional<std::string> {
        const auto found=resolver(pattern,false);if (!found) return {};
        const auto utf8=found->u8string();return std::string(reinterpret_cast<const char*>(utf8.data()),utf8.size());
    }; // UTF-8 경로로 파일 탐색 결과를 반환한다.
    hooks.open=[this](std::string_view name,MusicFileHeader& header) {
        const std::u8string utf8(reinterpret_cast<const char8_t*>(name.data()),name.size());return Open(std::filesystem::path(utf8),header);
    }; // 경로를 유니코드 Windows 파일로 열어 채널 format/토큰을 채운다.
    hooks.fallback=std::move(fallback);return hooks;
}
}
