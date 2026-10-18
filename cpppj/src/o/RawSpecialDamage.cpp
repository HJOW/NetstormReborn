// 원본 현재 raw 필드/TYPE/프레임과 기존4방향 기하를 같은 풀에서 읽는다.
#include "o/RawSpecialDamage.h"
#include "o/RawSquidNeighbors.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 비정렬 little endian DWORD를 읽으며 주변 필드를 포함하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{};
    // 원본 네 바이트의 순서를 그대로 합친다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(8*index);
    return value;
}
// PATCH 전용 몸체에 다른 판본이나 무효 SID를 섞지 않는다.
std::span<const std::uint8_t> Slot(const SidPool& pool,Sid sid) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || !sid.value || sid.value>=pool.Capacity()) throw std::invalid_argument("특수 피해 판본/SID 오류");
    return pool.Slot(sid);
}
}
// 건설 Form이 없으면 원본처럼 효과 없이1이며, 찾은 뒤에만 끝 시각/표시를 차례로 호출한다.
bool RawVortexDamage(const SidPool& pool,Sid victim,std::int32_t damage,std::uint32_t owner,Sid attacker,const VortexDamageHooks& hooks) {
    if (Slot(pool,victim)[40]&1U) {
        if (!hooks.constructionType || !hooks.find) throw std::logic_error("신전 피해 현재 건설 조회 미연결");
        const auto form=hooks.find(victim,hooks.constructionType(),0,false); // 현재 첫30 Form이며 예측 대체를 만들지 않는다.
        if (form.value) {
            if (!hooks.resetEnd || !hooks.refresh) throw std::logic_error("신전 피해 건설 끝 시각/표시 미연결");
            hooks.resetEnd(form,std::bit_cast<std::uint32_t>(damage));hooks.refresh(victim);
        }
        return true;
    }
    if (!hooks.general) throw std::logic_error("완공 신전 실제 일반 피해 미연결");
    return hooks.general(victim,damage,owner,attacker);
}
// 원본은 공격자 제외 조건을 먼저 검사하며 제외되면 현재 피해자 프레임을 읽지 않는다.
bool RawWindTowerDefense(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    Sid victim,Sid attacker,std::uint32_t arcSpireType) {
    const auto source=Slot(pool,attacker);const auto type=source[10]; // 현재 공격자의 TYPE과 genus다.
    if (type>=types.size()) throw std::out_of_range("윈드 타워 공격자 TYPE 범위 오류");
    if ((types[type].flags2&0x100100U) || type==arcSpireType) return false;
    const auto target=Slot(pool,victim);const auto targetType=target[10]; // 제외되지 않은 뒤에만 실제 방어자 자료를 읽는다.
    if (targetType>=types.size() || targetType>=frames.size()) throw std::out_of_range("윈드 타워 방어자 TYPE 범위 오류");
    // 실제4adea0처럼 발자국 절반을 넓게 계산한 뒤 각 중심 성분을float로 저장한다.
    const auto center=[&types](std::span<const std::uint8_t> raw) {
        const auto& record=types[raw[10]]; // 이 슬롯의 실제 현재 TYPE다.
        return std::array<float,2>{static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(raw,14)))-record.footX*0.5),static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(raw,18)))-record.footY*0.5)};
    };
    const auto from=center(target),to=center(source);const auto direction=RawSquidNeighbors::Direction(from[0],from[1],to[0],to[1]); // 원본 동률의 세로 우선도 보존한다.
    const auto number=Read(target,36);const auto codes=frames[targetType].Codes(); // 현재 raw 프레임 코드의 첫BYTE를 읽는다.
    if (number>=codes.size()) throw std::out_of_range("윈드 타워 현재 프레임 범위 오류");
    const auto side=codes[number].side; // 실제 네 방향L/M/N/O만 방어를 허용한다.
    return (side=='L' && direction==0) || (side=='M' && direction==2) || (side=='N' && direction==4) || (side=='O' && direction==6);
}
}
