// 원본 정규식의 부분 검색·greedy 캡처와 그림 선택/옵션 우선순위를 유지한다.
#include "client/StyleTextMarkup.h"
#include "client/Sound.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <optional>
#include <stdexcept>

namespace netstorm::client {
namespace {
// Regexp::Get가 각 캡처에서 복사하는 최대 바이트 수다. 프레임의 원본 지역 배열도 80바이트다.
constexpr std::size_t kCaptureBytes=80;
// 패치의 검사 순서다. CD StyleText에서는 I/T 두 형식만 검사한다.
constexpr std::array<char,6> kCommandOrder{'I','S','T','C','B','E'};
// 원본 E 좌표의 허용 범위다. 어느 한 축만 이 범위에 있어도 두 값을 함께 적용한다.
constexpr std::int32_t kOffsetMinimum=-4,kOffsetMaximum=4;
// CD 다중 줄은 W 두 캡처를 소리 이름 첫 캡처로 소비한다. 제한 그림은 너비 99999/높이 12에 맞춘다.
constexpr std::array<char,3> kCdScrollingOrder{'I','W','T'};
constexpr std::int32_t kRestrictedWidth=99999,kRestrictedHeight=12;
// 원본 SHP +1a의 signed 높이 허용 상한이다. 두 판본은 같은 제한 맞춤에도 서로 다른 문턱을 사용한다.
constexpr std::int16_t kPatchHeightLimit=16,kCdHeightLimit=12;

// 두 캡처는 입력 버퍼를 참조한다. 단일 캡처 형식에서는 second를 사용하지 않는다.
struct CommandMatch { /* first: 구간 또는 연결 목록의 첫 항목. */ /* second: 두번째. */ std::string_view first,second; };

// 원본 [^.\r\n ]+에 들어갈 수 있는 BYTE다. 탭은 캡처에 포함되며 첫 NUL은 호출자가 미리 자른다.
bool IsToken(char value) { return value!='.' && value!='\r' && value!='\n' && value!=' '; }

// 원본 공백 반복은 ASCII space만 소비한다. 일반 isspace로 확대하지 않는다.
std::size_t Spaces(std::string_view text,std::size_t position) {
    // greedy 공백 반복의 가장 긴 끝을 찾는다.
    while (position<text.size() && text[position]==' ') ++position;
    return position;
}

// 원본 캡처의 가장 긴 끝을 찾는다. 이후 형식이 실패하면 호출자가 끝점을 역순으로 줄인다.
std::size_t TokenEnd(std::string_view text,std::size_t position) {
    // 정규식의 제외 클래스와 동일한 BYTE만 연속으로 소비한다.
    while (position<text.size() && IsToken(text[position])) ++position;
    return position;
}

// ' *. *([^...]+)'의 첫 캡처다. 원본의 점은 이스케이프되지 않아 임의 BYTE 한 개를 소비한다.
std::optional<std::string_view> Separated(std::string_view text,std::size_t position) {
    auto separator=Spaces(text,position);
    // 앞 공백은 greedy 끝부터 되돌린다. 점으로 space/CR/LF를 소비하는 원본 형식도 유지한다.
    while (true) {
        if (separator<text.size()) {
            const auto begin=Spaces(text,separator+1);
            const auto end=TokenEnd(text,begin);
            if (end>begin) return text.substr(begin,end-begin);
        }
        if (separator==position) return std::nullopt;
        --separator;
    }
}

// ROM의 제한된 형식을 부분 검색한다. 전체 Regexp 엔진이나 다른 HTML 문법을 해석하지 않는다.
std::optional<CommandMatch> Match(std::string_view text,char command) {
    // 정규식의 첫 일치 위치를 찾는다. 형식에 ^/$가 없으므로 문자열 전체 일치를 요구하지 않는다.
    for (std::size_t mark=0;mark<text.size();++mark) {
        if (text[mark]!=command) continue;
        const auto begin=Spaces(text,mark+1);
        if (command=='T') {
            auto end=begin;
            // T는 숫자 캡처만 허용한다. 부호/다른 글자는 뒤의 부분 검색 후보로 남긴다.
            while (end<text.size() && text[end]>='0' && text[end]<='9') ++end;
            if (end>begin) return CommandMatch{text.substr(begin,end-begin),{}};
        } else if (command=='I' || command=='E' || command=='W') {
            auto end=TokenEnd(text,begin);
            // 첫 캡처의 가장 긴 길이부터 두 번째 캡처가 성립하는 첫 조합을 선택한다.
            while (end>begin) {
                if (const auto second=Separated(text,end)) return CommandMatch{text.substr(begin,end-begin),*second};
                --end;
            }
        } else if (const auto first=Separated(text,mark+1)) return CommandMatch{*first,{}};
    }
    return std::nullopt;
}

// 원본 Get(capture,0,80,0)와 같이 캡처의 첫 80바이트만 숫자/이름 처리에 사용한다.
std::string_view Capture(std::string_view value) { return value.substr(0,kCaptureBytes); }

// CRT 정수 변환은 같은 프로젝트의 공백/부호/숫자/DWORD 감김 복원을 사용한다.
std::int32_t Number(std::string_view value) { return o::ConfigParseLong(Capture(value)); }

// 프레임 글자 구간 첫 인덱스와 숫자 suffix의 원본 DWORD 덧셈이다.
std::int32_t Add(std::int32_t first,std::int32_t second) {
    return std::bit_cast<std::int32_t>(std::uint32_t(first)+std::uint32_t(second));
}
}

// 원본 전역 대신 호출자가 같은 옵션/타입/조각/출력 저장소를 소유한다.
StyleTextMarkup::StyleTextMarkup(StyleTextState& fragments,const Markup& pictures,StyleTextMarkupState& state,
    StyleTextMarkupHooks hooks,o::OriginalEdition edition)
    :fragments_(fragments),pictures_(pictures),state_(state),hooks_(std::move(hooks)),edition_(edition) {}

// 타입 번호 범위는 실제 자산 공급 경계에서 검사하고 현재 필드를 반환한다.
StyleTextMarkupType StyleTextMarkup::Type(std::int32_t type) const {
    if (!hooks_.type) throw std::invalid_argument("StyleText 대괄호 현재 타입 경계 필요");
    return hooks_.type(type);
}

// I 명령은 타입 번호 0을 이름으로 다시 조회하고 -1일 때만 그림 생성을 생략한다.
void StyleTextMarkup::Picture(std::string_view typeExpression,std::string_view frameExpression,
    std::uint32_t& head,std::uint32_t metadata) const {
    const auto name=Capture(typeExpression);
    const auto expression=Capture(frameExpression);
    // 원본은 80바이트 배열에 NUL까지 strcpy한다. 경계 밖 쓰기를 발생시키는 캡처만 거부한다.
    if (expression.size()>=kCaptureBytes) throw std::length_error("StyleText 그림 프레임 지역 배열 범위 초과");
    auto type=Number(name);
    if (!type) {
        if (!hooks_.findType) throw std::invalid_argument("StyleText 대괄호 타입 이름 경계 필요");
        type=hooks_.findType(name);
    }
    if (type==-1) return;
    const auto first=expression.front();
    std::int32_t frame=0;
    bool fit=false,mapped=false;
    if (first=='*' || first=='#' || first=='&') {
        fit=first=='#' || first=='&';
        mapped=first=='&';
        const auto gump=Type(type).gumpFrame;
        frame=edition_==o::OriginalEdition::Patch1078
            ? (gump==-1 ? Type(type).defaultFrame : Type(type).gumpFrame)
            : (gump==-1 ? Type(type).defaultFrame : gump);
    } else {
        auto suffix=expression;
        const auto letter=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(first));
        if (letter>='A') {
            const auto view=Type(type);
            if (view.letterKinds<0 || view.letterKinds>16) throw std::out_of_range("StyleText 프레임 글자 표 범위 초과");
            if (letter<'A'+view.letterKinds) {
                frame=edition_==o::OriginalEdition::Patch1078 ? Type(type).first[letter-'A'] : view.first[letter-'A'];
                suffix.remove_prefix(1);
            }
        }
        frame=Add(frame,Number(suffix));
    }
    const auto count=Type(type).frameCount;
    // 원본 상한은 count-1이 아닌 count다. count 프레임은 이후 Markup::Lookup에서 무자산이 된다.
    if (std::min(frame,count)<0) frame=0;
    else if (edition_==o::OriginalEdition::Patch1078) {
        if (frame>=Type(type).frameCount) frame=Type(type).frameCount;
    } else if (frame>=count) frame=count;
    const auto picture=pictures_.NewPicture(static_cast<std::uint32_t>(type),static_cast<std::uint32_t>(frame),metadata);
    pictures_.SetMapped(picture,mapped ? 1U : 0U);
    if (fit) pictures_.Fit(picture,state_.fitWidth,state_.fitHeight);
    AppendStyleTextFragment(fragments_,head,picture);
}

