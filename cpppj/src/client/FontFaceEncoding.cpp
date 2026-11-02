// 새 인코딩 호환은 글꼴 이름에만 적용하며 원문 글리프 0..255/CHFNT 형식은 바꾸지 않는다.
#include "client/FontFaceEncoding.h"
#include <limits>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace netstorm::client { //실제 Windows NLS의 현재 ACP와 명시 UTF-8만 같은 문자열 경계에 사용한다.
namespace { //변환 실패를 임의 글꼴/물음표 성공으로 바꾸지 않는다.
//문자열의 원문 첫NUL까지만 보며 Win32 signed 길이 범위를 검사한다.
std::string_view FaceText(std::string_view value) { value=value.substr(0,value.find('\0'));if (value.size()>static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("글꼴 이름 Win32 길이 범위");return value; }
//UTF-8 ACP도 별도 flags/default 포인터 규칙을 사용하도록 현재 코드 페이지를 확정한다.
UINT FacePage(std::uint32_t page) { return page ? static_cast<UINT>(page):GetACP(); }
//잘린DBCS/잘못된UTF-8을 U+FFFD로 바꾸지 않고 실제 UTF-16 문자로 읽는다.
std::wstring FaceWide(std::string_view text,UINT page) {
    text=FaceText(text);if (text.empty()) return {};const auto count=MultiByteToWideChar(page,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0);
    if (!count) throw std::invalid_argument("글꼴 이름의 현재 코드 페이지 바이트 형식");std::wstring output(static_cast<std::size_t>(count),L'\0');
    if (MultiByteToWideChar(page,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),output.data(),count)!=count) throw std::runtime_error("글꼴 이름 UTF-16 변환 실패");return output;
}
//ACP에서 표현할 수 없는 문자와 best-fit을 거부한다. UTF-8/GB18030은 NLS의 별도 매개변수 계약을 따른다.
std::string FaceNarrow(std::wstring_view text,UINT page) {
    if (text.empty()) return {};const bool unicode=page==CP_UTF8 || page==54936;BOOL substituted=FALSE;const auto flags=unicode ? WC_ERR_INVALID_CHARS:WC_NO_BEST_FIT_CHARS;
    const auto count=WideCharToMultiByte(page,flags,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,unicode ? nullptr:&substituted);
    if (!count || substituted) throw std::invalid_argument("글꼴 이름의 현재 코드 페이지 표현 불가");std::string output(static_cast<std::size_t>(count),'\0');substituted=FALSE;
    if (WideCharToMultiByte(page,flags,text.data(),static_cast<int>(text.size()),output.data(),count,nullptr,unicode ? nullptr:&substituted)!=count || substituted) throw std::invalid_argument("글꼴 이름 코드 페이지의 손실 변환");return output;
}
}
//Config/디스크에 ACP 바이트를 UTF-8로 오인해 전달하지 않는다.
std::string FontFaceToUtf8(std::string_view ansi,std::uint32_t codePage) { return FaceNarrow(FaceWide(ansi,FacePage(codePage)),CP_UTF8); }
//설정 내부의 UTF-8을 GDI A/원문 현재이름의 ACP로 되돌린다.31BYTE 절삭은 기존 FontStore의 원문 위치에 맡긴다.
std::string FontFaceFromUtf8(std::string_view utf8,std::uint32_t codePage) { return FaceNarrow(FaceWide(utf8,CP_UTF8),FacePage(codePage)); }
//미완성 원문 바이트는 디스크 경로 성공으로 합성하지 않고 뒤 raw TAFF/GDI 경로를 허용한다.
std::optional<std::string> FontCacheDiskName(std::string_view ansi,std::uint32_t codePage) { try { return FontFaceToUtf8(ansi,codePage); }catch (const std::invalid_argument&) { return {}; } }
}
