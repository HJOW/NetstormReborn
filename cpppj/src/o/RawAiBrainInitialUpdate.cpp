// 재호출의누적count/원본최대비용의ftol/중립원소치환·압축규칙을보존한다.
#include "o/RawAiBrainInitialUpdate.h"
#include "o/RawMissionAiSettings.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 비정렬실제DWORD를읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) { if (offset+4>raw.size()) throw std::out_of_range("AI초기갱신실제필드범위오류");return static_cast<std::uint32_t>(raw[offset])|(static_cast<std::uint32_t>(raw[offset+1])<<8)|(static_cast<std::uint32_t>(raw[offset+2])<<16)|(static_cast<std::uint32_t>(raw[offset+3])<<24); }
// 원본DWORD의바이트만쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    if (offset+4>raw.size()) throw std::out_of_range("AI초기갱신실제필드범위오류");
    // 주변원문을변경하지않는다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 원본ftol의signed64절삭하위DWORD다. FISTP무효결과의하위DWORD는0이다.
std::uint32_t Truncate(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value))); }
// 실제NUL문자열을읽되소유메모리밖읽기는명시거부한다.
std::string Text(std::span<const std::uint8_t> raw,std::size_t offset) { const auto begin=raw.begin()+static_cast<std::ptrdiff_t>(offset);const auto end=std::find(begin,raw.end(),std::uint8_t{});if (end==raw.end()) throw std::out_of_range("AI초기갱신NUL문자열소유메모리범위오류");return {begin,end}; }
// 원본TYPE에너지C문자열길이이며현대문자열뒤의NUL이후를포함하지않는다.
std::size_t Length(std::string_view value) { const auto nul=value.find('\0');return nul==std::string_view::npos ? value.size() : nul; }
}
// 같은실제세계/워킹목록/TYPE를연결한다.
RawAiBrainInitialUpdate::RawAiBrainInitialUpdate(RawAiBrainMemory& memory,RawPlayerWorld& player,RawPlayerTechnology& technology,const ContainedFinderState& finder,std::span<const RiftTypeRecord> types,AiBrainInitialUpdateGlobals globals,AiBrainInitialUpdateHooks hooks)
    :memory_(memory),player_(player),technology_(technology),finder_(finder),types_(types),globals_(globals),hooks_(std::move(hooks)) {
    if (&player.Pool()!=&technology.Pool() || player.Pool().Edition()!=OriginalEdition::Patch1078 || types.empty() || globals.workshops.size()!=4) throw std::invalid_argument("AI초기갱신같은PATCH세계/TYPE/워크샾4개누락");
}
// 타입0도원본처럼조회가능하며표밖은host메모리계약오류다.
const RiftTypeRecord& RawAiBrainInitialUpdate::Type(std::uint32_t number) const { if (number>=types_.size()) throw std::out_of_range("AI초기갱신실제TYPE범위오류");return types_[number]; }
// 실제유효분류theme은0..4이며4의workshop은0이다. 잘못된theme의원본stack손상은명시거부한다.
std::uint32_t RawAiBrainInitialUpdate::Workshop(std::int32_t theme) const { if (theme<0 || theme>5) throw std::out_of_range("AI초기갱신실제원소범위오류");return theme<4 ? globals_.workshops[static_cast<std::size_t>(theme)] : 0U; }

