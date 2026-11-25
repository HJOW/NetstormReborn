// 원본 타일 행별 표시 묶음의 실제 SID 풀과 0단계 공간 해시 수집을 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

/// <summary>원본 포인터의 호스트 독립 토큰 기준이다.</summary>
/// <param name="ScreenHeight">게임 화면의 픽셀 높이.</param>
/// <param name="ObjectTokenBase">객체 토큰 기본.</param>
/// <param name="GroupTokenBase">묶음 토큰 기본.</param>
public readonly record struct RawTileGroupState(int ScreenHeight = 768, uint ObjectTokenBase = 0, uint GroupTokenBase = 0x80000000U);

/// <summary>수집 순서의 연속 멤버 배열이다. 각 행 내부는 공통 정렬을 적용하지 않는다.</summary>
/// <param name="Token">저장소나 분배기에서 대상을 구별하는 토큰.</param>
/// <param name="Entries">현재 수집의 묶음 멤버다.</param>
/// <param name="Count">현재 항목 수다. 그리기 중 바뀔 수 있어 매 차례 읽는다.</param>
public sealed class RawTileGroupView
{
    /// <summary>저장소나 분배기에서 대상을 구별하는 토큰이다.</summary>
    public uint Token { get; }
    /// <summary>현재 수집의 묶음 멤버다. 다음 수집 때까지만 유효하다.</summary>
    public IReadOnlyList<RenderCollectionEntry> Entries { get; }
    /// <summary>현재 항목 수다. 그리기 중 바뀔 수 있어 매 차례 읽는다.</summary>
    public int Count => Entries.Count;
    /// <summary>토큰과 멤버로 묶음 보기를 만든다.</summary>
    /// <param name="token">묶음 토큰이다.</param>
    /// <param name="entries">묶음 멤버다.</param>
    public RawTileGroupView(uint token, IReadOnlyList<RenderCollectionEntry> entries)
    {
        Token = token;
        Entries = entries;
    }
}

/// <summary>ClientBoard의 지면/다리 행별 표시 묶음을 실제 SID 풀과 0단계 공간 해시에서 복원한다.</summary>
/// <remarks>cpppj client/TileGroupCollection.cpp의 00433410 ↔ CD 004884a0 대응이다. 화면 투영과 실제 SHP 그리기는 후속이며 이 계층은 raw 월드 키와 SID 토큰만 제출한다. 2026-11-26 1.064에서 R5-2b용으로 추가했다.</remarks>
public sealed class RawTileGroupCollection
{
    /// <summary>좌표별 객체 후보를 찾는 공간 해시다. 풀보다 오래 살아야 한다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>객체 슬롯의 생성·조회·반납에 쓰는 SID 풀이다. 해시보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>화면 높이와 토큰 전체 범위를 검증한 상태다.</summary>
    private readonly RawTileGroupState _state;
    /// <summary>현재 프레임의 연속 멤버 배열이다. 다음 수집 때는 이전 내용을 사용할 수 없다.</summary>
    private readonly List<RenderCollectionEntry> _entries = [];
    /// <summary>현재 수집의 묶음 보기다. 다음 수집 때는 이전 내용을 사용할 수 없다.</summary>
    private readonly List<RawTileGroupView> _groups = [];
    /// <summary>각 묶음의 행별 개수다. 인덱스는 묶음 순서다.</summary>
    private readonly int[] _counts = new int[RawSquidHash.WorldCells];

    /// <summary>풀/해시는 객체보다 오래 살아야 한다. 화면 높이와 토큰 범위를 먼저 검사한다.</summary>
    /// <param name="hash">공간 해시다.</param>
    /// <param name="pool">SID 풀이다.</param>
    /// <param name="state">화면 높이와 토큰 기준이다.</param>
    public RawTileGroupCollection(RawSquidHash hash, RawSidPool pool, RawTileGroupState state = default)
    {
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(pool);
        // 화면 높이는 원문 행 개수 assert 상한 계산에 쓰인다
        if (state.ScreenHeight < 0 || state.ScreenHeight > int.MaxValue - 66)
        {
            throw new ArgumentOutOfRangeException(nameof(state), "타일 묶음 화면 높이가 범위를 벗어났습니다");
        }
        // 두 영역이 겹치면 토큰으로 객체와 묶음을 구별할 수 없다
        ulong end = (ulong)state.ObjectTokenBase + (ulong)(uint)pool.Capacity * (ulong)(uint)pool.SlotStride;
        ulong groupEnd = (ulong)state.GroupTokenBase + (ulong)RawSquidHash.WorldCells * 8;
        if (end > 0x100000000UL || groupEnd > 0x100000000UL ||
            (state.ObjectTokenBase < groupEnd && state.GroupTokenBase < end))
        {
            throw new ArgumentException("타일 묶음 토큰 영역이 겹칩니다", nameof(state));
        }
        _hash = hash;
        _pool = pool;
        _state = state;
    }

    /// <summary>좌표 보조의 +0.5와 CRT 64비트 절삭을 거쳐 확장 전 signed 셀 키를 반환한다.</summary>
    /// <param name="coordinate">월드 좌표다.</param>
    /// <returns>확장 전 셀 키다.</returns>
    public static int Cell(float coordinate)
    {
        // NaN/범위 밖 FISTP의 정수 indefinite는 하위 DWORD 0이다. 큰 유한 수도 하위 DWORD를 보존한다.
        double value = (double)coordinate + 0.5;
        long integer = 0;
        if (double.IsFinite(value) && value >= -9.223372036854776E18 && value < 9.223372036854776E18)
        {
            integer = (long)value;
        }
        int cell = unchecked((int)(uint)integer);
        // 원문은 상한만 자르고 하한은 확장 단계에서 자른다
        return Math.Min(cell, RawSquidHash.WorldCells - 1);
    }

