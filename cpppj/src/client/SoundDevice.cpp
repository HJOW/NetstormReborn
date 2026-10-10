// WAVE와 DirectSound를 원본 순서대로 다루며 모든 실패 경로에서 이 계층이 확보한 OS 자원을 해제한다.
#include "client/SoundDevice.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본 효과음 버퍼 플래그다: 정적 버퍼(0x2)와 주파수·좌우·음량 제어(0xe0), 합계 0xe2다.
constexpr DWORD kEffectFlags=0xe2;
// RIFF 청크 탐색에 쓰는 WAVE/fmt/data FourCC다.
constexpr FOURCC kWave=mmioFOURCC('W','A','V','E'),kFormat=mmioFOURCC('f','m','t',' '),kData=mmioFOURCC('d','a','t','a');
// mmio 파일을 모든 반환/예외 경로에서 닫는 지역 소유자다. 실제 오디오 장치와 무관하다.
struct WaveFile {
    HMMIO handle{};
    // 확보한 읽기 파일을 닫는다.
    ~WaveFile() { if (handle) mmioClose(handle,0); }
};
// 디렉터리와 파일 이름을 원본의 '\' 구분자로 합친다. 빈 디렉터리는 생략한다.
std::string Join(std::string_view directory,std::string_view name) {
    if (directory.empty()) return std::string(name);
    return std::string(directory)+((directory.back()=='\\' || directory.back()=='/') ? "" : "\\")+std::string(name);
}
}

// fmt는 최대 18바이트를 읽고 짧은 PCM은 평균 바이트율로 bits를 다시 계산한다. data는 정확한 크기만 읽는다.
std::optional<SoundWave> ReadSoundWave(const std::filesystem::path& path,o::OriginalEdition edition,SoundAssertReport report) {
    auto filename=path.wstring();
    WaveFile file{mmioOpenW(filename.data(),nullptr,MMIO_ALLOCBUF|MMIO_READ)};
    if (!file.handle) return {};
    MMCKINFO riff{};riff.fccType=kWave;
    if (mmioDescend(file.handle,&riff,nullptr,MMIO_FINDRIFF)!=0) return {};
    MMCKINFO chunk{};chunk.ckid=kFormat;
    if (mmioDescend(file.handle,&chunk,&riff,MMIO_FINDCHUNK)!=0 || chunk.cksize<16) return {};
    SoundWave wave;WAVEFORMATEX format{};
    const auto size=std::min<DWORD>(chunk.cksize,18);
    if (mmioRead(file.handle,reinterpret_cast<HPSTR>(&format),static_cast<LONG>(size))!=static_cast<LONG>(size)) return {};
    if (size<18) {
        if (format.wFormatTag!=WAVE_FORMAT_PCM && report)
            report("wfFormat.wFormatTag == WAVE_FORMAT_PCM",0xeb);
        const auto denominator=static_cast<std::uint32_t>(format.nChannels)*format.nSamplesPerSec;
        if (denominator==0) return {};
        format.cbSize=0;
        format.wBitsPerSample=static_cast<WORD>((format.nAvgBytesPerSec<<3)/denominator);
    }
    // 원본은 추가 코덱 자료를 읽지 않는다. 비PCM cbSize가 있으면 COM의 범위 밖 읽기를 막는다. PCM은 이 값을 무시한다.
    if (format.wFormatTag!=WAVE_FORMAT_PCM && format.cbSize!=0) return {};
    static_cast<void>(edition); // 헤더 처리와 보고 줄은 세 판본이 같다. 후속 판본 확장에서도 호출 계약을 유지한다.
    std::memcpy(wave.format.data(),&format,wave.format.size());
    if (mmioAscend(file.handle,&chunk,0)!=0) return {};
    chunk={};chunk.ckid=kData;
    if (mmioDescend(file.handle,&chunk,&riff,MMIO_FINDCHUNK)!=0 || chunk.cksize==0 || chunk.cksize>LONG_MAX) return {};
    wave.dataOffset=chunk.dwDataOffset;
    // Winmm의 버퍼/seek는 잘린 파일도 읽을 수 있다. 실제 디스크 크기로 검사하여 큰 확보와 범위 밖 읽기를 막는다.
    std::error_code error;const auto diskSize=std::filesystem::file_size(path,error);
    if (error || chunk.dwDataOffset>diskSize || chunk.cksize>diskSize-chunk.dwDataOffset) return {};
    wave.samples.resize(chunk.cksize);
    if (mmioRead(file.handle,reinterpret_cast<HPSTR>(wave.samples.data()),static_cast<LONG>(chunk.cksize))!=static_cast<LONG>(chunk.cksize)) return {};
    return wave;
}