// 형식 일치 뒤 수행하는 태그/색/그림자/소리다. 비활성 옵션도 호출자는 이미 소비한 것으로 처리한다.
void StyleTextMarkup::Option(char code,std::string_view first,std::string_view second,
    std::array<std::uint32_t,8>& context,std::uint32_t& metadata,bool unconditionalSound) const {
    switch (code) {
    case 'S':
        if (unconditionalSound || state_.allowSounds) {
            if (!hooks_.sound) throw std::invalid_argument("StyleText 대괄호 소리 경계 필요");
            hooks_.sound(Capture(first));
        }
        break;
    case 'T': metadata=static_cast<std::uint32_t>(Number(first));break;
    case 'C': if (state_.allowForegroundColor) context[2]=static_cast<std::uint32_t>(Number(first));break;
    case 'B': if (state_.allowBackgroundColor) context[5]=static_cast<std::uint32_t>(Number(first));break;
    case 'E':
        if (state_.allowOffset) {
            const auto x=Number(first),y=Number(second);
            if ((x>=kOffsetMinimum && x<=kOffsetMaximum) || (y>=kOffsetMinimum && y<=kOffsetMaximum)) {
                if (!hooks_.black) throw std::invalid_argument("StyleText 대괄호 검정색 경계 필요");
                context[0]|=2U;context[5]=hooks_.black();
                context[6]=static_cast<std::uint32_t>(x);context[7]=static_cast<std::uint32_t>(y);
            }
        }
        break;
    default: break;
    }
}

