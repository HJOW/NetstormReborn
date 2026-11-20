using Netstorm.Client.Player;

// 실제팔레트설치와첫메뉴사이의Client초기화호출순서를유지한다.
namespace Netstorm.Client.Host;

/// <summary>같은시계/Player의Native시작prefix를제품호스트에연결한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>같은Client의실제시계/Frame/보정owner다.</summary>
    private readonly ClientClockGlobals _clientClock;
    /// <summary>같은호스트의콘텐츠재연결이Native초기화를반복하지않게하는수명표식이다.</summary>
    private bool _nativePrefixInitialized;
    /// <summary>같은Frame캡처/음향프레임/메뉴Tick/편집Poll을스크립트입력전에원문순서로실행한다.</summary>
    private void PrepareNativeFrameInput()
    {
        if (_clientClock.Initialized) _clientClock.CaptureFrame(_clock.Milliseconds);
        _missionAudio?.SetFrame(unchecked((uint)_clientClock.Frame.Value.Number));
        _menuInput?.Tick();
        _editor?.Poll();
    }
    /// <summary>팔레트뒤Clock값대입/보정연결/실제Player초기화를순서대로실행하고정상복귀뒤만완료를기록한다.</summary>
    private void InitializeNativePrefix()
    {
        if (_nativePrefixInitialized) return;
        _clientClock.InitializeFullTime(_clock.Milliseconds); ClientPlayerRuntime.Initialize(_gameplay); _nativePrefixInitialized = true;
    }
}
