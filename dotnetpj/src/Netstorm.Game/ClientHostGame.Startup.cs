using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // GPU content와 장기 Client domain의 최초 시작을 구별한다.

/// <summary>시작의 성공 접두/첫 실패를 유지하고 원문 Issue→Intro→Scene→Chat 상대 순서를 연결한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>메뉴/입력/미션 domain 전체는 같은 Client에 한 번만 만든다.</summary>
    private readonly ClientStartupStep _nativeDomainStartup = new("Native domain");
    /// <summary>같은 Clock 초기화의 정상 복귀를 별도로 보관한다.</summary>
    private readonly ClientStartupStep _nativeClockStartup = new("Clock");
    /// <summary>같은 Player 초기화의 정상 복귀를 별도로 보관한다.</summary>
    private readonly ClientStartupStep _nativePlayerStartup = new("Player");
    /// <summary>부분 명령 등록 뒤 Issue 전체를 자동 재시도하지 않는다.</summary>
    private readonly ClientStartupStep _nativeIssueStartup = new("Issue");
    /// <summary>실제 인트로 감지의 완료/실패를 보관한다.</summary>
    private readonly ClientStartupStep _nativeIntroStartup = new("Intro");
    /// <summary>첫 Scene Start가 소비한 곡/기한/난수를 반복하지 않는다.</summary>
    private readonly ClientStartupStep _nativeSceneStartup = new("Scene Start");
    /// <summary>장기 Chat manager의 정상 완료와 부분 실패를 구별한다.</summary>
    private readonly ClientStartupStep _nativeChatStartup = new("Chat manager");

    /// <summary>같은 실제 Scene/월드/RNG와 장기 음향을 바인딩한다. 이 함수는 Start/Frame을 호출하지 않는다.</summary>
    private void BindNativeSceneMusic()
    {
        HostMissionAudio audio = _missionAudio ?? throw new InvalidOperationException("실제 Scene 음향 소유자 미설치");
        _stateResources.RequireMusicOwner(audio.Scene);
        audio.BindSceneMusic(_gameRandom, SceneMusicWorld, RefreshNativeSceneDisplay, LoadScenePalette, () => RequestNativeThunderFlash());
    }

    /// <summary>원문처럼 실제 메뉴가 준비된 경우에만 같은 SP 표시를 갱신한다.</summary>
    private void RefreshNativeSceneDisplay()
    {
        if (_mainMenu is not null) RefreshNativeStormPower();
    }

    /// <summary>실제 Issue 뒤·Scene 시작 전에 인트로 경로/존재를 조회한다. UI 객체를 요구하지 않는다.</summary>
    private void DetectNativeIntro()
    {
        HostStartupScene startup = _startup ?? throw new InvalidOperationException("인트로 감지의 실제 설정 미설치");
        RawIntroMenu.DetectIntroMovie(_localState.IntroAvailable,
            () => { string path = ""; startup.Configuration.ReadString("introMovie", ref path); return path; },
            name => startup.Files.TryReadAllBytes(name) is not null ? 1u : 0u);
    }

    /// <summary>첫 메뉴보다 먼저 Issue/Intro/Scene/Chat을 실제 순서로 한 번씩 실행한다.</summary>
    private void InitializeNativeMenuPrefix()
    {
        _nativeIssueStartup.Run(InitializeNativeIssueCommands);
        _nativeIntroStartup.Run(DetectNativeIntro);
        _nativeSceneStartup.Run(() => (_missionAudio ?? throw new InvalidOperationException("첫 Scene의 실제 음향 미설치")).StartScene(false));
        _nativeChatStartup.Run(InitializeNativeChatManager);
    }
}