    /// <summary>월드 경계를 한 칸 확장·클램프한 뒤 y/x 순서로 첫 SID만 수집한다.</summary>
    /// <param name="view">수집에 쓰는 월드 사각형이다.</param>
    /// <param name="rowLimit">각 행의 엄격한 count&lt;limit assert다. 누적 표시 용량이 아니다.</param>
    /// <param name="backgroundCovered">수집한 항목이 배경 영역을 덮는지. false면 원본은 월드 배경을 다시 그린다.</param>
    /// <returns>현재 수집의 묶음 보기다. 다음 수집 때까지만 유효하다.</returns>
    public IReadOnlyList<RawTileGroupView> Collect(RenderWorldRect view, int rowLimit, out bool backgroundCovered)
    {
        _entries.Clear();
        _groups.Clear();
        _hash.Select(0);
        bool covered = true;
        // 원본 DWORD 확장의 넘침을 C# signed overflow 없이 재현하고 각 끝을 독립적으로 클램프한다.
        static int Expand(int cell, uint step) => Math.Clamp(unchecked((int)((uint)cell + step)), 0, RawSquidHash.WorldCells - 1);
        int left = Expand(Cell(view.Left), 0xffffffffU);
        int top = Expand(Cell(view.Top), 0xffffffffU);
        int right = Expand(Cell(view.Right), 1);
        int bottom = Expand(Cell(view.Bottom), 1);
        int maxRows = _pool.Edition == OriginalEdition.Patch1078 ? _state.ScreenHeight / 11 + 6 : 49;
        // 빈 행은 헤더를 만들지 않으며 뒤 행은 같은 연속 멤버 배열의 다음 위치에 기록한다.
        for (int y = top; y <= bottom; y++)
        {
            int start = _entries.Count;
            int count = 0;
            // 원본은 한 칸의 머리만 읽는다. next/type/state/실제 프레임 크기는 이 수집기의 필터가 아니다.
            for (int x = left; x <= right; x++)
            {
                int sid = _hash.At(x, y);
                if (sid == 0)
                {
                    covered = false;
                    continue;
                }
                ReadOnlySpan<byte> raw = _pool.Slot(sid);
                var entry = new RenderCollectionEntry(
                    BinaryPrimitives.ReadSingleLittleEndian(raw[14..]),
                    BinaryPrimitives.ReadSingleLittleEndian(raw[18..]),
                    unchecked(_state.ObjectTokenBase + (uint)sid * (uint)_pool.SlotStride),
                    unchecked((short)(sbyte)raw[_pool.Edition == OriginalEdition.Patch1078 ? 35 : 33]),
                    1,
                    0);
                _entries.Add(entry);
                count++;
                if (count >= rowLimit)
                {
                    throw new InvalidOperationException("타일 묶음 행 개수가 rowLimit에 닿았습니다");
                }
            }
            if (count > 0)
            {
                _counts[_groups.Count] = count;
                _groups.Add(new RawTileGroupView(
                    unchecked(_state.GroupTokenBase + (uint)_groups.Count * 8),
                    _entries.GetRange(start, count)));
                if (_groups.Count >= maxRows)
                {
                    throw new InvalidOperationException("타일 묶음 개수가 MAX_TILE_ROWS에 닿았습니다");
                }
            }
        }
        _hash.Select(2);
        backgroundCovered = covered;
        return _groups;
    }

    /// <summary>kind 1이 받은 헤더 토큰을 현재 프레임의 멤버 span과 signed 개수로 해석한다.</summary>
    /// <param name="token">묶음 토큰이다.</param>
    /// <returns>현재 수집의 묶음 보기다.</returns>
    public RawTileGroupView Group(uint token)
    {
        if (token < _state.GroupTokenBase || (token - _state.GroupTokenBase) % 8 != 0)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "타일 묶음 토큰이 범위를 벗어났습니다");
        }
        return _groups[(int)((token - _state.GroupTokenBase) / 8)];
    }

    /// <summary>raw 객체 토큰의 기준/stride/범위를 검사한 뒤 sprite 경계가 사용할 실제 SID를 반환한다.</summary>
    /// <param name="token">객체 토큰이다.</param>
    /// <returns>실제 SID 번호다.</returns>
    public int Object(uint token)
    {
        if (token < _state.ObjectTokenBase)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "타일 객체 토큰이 범위를 벗어났습니다");
        }
        uint offset = token - _state.ObjectTokenBase;
        uint stride = (uint)_pool.SlotStride;
        if (offset % stride != 0 || offset / stride == 0 || offset / stride >= (uint)_pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "타일 객체 SID가 범위를 벗어났습니다");
        }
        return (int)(offset / stride);
    }

    /// <summary>현재 프레임의 연속 멤버 배열을 복사 없이 읽는다. 다음 수집 때는 이전 내용을 사용할 수 없다.</summary>
    /// <returns>연속 멤버 배열이다.</returns>
    public IReadOnlyList<RenderCollectionEntry> Entries() => _entries;
}
