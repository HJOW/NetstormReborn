// 실제 디코더 장치의 실패/원래 메타데이터/프레임 수명/Windows 한글 경로를 확인한다.
#include "TestSupport.h"
#include "client/SmackerDecoder.h"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace { // 보호 CD 원문을 읽기만 하고 임시 사본은 빌드 전용 경로에 둔다.
using namespace netstorm::client;
const std::filesystem::path kMovie=std::filesystem::path(NETSTORM_SMACKER_DISC_ROOT)/L"englishintro2x.smk"; // 실제 CD 영상의 고정 입력이다.
}

// 잘못된 입력과 누락 DLL은 프레임 성공으로 바뀌지 않는다.
TEST_CASE(SmackerDecoder_FailedOpenDoesNotExposeAFrame) {
    CHECK(!SmackerDecoder::Open({}));CHECK(!SmackerDecoder::Open({'B','A','D','!'}));CHECK(!SmackerDecoder::Open({'S','M','K','2'},L"NetstormSmacker-missing-591.dll"));
}
// ffprobe가 독립 관측한 실제 규격과 첫/다음 프레임의 빌린 버퍼 계약을 확인한다.
TEST_CASE(SmackerDecoder_DiscMetadataAndCurrentFrameViews) {
    auto decoder=SmackerDecoder::OpenFile(kMovie);CHECK(decoder!=nullptr);if (!decoder) return;const auto& info=decoder->Info();
    CHECK(info.width==640 && info.height==320 && info.frameCount==1890 && info.microsecondsPerFrame==66660 && info.yScaleMode==0);
    CHECK(info.audio[0].exists && info.audio[0].channels==2 && info.audio[0].bitsPerSample==16 && info.audio[0].sampleRate==22050);
    bool rejected{};try { static_cast<void>(decoder->Pixels()); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    CHECK(decoder->Next()==SmackerFrameStatus::More && decoder->FrameIndex()==0);CHECK(decoder->Pixels().size()==640*320 && decoder->Palette().size()==768 && !decoder->Audio(0).empty() && decoder->Audio(1).empty());
    CHECK(decoder->Next()==SmackerFrameStatus::More && decoder->FrameIndex()==1 && decoder->Pixels().size()==640*320);
}
// 원본 ANSI 경로 대신 Windows10/11의 유니코드 파일 장치를 사용하는 실제 차이를 확인한다.
TEST_CASE(SmackerDecoder_KoreanPathReadsTheSameMediaBytes) {
    const auto folder=std::filesystem::path(NETSTORM_SMACKER_TEST_ROOT)/L"한글 영상";std::filesystem::create_directories(folder);const auto copy=folder/L"인트로.smk";
    if (!std::filesystem::exists(copy)) std::filesystem::copy_file(kMovie,copy); // 새 검사 사본만 만들고 기존 파일/보호 CD를 덮어쓰지 않는다.
    auto first=SmackerDecoder::OpenFile(kMovie),second=SmackerDecoder::OpenFile(copy);CHECK(first && second);if (!first || !second) return;
    CHECK(first->Next()==SmackerFrameStatus::More && second->Next()==SmackerFrameStatus::More);CHECK(std::equal(first->Pixels().begin(),first->Pixels().end(),second->Pixels().begin()));
    CHECK(std::equal(first->Palette().begin(),first->Palette().end(),second->Palette().begin()));CHECK(first->Audio(0).size()==second->Audio(0).size() && std::equal(first->Audio(0).begin(),first->Audio(0).end(),second->Audio(0).begin()));
    first.reset();CHECK(second->Next()==SmackerFrameStatus::More && second->FrameIndex()==1); // 한 스트림 반납이 살아 있는 다른 DLL 소비자를 무효화하지 않는다.
}