// 닫는 대괄호 내부의 첫 NUL까지만 읽고 우선순위상 처음 성립한 형식을 처리한다.
void StyleTextMarkup::Apply(std::string_view command,std::array<std::uint32_t,8>& context,
    std::uint32_t& head,std::uint32_t& metadata) const {
    command=command.substr(0,command.find('\0'));
    const bool patch=edition_==o::OriginalEdition::Patch1078;
    // 형식이 성립하면 옵션이 비활성이어도 다음 형식을 시도하지 않고 소비한다.
    for (const auto code:kCommandOrder) {
        if (!patch && code!='I' && code!='T') continue;
        const auto match=Match(command,code);
        if (!match) continue;
        if (code=='I') Picture(match->first,match->second,head,metadata);
        else Option(code,match->first,match->second,context,metadata);
        return;
    }
}

// 다중 줄의 프레임 식은 *와 #만 특수 처리한다. 소비한 접두 뒤의 #가 맞춤을 결정한다.
void StyleTextMarkup::ScrollingPicture(std::string_view typeExpression,std::string_view frameExpression,
    const std::function<bool()>& restricted,
    const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& appendPicture) const {
    const bool patch=edition_==o::OriginalEdition::Patch1078;
    const auto name=Capture(typeExpression),expression=Capture(frameExpression);
    if (expression.size()>=kCaptureBytes) throw std::length_error("다중 줄 그림 프레임 지역 배열 범위 초과");
    auto type=Number(name);
    if (!type) {
        if (!hooks_.findType) throw std::invalid_argument("다중 줄 그림 타입 이름 경계 필요");
        type=hooks_.findType(name);
    } else if (patch) {
        if (!hooks_.typeCount) throw std::invalid_argument("다중 줄 그림 현재 타입 수 경계 필요");
        if (type>=hooks_.typeCount()) return;
    }
    if (type==-1) return;
    std::int32_t frame=0;
    auto suffix=expression;
    if (expression.front()=='*' || expression.front()=='#') {
        const auto gump=Type(type).gumpFrame;
        frame=patch ? (gump==-1 ? Type(type).defaultFrame : Type(type).gumpFrame)
                    : (gump==-1 ? Type(type).defaultFrame : gump);
        suffix.remove_prefix(1);
    } else {
        const auto letter=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(expression.front()));
        if (letter>='A') {
            const auto view=Type(type);
            if (view.letterKinds<0 || view.letterKinds>16) throw std::out_of_range("다중 줄 그림 글자 표 범위 초과");
            if (letter<'A'+view.letterKinds) {
                frame=patch ? Type(type).first[letter-'A'] : view.first[letter-'A'];suffix.remove_prefix(1);
            }
        }
        frame=Add(frame,Number(suffix));
    }
    std::int32_t width=0,height=0;
    if (!suffix.empty() && suffix.front()=='#') { width=state_.fitWidth;height=state_.fitHeight; }
    const auto count=Type(type).frameCount;
    if (std::min(frame,count)<0) frame=0;
    else if (patch) { if (frame>=Type(type).frameCount) frame=Type(type).frameCount; }
    else if (frame>=count) frame=count;
    if (!restricted) throw std::invalid_argument("다중 줄 그림 현재 제한 필드 경계 필요");
    if (restricted()) {
        if (!hooks_.typeCount) throw std::invalid_argument("다중 줄 제한 그림 타입 수 경계 필요");
        if (type<=0 || type>=hooks_.typeCount() || frame<0 || frame>=Type(type).frameCount) return;
        if (!hooks_.frameHeight) throw std::invalid_argument("다중 줄 제한 그림 SHP 헤더 경계 필요");
        const auto heightWord=hooks_.frameHeight(type,frame);
        if (!heightWord || *heightWord>(patch ? kPatchHeightLimit : kCdHeightLimit)) return;
        width=kRestrictedWidth;height=kRestrictedHeight;
    }
    if (!appendPicture) throw std::invalid_argument("다중 줄 그림 실제 버퍼 추가 경계 필요");
    appendPicture(static_cast<std::uint32_t>(type),static_cast<std::uint32_t>(frame),width,height);
}

