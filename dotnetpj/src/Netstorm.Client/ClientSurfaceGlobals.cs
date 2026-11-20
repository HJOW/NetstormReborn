using Netstorm.Core.Original;

// 실제 Client의 같은 표면/프로세스 저장소와 장기 장치 주소를 유지한다.
namespace Netstorm.Client;

/// <summary>미션 전에 만들고 미션 반납 뒤에도 유지하는 실제 Surface 소유자다.</summary>
public sealed class ClientSurfaceGlobals
{
    /// <summary>Graph의각조회위치에서현재Client밀리초를읽는실제장치다.</summary>
    private readonly Func<uint> _milliseconds;
    /// <summary>같은 권한/게임 보조 전역을 소유하는 실제 Client다.</summary>
    public ClientGameplayGlobals Gameplay { get; }
    /// <summary>프로세스와 그래프가 같은 Frame 주소를 읽도록 보유하는 실제 시계다.</summary>
    public ClientClockGlobals Clock { get; }
    /// <summary>모든 미션과 장기 프로세스 Host가 빌리는 같은 커널이다.</summary>
    public Kernel Kernel { get; }
    /// <summary>미션 교체 때 새로 만들지 않는 Pool/공간/바람/자산 저장소다.</summary>
    public RawSurfaceStorage Storage { get; }
    /// <summary>같은 저장소의 장기 Host와 현재 Runtime 대여 슬롯이다.</summary>
    public RawProcessHostStorage Processes => Storage.Processes();

    /// <summary>프레임캐시를진행하지않고호출시점의같은GameClock/보정/정지값을읽는다.</summary>
    public double GameSeconds() => Clock.Clock.GameSeconds(_milliseconds());

    /// <summary>Frame캐시를사용하지않고선택시점의실제Client밀리초로벽시각을읽는다.</summary>
    public double WallSeconds() => Clock.Clock.WallSeconds(_milliseconds());

    /// <summary>현재 게임값을 초기화하지 않고 실제 권한/보조 전역과 Kernel/Frame/그래프 시각을 연결한다.</summary>
    public ClientSurfaceGlobals(ClientGameplayGlobals gameplay, ClientClockGlobals clock, Kernel kernel, Func<uint> milliseconds)
    {
        ArgumentNullException.ThrowIfNull(gameplay); ArgumentNullException.ThrowIfNull(clock); ArgumentNullException.ThrowIfNull(kernel); ArgumentNullException.ThrowIfNull(milliseconds);
        Gameplay = gameplay; Clock = clock; Kernel = kernel; _milliseconds = milliseconds;
        Storage = new RawSurfaceStorage(OriginalEdition.Patch1078, gameplay.Authority, gameplay.GameAux);
        Processes.BindDevices(kernel, clock.Frame);
        Storage.BindGraphClock(GameSeconds);
    }
}
