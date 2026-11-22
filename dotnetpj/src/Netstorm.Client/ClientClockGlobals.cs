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
}