// CD W는 한 줄 파서에 없고 다중 줄에서만 두 캡처를 요구한다. 같은 I 그림 우선순위를 유지한다.
void StyleTextMarkup::ApplyScrolling(std::string_view command,std::array<std::uint32_t,8>& context,
    std::uint32_t& metadata,const std::function<bool()>& restricted,
    const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& appendPicture) const {
    command=command.substr(0,command.find('\0'));
    const bool patch=edition_==o::OriginalEdition::Patch1078;
    const auto order=patch ? std::span<const char>(kCommandOrder) : std::span<const char>(kCdScrollingOrder);
    // 처음 성립한 명령은 무효 타입/그림 제한/옵션 비활성에서도 다음 형식으로 넘어가지 않는다.
    for (const auto code:order) {
        const auto match=Match(command,code);if (!match) continue;
        if (code=='I') ScrollingPicture(match->first,match->second,restricted,appendPicture);
        else Option(code=='W' ? 'S' : code,match->first,match->second,context,metadata,!patch);
        return;
    }
}

// 같은 공통 태그를 참조로 받아 이후 문자열/그림 생성까지 전달한다.
StyleTextHooks StyleTextMarkup::Hooks(StyleTextHooks hooks) const {
    hooks.bracket=[this](std::string_view source,std::array<std::uint32_t,8>& context,std::uint32_t& head,std::uint32_t& metadata) {
        Apply(source,context,head,metadata);
    };
    return hooks;
}

