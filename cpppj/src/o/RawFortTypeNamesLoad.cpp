// 같은stream주소/현재conversion/현재typeCount와signedchar해시의원문순서를보존한다.
#include "o/RawFortTypeNamesLoad.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kConversionBytes=255,kExistingConversionLine=608; //원본할당폭과Template.cpp진단행이다.
// 실제도달한필수경계를빈성공으로대체하지않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("TypeNames실제경계미연결");return function;
}
// 현재stream/data에서비정렬little endian DWORD를읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::uint64_t offset) {
    if (offset>bytes.size() || bytes.size()-static_cast<std::size_t>(offset)<4) throw std::out_of_range("TypeNames DWORD물리읽기범위");std::uint32_t result{};
    // 지정네BYTE만조립한다.
    for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[static_cast<std::size_t>(offset)+byte])<<(byte*8);return result;
}
// 같은stream의pos/end 네BYTE만기록한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("TypeNames stream쓰기범위");
    // 다른stream필드는보존한다.
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 첫NUL까지signedchar를위치mod4만큼밀어32비트합을감는다.
std::uint32_t Hash(std::span<const std::uint8_t> name) {
    std::uint32_t result{}; //원본EDI의감김합이다.
    // 빈이름은0이고NUL뒤바이트는해시에포함하지않는다.
    for (std::size_t index=0;index<name.size();++index) {
        const auto byte=name[index];if (!byte) return result;
        const auto value=static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(byte)));result+=value<<((index&3U)*8);
    }
    throw std::out_of_range("TypeNames현재이름NUL물리범위");
}
// 현재DWORD를쓰고실제소비자에같은깊이를통지한다.
void Depth(std::uint32_t value,const FortTypeNamesLoadGlobals& globals,const FortTypeNamesLoadHooks& hooks) {
    globals.loadingDepth=value;Need(hooks.loadingDepthChanged)(value);
}
}
// count는할당전캡처하고hash주소는할당/zero뒤캡처하며각변환쓰기주소는그때재조회한다.
void LoadFortTypeNames(std::int32_t slot,const FortTypeNamesLoadGlobals& globals,const FortTypeNamesLoadHooks& hooks) {
    Depth(globals.loadingDepth+1U,globals,hooks);
    const auto token=Need(hooks.select)(Need(hooks.manager)(slot),"TypeNames",1);
    const auto load=[&](std::span<std::uint8_t> fields,const auto& buffer) { //선택된같은필드/버퍼의동기빌림전체에서기존본문을실행한다.
        const auto data=Read(fields,0);
        if (!data || !Read(fields,8)) globals.conversion=0; //기존pointer를해제하지않는다.빈경로에는할당/TYPE조회도없다.
        else {
            const auto position=Read(fields,4);const auto bytes=Need(buffer)(data);
            if (position>=bytes.size()) throw std::out_of_range("TypeNames count BYTE물리범위");
            const auto count=bytes[position];Put(fields,4,position+1U);
            if (globals.conversion) Need(hooks.assertion)(kExistingConversionLine);
            const auto allocated=Need(hooks.allocate)(kConversionBytes);globals.conversion=allocated;auto conversion=Need(hooks.conversionBuffer)(allocated);
            // 원문rep stosd의63번DWORD쓰기를보존하여짧은물리블록에도반DWORD를쓰지않는다.
            for (std::size_t word=0;word<63;++word) Put(conversion,word*4,0);
            if (conversion.size()<254) throw std::out_of_range("TypeNames할당WORD물리범위");conversion[252]=conversion[253]=0; //stosw의정확한2BYTE다.
            if (conversion.size()<kConversionBytes) throw std::out_of_range("TypeNames할당BYTE물리범위");conversion[254]=0; //마지막stosb뒤의바이트는유지한다.
            const auto hashPosition=Read(fields,4),hashData=Read(fields,0); //callback이바꾼현재stream에서주소를한번캡처한다.
            if (std::bit_cast<std::int32_t>(hashPosition)>std::bit_cast<std::int32_t>(Read(fields,8))) Put(fields,8,hashPosition);
            // 매반복끝현재signed count를읽는다.타입0과256이상의낮은BYTE wrap도그대로다.
            for (std::int32_t type=0;type<Need(hooks.typeCount)();++type) {
                const auto hash=Hash(Need(hooks.typeName)(type));
                // 같은hash가여러저장칸에있으면첫칸만갱신하고다음현재타입으로간다.
                for (std::uint32_t stored=0;stored<count;++stored) {
                    if (Read(Need(buffer)(hashData),std::uint64_t(hashPosition)+stored*4U)!=hash) continue;
                    auto current=Need(hooks.conversionBuffer)(globals.conversion);if (stored>=current.size()) throw std::out_of_range("TypeNames현재변환쓰기물리범위");
                    current[stored]=static_cast<std::uint8_t>(type);break; //이름getter가현재conversion을교체했어도새주소에쓴다.
                }
            }
        }
    };
    if (hooks.access) hooks.access(token,[&](auto fields,const MemStreamHooks& memory) { load(fields,memory.buffer); });
    else load(Need(hooks.stream)(token),hooks.buffer); //기존독립장치/664관측은동일getter경로를유지한다.
    Depth(globals.loadingDepth-1U,globals,hooks);
}
// 정상본문과별도로원문cleanup의현재값감소효과만공급한다.
void UnwindFortTypeNamesLoad(const FortTypeNamesLoadGlobals& globals,const FortTypeNamesLoadHooks& hooks) {
    Depth(globals.loadingDepth-1U,globals,hooks);
}
}
