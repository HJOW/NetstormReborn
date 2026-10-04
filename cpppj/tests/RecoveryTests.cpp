// 복원된 공용 계층의 경계·판본 차이·실행 순서를 검증한다.
#include "TestSupport.h"
#include "o/BaseFile.h"
#include "o/Config.h"
#include "o/GameClock.h"
#include "o/Kernel.h"
#include "o/OriginalText.h"
#include "o/RiftType.h"
#include "o/Xlat.h"
#include <algorithm>
#include <functional>
#include <stdexcept>

using namespace netstorm::o;
namespace {
// 예외가 필요한 잘린 데이터·잘못된 입력을 검사한다.
bool Throws(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return true; }
    return false;
}
// 테스트 바이트 배열에 원본 리틀 엔디언 정수를 기록한다.
void PutU32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
    // 낮은 바이트부터 정확히 4바이트를 쓴다.
    for (std::size_t i = 0; i < 4; ++i) bytes.at(offset+i) = static_cast<std::uint8_t>(value >> (i*8));
}
// 헤더 인덱스가 순차 디렉터리 순서와 다른, 복호화 없는 작은 TAFF 파일을 만든다.
std::vector<std::uint8_t> MakeArchive() {
    std::vector<std::uint8_t> bytes(0x56);
    const std::string magic = "TAFF v0.2\x1a";
    std::copy(magic.begin(), magic.end(), bytes.begin());
    PutU32(bytes, 0x14, 2); PutU32(bytes, 0x18, 0);
    PutU32(bytes, 0x1c, 0x2c); PutU32(bytes, 0x20, 0x34);
    PutU32(bytes, 0x24, 0x20); PutU32(bytes, 0x28, 0x54);
    PutU32(bytes, 0x2c, 0x10); PutU32(bytes, 0x30, 0);
    PutU32(bytes, 0x34, 0); PutU32(bytes, 0x38, 1);
    PutU32(bytes, 0x44, 1); PutU32(bytes, 0x48, 1);
    const std::string first = "\\d\\a"; const std::string second = "\\d\\b";
    std::copy(first.begin(), first.end(), bytes.begin()+0x3c);
    std::copy(second.begin(), second.end(), bytes.begin()+0x4c);
    bytes[0x54] = 'A'; bytes[0x55] = 'B';
    return bytes;
}
// 프레임 콜백이 새 프로세스를 등록·제거할 수 있는 검증용 파생 프로세스.
class CallbackProcess final : public BaseProcess {
public:
    // 매 프레임 호출할 검증 동작을 저장한다.
    explicit CallbackProcess(std::function<void()> callback) : callback_(std::move(callback)) {}
    // 원본 파생 프로세스 실행 메서드에 해당하는 콜백을 호출한다.
    void RunFrame() override { callback_(); }
private:
    std::function<void()> callback_;
};
}

// 이름 인덱스·비암호화 플래그·파일 경계 처리 검증.
TEST_CASE(Taff_IndexAndFlags_AreReadFromHeader) {
    const auto archive = TaffArchive(MakeArchive());
    CHECK(archive.Entries().front().name == "\\d\\b");
    CHECK(archive.Read(".\\D\\A") == std::vector<std::uint8_t>{'A'});
    CHECK(archive.Read("d/b") == std::vector<std::uint8_t>{'B'});
    CHECK(Throws([&] { archive.Read("d/missing"); }));
    auto corrupt = MakeArchive();
    PutU32(corrupt, 0x2c, 0xfffffffc);
    CHECK(Throws([&] { TaffArchive invalid(corrupt); }));
    corrupt = MakeArchive(); corrupt.pop_back();
    CHECK(Throws([&] { TaffArchive invalid(corrupt); }));
    corrupt = MakeArchive(); PutU32(corrupt, 0x14, 0xffffffff);
    CHECK(Throws([&] { TaffArchive invalid(corrupt); }));
}

// 같은 키라도 앞 파일이 우선하며, 원시 공백·이스케이프는 보존한다.
TEST_CASE(Config_FirstMatchAndQuotedComments_ArePreserved) {
    ConfigText config("A=\"first\"\r\nA=\"last\"\nQuick Help Spec=\"a//b`\"c\" //tail");
    CHECK(config.GetRaw("a") == "first");
    CHECK(config.GetRaw("quick help spec") == "a//b`\"c ");
    CHECK(!config.GetRaw("Quick Help"));
    config.Append("\nA=\"third\"\nB=ok");
    CHECK(config.GetRaw("A") == "first");
    CHECK(config.GetRaw("b") == "ok");
    ConfigText appended("");
    appended.Append("A=one\n\r\n");
    appended.Append("B=two\r");
    CHECK(appended.Text() == "A=one\r\nB=two\r\n");
    CHECK(appended.GetRaw("B") == "two");
}

