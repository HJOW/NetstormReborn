// Squid 소유자 지정(vtable +0x74)을 저장된 실제 PE 제한 x86 관찰과 비교한다.
#include "TestSupport.h"
#include "o/SquidOwner.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 분석 도구와 같은 합성 풀 크기·객체 번호·타입 번호·목록 저장 칸 수다. 실제 게임 월드 배치를 뜻하지 않는다.
constexpr std::uint32_t kCapacity = 24000, kObjectType = 82, kListSlots = 6, kOwners = 9;
constexpr Sid kTarget{50};
// 명시적인 C++ 보호 예외를 원본 assert 실행 없이 확인한다.
template<class F> bool Throws(F action) { try { action(); return false; } catch (const std::exception&) { return true; } }
// 주석/빈 행을 제외하되 마지막 빈 필드도 보존하는 구분자 판독기다.
std::vector<std::string> Split(std::string_view text, char separator) {
    std::vector<std::string> result;
    std::size_t begin = 0;
    // 남은 마지막 필드까지 추가한다.
    for (;;) {
        const auto end = text.find(separator, begin);
        result.emplace_back(text.substr(begin, end == std::string_view::npos ? end : end - begin));
        if (end == std::string_view::npos) return result;
        begin = end + 1;
    }
}
// Git에 저장된 실제 기계어 기대값만 읽는다. Python/원본 PE는 테스트에 필요 없다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows = [] {
        std::ifstream input(NETSTORM_OWNER_FIXTURE);
        if (!input) throw std::runtime_error("Missing owner fixture");
        std::vector<std::vector<std::string>> result;
        std::string line;
        // 설명 주석만 건너뛴다.
        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty() && line.front() != '#') result.push_back(Split(line, '\t'));
        }
        return result;
    }();
    return rows;
}
// fixture의 부호 없는 DWORD 필드를 읽는다.
std::uint32_t Number(const std::string& value) { return static_cast<std::uint32_t>(std::stoul(value)); }
// Python zlib와 같은 전체 버퍼 Adler-32다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // 표준 소수다. 슬롯은 짧아 매 바이트마다 나머지를 취해도 된다.
    constexpr std::uint64_t prime = 65521;
    std::uint64_t a = 1, b = 0;
    for (auto byte : bytes) { a = (a + byte) % prime; b = (b + a) % prime; }
    return static_cast<std::uint32_t>((b << 16) | a);
}
// 판본마다 풀과 장부를 한 번 만들고, genus 값마다 타입 표가 다른 SquidOwner를 만들어 둔다.
struct Scene {
    OriginalEdition edition;
    SidPool pool;
    SquidPostPopState bookkeeping;
    SquidOwnerMode mode;
    std::map<std::uint32_t, std::unique_ptr<SquidOwner>> owners;
    explicit Scene(OriginalEdition which) : edition(which), pool(which, kCapacity, false) {
        // 클라이언트 영역의 앞 슬롯들을 할당해 합성 번호 50을 준비한다.
        for (int i = 5; i <= 70; ++i) CHECK(pool.Allocate(2).value == i);
    }
    bool Patch() const { return edition == OriginalEdition::Patch1078; }
    // 판본별 소유자·extra 필드 위치다.
    std::size_t OwnerOffset() const { return Patch() ? 0x22 : 0x20; }
    std::size_t ExtraOffset() const { return Patch() ? 0x28 : 0x23; }
    // 타입 표는 SquidOwner가 복사하므로 genus 값마다 하나씩 만들어 재사용한다.
    SquidOwner& For(std::uint32_t genus) {
        auto& slot = owners[genus];
        if (!slot) {
            std::vector<RiftTypeRecord> types(Patch() ? 188 : 171);
            types[kObjectType].flags2 = genus;
            slot = std::make_unique<SquidOwner>(pool, types, bookkeeping, mode);
        }
        return *slot;
    }
};
// 판본 이름으로 고른 행을 모두 재생한다.
void ReplayEdition(const std::string& name, std::size_t minimum) {
    Scene scene(name == "originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::size_t rows = 0;
    // 한 줄이 한 입력이다.
    for (const auto& row : Fixture()) {
        if (row[0] != "Owner" || row[1] != name) continue;
        CHECK(row.size() == 14);
        scene.mode.fort = Number(row[2]) != 0;
        scene.mode.battle = Number(row[3]) != 0;
        scene.mode.challenge = Number(row[4]) != 0;
        // 슬롯을 0으로 지우고 타입·상태·소유자·extra만 채운다.
        auto raw = scene.pool.AllocatedBytes(kTarget);
        std::fill(raw.begin(), raw.end(), std::uint8_t{});
        raw[10] = static_cast<std::uint8_t>(kObjectType);
        raw[11] = static_cast<std::uint8_t>(Number(row[6]));
        raw[scene.OwnerOffset()] = static_cast<std::uint8_t>(Number(row[8]));
        raw[scene.ExtraOffset()] = static_cast<std::uint8_t>(Number(row[7]));
        // 소유자마다 "용량:개수:항목들"이다. 용량까지만 장부 목록에 넣고 그 뒤 칸은 원본 저장소의 옛 메모리로 따로 든다.
        const auto lists = Split(row[10], '|');
        CHECK(lists.size() == kOwners);
        std::vector<std::vector<std::uint32_t>> stale(kOwners);
        for (std::uint32_t owner = 0; owner < kOwners && owner < lists.size(); ++owner) {
            const auto parts = Split(lists[owner], ':');
            const auto entries = Split(parts.at(2), ',');
            const std::uint32_t capacity = Number(parts.at(0));
            auto& list = scene.bookkeeping.ownerFactories[owner];
            list.entries.clear();
            // 앞쪽 용량만큼이 실제 목록 배열이다.
            for (std::uint32_t i = 0; i < kListSlots; ++i) {
                const auto value = Number(entries.at(i));
                if (i < capacity) list.entries.push_back(value); else stale[owner].push_back(value);
            }
            list.count = Number(parts.at(1));
        }
        scene.For(Number(row[5])).Set(kTarget, Number(row[9]));
        // 결과: 소유자 바이트, 소유자마다 "개수:저장 칸 전체", 슬롯 Adler-32.
        std::string actual;
        for (std::uint32_t owner = 0; owner < kOwners; ++owner) {
            const auto& list = scene.bookkeeping.ownerFactories[owner];
            if (owner) actual += '|';
            actual += std::to_string(list.count) + ':';
            std::vector<std::uint32_t> all(list.entries);
            all.insert(all.end(), stale[owner].begin(), stale[owner].end());
            // 저장 칸 전체를 쉼표로 잇는다.
            for (std::size_t i = 0; i < all.size(); ++i) { if (i) actual += ','; actual += std::to_string(all[i]); }
        }
        const auto after = scene.pool.Slot(kTarget);
        const bool same = after[scene.OwnerOffset()] == Number(row[11]) && actual == row[12] && Adler(after) == Number(row[13]);
        CHECK(same);
        if (!same) {
            std::printf("  %s 소유자 %s→%s genus %s 모드 %s%s%s\n    기대 %s | %s\n    실제 %u | %s\n", name.c_str(), row[8].c_str(), row[9].c_str(),
                row[5].c_str(), row[2].c_str(), row[3].c_str(), row[4].c_str(), row[11].c_str(), row[12].c_str(),
                static_cast<unsigned>(after[scene.OwnerOffset()]), actual.c_str());
            break;
        }
        ++rows;
    }
    CHECK(rows >= minimum);
}
}

