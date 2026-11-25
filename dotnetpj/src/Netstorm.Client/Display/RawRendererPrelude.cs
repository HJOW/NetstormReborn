// 원문004994b0..00499758의 화면 flags/override/구름 행 스캔과 전체 dirty 준비 순서다.
namespace Netstorm.Client.Display;

/// <summary>원문 DWORD 관측과 실제 호스트 signed 저장소를 복사하지 않고 같은 현재 값으로 읽는다.</summary>
public sealed class RendererPreludeSignedRef
{
    /// <summary>빌린 DWORD 저장소다. signed 비트로 읽는다.</summary>
    private readonly uint[]? _word;
    /// <summary>빌린 signed 저장소다.</summary>
    private readonly int[]? _integer;
    /// <summary>빌린 위치다.</summary>
    private readonly int _at;

    /// <summary>원문 fixture/packed DWORD를 signed 비트로 읽을 때 사용한다.</summary>
    /// <param name="word">DWORD 배열이다.</param>
    /// <param name="at">읽을 위치다.</param>
    public RendererPreludeSignedRef(uint[] word, int at)
    {
        ArgumentNullException.ThrowIfNull(word);
        _word = word;
        _at = at;
    }

    /// <summary>현재 Screen/지도/카메라의 실제 signed 필드에 연결한다.</summary>
    /// <param name="integer">signed 배열이다.</param>
    /// <param name="at">읽을 위치다.</param>
    public RendererPreludeSignedRef(int[] integer, int at)
    {
        ArgumentNullException.ThrowIfNull(integer);
        _integer = integer;
        _at = at;
    }

    /// <summary>하위 효과 뒤 현재 원본 저장소를 다시 읽는다.</summary>
    /// <returns>signed 값이다.</returns>
    public int Read() => _integer is not null ? _integer[_at] : unchecked((int)_word![_at]);
}

/// <summary>현재 인접 전역과 Renderer 카운터를 빌려 원문과 같은 저장소에 부분 효과를 남긴다.</summary>
public sealed class RendererPreludeState
{
    /// <summary>index·16행·초기화·이전 flags가 같은 인접 물리 19DWORD다.</summary>
    public uint[] Scan { get; } = new uint[19];
    /// <summary>같은 Renderer+60이며 비DIB 전체화면 시차 분기만 1을 쓴다.</summary>
    public uint BackgroundRedraw { get; set; }
    /// <summary>같은 Renderer+64이며 force redraw가 현재 표를 비운 뒤 새 전체 사각형을 요청한다.</summary>
    public uint DirtyCount { get; set; }
    /// <summary>별도 구름 dirty 사각형 개수이며 모든 정상 경로가 마지막에 0을 쓴다.</summary>
    public uint DrawCounter { get; set; }

    /// <summary>같은 인접 물리를 빌려 쓴다.</summary>
    /// <param name="scan">19DWORD 인접 표다.</param>
    public RendererPreludeState(uint[] scan)
    {
        ArgumentNullException.ThrowIfNull(scan);
        if (scan.Length != 19)
        {
            throw new ArgumentException("앞단 행 표는 19DWORD입니다", nameof(scan));
        }
        Scan = scan;
    }
}

/// <summary>화면·카메라·지도 clip은 호출자가 소유하며 각 장치 효과 뒤에도 같은 참조로 읽는다.</summary>
public sealed class RendererPreludeGlobals
{
    /// <summary>현재 화면 flags이며 효과 뒤 재조회한다.</summary>
    public uint Mode { get; set; }
    /// <summary>같은 세계 redraw 요청이며 이 앞단은 지우지 않는다.</summary>
    public uint Force { get; set; }
    /// <summary>현재 논리 화면 가로다.</summary>
    public RendererPreludeSignedRef Width { get; set; } = null!;
    /// <summary>현재 논리 화면 세로다.</summary>
    public RendererPreludeSignedRef Height { get; set; } = null!;
    /// <summary>별도 초기 화면 높이며 행 표 최초 구성에만 쓴다.</summary>
    public RendererPreludeSignedRef InitialHeight { get; set; } = null!;
    /// <summary>현재 카메라 세로 픽셀이며 매 행 조회 직전에 읽는다.</summary>
    public RendererPreludeSignedRef CameraY { get; set; } = null!;
    /// <summary>현재 지도 왼쪽이다.</summary>
    public RendererPreludeSignedRef Left { get; set; } = null!;
    /// <summary>현재 지도 위쪽이다.</summary>
    public RendererPreludeSignedRef Top { get; set; } = null!;
    /// <summary>현재 지도 오른쪽이다.</summary>
    public RendererPreludeSignedRef Right { get; set; } = null!;
    /// <summary>현재 지도 아래쪽이다.</summary>
    public RendererPreludeSignedRef Bottom { get; set; } = null!;
}

