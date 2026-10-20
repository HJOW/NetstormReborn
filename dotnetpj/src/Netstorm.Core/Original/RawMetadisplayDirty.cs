namespace Netstorm.Core.Original;

/// <summary>
/// 미니맵 갱신 큐의 한 항목. 월드 좌표를 4로 나눈 4×4 칸 묶음 번호와 원본이 지우지 않는 세 번째 DWORD 를 담는다.
/// 세 번째 값은 큐에 넣을 때 0 으로 쓰지만, 큐 밖의 옛 항목은 원본처럼 초기화하지 않는다.
/// </summary>
public struct MetadisplayDirtyCell
{
    /// <summary>월드 X 좌표를 절삭한 뒤 4로 나눈 값(음수는 0 쪽 절삭).</summary>
    public int X;

    /// <summary>월드 Y 좌표를 절삭한 뒤 4로 나눈 값(음수는 0 쪽 절삭).</summary>
    public int Y;

    /// <summary>원본 큐가 쓰지 않는 세 번째 DWORD. 새 항목은 0 이고, 큐 밖 옛 항목은 보존된다.</summary>
    public uint Unused;

    /// <summary>세 값을 지정해 항목을 만든다. 주로 검사에서 원본 큐의 옛 값을 그대로 채울 때 쓴다.</summary>
    /// <param name="x">4×4 칸 묶음 X 번호.</param>
    /// <param name="y">4×4 칸 묶음 Y 번호.</param>
    /// <param name="unused">세 번째 DWORD(기본 0).</param>
    public MetadisplayDirtyCell(int x, int y, uint unused = 0)
    {
        X = x;
        Y = y;
        Unused = unused;
    }
}

/// <summary>
/// 한 번의 Refresh 가 보는 현재 카메라 좌표(float 비트)와 화면 사각형이다.
/// 카메라는 비트 그대로 받아 -0·NaN 비교 차이를 보존하고, 사각형은 네 정수다.
/// </summary>
public struct MetadisplayDirtyView
{
    /// <summary>현재 카메라 X 좌표의 float 비트(원본 0059a900).</summary>
    public uint CameraX;

    /// <summary>현재 카메라 Y 좌표의 float 비트.</summary>
    public uint CameraY;

    /// <summary>현재 화면 사각형 왼쪽(원본 005ca9cc).</summary>
    public int Left;

    /// <summary>현재 화면 사각형 위쪽.</summary>
    public int Top;

    /// <summary>현재 화면 사각형 오른쪽.</summary>
    public int Right;

    /// <summary>현재 화면 사각형 아래쪽.</summary>
    public int Bottom;
}

/// <summary>
/// 원본 BSS 의 미니맵 큐·포화 표시·캐시·bitmap 을 한데 모은 상태. 호출자가 소유하며 이 클래스가 그대로 고친다.
/// 큐 배열은 원본처럼 100칸 고정이고, count 밖의 옛 항목도 검사에서 비교할 수 있도록 보존한다.
/// </summary>
public sealed class MetadisplayDirtyState
{
    /// <summary>큐 칸 수. 원본 배열 크기이며 count 는 0~100 이다.</summary>
    public const int CellCapacity = 100;

    /// <summary>bitmap 한 변(원본 128). 월드 256칸을 2칸씩 묶은 해상도다.</summary>
    public const int BitmapSide = 128;

    /// <summary>전체 다시 그리기 한 변(원본 64). bitmap 을 2×2 묶음으로 본다.</summary>
    public const int RedrawSide = 64;

    /// <summary>표시 객체가 존재하는지(원본 005949d0). 꺼져 있으면 등록·그리기를 하지 않는다.</summary>
    public bool DisplayPresent;

    /// <summary>표시 객체 +4 의 활성 비트(bit 1). 꺼져 있으면 등록·그리기를 하지 않는다.</summary>
    public byte DisplayFlags;

    /// <summary>최초 Refresh 가 확보하는 128×128 BYTE bitmap. 비어 있으면 아직 확보 전이다.</summary>
    public byte[] Bitmap = [];

    /// <summary>bitmap 한 변. bitmap 이 있으면 양수이고 크기의 제곱근과 같아야 한다.</summary>
    public int BitmapWidth;

    /// <summary>갱신할 셀 큐 100칸(앞 count 칸만 유효).</summary>
    public MetadisplayDirtyCell[] Cells = new MetadisplayDirtyCell[CellCapacity];