// 원본은owner를600byte복사전에캡처하며이후기술처리는현재owner를다시읽는다.
void RawAiBrainInitialUpdate::Update(std::uint32_t token,std::span<const std::uint8_t> settings) const {
    auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes || settings.size()!=RawMissionAiSettings::kSize) throw std::invalid_argument("AI초기갱신실제본체/600byte설정누락");
    const auto nameOwner=Read(raw,0x260);if (nameOwner>=9) throw std::out_of_range("AI초기갱신실제이름owner범위오류");std::copy(settings.begin(),settings.end(),raw.begin());
    if (globals_.missionMode) { std::string name;if (raw[0]) name=Text(raw,0);else { if (!hooks_.defaultName) throw std::logic_error("AI실제기본이름언어조회누락");name=hooks_.defaultName();name.resize(Length(name)); }auto records=player_.Records();const auto offset=nameOwner*180+12;if (offset+name.size()+1>records.size()) throw std::out_of_range("AI초기갱신실제이름저장소범위오류");std::copy(name.begin(),name.end(),records.begin()+offset);records[offset+name.size()]=0; }
    const auto owner=[&] { return std::bit_cast<std::int32_t>(Read(raw,0x260)); }; //현재원문owner를매원본위치에서다시읽는다.
    if (!player_.Technology(owner())) player_.Create(owner());if (!player_.Technology(owner())) return;
    if (raw[0x1e]) technology_.AddList(owner(),Text(raw,0x1e));std::array<std::int32_t,10> counts{};
    // 원본은이5개분류플래그와최대비용만초기화한다. buildercount/원소목록count/394/390/ad8은유지한다.
    for (std::size_t index=0;index<5;++index) Put(raw,0x3b4+index*4,0);Put(raw,0x290,0);
    const auto parent=player_.Technology(owner());if (parent>65535 || parent>=player_.Pool().Capacity()) throw std::out_of_range("AI초기갱신실제기술부모범위오류");RawContainedFinder cursor(player_.Pool(),finder_);
    // 기존커서는다음SID를먼저저장하고현재flags4/TYPE를방문시읽는다.
    for (auto sid=cursor.Begin(Sid{static_cast<std::uint16_t>(parent)});sid.value;sid=cursor.Next()) {
        const auto piece=player_.Pool().Slot(sid);const auto type=static_cast<std::uint32_t>(piece[18])|(static_cast<std::uint32_t>(piece[19])<<8);const auto& entry=Type(type);if (!(piece[20]&4)) continue;
        const auto builderCount=std::bit_cast<std::int32_t>(Read(raw,0x28c)),builderCapacity=std::bit_cast<std::int32_t>(Read(raw,0x288));
        if ((entry.flags2&0x30000U) && builderCount<builderCapacity) { if (builderCount<0) throw std::out_of_range("AI초기갱신실제빌더count범위오류");auto builders=memory_.Bytes(Read(raw,0x284));Put(builders,static_cast<std::size_t>(builderCount)*4,type);Put(raw,0x28c,static_cast<std::uint32_t>(builderCount)+1U); }
        if (!hooks_.requiredEnergy) throw std::logic_error("AI초기갱신실제TYPE요구에너지누락");const auto firstLength=Length(hooks_.requiredEnergy(type));if (Read(raw,0xad8)<=firstLength) Put(raw,0xad8,static_cast<std::uint32_t>(Length(hooks_.requiredEnergy(type))));
        const auto maximum=static_cast<double>(std::bit_cast<std::int32_t>(Read(raw,0x290)));const auto cost=static_cast<double>(entry.cost);Put(raw,0x290,Truncate(maximum>cost ? maximum : cost));if (type==globals_.excludedType) continue;
        const auto theme=entry.theme;if (theme<0 || theme>=5) throw std::out_of_range("AI초기갱신원본분류stack배열범위오류");const auto index=static_cast<std::size_t>(theme);++counts[index];
        if (entry.flags2&0x800U) { ++counts[index+5];if (Read(raw,0x390)==3) Put(raw,0x390,static_cast<std::uint32_t>(theme)); }
        if (counts[index]%5==1) { const auto count=std::bit_cast<std::int32_t>(Read(raw,0x3b0));if (count<0) throw std::out_of_range("AI초기갱신실제원소count범위오류");Put(raw,0x398+static_cast<std::size_t>(count)*4,static_cast<std::uint32_t>(theme));Put(raw,0x3b0,Read(raw,0x3b0)+1U);const auto candidate=Workshop(theme),previous=Read(raw,0x394);if (!previous || std::bit_cast<std::int32_t>(Truncate(Type(candidate).cost))>std::bit_cast<std::int32_t>(Truncate(Type(previous).cost))) Put(raw,0x394,candidate); }
    }
    // 원본은단독battery원소를중립3으로치환하거나한번제거하며기존목록count전체를압축한다.
    for (std::size_t theme=0;theme<5;++theme) if (counts[theme+5]==1 && counts[theme]==1) {
        std::uint32_t removed=0;
        // 제거뒤에도source는원래index로진행하며destination만한칸앞으로쓴다.
        for (std::int32_t index=0;index<std::bit_cast<std::int32_t>(Read(raw,0x3b0));++index) { const auto value=Read(raw,0x398+static_cast<std::size_t>(index)*4);Put(raw,0x398+(static_cast<std::size_t>(index)-removed)*4,value);if (value==theme) { if (counts[3]%5==0) { Put(raw,0x398+static_cast<std::size_t>(index)*4,3);Put(raw,0x3b4+theme*4,2);++counts[3];break; }if (!removed) { removed=1;Put(raw,0x3b4+theme*4,1);++counts[3]; } } }
        Put(raw,0x3b0,Read(raw,0x3b0)-removed);
    }
}
}