/// <summary>실제 구름/dirty/override 장치는 선택된 위치에서만 요구하므로 생략 경로에 불필요한 의존을 만들지 않는다.</summary>
public sealed class RendererPreludeHooks
{
    /// <summary>같은 실제 all/partial DWORD 쓰기다.</summary>
    public Action? MarkCloudDirty { get; set; }
    /// <summary>별도 구름 dirty 4칸 저장이며 일반 화면 dirty를 추가하지 않는다.</summary>
    public Action<ScreenRect>? QueueCloudDirty { get; set; }
    /// <summary>실제 overrideHead 조회다. 일반 모달 개수와 구별한다.</summary>
    public Func<uint>? OverrideActive { get; set; }
    /// <summary>같은 난수/캐시/옵션 갱신이며 선택한 경우에만 실행한다.</summary>
    public Action? UpdateClouds { get; set; }
    /// <summary>현재 행/현재 cameraY 조회다.</summary>
    public Func<int, int, uint>? RowDirty { get; set; }
    /// <summary>clip된 행 또는 force 전체 dirty 추가다.</summary>
    public Action<ScreenRect, uint>? Invalidate { get; set; }
    /// <summary>별도 시차 배경 dirty 처리다.</summary>
    public Action? InvalidateBackground { get; set; }
    /// <summary>호스트 vector 표를 쓰는 경우 같은 dirtyCount 0 store 직후 실제 표를 비운다.</summary>
    public Action? ClearDirty { get; set; }
}

/// <summary>프레임 앞단의 게임 계산과 실제 화면/구름 장치를 분리한다.</summary>
/// <remarks>cpppj client/RawRendererPrelude.cpp 대응이다. 원문 Renderer 앞단은 OS/월드 수집이나 게임 실행에 의존하지 않는다. 2026-11-26 1.067에서 R5-2d2용으로 추가했다.</remarks>
public static class RawRendererPrelude
{
    /// <summary>index와 16행 뒤의 인접 DWORD 위치다.</summary>
    private const int Initialized = 17;
    /// <summary>index와 16행 뒤의 인접 DWORD 위치다.</summary>
    private const int PreviousMode = 18;

    /// <summary>원문 DWORD 비트를 signed 비교로만 해석한다.</summary>
    /// <param name="value">DWORD 비트다.</param>
    /// <returns>signed 값이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>ADD/LEA의 하위 32비트 결과다.</summary>
    /// <param name="value">첫 피연산자다.</param>
    /// <param name="delta">더할 값이다.</param>
    /// <returns>감긴 합이다.</returns>
    private static int Add(int value, int delta) => Signed(unchecked((uint)value + unchecked((uint)delta)));

    /// <summary>실제로 도달한 하위만 요구한다.</summary>
    /// <typeparam name="T">연결된 장치의 형태다.</typeparam>
    /// <param name="callback">요구할 실제 장치다.</param>
    /// <returns>같은 장치다.</returns>
    private static T Need<T>(T? callback) where T : Delegate =>
        callback ?? throw new InvalidOperationException("앞단의 선택된 현재 장치 미연결");

    /// <summary>호출 직전 현재 화면 크기다.</summary>
    /// <param name="globals">현재 전역이다.</param>
    /// <returns>전체 사각형이다.</returns>
    private static ScreenRect Full(RendererPreludeGlobals globals) => new(0, 0, globals.Width.Read(), globals.Height.Read());

    /// <summary>행 조회 뒤 바뀐 현재 지도 clip도 반영한다.</summary>
    /// <param name="globals">현재 전역이다.</param>
    /// <returns>지도 사각형이다.</returns>
    private static ScreenRect View(RendererPreludeGlobals globals) => new(globals.Left.Read(), globals.Top.Read(), globals.Right.Read(), globals.Bottom.Read());