    /// <summary>큐의 유효 칸 수(0~100).</summary>
    public int Count;

    /// <summary>0 이 아니면 다음 Refresh 가 64×64 전체 셀을 다시 그린다(큐 포화 등).</summary>
    public uint FullRefresh;

    /// <summary>카메라·사각형 static 초기화 비트(1·2).</summary>
    public uint Initialized;

    /// <summary>마지막 rebuild 가 본 view. 카메라 비트와 사각형을 보관한다.</summary>
    public MetadisplayDirtyView Cached;

    /// <summary>활성 표시에서 Refresh 가 끝난 횟수(원본 DWORD 카운터).</summary>
    public uint Frames;
}

/// <summary>
/// Refresh 가 표시 객체를 다시 만들거나 셀을 그릴 때 부르는 효과. 필요 없는 분기에서는 null 이어도 된다.
/// rebuild 는 bitmap·카메라·사각형을 다시 만들고, redrawCell 은 2×2 미니맵 셀 하나를 그린다.
/// </summary>
public sealed class MetadisplayDirtyHooks
{
    /// <summary>표시·카메라 재생성 효과(원본 00480270). 큐·view·초기화 조건이 맞을 때만 요구된다.</summary>
    public Action? Rebuild;

    /// <summary>미니맵 셀 하나를 그리는 효과(원본 004808d0). 전체/부분 갱신에서 요구된다.</summary>
    public Action<int, int>? RedrawCell;
}

/// <summary>
/// Pop·Unpop·Frame·Graph 가 알리는 미니맵 갱신 큐(원본 RawMetadisplayDirty, 10.78).
/// 등록은 "표시 활성 검사 → 상태 검사 → 4×4 칸 번호 변환 → 같은 칸 중복 검사 → 꽉 차면 전체 갱신 요구, 아니면 추가"이고,
/// Refresh 는 "bitmap 최초 확보(128×128, 전체 갱신 요구) → 비활성 반환 → 초기화 비트 → rebuild 조건 → 프레임 증가 →
/// 전체 64×64 또는 큐의 부분 셀 그리기 → count·full 지우기" 순서다.
/// 셀 번호는 float 를 0 쪽으로 절삭한 뒤 4로 나눈 signed 정수라 -3.9 는 0, -4.0 은 -1 이다(floor 아님).
/// bitmap 이 없으면 비활성 표시에서도 확보하고 전체 갱신을 요구하지만, 그 뒤 비활성 분기에서는 프레임·count 를 바꾸지 않는다.
/// 기준 판본은 10.78 이다. CD 10.72 의 NaN 카메라 동등 처리는 옮기지 않았다(이식 규칙 3번).
/// 이 클래스는 Pop 의 <see cref="IRawPopMetadisplay"/> 와 Unpop 의 <see cref="IRawMinimapQueue"/> 를 함께 구현한다.
/// 근거: docs/exe/cpp-metadisplay-*.md, cpppj/src/o/RawMetadisplayDirty.cpp (W02 열두째 조각, 2026-10-20).
/// </summary>
public sealed class RawMetadisplayDirty : IRawPopMetadisplay, IRawMinimapQueue
{
    /// <summary>슬롯 안 x 좌표(float, 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float, 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>호출 SID 의 현재 좌표를 읽을 슬롯 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>호출자가 소유한 큐·표시 상태. 이 클래스가 직접 고친다.</summary>
    private readonly MetadisplayDirtyState _state;

    /// <summary>rebuild·셀 그리기 효과. 필요 없는 분기에서는 null 일 수 있다.</summary>
    private readonly MetadisplayDirtyHooks _hooks;

    /// <summary>
    /// 처리기를 만든다. 상태의 count·bitmap 모양이 원본 범위를 벗어나면 만들기 전에 거부한다.
    /// </summary>
    /// <param name="pool">Pop·Unpop 과 같은 슬롯 풀.</param>
    /// <param name="state">큐·표시 상태(호출자 소유).</param>
    /// <param name="hooks">rebuild·셀 그리기 효과. 없으면 null.</param>
    public RawMetadisplayDirty(RawSidPool pool, MetadisplayDirtyState state, MetadisplayDirtyHooks? hooks = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        _pool = pool;
        _state = state;
        _hooks = hooks ?? new MetadisplayDirtyHooks();
        ValidateState();
    }

