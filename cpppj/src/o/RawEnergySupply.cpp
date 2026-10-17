// 원본의 전체 DWORD 반환·공급 목록·문자 소모 순서와 x87 중간 저장 위치를 보존한다.
#include "o/RawEnergySupply.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace netstorm::o {
namespace {
#include "EnergyProviderVtables.inc"
constexpr std::string_view kLetters="wrtsWRTS.?>"; // 00540ad8의 실제 문자표이며 끝 NUL도 인덱스11로 읽을 수 있다.
// 원본 표 밖을 호스트 메모리로 해석하지 않는다.
char Letter(std::int32_t theme) {
    if (theme<0 || static_cast<std::size_t>(theme)>kLetters.size()) throw std::out_of_range("에너지 원소 문자표 범위 오류");
    return kLetters.data()[theme];
}
// raw float를 비정렬 little endian DWORD에서 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{};
    // 필요한 네 바이트만 조립한다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return std::bit_cast<float>(value);
}
// 원본 C 문자열은 첫 NUL에서 끝난다.
std::string CString(std::string_view text) { return std::string(text.substr(0,text.find('\0'))); }
}
// 동일 함수 주소를 공유하는 가상 표만 현재 원본 공급 몸체에 연결한다.
std::uint32_t EnergyProviderEntry(std::uint32_t vtable) {
    // 표 밖의 파생이나 파괴 중 표를 임의로 허용하지 않는다.
    for (const auto& entry:kEnergyProviderEntries) if (entry[0]==vtable) return entry[1];
    throw std::logic_error("에너지 공급의 현재 가상 표 미복원");
}
// 원본은 PATCH188TYPE와9×9 동맹표를 사용한다. 공급/선택 count는 사용 시 현재 값으로 검증한다.
RawEnergySupply::RawEnergySupply(const SidPool& pool,std::span<const RiftTypeRecord> types,const SquidPostPopList& providers,
    SquidPostPopList& selected,std::span<const std::uint32_t> alliances,const std::int32_t& mapLimit,
    EnergySupplyState& state,EnergySupplyHooks hooks,EnergySupplyReads reads)
    :pool_(pool),types_(types),providers_(providers),selected_(selected),alliances_(alliances),mapLimit_(mapLimit),state_(state),hooks_(std::move(hooks)),reads_(std::move(reads)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || alliances.size()!=81 || !hooks_.providerEntry || !hooks_.requirement) throw std::invalid_argument("에너지 실제 월드/TYPE/조회 계약 오류");
}
// 가상 효과가 바꿀 수 있는 전역은 각 원본 읽기 위치에서 조회한다.
std::uint32_t RawEnergySupply::Read(std::uint32_t fallback,const std::function<std::uint32_t()>& current) const { return current ? current() : fallback; }
// 이전 호출의 우회 결과를 저장하지 않는다.
bool RawEnergySupply::Override() const { return (Read(state_.special,reads_.special) || (Read(state_.mission,reads_.mission) && state_.userCheat)) && state_.ignore; }
// 실제 TYPE를 다른 번호/그림 블록으로 치환하지 않는다.
const RiftTypeRecord& RawEnergySupply::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("에너지 TYPE 범위 오류");return types_[type]; }
// 예약 슬롯도 원본처럼 읽을 수 있으나 SID0/풀 밖은 거부한다.
std::span<const std::uint8_t> RawEnergySupply::Raw(Sid sid) const { if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("에너지 공급 SID 범위 오류");return pool_.Slot(sid); }
// 40e880/40e8f0은 들어온 모서리만 클램프한 뒤 현재 양 끝점의 유효 여부로 min/max를 선택한다.
std::array<float,4> RawEnergySupply::Bounds(std::uint32_t number,float x,float y) const {
    const auto& type=Type(number);float lowX=static_cast<float>(static_cast<double>(x)-(type.footX-1)),lowY=static_cast<float>(static_cast<double>(y)-(type.footY-1));
    const float limit=static_cast<float>(mapLimit_);
    // 실제41d0d0은 가변 유효 경계와 별도로 고정 [1,255]에 제한한다.
    lowX=std::clamp(lowX,1.0f,255.0f);lowY=std::clamp(lowY,1.0f,255.0f);
    const bool valid=x>0 && y>0 && x<limit && y<limit;
    return valid ? std::array<float,4>{std::min(x,lowX),std::min(y,lowY),std::max(x,lowX),std::max(y,lowY)} : std::array<float,4>{lowX,lowY,lowX,lowY};
}
// 대상 사각형과 flags는 실제 공급 가상 몸체가 읽지 않는 인자지만 ABI 의미를 보존한다.
bool RawEnergySupply::Covers(Sid provider,const std::array<float,4>&,float centerX,float centerY,float sizeSquared,std::uint32_t) const {
    const auto raw=Raw(provider);std::uint32_t table{};
    // 가상 표 DWORD를 호스트 주소로 실행하지 않는다.
    for (std::size_t index=0;index<4;++index) table|=static_cast<std::uint32_t>(raw[index])<<(index*8);
    const auto entry=hooks_.providerEntry(table);if (entry==0x41c0f0) return false;if (entry!=0x41c170) throw std::logic_error("에너지 공급 가상+a0 미복원");
    const auto& type=Type(raw[10]);
    const float dx=static_cast<float>((static_cast<double>(Coordinate(raw,14))-type.footX*0.5)-centerX);
    const double dy=(static_cast<double>(Coordinate(raw,18))-type.footY*0.5)-centerY;
    const double distance=dy*dy+static_cast<double>(dx)*dx;
    return distance<static_cast<double>(state_.radiusSquared)+sizeSquared;
}
// 공급량은 선택 목록의 용량과 무관하게 집계하며 문자열은0..5 원소만254개까지 출력한다.
void RawEnergySupply::Collect(std::string& output,float x,float y,std::uint32_t type,std::uint32_t owner,std::int32_t theme) const {
    const auto bounds=Bounds(type,x,y);const float centerX=static_cast<float>((static_cast<double>(bounds[0])+bounds[2])*0.5),centerY=static_cast<float>((static_cast<double>(bounds[1])+bounds[3])*0.5);
    constexpr float kDiagonal=0.7071067690849304f; // 실제0050c36c의 단정도 대각 배율이다.
    const float size=static_cast<float>((static_cast<double>(bounds[2])-bounds[0])*kDiagonal);const float square=size*size;
    const auto flags=Type(type).flags2;std::array<std::uint32_t,8> counts{};selected_.count=0;
    // 현재 공급 목록의 count/항목과 현재 소유자/동맹을 각 반복에서 읽는다.
    for (std::uint32_t index=0;index<providers_.count;++index) {
        const auto value=providers_.Items()[index];if (!value || value>=pool_.Capacity() || value>65535) throw std::out_of_range("에너지 공급 목록 DWORD 범위 오류");
        const Sid sid{static_cast<std::uint16_t>(value)};const auto raw=Raw(sid);const auto sourceOwner=static_cast<std::uint32_t>(raw[34]);bool allowed=Read(state_.editor,reads_.editor) || sourceOwner==owner;
        if (!allowed && state_.allied) { if (sourceOwner>=9 || owner>=9) throw std::out_of_range("에너지 동맹 소유자 범위 오류");allowed=alliances_[sourceOwner*9+owner]!=0; }
        if (!allowed || !Covers(sid,bounds,centerX,centerY,square,flags)) continue;
        const auto current=Raw(sid);const auto kind=Type(current[10]).theme;if (kind<0 || kind>=8) throw std::out_of_range("에너지 공급 원소 카운터 범위 오류");++counts[static_cast<std::size_t>(kind)];
        if (selected_.count<selected_.entries.size()) selected_.entries[selected_.count++]=sid.value;
    }
    output.clear();
    // 특정 원소 요청은 그것을 먼저 쓰고 다른 모든0..5 원소를 범용s로 출력한다.
    if (theme!=-1) { if (theme<0 || theme>=8) throw std::out_of_range("에너지 수집 원소 범위 오류");output.append(std::min<std::uint32_t>(counts[theme],254),Letter(theme));counts[theme]=0; }
    // 원본 반복 상한은6이며6/7 공급원은 목록에는 남지만 문구에는 없다.
    for (std::size_t index=0;index<6;++index) output.append(std::min<std::uint32_t>(counts[index],254-static_cast<std::uint32_t>(output.size())),theme==-1 ? Letter(static_cast<std::int32_t>(index)) : 's');
}
// 일반 원소는 같은 문자만, 범용s는 s→다른 비점 원소→대체 원소 순서로 소비한다.
std::uint32_t RawEnergySupply::Match(std::string& output,std::string_view required,std::string* remainder,std::int32_t theme) const {
    auto available=CString(output);const auto requirements=CString(required);if (available.size()>255) throw std::out_of_range("에너지 원본256바이트 임시 문자열 범위 오류");
    const char substitute=theme==-1 ? 's' : Letter(theme);std::string result;std::uint32_t success=1;
    // 이미 사용한 문자는 점으로 바꾸어 다음 요구가 같은 공급원을 재사용하지 못하게 한다.
    for (const char wanted:requirements) {
        if (wanted!='s' && wanted!='w' && wanted!='r' && wanted!='t') throw std::logic_error("에너지 원본 요구 문자 진단");
        auto found=available.find(wanted);
        if (wanted=='s' && found==std::string::npos) { found=available.find_first_not_of(std::string(".")+substitute);if (found==std::string::npos) found=available.find(substitute); }
        if (found==std::string::npos) { result+=wanted;success=0; }
        else { available[found]='.';result+=static_cast<char>(wanted-'a'+'A'); }
    }
    if (Override()) success=1;output=std::move(result);if (remainder) *remainder=std::move(available);return success;
}
// 우회 경로는 출력/공급 목록을 건드리지 않는 원본 조기 반환이다.
std::uint32_t RawEnergySupply::Evaluate(std::string& output,float x,float y,std::uint32_t type,std::uint32_t owner) const {
    if (Override()) return 1;Collect(output,x,y,type,owner);const auto length=static_cast<std::uint32_t>(output.size());const auto required=hooks_.requirement(type);
    if (required.empty() || required[0]=='\0') { output.clear();return 1; }return Match(output,required) ? length : 0;
}
// Builder가 같은 목록을 순서대로 소비하게 한다.
std::uint32_t RawEnergySupply::TakeSource() const { if (std::bit_cast<std::int32_t>(selected_.count)<=0) return 0;for (std::uint32_t index=0;index<selected_.count;++index) { auto& value=selected_.entries.at(index);if (value) { const auto result=value;value=0;return result; } }return 0; }
// 41cfc0의 제곱근 거리와float 임계값 저장을 사용한다. 원소/중심으로 후보를 다시 거르지 않는다.
void RawEnergySupply::SortSources(std::string_view demand,float x,float y) const {
    constexpr std::size_t kKept=10; // 원본 new28의 실제10DWORD이며 요구가 길어도 이 수만 다시 보관한다.
    auto kept=std::make_unique<std::array<std::uint32_t,kKept>>();std::size_t count{};
    const auto report=[this](EnergySortDiagnostic diagnostic) { if (!hooks_.sortDiagnostic) throw std::logic_error("에너지 정렬 원본 진단");hooks_.sortDiagnostic(diagnostic); }; // 원본 보고 뒤 현재 조건을 다시 검사한다.
    // 문자 내용은 사용하지 않으며 첫NUL 또는 첫 미선택에서 끝난다.
    for (const char letter:demand) {
        if (!letter) break;std::int32_t best=-1;float distanceLimit=999.0f;
        // 선택/진단 뒤에도 현재signed count와 현재 슬롯을 읽는다.
        for (std::int32_t index=0;index<std::bit_cast<std::int32_t>(selected_.count);++index) {
            const auto value=selected_.entries.at(static_cast<std::size_t>(index));if (!value) continue;
            const auto valid=[this,value] { return value>=1 && value<pool_.Capacity() && value<=65535 && !(pool_.Slot(Sid{static_cast<std::uint16_t>(value)})[11]&1); }; // 예약슬롯1..4도 원본 범위라면 허용한다.
            if (!valid()) report(EnergySortDiagnostic::InvalidSource);
            if (!valid()) continue;const auto raw=pool_.Slot(Sid{static_cast<std::uint16_t>(value)});
            const double dx=static_cast<double>(x)-Coordinate(raw,14),dy=static_cast<double>(y)-Coordinate(raw,18);
            const double distance=std::sqrt(dx*dx+dy*dy);
            if (distance<distanceLimit) { distanceLimit=static_cast<float>(distance);best=index; }
        }
        if (best==-1) break;
        if (best<0 || best>std::bit_cast<std::int32_t>(selected_.count)) report(EnergySortDiagnostic::InvalidBestIndex);
        if (best>=0 && best<=std::bit_cast<std::int32_t>(selected_.count)) {
            auto& value=selected_.entries.at(static_cast<std::size_t>(best));if (count<kKept) (*kept)[count++]=value;value=0;
        }
    }
    selected_.count=0;
    // 임시 목록을 현재 용량까지만 앞에 쓰고 선택하지 않은 꼬리는 지우지 않는다.
    for (std::size_t index=0;index<count;++index) if (selected_.count<selected_.entries.size()) selected_.entries[selected_.count++]=(*kept)[index];
}
// 연결 시 단일 풀의 정체성을 확인한다.
const SidPool& RawEnergySupply::Pool() const { return pool_; }
}