// XOR 설정과 원본 Windows-1252 문자를 읽되 새 UTF-8 입력을 다시 변환하지 않는다.
TEST_CASE(Config_EncodedSignatureAndWindows1252_AreDecoded) {
    const std::string original = "mQdsTA=\"caf\xe9\"\n";
    std::vector<std::uint8_t> bytes(original.begin(), original.end());
    ApplyXor(bytes);
    CHECK(bytes[0] == 0 && bytes[2] == 0);
    CHECK(ConfigText::FromBytes(bytes).GetRaw("A") == "caf\xc3\xa9");
    CHECK(DecodeOriginalText(std::vector<std::uint8_t>{0x80,0x97}) == "\xe2\x82\xac\xe2\x80\x94");
    CHECK(DecodeOriginalText(std::vector<std::uint8_t>{0xef,0xbb,0xbf,0xea,0xb0,0x80}) == "가");
    bytes[3] ^= 1;
    CHECK(Throws([&] { ConfigText::FromBytes(bytes); }));
}

// 번역의 구분자는 첫 글자만 보며, 나중 중복·영어 폴백·불완전 블록 처리도 원본과 같다.
TEST_CASE(Xlat_DuplicateAndIncompleteBlocks_AreHandled) {
    XlatTable table("header\r\n*first\r\nNone\r\n-translation\r\nAlt\r\n=done\r\n"
                    "*\nNone\n-\nNeu\n=\n*\nBroken\n-\nIncomplete");
    CHECK(table.Size() == 1);
    CHECK(table.Translate("None", 3) == "Neu");
    CHECK(table.Translate("None", 1) == "None");
    CHECK(table.Translate("none", 3) == "none");
    CHECK(table.Translate("Broken", 3) == "Broken");
}

// 일시정지 중 UI 벽시계는 흐르고 게임 시계만 정지한다. 중첩 재개도 마지막에 적용된다.
TEST_CASE(GameClock_NestedPauseAndWrap_AreReproduced) {
    GameClock clock(1000);
    CHECK_NEAR(clock.Capture(2000).game, 1, 1e-12);
    clock.Pause(2000); clock.Pause(2500);
    CHECK_NEAR(clock.Capture(3000).wall, 2, 1e-12);
    CHECK_NEAR(clock.GameSeconds(3000), 1, 1e-12);
    clock.Resume(3500); CHECK(clock.IsPaused());
    clock.Resume(4000); CHECK(!clock.IsPaused());
    const auto frame = clock.Capture(4500);
    CHECK_NEAR(frame.game, 1.5, 1e-12);
    CHECK_NEAR(frame.delta, 0.5, 1e-12);
    clock.Resume(4600); CHECK_NEAR(clock.GameSeconds(4600), 1.6, 1e-12);
    GameClock wrapping(0xfffffff0);
    CHECK_NEAR(wrapping.WallSeconds(0x10), 0.032, 1e-12);
}

// 실행 중 뒤 슬롯 생성은 같은 프레임에 반영하고, 지난 슬롯 생성은 다음 프레임까지 기다린다.
TEST_CASE(Kernel_MutationUsesLiveSlots_InOriginalOrder) {
    Kernel kernel;
    std::vector<int> calls;
    bool added = false;
    bool reused = false;
    int phase = 0;
    kernel.Add(std::make_unique<CallbackProcess>([&] {
        calls.push_back(1);
        if (!added) {
            added = true;
            kernel.Add(std::make_unique<CallbackProcess>([&] {
                calls.push_back(2);
                if (phase == 1 && !reused) {
                    reused = true;
                    kernel.Add(std::make_unique<CallbackProcess>([&] { calls.push_back(4); }));
                }
            }));
        }
    }));
    kernel.RunFrame();
    CHECK((calls == std::vector<int>{1,2}));
    kernel.Remove(1);
    phase = 1;
    calls.clear(); kernel.RunFrame(); CHECK((calls == std::vector<int>{2}));
    calls.clear(); kernel.RunFrame(); CHECK((calls == std::vector<int>{4,2}));
    CHECK(kernel.Size() == 2);
    CHECK(kProcessCapacity == 39999);
    CHECK(Throws([&] { kernel.Remove(0); }));
    CHECK(Throws([&] { kernel.Remove(39999); }));
}