    /// <summary>이 큐가 쓰는 슬롯 풀. Pop·Unpop 이 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 월드 좌표를 4×4 칸 번호로 바꾼다. float 를 double 로 넓혀 0 쪽으로 절삭하고 signed 32비트 범위를 검사한 뒤 4로 나눈다.
    /// </summary>
    /// <param name="coordinate">월드 좌표(음수·NaN·무한대 포함).</param>
    private static int Cell(float coordinate)
    {
        double value = Math.Truncate((double)coordinate);
        if (!double.IsFinite(value) || value < int.MinValue || value > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(coordinate), "미니맵 dirty 유한 signed32 좌표가 필요합니다");
        }
        // C# 정수 나눗셈도 0 쪽 절삭이므로 원본 __ftol/4 와 같다
        return (int)value / 4;
    }

    /// <summary>큐 count 와 bitmap 폭·크기가 원본 범위인지 확인한다. 어긋나면 아무것도 바꾸지 않고 예외다.</summary>
    private void ValidateState()
    {
        if (_state.Cells.Length != MetadisplayDirtyState.CellCapacity)
        {
            throw new ArgumentException("미니맵 dirty 큐 칸 수 오류", nameof(_state));
        }
        if (_state.Count < 0 || _state.Count > MetadisplayDirtyState.CellCapacity)
        {
            throw new ArgumentOutOfRangeException(nameof(_state), "미니맵 dirty count 범위 오류");
        }
        if (_state.Bitmap.Length != 0
            && (_state.BitmapWidth <= 0 || (long)_state.BitmapWidth * _state.BitmapWidth != _state.Bitmap.Length))
        {
            throw new ArgumentException("미니맵 bitmap 폭/크기 불일치", nameof(_state));
        }
    }

    /// <summary>두 view 의 카메라 float 비트가 같은지 본다. 10.78 은 float 동등 비교이며 NaN 은 서로 다르다.</summary>
    /// <param name="current">이번 Refresh 의 view.</param>
    /// <param name="cached">마지막 rebuild 가 본 view.</param>
    private static bool SameCamera(in MetadisplayDirtyView current, in MetadisplayDirtyView cached) =>
        BitConverter.UInt32BitsToSingle(current.CameraX) == BitConverter.UInt32BitsToSingle(cached.CameraX)
        && BitConverter.UInt32BitsToSingle(current.CameraY) == BitConverter.UInt32BitsToSingle(cached.CameraY);

    /// <summary>두 view 의 화면 사각형 네 값이 같은지 본다.</summary>
    /// <param name="current">이번 Refresh 의 view.</param>
    /// <param name="cached">마지막 rebuild 가 본 view.</param>
    private static bool SameRectangle(in MetadisplayDirtyView current, in MetadisplayDirtyView cached) =>
        current.Left == cached.Left && current.Top == cached.Top && current.Right == cached.Right && current.Bottom == cached.Bottom;

    /// <summary>
    /// 월드 좌표를 큐에 넣는다. 표시가 꺼져 있으면 좌표 변환 없이 돌아가고, 같은 칸이 이미 있으면 아무것도 하지 않는다.
    /// 큐가 꽉 차 있으면 항목을 추가하지 않고 전체 갱신을 요구한다(원본은 중복 검사를 먼저 한다).
    /// </summary>
    /// <param name="x">월드 X 좌표.</param>
    /// <param name="y">월드 Y 좌표.</param>
    public void Enqueue(float x, float y)
    {
        if (!_state.DisplayPresent || (_state.DisplayFlags & 1) == 0)
        {
            return;
        }
        ValidateState();
        int cellX = Cell(x);
        int cellY = Cell(y);
        // 앞 count 칸에서 같은 4×4 칸을 찾으면 등록하지 않는다
        for (int i = 0; i < _state.Count; i++)
        {
            if (_state.Cells[i].X == cellX && _state.Cells[i].Y == cellY)
            {
                return;
            }
        }
        if (_state.Count >= MetadisplayDirtyState.CellCapacity)
        {
            _state.FullRefresh = 1;
            return;
        }
        _state.Cells[_state.Count] = new MetadisplayDirtyCell(cellX, cellY);
        _state.Count++;
    }

    /// <summary>슬롯의 현재 +14/+18 좌표를 큐에 넣는다. 저장한 사본이 아니라 그 시점 바이트를 읽는다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public void Enqueue(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        Enqueue(BitConverter.UInt32BitsToSingle(Read(raw, XOffset)),
            BitConverter.UInt32BitsToSingle(Read(raw, YOffset)));
    }

    /// <summary>
    /// 한 프레임의 미니맵 갱신을 처리한다. bitmap 이 없으면 먼저 128×128 을 확보하고 전체 갱신을 요구한다.
    /// 표시가 꺼져 있으면 확보 뒤 돌아가며 프레임·count 를 바꾸지 않는다.
    /// 활성 표시이면 초기화 비트를 세우고, 큐·포화·카메라·사각형 가운데 하나라도 바뀌었을 때 rebuild 효과를 요구해 부른다.
    /// 그다음 프레임을 세고 전체 64×64 또는 큐의 부분 셀을 그린 뒤 count 와 포화 표시를 지운다.
    /// 셀 그리기 효과 안에서 새 통지가 들어오면 현재 count 를 다시 읽어 같은 프레임에 처리한다.
    /// </summary>
    /// <param name="view">이번 프레임의 카메라 좌표 float 비트와 화면 사각형.</param>
    public void Refresh(MetadisplayDirtyView view)
    {
        ValidateState();
        if (_state.Bitmap.Length == 0)
        {
            _state.BitmapWidth = MetadisplayDirtyState.BitmapSide;
            _state.Bitmap = new byte[MetadisplayDirtyState.BitmapSide * MetadisplayDirtyState.BitmapSide];
            _state.FullRefresh = 1;
        }
        if (!_state.DisplayPresent || (_state.DisplayFlags & 1) == 0)
        {
            return;
        }
        _state.Initialized |= 1;
        if ((_state.Initialized & 2) == 0)
        {
            _state.Initialized |= 2;
            _state.Cached.Left = 0;
            _state.Cached.Top = 0;
            _state.Cached.Right = 0;
            _state.Cached.Bottom = 0;
        }
        if (_state.Count != 0 || _state.FullRefresh != 0 || !SameCamera(view, _state.Cached) || !SameRectangle(view, _state.Cached))
        {
            if (_hooks.Rebuild is null)
            {
                throw new InvalidOperationException("미니맵 표시 재생성 몸체가 연결되지 않았습니다");
            }
            _state.Cached = view;
            _hooks.Rebuild();
        }
        _state.Frames++;
        if (_state.FullRefresh != 0)
        {
            if (_hooks.RedrawCell is null)
            {
                throw new InvalidOperationException("미니맵 전체 셀 그림 몸체가 연결되지 않았습니다");
            }
            // 전체 갱신은 y 를 바깥으로 두고 x 를 안쪽으로 64×64 셀을 모두 그린다
            for (int y = 0; y < MetadisplayDirtyState.RedrawSide; y++)
            {
                // 이 순서는 새 큐 등록과 무관하게 고정이며 count 는 마지막에 지운다
                for (int x = 0; x < MetadisplayDirtyState.RedrawSide; x++)
                {
                    _hooks.RedrawCell(x, y);
                }
            }
        }
        else
        {
            if (_state.Count != 0 && _hooks.RedrawCell is null)
            {
                throw new InvalidOperationException("미니맵 부분 셀 그림 몸체가 연결되지 않았습니다");
            }
            // 셀 그림 효과 뒤 count 를 다시 읽어 그 효과의 새 통지도 같은 프레임에 처리한다
            for (int i = 0; i < _state.Count; i++)
            {
                ValidateState();
                MetadisplayDirtyCell cell = _state.Cells[i];
                _hooks.RedrawCell!(cell.X, cell.Y);
            }
        }
        _state.Count = 0;
        _state.FullRefresh = 0;
    }

    /// <summary>
    /// 같은 풀의 좌표 통지 함수를 만든다. 다른 풀을 주면 만들지 않고 예외다(반환 함수는 큐보다 오래 살면 안 된다).
    /// </summary>
    /// <param name="pool">이 큐가 쓰는 풀과 같은 풀이어야 한다.</param>
    /// <param name="dirty">좌표를 받을 큐.</param>
    public static Action<float, float> MakeHook(RawSidPool pool, RawMetadisplayDirty dirty)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(dirty);
        if (!ReferenceEquals(pool, dirty.Pool))
        {
            throw new ArgumentException("미니맵 dirty SID 풀이 다릅니다", nameof(pool));
        }
        return dirty.Enqueue;
    }

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 32비트를 읽는다(다른 필드는 읽지 않는다).</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset)
    {
        uint value = 0;
        // +14/+18 의 네 BYTE 만 조합한다
        for (int i = 0; i < 4; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }
}
