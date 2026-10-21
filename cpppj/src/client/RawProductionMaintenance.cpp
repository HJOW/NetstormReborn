// 원본의 signed 절삭/감김·서로 다른 두 시계·순회 중 삭제 순서를 보존한다.
#include "client/RawProductionMaintenance.h"
#include "o/CanonDecoder.h"
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <stdexcept>
namespace netstorm::client {
namespace {
// 비정렬 원본 WORD와 DWORD를 낮은 바이트부터 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=2) {
    std::uint32_t value{};
    // 지정한 물리 폭만 읽고 인접 필드는 보존한다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);return value;
}
// _ftol의 signed64 절삭/하위DWORD를 보존한다. NaN/범위 밖의 원본 indefinite 하위값은0이다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value>=9223372036854775808.0 || value< -9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
constexpr std::array<double,3> kStockIntervals{30,15,8}; // 원본 옵션BYTE2가 고르는 실제 초 간격이다.
}
// 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 실패한 설정은 기존 지정 순서/소비 횟수를 보존한다.
void ProductionMaintenanceGlobals::ConfigureTestBridgePatterns(std::string_view text) {
    std::vector<std::uint32_t> parsed;
    if (!text.empty() && text!="off") {
        std::size_t start{};
        // 모든 번호를 먼저 검증한 뒤 한 번에 새 테스트 순서로 교체한다.
        for (;;) {
            const auto end=text.find(',',start);auto field=text.substr(start,end==text.npos ? end : end-start);
            // 번호 양쪽의 공백만 제거하며 빈 항목은 허용하지 않는다.
            while (!field.empty() && field.front()==' ') field.remove_prefix(1);
            while (!field.empty() && field.back()==' ') field.remove_suffix(1);
            std::uint32_t value{};const auto result=std::from_chars(field.data(),field.data()+field.size(),value);
            if (field.empty() || result.ec!=std::errc{} || result.ptr!=field.data()+field.size() || value>=o::BridgePatterns().size()) throw std::invalid_argument("테스트 다리 조각은0~25의 쉼표 목록 또는 off여야 합니다");
            parsed.push_back(value);if (end==text.npos) break;start=end+1;
        }
    }
    testBridgePatterns=std::move(parsed);testBridgeDraws=0;
}
// 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 0번도 명시 지정으로 처리한다.
std::optional<std::uint32_t> ProductionMaintenanceGlobals::NextTestBridgePattern() {
    if (testBridgePatterns.empty()) return std::nullopt;const auto value=testBridgePatterns[static_cast<std::size_t>(testBridgeDraws%testBridgePatterns.size())];++testBridgeDraws;return value;
}
// 계산 입력이나 효과가 누락된 모듈은 실제 게임에 설치하지 않는다.
RawProductionMaintenance::RawProductionMaintenance(o::SidPool& pool,std::size_t typeCount,const o::ContainedFinderState& finder,ProductionMaintenanceGlobals& globals,ProductionMaintenancePanel& panel,ProductionMaintenanceReads reads,ProductionMaintenanceHooks hooks)
    :pool_(pool),typeCount_(typeCount),finder_(finder),globals_(globals),panel_(panel),reads_(std::move(reads)),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || !reads_.active || !reads_.technology || !reads_.templateTechnology || !reads_.temple || !reads_.editor || !reads_.generatorTimer || !reads_.stockRate || !reads_.artificial || !hooks_.wallSeconds || !hooks_.gameSeconds || !hooks_.money || !hooks_.touchDeck || !hooks_.pattern || !hooks_.create || !hooks_.destroy || !hooks_.allowed || !hooks_.cost || !hooks_.refresh) throw std::invalid_argument("생산 유지의 실제 연결 누락");
}
// 부모0도 기존 검색기처럼 실제0번 슬롯의 머리를 읽는다.
o::Sid RawProductionMaintenance::Parent(std::uint32_t value) const { if (value>=pool_.Capacity()) throw std::out_of_range("생산 유지 현재 부모 범위");return {static_cast<std::uint16_t>(value)}; }
// 원본 기준 정밀도53비트의 산술이며 보조64비트와 다른6.1초 입력도 원본 자료에 보존한다.
void RawProductionMaintenance::Run() const {
    if (!reads_.active() || !reads_.technology()) return;
    if (globals_.stockUntil.empty()) globals_.stockUntil.resize(typeCount_);
    if (globals_.stockUntil.size()!=typeCount_) throw std::logic_error("생산 TYPE별 시계 크기 변경");
    auto parent=Parent(reads_.templateTechnology());o::RawContainedFinder count(pool_,finder_);std::int32_t total{},single{};
    // 원본은 templateOwner 기술의 flags40과 pattern0을 별도로 센다.
    for (auto sid=count.Begin(parent);sid.value;sid=count.Next()) { const auto raw=pool_.Slot(sid);if (Read(raw,20)&0x40) { ++total;if (!Read(raw,12)) ++single; } }
    const bool room=total<panel_.slots; // 신전 소실의 삭제보다 앞서 계산한 결과를 유지한다.
    if (!reads_.editor() && panel_.temple && !reads_.temple()) {
        o::RawContainedFinder remove(pool_,finder_);
        // 신전 소실은 삭제한 현재 슬롯의 next를 뒤에서 읽는다. 재고 금지 삭제의 순서와 다르다.
        for (auto sid=remove.Begin(Parent(reads_.technology()));sid.value;sid=remove.Next()) if (Read(pool_.Slot(sid),20)&0x40) hooks_.destroy(sid);
        panel_.temple=0;
    }
    panel_.temple=reads_.temple();hooks_.touchDeck();
    if (!reads_.artificial(parent) && (reads_.editor() || reads_.temple()) && (panel_.bridgeUntil<=hooks_.wallSeconds() || !reads_.generatorTimer())) {
        panel_.bridgeUntil=hooks_.wallSeconds()+static_cast<float>(globals_.bridgeDelay);
        if (room) {
            auto pattern=hooks_.pattern();const auto before=std::bit_cast<std::int32_t>(globals_.sequence++); // 원본 signed나머지와 DWORD 증가 감김이다.
            if (before%5==0 && !single) pattern=0;
            if (globals_.forcedPattern) pattern=globals_.forcedPattern;
            // 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 원본 난수/한 칸 보충 결정 후 지정 결과만 덮어쓴다.
            if (const auto specified=globals_.NextTestBridgePattern()) pattern=*specified;
            hooks_.create(parent,globals_.bridgeType,pattern,0x40,0x10,0);
        }
    }
    bool teleportChanged{},bridgeChanged{};o::RawContainedFinder contents(pool_,finder_);auto sid=contents.Begin(Parent(reads_.technology()));
    // 실제 체인을 순회하며 원본과 같은 위치에서만 내용물 필드를 변경한다.
    while (sid.value) {
        auto raw=pool_.AllocatedBytes(sid);
        if ((Read(raw,20)&0x40) && !reads_.editor() && std::bit_cast<std::int32_t>(Read(raw,31,4))<=Truncate(hooks_.gameSeconds()*10.0) && !(Read(raw,20)&0x100)) {
            if (globals_.quickBridges) { if (!raw[30]) { raw[30]=2;bridgeChanged=true; } }
            else if (!raw[30]) {
                raw[30]=1;const auto until=60U-static_cast<std::uint32_t>(Truncate(hooks_.gameSeconds()*-10.0));
                // 실제 timer는 게임 시각의10배에 대한 signed DWORD다.60은6초를 뜻한다.
                for (std::size_t i=0;i<4;++i) raw[31+i]=static_cast<std::uint8_t>(until>>(i*8));bridgeChanged=true;
            } else if (raw[30]==1) { raw[30]=2;bridgeChanged=true; }
        }
        raw=pool_.AllocatedBytes(sid);
        if (Read(raw,20)&8) {
            const auto kind=Read(raw,18); // 현재 내용물의 실제 kind WORD다.
            if (kind>=typeCount_) throw std::out_of_range("생산 유지 내용물 종류 범위");
            if (globals_.stockEnabled && globals_.stockUntil[kind]<=hooks_.wallSeconds()) {
                auto next=static_cast<std::int32_t>(raw[30])+1;if (globals_.stockLimit && globals_.stockLimit<next) next=globals_.stockLimit;raw[30]=static_cast<std::uint8_t>(next);
                const auto rate=reads_.stockRate();if (rate>=kStockIntervals.size()) throw std::out_of_range("생산 재고 옵션 범위");globals_.stockUntil[kind]=kStockIntervals[rate]+hooks_.wallSeconds();
                if (reads_.editor()) globals_.stockUntil[kind]=hooks_.wallSeconds();teleportChanged=true;
            }
            const auto money=hooks_.money(); // 원본은 각 내용물마다 현재 SP를 다시 읽는다.
            if (static_cast<double>(money)!=static_cast<double>(panel_.oldMoney)) {
                if (!hooks_.allowed(kind)) { const auto removed=sid;sid=contents.Next();hooks_.destroy(removed);continue; }
                const auto cost=hooks_.cost(kind); // 이전금액 비교는 정수끼리, 현재금액은 실제 float32 비용과 비교한다.
                if ((cost<=panel_.oldMoney)!=(static_cast<float>(cost)<=hooks_.money())) teleportChanged=true;
            }
        }
        sid=contents.Next();
    }
    panel_.oldMoney=Truncate(hooks_.money());
    if (teleportChanged) hooks_.refresh(1);
    if (bridgeChanged) hooks_.refresh(2);
}
}