// 이름 표는 원본 C 문자열 이름을 사용하며 프레임은 물리 레이어가 아닌 논리 클러스터에서 공급한다.
void BindStyleTextMarkupAssets(StyleTextMarkupHooks& hooks,const GameAssets& assets) {
    // 타입 번호는 내장 무자산 슬롯도 포함한다. 원본 signed 상한 밖 로딩은 거부한다.
    hooks.typeCount=[&assets] {
        if (assets.TypeTable().Types().size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
            throw std::length_error("다중 줄 그림 실제 타입 수 범위 초과");
        return static_cast<std::int32_t>(assets.TypeTable().Types().size());
    };
    // 제한 그림은 추가 헤더 +1a의 원본 signed 높이를 읽는다. 무자산 타입은 생성하지 않는다.
    hooks.frameHeight=[&assets](std::int32_t type,std::int32_t frame) -> std::optional<std::int16_t> {
        if (type<o::kFirstAssetTypeNumber || frame<0) return std::nullopt;
        const auto index=static_cast<std::size_t>(type-o::kFirstAssetTypeNumber);
        if (index>=assets.Types().size()) return std::nullopt;
        const auto& asset=assets.Types()[index];
        if (static_cast<std::size_t>(frame)>=asset.definition.clusters.size()) return std::nullopt;
        const auto header=assets.Shapes().SquidHeader(asset.block,static_cast<std::size_t>(frame));
        return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(header[0x1a] | (std::uint16_t(header[0x1b])<<8)));
    };
    hooks.findType=[&assets](std::string_view name) { return assets.TypeTable().Find(name); };
    // 원본 내장 무자산 타입은 빈 프레임 선택 필드를 반환하고 잘못된 타입 번호는 거부한다.
    hooks.type=[&assets](std::int32_t type) {
        if (type<0 || static_cast<std::size_t>(type)>=assets.TypeTable().Types().size())
            throw std::out_of_range("StyleText 대괄호 실제 타입 범위 초과");
        StyleTextMarkupType view;
        if (type<o::kFirstAssetTypeNumber) return view;
        const auto index=static_cast<std::size_t>(type-o::kFirstAssetTypeNumber);
        if (index>=assets.Types().size()) return view;
        const auto& definition=assets.Types()[index].definition;
        if (definition.clusters.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
            throw std::length_error("StyleText 대괄호 실제 프레임 수 범위 초과");
        view.frameCount=static_cast<std::int32_t>(definition.clusters.size());
        view.defaultFrame=definition.specialFrames.defaultFrame;view.gumpFrame=definition.specialFrames.gumpFrame;
        const auto frames=definition.FrameTable();
        view.letterKinds=frames.LetterKinds();
        // A~P의 실제 첫 인덱스를 같은 원본 타입 정의에서 읽는다. 없는 글자를 0으로 바꾸지 않는다.
        for (std::size_t letter=0;letter<view.first.size();++letter)
            view.first[letter]=frames.Run(static_cast<std::uint8_t>('A'+letter)).first;
        return view;
    };
}

// E는 명령의 숫자 색 대신 현재 기본 검정 별칭을 설치한다.
void BindStyleTextMarkupPalette(StyleTextMarkupHooks& hooks,const PaletteColorTable& palette) {
    hooks.black=[&palette] { return palette.basic[0]; };
}

// 원본 Client 초기화의 ReadInt처럼 키가 없으면 현재 기본/이전 값을 유지한다.
void LoadStyleTextMarkupOptions(StyleTextMarkupState& state,o::ConfigInterface& configuration) {
    configuration.ReadInt("allowzonesounds",state.allowSounds);
    configuration.ReadInt("zonebgcolor",state.allowBackgroundColor);
    configuration.ReadInt("zonefgcolor",state.allowForegroundColor);
    configuration.ReadInt("zoneoffset",state.allowOffset);
}

// 원본 S 명령은 위치·반복·우선·동시 재생 한도를 추가하지 않고 이름 재생을 요청한다.
void BindStyleTextMarkupSound(StyleTextMarkupHooks& hooks,SoundPlayer& sound) {
    hooks.sound=[&sound](std::string_view name) { sound.PlayByName(name,0,0,0,0,0); };
}
}
