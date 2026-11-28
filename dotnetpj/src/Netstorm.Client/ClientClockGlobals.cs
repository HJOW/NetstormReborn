using Netstorm.Core.Original;

// 같은Client시계/Frame주소와실제보정DWORD의수명을유지한다.
namespace Netstorm.Client;

/// <summary>ClientMain의init full time과장기시계/프레임소유자를연결한다.</summary>
public sealed class ClientClockGlobals
{
    /// <summary>같은실제0055b4ac와그backing을소유하는장기전역이다.</summary>
    private readonly ClientLocalStateGlobals _local;
    /// <summary>같은 시계 보정과 Host가 빌리는 실제 장기 로컬 상태 owner다.</summary>
    public ClientLocalStateGlobals LocalState => _local;
    /// <summary>같은FrameTime.game필드를읽고쓰는프로세스시각주소다.</summary>
    private sealed class FrameGame(FrameTimeBox frame) : DoubleBox
    {
        /// <summary>같은실제프레임저장소를대여끝까지보유한다.</summary>
        private readonly FrameTimeBox _frame = frame;
        /// <summary>별도시각복사없이현재프레임의game을직접읽고쓴다.</summary>
        public override double Value { get => _frame.Value.Game; set => _frame.Value.Game = value; }
    }
    /// <summary>Client생성전에존재하고init full time에서도교체하지않는같은시계다.</summary>
    public GameClock Clock { get; } = new();
    /// <summary>시계내부프레임과구별되는Client의같은FrameTime주소다.</summary>
    public FrameTimeBox Frame { get; } = new();
    /// <summary>같은실제Frame.game을빌리는안정된게임시각상자다.</summary>
    public DoubleBox GameTime { get; }
    /// <summary>같은Client의명시초기화가정상복귀했는지다.</summary>
    public bool Initialized { get; private set; }
    /// <summary>메뉴/브리핑 UI가 한 번 보유한 정지 표식이다. 실제 시계의 중첩 깊이와 구별한다.</summary>
    private bool _uiPaused;
    /// <summary>현재 UI의 단일 정지 보유 여부를 읽는다. 미션 스크립트와 영상 갱신 잠금은 별도 소유자다.</summary>
    public bool UiPauseHeld => _uiPaused;
    /// <summary>현재값을읽거나초기화하지않고같은장기보정owner만보관한다.</summary>
    public ClientClockGlobals(ClientLocalStateGlobals local) { ArgumentNullException.ThrowIfNull(local); _local = local; GameTime = new FrameGame(Frame); }
    /// <summary>팔레트뒤원문위치에서한번호출한다. 시간값대입뒤같은실제보정주소를연결하고외부Frame값은보존한다.</summary>
    public void InitializeFullTime(uint milliseconds)
    {
        if (Initialized) throw new InvalidOperationException("같은Client의init full time중복호출");
        Clock.Initialize(milliseconds); Clock.BindSynchronizationTicks(() => _local.GameTicks.Value, _local); Initialized = true;
    }
    /// <summary>실제한번의Capture결과를같은외부Frame주소에도대입한다. 호출자는원문프레임위치에서시간값을공급한다.</summary>
    public FrameTime CaptureFrame(uint milliseconds)
    {
        if (!Initialized) throw new InvalidOperationException("Client시계초기화전프레임캡처");
        FrameTime value = Clock.Capture(milliseconds); Frame.Value = value; return value;
    }
    /// <summary>UI의 정지 값이 바뀐 경우만 현재 밀리초를 읽어 한 깊이를 보유/반납하고 성공 뒤 표식을 기록한다.</summary>
    /// <remarks>cpppj ClientMain.cpp의 Pause와 같다. 중복 요청은 시각 공급도 호출하지 않으며 프레임을 재캡처하지 않는다.</remarks>
    public void Pause(bool paused, Func<uint> milliseconds)
    {
        if (paused == _uiPaused) return;
        ArgumentNullException.ThrowIfNull(milliseconds);
        if (paused) PushGamePause(milliseconds());
        else PopGamePause(milliseconds());
        _uiPaused = paused;
    }
    /// <summary>원문00460df0처럼 같은 실제 시계의 정지 깊이를 한 번 증가시킨다. UI 표식과 음향/영상 잠금은 변경하지 않는다.</summary>
    public void PushGamePause(uint milliseconds) => Clock.Pause(milliseconds);
    /// <summary>원문00460e30처럼 실제 정지 깊이를 한 번 줄이고 마지막 깊이가 풀린 경우만 UI 보유 표식도 반납한다.</summary>
    public void PopGamePause(uint milliseconds)
    {
        Clock.Resume(milliseconds);
        if (!Clock.IsPaused()) _uiPaused = false;
    }
}
