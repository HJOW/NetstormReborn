// Chat.cpp의 실제 manager 생성/초기화와 Chatgump.cpp의 랭크 prefix를 같은 현재 전역에 적용한다.
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::o {
inline constexpr std::uint32_t kChatManagerBytes=0xa98; //0042e050의 실제 manager 할당 폭이다.
inline constexpr std::uint32_t kChatManagerBufferBytes=0x80; //0042e000의 별도 내부 할당 폭이며 초기화하지 않는다.
inline constexpr std::uint32_t kChatManagerRankPrefixOffset=0x244; //ChatWindow+b8 입력 접두와 별개인 manager 랭크 문자열이다.
inline constexpr std::uint32_t kChatRankOutputBytes=256; //0042f480은 NUL 뒤 미작성 바이트까지256BYTE를 복사한다.

// 실제 strncpy60/500의 NUL padding과 원문 물리 범위를 보존하는 작은 CRT 어댑터다.
void CopyChatManagerFormat(std::span<std::uint8_t> destination,std::string_view source,std::uint32_t count);
// 현재256BYTE 임시 C문자열 끝에 고정 icon.B/C 서식을 붙이는 실제 순수 어댑터다.
void AppendChatRankIcon(std::span<std::uint8_t> destination,char kind,std::int32_t value);

// Client가 소유할 실제 current 슬롯/현재 subscriber/CoreData 활성 DWORD만 빌린다.
struct ChatManagerPrefixGlobals {
    std::uint32_t& current; //00548714의 현재 manager 토큰이며 초기화/외부 callback이 같은 슬롯을 쓴다.
    const std::uint32_t& subscriber; //00540c74를 각 signed 서식 직전에 다시 읽는다.
    const std::uint32_t& coreDataActive; //00595348이며 PlayerKnown/선택 SID와 구별한다.
};
// 할당은 실제0xa98/0x80 저장소를 소유하고 memory는 그 같은 주소의 안정된 BYTE뷰를 제공해야 한다.
struct ChatManagerPrefixHooks {
    std::function<std::uint32_t(std::uint32_t)> allocate; //004e4391의 현재 할당 결과0/토큰이다. 이전 manager를 임의로 free하지 않는다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> memory; //현재 토큰의 실제 물리 할당이다.0/해제 토큰은 장치가 거부한다.
    std::function<void(std::uint32_t)> assertion; //Chat.cpp220/898,Chatgump.cpp2474의 실제 진단이며 복귀 시 원문을 계속한다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> registerCommand; //format/back/front 이름과 실제 callback 식별자/flag1을 등록한다.
    std::function<std::optional<std::string>(std::string_view)> readConfig; //현재 Config.ReadString의 지역 복사 결과다. 누락은 빈 지역 C문자열을 유지한다.
    std::function<void(std::uint32_t,std::uint32_t,std::string_view,std::uint32_t)> copyFormat; //실제 target토큰/offset을 받는 CRT 경계다.0대상도 호출 진입 뒤 물리 오류가 나야 하므로 span조회로 선행 거부하지 않는다.
    std::function<std::int32_t()> maximumRank,minorCount; //004c6c20/004c6c00의 매호출 현재 icon.C/B 개수 getter다.
    std::function<void(std::span<std::uint8_t>,char,std::int32_t)> appendIcon{AppendChatRankIcon}; //004a8a00의 실제 고정 서식 append다.
    std::function<std::span<const std::uint8_t>()> core; //0048f0d0의 현재 포인터/메타맵 진단을 포함한 같은 CoreData뷰다. 사본을 현재 포인터로 가장하지 않는다.
};
// 미공급 copyFormat만 같은 memory와 실제 CopyChatManagerFormat에 연결한다. 기존 사용자 장치는 덮어쓰지 않는다.
ChatManagerPrefixHooks BindChatManagerMemoryCopy(ChatManagerPrefixHooks hooks);
//0042e000:내부 할당→같은this+8→조건부진단→지정 필드만 쓰고 같은 토큰을 반환한다.
std::uint32_t ConstructChatManager(std::uint32_t object,const ChatManagerPrefixHooks& hooks);
//0042e050:new/현재대입/명령등록/두현재subscriber 서식/Config/현재manager별 strncpy 순서다.
void InitializeChatManager(ChatManagerPrefixGlobals globals,const ChatManagerPrefixHooks& hooks);
//0042d620:같은 source에서 NUL 포함 순방향 복사한다. 의미상256절단/입력창 접두 변경을 추가하지 않는다.
void SetChatManagerRankPrefix(std::span<std::uint8_t> manager,std::span<const std::uint8_t> source);
//0042f480:temporary의 미작성 입력을 보존하고 output에 실제256BYTE를 복사한 같은 출력뷰를 반환한다.
//두 span은 각각 실제 최소256BYTE여야 하며 미정 temporary를 임의0으로 채우지 않는다.
std::span<const std::uint8_t> ChatPrefixIcons(std::span<std::uint8_t> output,std::span<std::uint8_t> temporary,std::int32_t rank,const ChatManagerPrefixHooks& hooks);
//0042f540:현재manager/active의 자연관문 뒤 현재Core+4 BYTE→formatter→다시현재manager setter다.
void RefreshChatManagerRankPrefix(ChatManagerPrefixGlobals globals,const ChatManagerPrefixHooks& hooks,std::span<std::uint8_t> output,std::span<std::uint8_t> temporary);
}