// FindFirstFile는 디스크가 반환한 첫 일치를 사용한다. 주/보조를 합치거나 정렬하지 않는다.
SoundFileResolver MakeDiskSoundResolver(std::filesystem::path primary,std::filesystem::path secondary) {
    // 경로는 값으로 보관하므로 호출자 지역 변수가 사라져도 resolver는 살아 있다.
    return [primary=std::move(primary),secondary=std::move(secondary)](std::string_view pattern,bool alternate)->std::optional<std::filesystem::path> {
        const auto& root=alternate ? secondary : primary;
        if (root.empty()) return {};
        const auto query=root/std::filesystem::path(pattern);WIN32_FIND_DATAW found{};
        const auto handle=FindFirstFileW(query.c_str(),&found);
        if (handle==INVALID_HANDLE_VALUE) return {};
        FindClose(handle);
        if ((found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0) return {};
        return query.parent_path()/found.cFileName;
    };
}

// 각 위치에서 먼저 감쇠 파일을 찾고, 없으면 일반 파일을 찾는다. 이름 뒤 '*'도 원본 그대로다.
std::optional<std::filesystem::path> FindSoundFile(std::string_view name,std::string_view soundDirectory,
    std::string_view language,const SoundFileResolver& resolver) {
    if (!resolver || name.empty() || name.find('\0')!=std::string_view::npos) throw std::invalid_argument("소리 파일 조회 인자가 올바르지 않습니다");
    const auto dot=name.rfind('.');
    const auto stem=dot==std::string_view::npos ? name : name.substr(0,dot),extension=dot==std::string_view::npos ? std::string_view{} : name.substr(dot);
    const std::array<std::string,2> patterns={std::string(stem)+"-*"+std::string(extension),std::string(stem)+"*"+std::string(extension)};
    // 주 경로의 언어/기본 다음 보조 경로의 언어/기본을 탐색한다(총 최대 여덟 번).
    for (const bool alternate:{false,true}) {
        // 언어 디렉터리 다음 기본 소리 디렉터리를 탐색한다.
        for (const auto& directory:{Join(soundDirectory,language),std::string(soundDirectory)}) {
            // 감쇠 패턴 다음 일반 패턴을 탐색하며 첫 일치를 즉시 반환한다.
            for (const auto& pattern:patterns) if (auto file=resolver(Join(directory,pattern),alternate)) return file;
        }
    }
    return {};
}

// sscanf 반환이 1이면 뒤쪽 패턴의 성공 여부와 관계없이 숫자를 사용한다.
std::int32_t SoundFileAttenuation(std::string_view path) {
    long attenuation=0;
    return std::sscanf(std::string(path).c_str(),"%*[^-]-%ld.%*[^-]",&attenuation)==1 ? static_cast<std::int32_t>(attenuation) : 0;
}

// COM 포인터를 토큰에 매핑한다. DLL을 해제하기 전에 모든 버퍼와 장치를 해제해야 한다.
struct SoundDevice::Impl {
    SoundList& list;SoundState& state;SoundFileResolver resolver;std::string language,directory;SoundAssertReport report;
    HMODULE module{};LPDIRECTSOUND device{};LPDIRECTSOUNDBUFFER primary{};
    std::map<SoundBuffer,LPDIRECTSOUNDBUFFER> buffers;SoundBuffer next{1};
    // 클라이언트가 소유한 표/상태와 파일 경계를 보관한다. OS 자원은 확보하지 않는다.
    Impl(SoundList& sounds,SoundState& current,SoundFileResolver find,std::string lang,std::string dir,SoundAssertReport diagnostic)
        :list(sounds),state(current),resolver(std::move(find)),language(std::move(lang)),directory(std::move(dir)),report(std::move(diagnostic)) {}
    // 실제 버퍼를 새 토큰에 등록한다. 등록 실패 시 COM 참조를 반납한다.
    SoundBuffer Add(LPDIRECTSOUNDBUFFER buffer) {
        if (next==kSilentSoundBuffer) { buffer->Release();throw std::length_error("소리 버퍼 토큰이 소진됐습니다"); }
        const auto token=next++;
        try { buffers.emplace(token,buffer); } catch (...) { buffer->Release();throw; }
        return token;
    }
    // 알려진 토큰의 COM 버퍼를 찾는다. 0/표식/다른 장치의 값은 진단 예외다.
    LPDIRECTSOUNDBUFFER Get(SoundBuffer token) const {
        const auto found=buffers.find(token);
        if (found==buffers.end()) throw std::out_of_range("알 수 없는 소리 장치 버퍼입니다");
        return found->second;
    }
    // WAV를 찾아 0xe2 버퍼에 한 번 잠금/복사/해제한다. 잘린 파일과 COM 실패는 소리 없음 표식이다.
    SoundLoad Load(std::string_view name) {
        if (!device) return {};
        const auto path=FindSoundFile(name,directory,language,resolver);
        if (!path) return {kSilentSoundBuffer,0};
        const auto attenuation=SoundFileAttenuation(path->string());
        const auto wave=ReadSoundWave(*path,list.Edition(),report);
        if (!wave) return {kSilentSoundBuffer,attenuation};
        WAVEFORMATEX format{};std::memcpy(&format,wave->format.data(),wave->format.size());
        // 원본 PCM 파일 중 dropPiece-500.wav의 무시되는 cbSize는 20이다. PCM에는 추가 자료가 필요하지 않다.
        if (format.wFormatTag==WAVE_FORMAT_PCM) format.cbSize=0;
        DSBUFFERDESC desc{};desc.dwSize=sizeof(desc);desc.dwFlags=kEffectFlags;
        desc.dwBufferBytes=static_cast<DWORD>(wave->samples.size());desc.lpwfxFormat=&format;
        LPDIRECTSOUNDBUFFER buffer{};
        if (FAILED(device->CreateSoundBuffer(&desc,&buffer,nullptr))) return {kSilentSoundBuffer,attenuation};
        void* first{};void* second{};DWORD firstSize{},secondSize{};
        const auto locked=buffer->Lock(0,desc.dwBufferBytes,&first,&firstSize,&second,&secondSize,0);
        if (FAILED(locked)) { buffer->Release();return {kSilentSoundBuffer,attenuation}; }
        if (firstSize!=desc.dwBufferBytes) {
            buffer->Unlock(first,firstSize,second,secondSize);buffer->Release();return {kSilentSoundBuffer,attenuation};
        }
        std::memcpy(first,wave->samples.data(),firstSize);
        buffer->Unlock(first,firstSize,second,secondSize);
        return {Add(buffer),attenuation};
    }
};

// 장치보다 오래 사는 이름 표/상태를 참조하고 필수 파일 경계를 검사한다.
SoundDevice::SoundDevice(SoundList& list,SoundState& state,SoundFileResolver resolver,std::string language,std::string soundDirectory,SoundAssertReport report)
    :impl_(std::make_unique<Impl>(list,state,std::move(resolver),std::move(language),std::move(soundDirectory),std::move(report))) {
    if (!impl_->resolver) throw std::invalid_argument("소리 파일 resolver가 없습니다");
}
// 객체 소유 자원을 해제한다.
SoundDevice::~SoundDevice() { Shutdown(); }

// 초기화의 음악 부착은 후속이며 효과음 쪽 음질 하향/준비 상태는 원본대로다.
bool SoundDevice::Initialize(std::uintptr_t window,int quality) {
    auto& p=*impl_;p.list.Initialize();
    if (p.state.initialized) return false;
    if (window==0) throw std::invalid_argument("소리 장치 초기화에 창 핸들이 필요합니다");
    if (quality<0 || quality>3) quality=3;
    if (!p.module) p.module=LoadLibraryW(L"dsound.dll");
    if (!p.module) return false;
    // DLL 내보내기의 실제 stdcall 원형이다. 정적 dsound.lib 의존성은 만들지 않는다.
    using Create=HRESULT(WINAPI*)(LPCGUID,LPDIRECTSOUND*,LPUNKNOWN);
    const auto create=reinterpret_cast<Create>(GetProcAddress(p.module,"DirectSoundCreate"));
    if (!create) return false;
    const auto failed=[&]() { Shutdown();p.state.enabled=false;return false; }; // COM 생성 이후의 실패를 정리한다.
    if (FAILED(create(nullptr,&p.device,nullptr))) return failed();
    p.state.device=true;
    if (FAILED(p.device->SetCooperativeLevel(reinterpret_cast<HWND>(window),DSSCL_PRIORITY))) return failed();
    DSBUFFERDESC desc{};desc.dwSize=sizeof(desc);desc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    if (FAILED(p.device->CreateSoundBuffer(&desc,&p.primary,nullptr))) return failed();
    // 3=stereo16, 2=stereo8, 1=mono8. 0은 SetFormat 없이 장치 기본 형식을 유지한다.
    while (quality>0) {
        WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_PCM;format.nSamplesPerSec=22050;
        format.nChannels=quality==1 ? 1 : 2;format.wBitsPerSample=quality==3 ? 16 : 8;
        format.nBlockAlign=static_cast<WORD>(format.nChannels*format.wBitsPerSample/8);
        format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
        if (SUCCEEDED(p.primary->SetFormat(&format))) break;
        --quality;
    }
    p.state.initialized=true;return true;
}

// 이름/사슬 필드를 유지하여 다음 초기화/재생에서 같은 이름 표를 재사용한다.
void SoundDevice::Shutdown() {
    auto& p=*impl_;
    // 확보한 모든 효과음 버퍼를 반납한다(복제 버퍼 포함).
    for (const auto& [token,buffer]:p.buffers) { static_cast<void>(token);buffer->Stop();buffer->Release(); }
    p.buffers.clear();
    if (p.primary) { p.primary->Release();p.primary=nullptr; }
    if (p.device) { p.device->Release();p.device=nullptr; }
    if (p.module) { FreeLibrary(p.module);p.module=nullptr; }
    // 표의 각 가변 길이 항목을 순회하며 버퍼만 비운다. 이름/머리 크기가 다음 항목 위치를 정한다.
    for (std::uint32_t offset=0;offset+kSoundHeaderBytes<kSoundListBytes;) {
        const auto handle=kSoundListBase+offset;const auto name=p.list.Name(handle);
        if (name.empty()) break;
        p.list.SetField(handle,SoundField::Buffer,0);
        offset+=kSoundHeaderBytes+static_cast<std::uint32_t>(name.size())+1;
    }
    p.state.initialized=p.state.device=false;p.state.playing=0;
}

// 각 경계는 토큰을 실제 COM 포인터로 바꾸고 원본 메서드를 호출한다. load도 이 객체의 실제 WAV 적재를 쓴다.
SoundDeviceHooks SoundDevice::Hooks() {
    auto* p=impl_.get();SoundDeviceHooks hooks;
    hooks.status=[p](SoundBuffer token) { DWORD status=0;p->Get(token)->GetStatus(&status);return status; }; // 재생/손실 상태를 조회한다.
    hooks.play=[p](SoundBuffer token,std::uint32_t flags) { p->Get(token)->Play(0,0,flags); }; // 원본 반복 플래그를 전달한다.
    hooks.setPosition=[p](SoundBuffer token,std::uint32_t position) { p->Get(token)->SetCurrentPosition(position); }; // 재생 위치를 설정한다.
    hooks.setVolume=[p](SoundBuffer token,std::int32_t volume) { return static_cast<std::int32_t>(p->Get(token)->SetVolume(volume)); }; // 음량 HRESULT를 반환한다.
    hooks.setPan=[p](SoundBuffer token,std::int32_t pan) { return static_cast<std::int32_t>(p->Get(token)->SetPan(pan)); }; // 좌우 HRESULT를 반환한다.
    hooks.stop=[p](SoundBuffer token) { p->Get(token)->Stop(); }; // 버퍼 재생을 멈춘다.
    // 복제 COM 실패는 토큰을 쓰지 않는다. 장치/원본 버퍼가 없으면 HRESULT 실패다.
    hooks.duplicate=[p](SoundBuffer token,SoundBuffer& copy)->std::int32_t {
        if (!p->device || token==0 || token==kSilentSoundBuffer) return static_cast<std::int32_t>(DSERR_INVALIDPARAM);
        LPDIRECTSOUNDBUFFER buffer{};const auto result=p->device->DuplicateSoundBuffer(p->Get(token),&buffer);
        if (SUCCEEDED(result)) copy=p->Add(buffer);
        return static_cast<std::int32_t>(result);
    };
    hooks.load=[p](std::string_view name) { return p->Load(name); }; // 실제 파일 선택/읽기/버퍼 확보다.
    return hooks;
}
// 파일 경계와 기록/잠금 관찰자는 호출자에게 두고 COM 버퍼 경계만 이 장치에 연결한다.
MusicChannelHooks SoundDevice::BindMusicBuffers(MusicChannelHooks files) {
    auto* p=impl_.get();
    files.setVolume=[p](SoundBuffer token,std::int32_t volume) { return static_cast<std::int32_t>(p->Get(token)->SetVolume(volume)); }; // 음악 음량을 적용한다.
    files.stop=[p](SoundBuffer token) { p->Get(token)->Stop(); }; // 음악 버퍼를 정지한다.
    files.release=[p](SoundBuffer token) {
        const auto result=p->Get(token)->Release();p->buffers.erase(token);return static_cast<std::int32_t>(result);
    }; // 해제한 토큰을 제거하여 장치 종료에서 두 번 반납하지 않는다.
    if (!files.report) files.report=p->report;
    return files;
}
// 생성한 음악 버퍼도 장치 토큰 표가 소유한다. 형식/커서/잠금 span은 호스트 포인터를 raw 채널에 넣지 않는다.
MusicBufferHooks SoundDevice::MusicBuffers() {
    auto* p=impl_.get();MusicBufferHooks hooks;
    // 파일 형식은 PCM의 추가 크기만 보정하고 원본 기능 비트/버퍼 크기를 그대로 전달한다.
    hooks.create=[p](std::span<const std::uint8_t> raw,std::uint32_t flags,std::uint32_t bytes,SoundBuffer& token)->std::int32_t {
        if (!p->device) return static_cast<std::int32_t>(DSERR_NODRIVER);
        if (raw.size()!=18) throw std::invalid_argument("음악 형식은 WAVEFORMATEX 18바이트여야 합니다");
        WAVEFORMATEX format{};std::memcpy(&format,raw.data(),raw.size());
        if (format.wFormatTag==WAVE_FORMAT_PCM) format.cbSize=0;
        // 추가 코덱 바이트를 소유하지 않으므로 COM이 형식 구조체 밖을 읽는 입력은 거부한다.
        else if (format.cbSize!=0) return static_cast<std::int32_t>(DSERR_BADFORMAT);
        DSBUFFERDESC desc{};desc.dwSize=sizeof(desc);desc.dwFlags=flags;desc.dwBufferBytes=bytes;desc.lpwfxFormat=&format;
        LPDIRECTSOUNDBUFFER buffer{};const auto result=p->device->CreateSoundBuffer(&desc,&buffer,nullptr);
        if (SUCCEEDED(result)) token=p->Add(buffer);
        return static_cast<std::int32_t>(result);
    };
    hooks.status=[p](SoundBuffer token,std::uint32_t& state) { DWORD value{};const auto result=p->Get(token)->GetStatus(&value);state=value;return static_cast<std::int32_t>(result); }; // 상태와 HRESULT를 모두 반환한다.
    hooks.restore=[p](SoundBuffer token) { return static_cast<std::int32_t>(p->Get(token)->Restore()); }; // 잃은 음악 버퍼를 복구한다.
    hooks.cursor=[p](SoundBuffer token,std::uint32_t& play,std::uint32_t& write) {
        DWORD first{},second{};const auto result=p->Get(token)->GetCurrentPosition(&first,&second);play=first;write=second;return static_cast<std::int32_t>(result);
    }; // 장치의 재생/쓰기 커서를 받는다.
    hooks.lock=[p](SoundBuffer token,std::uint32_t offset,std::uint32_t count,MusicBufferRegions& regions) {
        void* first{};void* second{};DWORD firstSize{},secondSize{};
        const auto result=p->Get(token)->Lock(offset,count,&first,&firstSize,&second,&secondSize,0);
        if (SUCCEEDED(result)) regions={{static_cast<std::uint8_t*>(first),firstSize},{static_cast<std::uint8_t*>(second),secondSize}};
        return static_cast<std::int32_t>(result);
    }; // 링 끝에서 나뉜 두 구간을 호스트 span으로 만든다.
    hooks.unlock=[p](SoundBuffer token,MusicBufferRegions regions) {
        return static_cast<std::int32_t>(p->Get(token)->Unlock(regions.first.data(),static_cast<DWORD>(regions.first.size()),regions.second.data(),static_cast<DWORD>(regions.second.size())));
    }; // 받은 포인터/크기를 그대로 반납한다.
    hooks.play=[p](SoundBuffer token,std::uint32_t flags) { return static_cast<std::int32_t>(p->Get(token)->Play(0,0,flags)); }; // 음악 버퍼 반복 재생의 실패도 반환한다.
    return hooks;
}
}
