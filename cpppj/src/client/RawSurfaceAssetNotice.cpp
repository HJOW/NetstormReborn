// 실제 가상34의 세 홈 통지를 같은 장기 물리 저장소와 현재 Host에 연결한다.
#include "client/RawSurfaceWorld.h"
#include "client/RawProcessHostStorage.h"
#include "o/RawSunAviaryProcess.h"
#include "o/RawRainAviaryProcess.h"
#include "o/RawWindAviaryProcess.h"
#include "o/RawGameAuxState.h"
#include <stdexcept>

namespace netstorm::client { // 짧은 Session/임시 World를 보유하지 않고 실제 현재 process를 조회한다.
void RawSurfaceStorage::NotifyAviaryHome(o::Sid home,std::uint32_t method) {
    if (!authority_->server) return; // 원문은 서버0이면 Host/TYPE/시각/난수/프로세스 필드를 읽지 않는다.
    if (method==0x0045b4a0U) {
        const auto form=o::FindAttachedProcess(pool,home,gameAux_->SunAviaryProcessType());
        auto* process=form.value ? dynamic_cast<o::SunAviaryProcess*>(Processes().Host().FromForm(form)):nullptr;
        o::RawSunAviaryProcess::FlyerDiedIn(pool,process ? &process->Fields():nullptr,home,
            [this](auto sid,auto flags) { UnpopAsset(sid,flags); },[this](auto sid,auto flags) { RepopAsset(sid,flags); },
            [this](o::SunAviaryProcessFields& fields) { // 현재 Form/필드 정체성을 확인하고 같은 Host의 실제 갱신을 실행한다.
                auto* current=dynamic_cast<o::SunAviaryProcess*>(Processes().Host().FromForm(fields.Form()));
                if (!current || &current->Fields()!=&fields) throw std::logic_error("장기 훨리베이스 홈 통지의 Host/raw 이미지 불일치");
                Processes().Host().RefreshForm(*current);
            },true);
        return;
    }
    if (method==0x0045b5b0U) {
        const auto form=o::FindAttachedProcess(pool,home,gameAux_->RainAviaryProcessType());
        auto* process=form.value ? dynamic_cast<o::RainAviaryProcess*>(Processes().Host().FromForm(form)):nullptr;
        if (!process) throw std::logic_error("맨 오 워 홈 통지의 현재 프로세스 NULL 원본 경계");
        o::RawRainAviaryProcess::FlyerDiedIn(process->Fields(),[this] { return Processes().State().Now(); },
            [this]()->o::GameRandom& { const auto current=resourceDevices_.random;if (!current) throw std::logic_error("장기 맨 오 워 홈 통지의 실제 난수 미결합");return current(); },
            [this](o::RainAviaryProcessFields& fields) { // 난수 뒤 다시 읽는 시각과 실제 현재 Form 갱신 순서를 보존한다.
                auto* current=dynamic_cast<o::RainAviaryProcess*>(Processes().Host().FromForm(fields.Form()));
                if (!current || &current->Fields()!=&fields) throw std::logic_error("장기 맨 오 워 홈 통지의 Host/raw 이미지 불일치");
                Processes().Host().RefreshForm(*current);
            });
        return;
    }
    if (method==0x0045b680U) {
        const auto form=o::FindAttachedProcess(pool,home,gameAux_->WindAviaryProcessType());
        auto* process=form.value ? dynamic_cast<o::WindAviaryProcess*>(Processes().Host().FromForm(form)):nullptr;
        if (!process) throw std::logic_error("데빌메이커 홈 통지의 현재 프로세스 NULL 원본 경계");
        o::RawWindAviaryProcess::FlyerDiedIn(process->Fields(),[this](o::WindAviaryProcessFields& fields) { // AP2/dirty4 뒤 같은 현재 Form만 갱신한다.
            const o::Sid currentForm{static_cast<std::uint16_t>(std::uint32_t(fields.bytes[60])|(std::uint32_t(fields.bytes[61])<<8))}; // 실제 inner+4/outer+60의 현재 Form WORD를 읽는다.
            auto* current=dynamic_cast<o::WindAviaryProcess*>(Processes().Host().FromForm(currentForm));
            if (!current || &current->Fields()!=&fields) throw std::logic_error("장기 데빌메이커 홈 통지의 Host/raw 이미지 불일치");
            Processes().Host().RefreshForm(*current);
        });
        return;
    }
    throw std::logic_error("장기 홈 통지의 실제 가상34 주소 오류");
}
}
