namespace Netstorm.Client.Display;

/// <summary>
/// 원본 번개가 바꾸는 색 번호 범위. 기본 팔레트의 구름 색 228~245번을 사용한다.
/// 근거: cpppj/src/client/PaletteFlash.h. (R2a, 2026-10-28)
/// </summary>
public static class PaletteFlashRange
{
    /// <summary>번개가 바꾸는 첫 색 번호.</summary>
    public const uint FirstColor = 228;

    /// <summary>번개가 바꾸는 색 개수.</summary>
    public const uint ColorCount = 18;

    /// <summary>밝기 단계 수. 원본은 RGB를 기본 64만큼 밝히고 매 단계 1씩 복구한다.</summary>
    public const int Steps = 64;

    /// <summary>생성 요청의 밝기 0은 기본 밝기 64를 뜻한다.</summary>
    /// <param name="requested">요청 밝기.</param>
    public static int ThunderOffset(int requested) => requested == 0 ? 64 : requested;

    /// <summary>생성 당시 전체화면 여부로 수명을 정한다. 창 모드는 진행만 하고 팔레트를 표시하지 않는다.</summary>
    /// <param name="fullScreen">전체화면이면 참이다.</param>
    public static double ThunderDuration(bool fullScreen) => fullScreen ? 0.5 : 1.0 / 60.0;
}

/// <summary>
/// 원본 +0xc/+0x40c의 원래 색·변경 색과 +0x810 이후 진행 상태. 색 배열은 부분 팔레트를 앞에서부터 담는다.
/// 근거: cpppj/src/client/PaletteFlash.h 의 PaletteFlashState. (R2a, 2026-10-28)
/// </summary>
public sealed class PaletteFlashState
{
    /// <summary>원래 색 256개(앞 count개만 쓴다).</summary>
    public ScreenColor[] Original { get; } = new ScreenColor[256];

    /// <summary>현재 색 256개(앞 count개만 쓴다).</summary>
    public ScreenColor[] Current { get; } = new ScreenColor[256];

    /// <summary>반복 갱신의 시간 간격.</summary>
    public double Interval { get; set; }

    /// <summary>구간의 마지막 시각.</summary>
    public double Last { get; set; }

    /// <summary>효과를 유지할 시간.</summary>
    public double Duration { get; set; }

    /// <summary>현재 처리 구간의 시작 색 번호.</summary>
    public uint Start { get; set; } = PaletteFlashRange.FirstColor;

    /// <summary>처리할 색 개수.</summary>
    public uint Count { get; set; } = PaletteFlashRange.ColorCount;

    /// <summary>밝히기에 더할 기준 오프셋.</summary>
    public int Offset { get; set; } = 64;

    /// <summary>offset 0인 일반 색 전환 경로의 목표 색.</summary>
    public ScreenColor Solid { get; set; }

    /// <summary>남은 진행 단계 수.</summary>
    public int Steps { get; set; }

    /// <summary>초기화가 끝나 저장소를 쓸 수 있는지.</summary>
    public bool Initialized { get; set; }
}

/// <summary>
/// 시각은 게임 정지와 무관한 프레임 실시간이다. 화면 모드는 생성 시 수명 결정 뒤에도 매 프레임 다시 읽는다.
/// 근거: cpppj/src/client/PaletteFlash.h 의 PaletteFlashHooks. (R2a, 2026-10-28)
/// </summary>
public sealed class PaletteFlashHooks
{
    /// <summary>게임 정지와 독립적인 벽 시계 초를 공급하는 콜백.</summary>
    public Func<double>? WallSeconds { get; set; }

    /// <summary>현재 화면 장치가 전체화면인지 조회하는 콜백.</summary>
    public Func<bool>? FullScreen { get; set; }

    /// <summary>화면 장치의 현재 팔레트를 읽는 콜백(시작 번호·개수·대상 배열).</summary>
    public Action<uint, uint, ScreenColor[]>? ReadPalette { get; set; }

    /// <summary>화면 장치에 계산한 팔레트를 적용하는 콜백(시작 번호·개수·색 배열).</summary>
    public Action<uint, uint, ScreenColor[]>? ApplyPalette { get; set; }

    /// <summary>당시의 저장 팔레트 전체를 복구한다. 생성 때 읽은 부분 배열을 복구하는 것이 아니다.</summary>
    public Action? RestorePalette { get; set; }
}

/// <summary>
/// 실제 팔레트 계산과 종료 판정을 맡는다. 소유자는 Init 뒤 Frame을 호출하고 중단·파괴 전에 Cancel을 호출한다.
/// 근거: cpppj/src/client/PaletteFlash.cpp (원본 00470d40~00471820). (R2a, 2026-10-28)
/// </summary>
public sealed class PaletteFlash
{
    /// <summary>연결한 진행 상태. 호출자가 소유하며 이 객체보다 오래 살아야 한다.</summary>
    private readonly PaletteFlashState _state;