// 패치 10.78의 실제 기계어 관찰을 재생한다.
TEST_CASE(owner_patch_x86_fixture) { ReplayEdition("originals", 1000); }
// CD 10.72 배포본의 관찰.
TEST_CASE(owner_cd_x86_fixture) { ReplayEdition("originalCD", 1000); }
// 추가 10.37 실행 파일은 CD와 같은 코드 배치지만 별도 PE의 출력을 재생한다.
TEST_CASE(owner_1037_x86_fixture) { ReplayEdition("original1037", 1000); }
// 원본이 assert를 보고하는 범위 밖 소유자와 목록이 없는 번호는 쓰기 전에 거부한다.
TEST_CASE(owner_rejects_out_of_range_players_before_writing) {
    Scene scene(OriginalEdition::Patch1078);
    auto raw = scene.pool.AllocatedBytes(kTarget);
    std::fill(raw.begin(), raw.end(), std::uint8_t{});
    raw[10] = static_cast<std::uint8_t>(kObjectType);
    raw[scene.OwnerOffset()] = 3;
    scene.mode.battle = true;
    auto& owner = scene.For(0x4000);
    scene.bookkeeping.ownerFactories[3].entries = {kTarget.value, 0};
    scene.bookkeeping.ownerFactories[3].count = 1;
    CHECK(Throws([&] { owner.Set(kTarget, 9); }));
    scene.mode.challenge = true;
    CHECK(Throws([&] { owner.Set(kTarget, 0); }));
    CHECK(Throws([&] { owner.Set(kTarget, 20); }));
    CHECK(Throws([&] { owner.Set(kTarget, 40); }));
    // 거부된 호출은 소유자 바이트와 목록을 바꾸지 않는다.
    CHECK(scene.pool.Slot(kTarget)[scene.OwnerOffset()] == 3);
    CHECK(scene.bookkeeping.ownerFactories[3].count == 1);
}
