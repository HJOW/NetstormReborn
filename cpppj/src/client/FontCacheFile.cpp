// 현재 owner와 256개 글리프를 파일 전체 snapshot으로 대체하지 않는다.
#include "client/FontCacheFile.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client { // 원문004a3240·004a3090의 순차 파일 부모다.
namespace { // 도달한 장치와 리틀엔디언 필드만 해석한다.
constexpr std::array<std::uint8_t,16> kSignature{'B','i','t','m','a','p','F','o','n','t','D','a','t','a',26,0}; // 원문 서명16BYTE다.
constexpr std::array<std::uint8_t,4> kCharset{0,1,0,0}; // writer가 사용하는256의 리틀엔디언 DWORD다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("글꼴 캐시의 선택된 파일 장치 미연결");return function; } // 미선택 장치는 미리 요구하지 않는다.
std::uint32_t Word(std::span<const std::uint8_t> bytes) { // 정렬/호스트 엔디언과 무관하게 같은 네BYTE를 읽는다.
    std::uint32_t value{};for (std::uint32_t index=0;index<4;++index) value|=std::uint32_t(bytes[index])<<(index*8);return value; // 표의 부호 비트도 원문DWORD로 유지한다.
}
}
bool ReadFontCacheFile(o::RawBaseFileState& state,std::string_view name,std::span<std::uint8_t,kFontCacheOwnerBytes> owner,
    const o::RawBaseFileIOHooks& io,const FontCacheFileHooks& hooks,std::span<std::uint8_t> scratch,
    std::array<std::uint8_t,16>& signature,std::array<std::uint8_t,4>& charset) { // 부분 쓰기를 같은 장기 owner에 남긴다.
    if (!Need(hooks.exists)(name)) return false;
    state.key=0;state.keyLength=0;Need(hooks.open)(state,name,0,1);bool loaded{}; // Open 완료 전에는 원문 cleanup 상태에 들어가지 않는다.
    try {
        if (state.file) {
            o::ReadBaseFile(state,signature,-1,io,scratch);signature.back()=0;
            if (signature==kSignature) {
                o::ReadBaseFile(state,owner.first<4>(),-1,io,scratch);
                if (Word(owner.first<4>())==1) {
                    for (std::size_t offset=4;offset<16;offset+=4) o::ReadBaseFile(state,owner.subspan(offset,4),-1,io,scratch); // 세 머리 DWORD는 각각 별도 Read다.
                    o::ReadBaseFile(state,charset,-1,io,scratch);if (Word(charset)!=256) Need(hooks.assertion)(0xb66);
                    for (std::size_t offset=16;offset<owner.size();offset+=1024) o::ReadBaseFile(state,owner.subspan(offset,1024),-1,io,scratch); // 네 표를 같은 owner에 직접 기록한다.
                    for (std::uint32_t code=0;code<256;++code) { // 길이0도 원문 할당/설치와 Read 호출을 생략하지 않는다.
                        const auto length=Word(owner.subspan(0xc10+code*4,4));const auto bytes=Need(hooks.allocate)(code,length);
                        if (bytes.size()<length) throw std::out_of_range("글꼴 캐시 글리프 할당의 실제 읽기 범위");o::ReadBaseFile(state,bytes.first(length),-1,io,scratch);
                    }
                    loaded=true;
                }
            }
            o::CloseBaseFile(state,io); // 서명/버전 실패에서도 명시 Close 뒤 소멸한다.
        }
    } catch (...) { o::DestructBaseFile(state,io);throw; } // 앞선 owner/글리프 설치를 보존하며 정상 소멸 실패와 구별한다.
    o::DestructBaseFile(state,io);return loaded;
}
bool WriteFontCacheFile(o::RawBaseFileState& state,std::string_view name,std::span<const std::uint8_t,kFontCacheOwnerBytes> owner,
    const o::RawBaseFileIOHooks& io,const FontCacheFileHooks& hooks) { // 현재 표/글리프를 원문 순서로 쓰며 부분 fwrite 반환은 BaseFile처럼 무시한다.
    state.key=0;state.keyLength=0;Need(hooks.open)(state,name,2,1);bool written{};
    try {
        if (state.file) {
            o::WriteBaseFile(state,kSignature,-1,io);
            for (std::size_t offset=0;offset<16;offset+=4) o::WriteBaseFile(state,owner.subspan(offset,4),-1,io); // 네 머리 DWORD를 각각 쓴다.
            o::WriteBaseFile(state,kCharset,-1,io);
            for (std::size_t offset=16;offset<owner.size();offset+=1024) o::WriteBaseFile(state,owner.subspan(offset,1024),-1,io); // 네 표의 쓰기 순서다.
            for (std::uint32_t code=0;code<256;++code) { // 현재 크기 표와 같은 code의 포인터를 각 순서에서 읽는다.
                const auto length=Word(owner.subspan(0xc10+code*4,4));const auto bytes=Need(hooks.glyph)(code,length);
                if (bytes.size()<length) throw std::out_of_range("글꼴 캐시 글리프의 실제 쓰기 범위");o::WriteBaseFile(state,bytes.first(length),-1,io);
            }
            o::CloseBaseFile(state,io);written=true;
        }
    } catch (...) { o::DestructBaseFile(state,io);throw; } // 원문 funclet은 Close 실패 때 같은 File 정리를 한 번 더 시도한다.
    o::DestructBaseFile(state,io);return written;
}
}