    /// <summary>외부 저장소 조회와 화면 적용을 연결하는 묶음.</summary>
    private readonly PaletteFlashHooks _hooks;

    /// <summary>
    /// 필수 경계와 부분 색 범위를 검사한다. OS 자원을 만들지 않는다.
    /// </summary>
    /// <param name="state">진행 상태.</param>
    /// <param name="hooks">외부 효과 묶음. 비어 있으면 안 된다.</param>
    public PaletteFlash(PaletteFlashState state, PaletteFlashHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.WallSeconds is null || hooks.FullScreen is null || hooks.ReadPalette is null
            || hooks.ApplyPalette is null || hooks.RestorePalette is null)
        {
            throw new ArgumentException("팔레트 번개 효과의 필수 경계가 비었습니다", nameof(hooks));
        }
        if (state.Start > 256 || state.Count > 256 - state.Start)
        {
            throw new ArgumentOutOfRangeException(nameof(state), "팔레트 번개 색 범위가 부분 배열을 벗어났습니다");
        }
        _state = state;
        _hooks = hooks;
    }

    /// <summary>
    /// 원본 00470e30: 현재 색을 두 번 읽고 밝기 변경 또는 목표 색 채우기를 준비한다. 표시하지는 않는다.
    /// </summary>
    public void Init()
    {
        _state.Last = _hooks.WallSeconds!();
        _state.Interval = _state.Duration / 64.0;
        _hooks.ReadPalette!(_state.Start, _state.Count, _state.Original);
        _hooks.ReadPalette!(_state.Start, _state.Count, _state.Current);
        _state.Initialized = true;
        // 부분 팔레트만 변경한다. 예약 바이트는 기존 값을 보존한다
        for (uint i = 0; i < _state.Count; i++)
        {
            ScreenColor color = _state.Current[i];
            _state.Current[i] = _state.Offset == 0
                ? _state.Solid
                : new ScreenColor(Brighten(color.Blue, _state.Offset), Brighten(color.Green, _state.Offset),
                    Brighten(color.Red, _state.Offset), color.Reserved);
        }
        _state.Steps = PaletteFlashRange.Steps;
    }

    /// <summary>
    /// 원본 00471650: interval보다 엄격히 큰 경과 시간마다 RGB를 1씩 복구한다. 계속 진행하면 참이다.
    /// 정상 종료 때 전체화면이면 저장 팔레트를 복구하며, 창 모드에서는 화면 팔레트를 전혀 바꾸지 않는다.
    /// </summary>
    public bool Frame()
    {
        if (!_state.Initialized)
        {
            return false;
        }
        double elapsed = _hooks.WallSeconds!() - _state.Last;
        // 지연된 프레임에서는 남은 단계를 한꺼번에 진행하되 last에 interval을 반복 더하는 원본 반올림 순서를 유지한다
        while (_state.Interval < elapsed && _state.Steps > 0)
        {
            for (uint i = 0; i < _state.Count; i++)
            {
                ScreenColor color = _state.Current[i];
                ScreenColor original = _state.Original[i];
                _state.Current[i] = new ScreenColor(Approach(color.Blue, original.Blue), Approach(color.Green, original.Green),
                    Approach(color.Red, original.Red), color.Reserved);
            }
            _state.Steps--;
            _state.Last += _state.Interval;
            elapsed -= _state.Interval;
        }
        if (_state.Steps < 1 || _state.Steps > PaletteFlashRange.Steps)
        {
            if (_hooks.FullScreen!())
            {
                _hooks.RestorePalette!();
            }
            _state.Initialized = false;
            return false;
        }
        if (_hooks.FullScreen!())
        {
            _hooks.ApplyPalette!(_state.Start, _state.Count, _state.Current);
        }
        return true;
    }

    /// <summary>원본 00471820: 초기화된 전체화면 효과를 중단할 때 저장 팔레트를 복구한다. 중복 취소는 무시한다.</summary>
    public void Cancel()
    {
        if (_state.Initialized && _hooks.FullScreen!())
        {
            _hooks.RestorePalette!();
        }
        _state.Initialized = false;
    }

    /// <summary>원본의 부호 있는 32비트 덧셈·256 상한 비교·byte 저장을 유지한다. 음수 밝기는 아래로 제한하지 않고 byte로 감긴다.</summary>
    private static byte Brighten(byte channel, int offset)
    {
        int sum = unchecked((int)(unchecked((uint)channel + unchecked((uint)offset))));
        return sum < 256 ? unchecked((byte)sum) : (byte)255;
    }

    /// <summary>원래 색보다 작으면 1 증가, 크면 1 감소한다.</summary>
    private static byte Approach(byte channel, byte original)
    {
        if (channel < original)
        {
            return (byte)(channel + 1);
        }
        return channel > original ? (byte)(channel - 1) : channel;
    }
}
