// Windows10/11의 한글·보충 문자 경로를 기존 설정 로더의 서명/BOM 경계에 연결한다.
#include "client/ClientOptionsFile.h"
#include "o/BaseFile.h"
#include "o/RawFileName.h"
#include <stdexcept>

namespace netstorm::client { // 기존 영어/서유럽 설정의 디스크 바이트는 그대로 유지한다.
namespace { // 폴백은 실제 UTF-8에만 허용하며 잘못된 입력을 숨기지 않는다.
bool ValidOptionsUtf8(std::string_view text) { // 최단 표현·서로게이트·최대 코드 포인트를 검사한다.
    for (std::size_t at=0;at<text.size();) { // 코드 포인트마다 필요한 후속 바이트만 소비한다.
        const auto lead=static_cast<unsigned char>(text[at++]);if (lead<0x80) continue;
        std::uint32_t value{},minimum{};unsigned count{}; // 현재 코드 포인트/최단 값/남은 바이트 수다.
        if (lead>=0xc2 && lead<=0xdf) { value=lead&0x1fU;minimum=0x80;count=1; }
        else if (lead>=0xe0 && lead<=0xef) { value=lead&0x0fU;minimum=0x800;count=2; }
        else if (lead>=0xf0 && lead<=0xf4) { value=lead&7U;minimum=0x10000;count=3; }
        else return false;
        if (count>text.size()-at) return false;
        for (unsigned byte=0;byte<count;++byte) { const auto next=static_cast<unsigned char>(text[at++]);if ((next&0xc0U)!=0x80U) return false;value=(value<<6)|(next&0x3fU); } // 현재 문자 후속 바이트만 조립한다.
        if (value<minimum || value>0x10ffffU || (value>=0xd800U && value<=0xdfffU)) return false;
    }
    return true;
}
}
std::vector<std::uint8_t> EncodeClientOptions(std::string_view text) { // 원본ANSI와 다른 UTF-8 폴백은 현대 Windows 경로 호환을 위한 명시 차이다.
    try { return o::EncodeConfigFile(text); }
    catch (const std::invalid_argument&) {
        if (!ValidOptionsUtf8(text) || !text.starts_with(o::kConfigSignature)) throw;
        std::vector<std::uint8_t> bytes(o::kConfigSignature.begin(),o::kConfigSignature.end());
        const auto body=text.substr(o::kConfigSignature.size());
        if (!body.starts_with("\xef\xbb\xbf")) bytes.insert(bytes.end(),{0xef,0xbb,0xbf}); // XOR 탐지에 필요한 원래 서명은 파일 앞에 남긴다.
        bytes.insert(bytes.end(),body.begin(),body.end());o::ApplyXor(bytes);return bytes;
    }
}
namespace { // 실제 경로 작성과 독립인 인코딩/changed의 같은 기존 저장 본체다.
void SaveOptionsName(o::ConfigInterface& config,const o::ConfigInterface::FileWriter& writer,std::string_view name,o::RawFileName* names=nullptr) { // 실제440f30의459b20 한 칸을 쓰며 독립 값형 호출자는 기존 계약을 유지한다.
    const bool changed=config.Configuration().changed;
    try {
        const auto section=names ? names->Name(name):name;const auto bytes=EncodeClientOptions(config.SaveText(section));config.Configuration().changed=changed; // 파일명 getter 내부 뷰로 같은 설정 section을 고르며 writer는 부모의 지역 전체 이름을 받는다.
        writer(name,bytes);config.Configuration().changed=false;
    } catch (...) { config.Configuration().changed=changed;throw; }
}
}
void SaveClientOptions(o::ConfigInterface& config,const o::ConfigInterface::FileWriter& writer) { SaveOptionsName(config,writer,"d\\options.cfg"); } // 기존 값형 호출자의 이름 계약을 유지한다.
void SaveClientOptions(o::ConfigInterface& config,const o::ConfigInterface::FileWriter& writer,o::RawFileName& names,std::optional<std::string_view> directory) { // 원문441d10의 작성→지역 복사→직렬화/쓰기 순서다.
    const std::array<o::FileNamePart,4> parts{{{-2,directory.value_or("")},{-2,""},{-2,"d"},{-3,"options.cfg"}}}; // NULL일 때 첫 명시 root 부품만 생략한다.
    const std::string name(names.Format(std::span(parts).subspan(directory ? 0 : 1))); // 원문 local_100처럼 다음 열 번의 고리 사용에도 살아 있는 소유 복사다.
    SaveOptionsName(config,writer,name,&names);
}
}
