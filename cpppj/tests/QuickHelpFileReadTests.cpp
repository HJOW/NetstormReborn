// 실제 Client의 Help 읽기가 snapshot/미발견/부분FILE/키 반납을 구별하는지 확인한다.
#include "QuickHelpFileReadTestSupport.h"
#include "client/QuickHelpFileRead.h"
#include "o/RawColorMapCache.h"
#include "o/RawFileName.h"

namespace { // 실제 파일 입력과 관측의 수명은 각 검사 안에만 둔다.
using namespace netstorm;using namespace netstorm::test::helpread; // 제품 Client와 검사 입력 작성기를 함께 사용한다.
}

TEST_CASE(QuickHelpFileRead_ActualClientArchiveReadsCurrentPayloadAndKeepsSnapshotApi) { // metadata는 등록 snapshot이어도 명시 Help는 현재 backing FILE을 다시 연다.
    const auto root=Work(),archive=root/"one.tarc";const std::array<std::uint8_t,4> first{{'F','I','R','S'}},second{{'N','E','W','!'}};Write(archive,Archive("d/help.english",first,true));client::Client instance(Options(root));
    CHECK(instance.Files().Read("d/help.english")==std::vector<std::uint8_t>(first.begin(),first.end()));Write(archive,Archive("d/help.english",second,true));const auto before=instance.Files().FileNameOwner()->Index();
    CHECK(client::ReadClientQuickHelpFile(instance,"d/help.english")==std::string("NEW!"));CHECK(instance.Files().FileNameOwner()->Index()==(before+2)%10);CHECK(instance.Files().Read("d/help.english")==std::vector<std::uint8_t>(first.begin(),first.end()));
    const auto secondary=Work();Text(secondary/"d/help.english","SECONDARY");instance.InitializeNativeSecondaryFiles(platform::PathToUtf8(secondary));std::filesystem::rename(archive,root/"moved.tarc");bool rejected{};
    try { static_cast<void>(client::ReadClientQuickHelpFile(instance,"d/help.english")); }catch (const o::ColorCacheUnableToOpen&) { rejected=true; }
    CHECK(rejected && o::ReadFileBytes(secondary/"d/help.english")==std::vector<std::uint8_t>({'S','E','C','O','N','D','A','R','Y'})); // 이미 선택한 TAFF의 열기 실패를 보조 정상 파일로 숨기지 않는다.
}

TEST_CASE(QuickHelpFileRead_ActualClientTruncatedEncryptedFileFailsAfterCloseAndCanRetry) { // metadata가 보장한 논리 길이보다 짧은 payload는 미정 tail의 성공으로 바꾸지 않는다.
    const auto root=Work(),archive=root/"one.tarc";const std::array<std::uint8_t,4> payload{{'H','E','L','P'}};auto bytes=Archive("d/help.english",payload,true);Write(archive,bytes);client::Client instance(Options(root));CHECK(instance.Files().ContainsArchiveOnly("d/help.english"));bytes.resize(bytes.size()-3);Write(archive,bytes);bool rejected{};
    try { static_cast<void>(client::ReadClientQuickHelpFile(instance,"d/help.english")); }catch (const std::runtime_error& error) { rejected=std::string_view(error.what())=="Help 전체읽기의 실제 부분 I/O"; }
    CHECK(rejected);std::filesystem::rename(archive,root/"short.tarc");Write(archive,Archive("d/help.english",payload,true));CHECK(client::ReadClientQuickHelpFile(instance,"d/help.english")==std::string("HELP")); // 예외 뒤에도 같은 파일명은 실제로 교체할 수 있고 새 읽기는 별도 FILE/키를 쓴다.
}

TEST_CASE(QuickHelpFileRead_ActualClientSecondaryRootRefreshAndUnicodeDirectName) { // 파일명 고리의 같은 owner를 빌리며 현재 보조 경로와 직접 Unicode 파일을 각각 읽는다.
    const auto root=Work(),first=Work(),second=Work();std::filesystem::create_directories(root);Text(first/"d/help.english","FIRST");Text(second/"d/help.english","SECOND");client::Client instance(Options(root));instance.InitializeNativeSecondaryFiles(platform::PathToUtf8(first));CHECK(client::ReadClientQuickHelpFile(instance,"d/help.english")==std::string("FIRST"));instance.InitializeNativeSecondaryFiles(platform::PathToUtf8(second));CHECK(client::ReadClientQuickHelpFile(instance,"d/help.english")==std::string("SECOND"));
    const auto direct=root/std::filesystem::path(u8"직접😀.txt");Text(direct,"DIRECT");const auto before=instance.Files().FileNameOwner()->Index();CHECK(client::ReadClientQuickHelpFile(instance,platform::PathToUtf8(direct))==std::string("DIRECT"));CHECK(instance.Files().FileNameOwner()->Index()==(before+1)%10);
}
