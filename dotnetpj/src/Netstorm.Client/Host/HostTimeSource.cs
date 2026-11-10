using System.Diagnostics;

namespace Netstorm.Client.Host;

/// <summary>고정 시각 모드의 설정이다. 프레임 밀리초가 0이면 꺼진 상태다.
/// 근거: cpppj/src/client/ClientTimeSource.h의 FixedClientTime. (2026-11-10 L2·L3 장치에서 추가했다)</summary>
/// <param name="FrameMilliseconds">프레임마다 전진하는 가상 밀리초(1~60000)다.</param>
/// <param name="OriginMilliseconds">첫 시계 원점으로 쓰는 가상 timeGetTime 값이다.</param>
/// <param name="UnixSeconds">가상 시계 원점에 대응하는 유닉스 초(기본 2000-01-01 00:00:00 UTC)다.</param>
public sealed record HostFixedTime(
    uint FrameMilliseconds,
    uint OriginMilliseconds = HostTimeSource.DefaultOriginMilliseconds,
    int UnixSeconds = HostTimeSource.DefaultUnixSeconds);

/// <summary>호스트가 읽는 OS 시각(밀리초·유닉스 초)의 단일 공급원이다.
/// 기본은 OS 시계를 그대로 읽어 기존 동작과 같다. 검사 전용 고정 시각 모드(--fixed-frame-ms)를 켜면
/// 프레임마다 정해진 밀리초만큼만 전진하는 가상 시계를 돌려주어, 같은 입력 단계표의 두 실행이 같은 시각을 쓰게 한다.
/// 근거: cpppj/src/client/ClientTimeSource.h·ClientTimeSource.cpp. (2026-11-10 L2·L3 장치에서 추가했다)
/// cpppj와 달리 프로세스 전역이 아니라 호스트가 소유하는 인스턴스다. 같은 프로세스에 호스트는 하나다.</summary>
public sealed class HostTimeSource
{
    /// <summary>고정 모드의 기본 가상 timeGetTime 원점이다.</summary>
    public const uint DefaultOriginMilliseconds = 1000000u;

    /// <summary>고정 모드의 기본 유닉스 초(2000-01-01 00:00:00 UTC)다.</summary>
    public const int DefaultUnixSeconds = 946684800;

    /// <summary>프레임 밀리초의 상한이다.</summary>
    public const uint MaxFrameMilliseconds = 60000u;

    /// <summary>기본 모드의 단조 실시간 시계다.</summary>
    private readonly Stopwatch _wall = Stopwatch.StartNew();

    /// <summary>켠 고정 설정이다. null이면 꺼진 상태다.</summary>
    private HostFixedTime? _fixed;

    /// <summary>원점 뒤 가상 경과 밀리초다.</summary>
    private ulong _elapsed;

    /// <summary>고정 시각 모드가 켜져 있는지 반환한다. 최대 FPS 바쁜 대기 생략에 쓴다.</summary>
    public bool FixedEnabled => _fixed is not null;

    /// <summary>고정 시각 모드를 켠다. 호스트 생성 뒤 실행 전에 한 번만 부르며, 범위 밖 값이나 두 번째 호출은 거부한다.</summary>
    /// <param name="setting">프레임 밀리초·원점·유닉스 초 설정.</param>
    public void EnableFixed(HostFixedTime setting)
    {
        ArgumentNullException.ThrowIfNull(setting);
        if (setting.FrameMilliseconds == 0 || setting.FrameMilliseconds > MaxFrameMilliseconds)
            throw new ArgumentException("고정 시각 프레임 밀리초는 1~60000이어야 합니다", nameof(setting));
        if (setting.UnixSeconds < 0)
            throw new ArgumentException("고정 시각 유닉스 초는 0 이상이어야 합니다", nameof(setting));
        if (FixedEnabled)
            throw new InvalidOperationException("고정 시각 모드는 한 번만 켤 수 있습니다");
        _fixed = setting;
        _elapsed = 0;
    }

    /// <summary>현재 호스트 밀리초다. 기본은 OS 틱(timeGetTime 상당), 고정 모드는 원점+가상 경과(unsigned 32비트 감김)다.</summary>
    public uint Milliseconds => _fixed is null
        ? unchecked((uint)Environment.TickCount)
        : unchecked(_fixed.OriginMilliseconds + (uint)_elapsed);

    /// <summary>현재 유닉스 초다. 기본은 OS 시계, 고정 모드는 설정 초+가상 경과 초(내림)다. time32 범위 밖은 거부한다.</summary>
    public int UnixSeconds
    {
        get
        {
            long now = _fixed is null
                ? DateTimeOffset.UtcNow.ToUnixTimeSeconds()
                : (long)_fixed.UnixSeconds + (long)(_elapsed / 1000u);
            if (now < 0 || now > int.MaxValue)
                throw new ArgumentOutOfRangeException(nameof(UnixSeconds), "실제 time32 지원 날짜 범위 초과");
            return (int)now;
        }
    }

    /// <summary>메뉴·위젯이 쓰는 현재 초(double)다. 기본은 단조 실시간 경과, 고정 모드는 가상 밀리초를 초로 나눈 값이다.</summary>
    public double NowSeconds => _fixed is null ? _wall.Elapsed.TotalSeconds : Milliseconds / 1000.0;

    /// <summary>고정 모드에서 가상 시계를 한 프레임만큼 전진한다. 꺼져 있으면 아무것도 하지 않는다.</summary>
    public void Advance()
    {
        if (_fixed is not null)
            _elapsed += _fixed.FrameMilliseconds;
    }
}