    /// <summary>원문은 0번 행을 쓰지 않는다. 기존값과 실제 store 순서를 유지하며 최초 높이만 한 번 참조한다.</summary>
    /// <param name="state">현재 상태다.</param>
    /// <param name="height">최초 높이만 한 번 참조한다.</param>
    private static void InitializeRows(RendererPreludeState state, uint height)
    {
        state.Scan[Initialized] |= 1U;
        state.Scan[2] = height - 30U; state.Scan[4] = height - 150U; state.Scan[6] = height - 60U; state.Scan[8] = height - 180U;
        state.Scan[10] = height - 90U; state.Scan[12] = height - 210U;
        state.Scan[3] = 120; state.Scan[5] = 30; state.Scan[7] = 150; state.Scan[9] = 60; state.Scan[11] = 180; state.Scan[13] = 90; state.Scan[15] = 210;
        state.Scan[14] = height - 120U; state.Scan[16] = height - 240U;
    }

    /// <summary>index-1/16/17의 실제 인접 alias는 허용하며 임의로 16칸에 정규화하지 않는다.</summary>
    /// <param name="state">현재 상태다.</param>
    /// <param name="index">들을 행 표 위치다.</param>
    /// <returns>행 값이다.</returns>
    private static int Row(RendererPreludeState state, int index)
    {
        long physical = (long)index + 1;
        if (physical < 0 || physical >= state.Scan.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "앞단 행 표의 현재 인접 물리 범위");
        }
        return Signed(state.Scan[(int)physical]);
    }

    /// <summary>Draw의 수집/표시 전에 현재 화면 모드·세계 요청·override와 구름 행 스캔을 순서대로 처리한다.</summary>
    /// <param name="state">현재 상태다.</param>
    /// <param name="globals">현재 전역이다.</param>
    /// <param name="hooks">실제 장치다.</param>
    public static void PrepareRendererFrame(RendererPreludeState state, RendererPreludeGlobals globals, RendererPreludeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (globals.Mode != state.Scan[PreviousMode])
        {
            Need(hooks.MarkCloudDirty)();
            Need(hooks.QueueCloudDirty)(Full(globals));
            state.Scan[PreviousMode] = globals.Mode;
        }
        if (globals.Force != 0)
        {
            Need(hooks.MarkCloudDirty)();
            Need(hooks.QueueCloudDirty)(Full(globals));
        }
        if ((globals.Mode & 8U) != 0 && Need(hooks.OverrideActive)() == 0)
        {
            // override getter 효과 뒤 현재 하위 BYTE를 캡처한다
            uint flags = globals.Mode & 255U;
            if ((flags & 32U) != 0)
            {
                Need(hooks.UpdateClouds)();
                flags = globals.Mode & 255U;
                if ((flags & 16U) == 0)
                {
                    if ((state.Scan[Initialized] & 1U) == 0)
                    {
                        InitializeRows(state, unchecked((uint)globals.InitialHeight.Read()));
                    }
                    int index = Signed(state.Scan[0]);
                    int row = Row(state, index);
                    if (row < Add(row, 30))
                    {
                        // 원문은 현재 30행 구간의 첫 dirty에서 멈추고 매 callback 뒤 현재 index/상한을 재조회한다
                        for (; ; )
                        {
                            uint dirty = Need(hooks.RowDirty)(row, globals.CameraY.Read());
                            index = Signed(state.Scan[0]);
                            if (dirty != 0)
                            {
                                int start = Row(state, index);
                                var rect = new ScreenRect(0, start, globals.Width.Read(), Add(start, 30));
                                Need(hooks.Invalidate)(ScreenGeometry.ClipScreenRect(rect, View(globals)), 0);
                                break;
                            }
                            row = Add(row, 1);
                            if (row >= Add(Row(state, index), 30))
                            {
                                break;
                            }
                        }
                        flags = globals.Mode & 255U;
                    }
                    state.Scan[0] = unchecked((uint)(Add(index, 1) % 16));
                }
            }
            Need(hooks.MarkCloudDirty)();
            if ((flags & 16U) != 0 && (flags & 1U) != 0 && (flags & 2U) == 0)
            {
                Need(hooks.InvalidateBackground)();
                state.BackgroundRedraw = 1;
            }
        }
        state.DrawCounter = 0;
        if (globals.Force != 0)
        {
            state.DirtyCount = 0;
            hooks.ClearDirty?.Invoke();
            Need(hooks.Invalidate)(Full(globals), 0);
        }
    }
}
