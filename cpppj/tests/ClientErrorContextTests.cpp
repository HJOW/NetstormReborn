// 실제 표시 상세/Regular 객체 문맥이 GUI 없이 사용자 로그에 남고 게임 바이트는 유지되는지 검사한다.
#include "TestSupport.h"
#include "FormTypeInputs.h"
#include "client/ClientDiagnostics.h"
#include "client/RawSurfaceWorld.h"
#include "platform/ErrorLog.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <windows.h>

namespace { // 검사 전용 로그 경로만 소유하고 실제 게임/사용자 저장소를 열지 않는다.
struct ContextLogs {
    std::filesystem::path base,logs; // 검사 프로세스의 새 임시 루트와 그 내부 logs다.
    std::wstring previousWindow; // 검사 중 원문 assertion 확인 창을 끄고 기존 환경 값을 복원한다.
    static std::uint32_t Next() { static std::uint32_t counter{};return ++counter; } // 같은 프로세스에서 임시 경로를 재사용하지 않는다.
    ContextLogs() {
        base=std::filesystem::temp_directory_path()/("netstorm-error-context-1377-"+std::to_string(GetCurrentProcessId())+'-'+std::to_string(Next()));logs=base/"logs";
        if (std::filesystem::exists(base)) throw std::runtime_error("진단 검사 임시 경로가 이미 존재합니다");
        std::array<wchar_t,1024> window{};const auto size=GetEnvironmentVariableW(L"NETSTORM_ASSERT_WINDOW",window.data(),static_cast<DWORD>(window.size()));if (size && size<window.size()) previousWindow.assign(window.data(),size);
        SetEnvironmentVariableW(L"NETSTORM_ASSERT_WINDOW",nullptr);netstorm::platform::ErrorLog::ResetForTesting();netstorm::platform::ErrorLog::Configure(logs,"11.01");
    }
    ~ContextLogs() { // 자신이 만든 고정 접두/임시 부모 경로만 지우고 공통 기록기와 환경을 되돌린다.
        netstorm::platform::ErrorLog::ResetForTesting();SetEnvironmentVariableW(L"NETSTORM_ASSERT_WINDOW",previousWindow.empty() ? nullptr:previousWindow.c_str());
        if (base.parent_path()==std::filesystem::temp_directory_path() && base.filename().string().starts_with("netstorm-error-context-1377-")) { std::error_code error;std::filesystem::remove_all(base,error); }
    }
    std::string Read() const { // 현재 로그가 실제 지정 경로에 존재하는지 확인하고 파일 바이트를 읽는다.
        const auto file=netstorm::platform::ErrorLog::CurrentFile();CHECK(file.parent_path()==std::filesystem::absolute(logs));std::ifstream input(file,std::ios::binary);return {std::istreambuf_iterator<char>(input),{}};
    }
};
}

// 이전 실제 실패의 TYPE89/frame25와 전체 상세를 같은 Storage→assertion 경계로 파일에 보존한다.
TEST_CASE(ClientErrorContext_ActualDisplayDetailReachesFileWithoutConsumption) {
    ContextLogs logs;auto types=netstorm::test::MakeFormTypeInputs(188);types[89].name="residence";std::vector<netstorm::o::RiftTypeFrames> frames(188,netstorm::o::RiftTypeFrames({}));std::vector<netstorm::o::SquidDisplayShape> shapes(188);shapes[89].frameCount=24;
    netstorm::client::RawSurfaceStorage storage(netstorm::o::OriginalEdition::Patch1078,types,frames,shapes);std::optional<std::string> detail;
    storage.BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { netstorm::client::LogClientDisplayAssertion(detail,expression,file,line); });
    storage.RecordDisplayFrameError(89,25,0);const auto before=detail;storage.AssertDisplay("frameCheck( abstract, frame, false )",R"(..\o\rifttype.h)",225);
    const auto text=logs.Read();CHECK(text.find("type=89 (residence) frame=25 abs=0")!=text.npos && text.find("Type has 24 frames")!=text.npos && text.find("rifttype.h:225 frameCheck")!=text.npos);CHECK(detail==before);
}

// 상세가 없는 일반 단언에는 이전 상세를 합성하지 않는다.
TEST_CASE(ClientErrorContext_EmptyDetailKeepsOrdinaryAssertion) {
    ContextLogs logs;netstorm::client::LogClientDisplayAssertion(std::nullopt,"known","source.cpp",17);const auto text=logs.Read();CHECK(text.find("source.cpp:17 known")!=text.npos && text.find("assertion-detail")==text.npos);
}

// 유효/범위 밖 SID 모두 문맥을 남기되 예외와 실제 풀 바이트를 바꾸지 않는다.
TEST_CASE(ClientErrorContext_RegularFailurePreservesOriginalExceptionAndPool) {
    ContextLogs logs;netstorm::o::SidPool pool(netstorm::o::OriginalEdition::Patch1078,32768);static_cast<void>(pool.Allocate());const auto sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=89;const std::uint32_t frame=25,baseFrame=13;std::memcpy(raw.data()+36,&frame,4);std::memcpy(raw.data()+42,&baseFrame,4);const auto before=std::vector(pool.Bytes().begin(),pool.Bytes().end());bool original{};
    try { try { throw std::runtime_error("원래 Regular 예외"); }catch (...) { netstorm::client::LogRegularFailure(pool,sid,3,0,0.0F);throw; } }catch (const std::runtime_error& error) { original=std::string_view(error.what())=="원래 Regular 예외"; }
    netstorm::client::LogRegularFailure(pool,{65535},7,2,1.0F);const auto text=logs.Read();CHECK(original && text.find("SID15001 TYPE89 event3 count0 payload00000000 frame25 raw42=0000000d")!=text.npos && text.find("SID65535 TYPEunknown event7")!=text.npos);CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));
}

// CD는36BYTE 슬롯의 프레임BYTE만 읽으며 PATCH 추가 필드를 가정하지 않는다.
TEST_CASE(ClientErrorContext_CdFrameUsesByteWithinActualSlot) {
    ContextLogs logs;netstorm::o::SidPool pool(netstorm::o::OriginalEdition::Cd1072,32768);const auto sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=89;raw[34]=23;const auto before=std::vector(pool.Bytes().begin(),pool.Bytes().end());netstorm::client::LogRegularFailure(pool,sid,3,0,0.0F);const auto text=logs.Read();CHECK(text.find("frame23")!=text.npos && text.find("raw42=")==text.npos);CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));
}
