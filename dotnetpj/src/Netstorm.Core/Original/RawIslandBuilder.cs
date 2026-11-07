// 실제 영역 패턴을 현재 ChunkMap과 3204BYTE 섬 목록에 원본 순서로 놓는다 (cpppj o/RawIslandBuilder.h/.cpp, 2026-11-07 F2c-12 이식).
using System;
using System.Collections.Generic;

namespace Netstorm.Core.Original;

/// <summary>
/// 실제 builder의 현재 청크 bounds와 원래 배치 소유자 필드다(cpppj <c>o/RawIslandBuilder.h</c>의 <c>RawIslandBuilderState</c>, 2026-11-07 F2c-12 이식).
/// </summary>
public sealed class RawIslandBuilderState
{
    /// <summary>최초 SetMap의 (1,1)~(15,15), 실제 Place가 다시 읽는 청크 경계다 (2026-11-07 F2c-12 이식).</summary>
    public ChunkRect Bounds { get; set; } = ChunkMap.DefaultChunkRect();

    /// <summary>SetMap이 지정한 기본 owner이며 Runtime.Add에만 사용한다 (2026-11-07 F2c-12 이식).</summary>
    public uint Owner { get; set; } = 1u;

    /// <summary>현재 Canon 진단 owner를 같은 섬·지형·설치 수명 동안 빌린다 (2026-11-07 F2c-12 이식).</summary>
    public CanonDecoderDiagnostics? DecoderDiagnostics { get; set; }

    /// <summary>기본 청크 경계(1,1)~(15,15)와 기본 소유자(1)로 상태를 초기화한다 (2026-11-07 F2c-12 이식).</summary>
    public RawIslandBuilderState()
    {
    }

    /// <summary>지정한 청크 경계·소유자·진단 묶음으로 상태를 초기화한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="bounds">청크 경계.</param>
    /// <param name="owner">기본 소유자.</param>
    /// <param name="decoderDiagnostics">Canon 진단 묶음.</param>
    public RawIslandBuilderState(ChunkRect bounds, uint owner = 1u, CanonDecoderDiagnostics? decoderDiagnostics = null)
    {
        Bounds = bounds;
        Owner = owner;
        DecoderDiagnostics = decoderDiagnostics;
    }

    /// <summary>현재 상태의 얕은 사본을 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>복사된 상태 인스턴스.</returns>
    public RawIslandBuilderState Clone() => new(Bounds, Owner, DecoderDiagnostics);
}

/// <summary>
/// 원본 helper의 보고 후 계속 진행을 보존하는 진단 종류다(cpppj <c>o/RawIslandBuilder.h</c>의 <c>RawIslandBuilderAssertion</c>, 2026-11-07 F2c-12 이식).
/// </summary>
public enum RawIslandBuilderAssertion
{
    /// <summary>Place의 builder cRect 네 필드 중 하나가 -1이다 (2026-11-07 F2c-12 이식).</summary>
    InvalidBounds,

    /// <summary>PlaceAll의 start==end이며 실제 순회는 없을 수 있다 (2026-11-07 F2c-12 이식).</summary>
    EqualRange,

    /// <summary>CD CanonDecoder constructor의 원본 (rot&amp;1)==0 진단이다 (2026-11-07 F2c-12 이식).</summary>
    OddDirection,

    /// <summary>원본 assert FindNumber가 해당 방향·변형·번호를 찾지 못한 뒤 -1을 반환한다 (2026-11-07 F2c-12 이식).</summary>
    MissingFrame,

    /// <summary>patch getter가 XY 범위의 combined 진단을 보고한 뒤 clamp한다. CD는 무진단이다 (2026-11-07 F2c-12 이식).</summary>
    ChunkRange,

    /// <summary>world Bounds helper의 실제 Exists 진단이다 (2026-11-07 F2c-12 이식).</summary>
    BoundsMissingIsland,

    /// <summary>CD world Bounds가 일치 청크를 찾지 못한 found 진단이다 (2026-11-07 F2c-12 이식).</summary>
    BoundsNotFound,
}

/// <summary>
/// 같은 world <see cref="ChunkMap"/> 참조일 때만 실제 <see cref="RawIslandRuntime.Add"/>·<see cref="RawIslandRuntime.Bounds(int, FortRandomRectangle)"/>를 실행하고,
/// 임시 지도는 전달 지역을 유지하는 섬 배치 빌더다(cpppj <c>o/RawIslandBuilder</c>, 2026-11-07 F2c-12 이식).
/// </summary>
public sealed class RawIslandBuilder
{
    /// <summary>원본 Territory 레코드 1개의 바이트 수(6)다 (2026-11-07 F2c-12 이식).</summary>
    public const int RawTerritoryRecord = 6;

    /// <summary>실제 20개 Territory 레코드 저장소의 바이트 수(120)다 (2026-11-07 F2c-12 이식).</summary>
    public const int RawTerritoryBytes = 120;

    /// <summary>원본 this+0의 배치 지도다 (2026-11-07 F2c-12 이식).</summary>
    private readonly ChunkMap _target;

    /// <summary>실제 world 전역 지도다 (2026-11-07 F2c-12 이식).</summary>
    private readonly ChunkMap _world;

    /// <summary>count/Exists/current owner/원래 owner/Bounds를 실제 3204BYTE에 기록하는 섬 런타임이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly RawIslandRuntime _islands;

    /// <summary>실제 decoder가 현재 읽는 타입 프레임 표다 (2026-11-07 F2c-12 이식).</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>실제 decoder가 현재 읽는 puzzlePiece 타입 전역 읽기 대리자다 (2026-11-07 F2c-12 이식).</summary>
    private readonly Func<uint> _puzzlePieceType;

    /// <summary>현재 청크 범위·owner를 같은 builder 수명으로 보관하는 상태다 (2026-11-07 F2c-12 이식).</summary>
    private readonly RawIslandBuilderState _state;

    /// <summary>원본 진단을 요청하며 빈 효과로 성공을 대신하지 않는 콜백이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly Action<RawIslandBuilderAssertion> _diagnostic;

    /// <summary>원본 owner/bounds/프레임 전역 대리자와 target/world 지도를 연결해 빌더를 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="target">배치 대상 청크 지도.</param>
    /// <param name="world">실제 월드 전역 청크 지도.</param>
    /// <param name="islands">3204BYTE 섬 런타임.</param>
    /// <param name="frames">판본별 타입 프레임 표.</param>
    /// <param name="puzzlePieceType">현재 puzzlePiece 타입 번호를 읽는 대리자.</param>
    /// <param name="state">빌더 경계·소유자 상태.</param>
    /// <param name="diagnostic">원본 진단 콜백.</param>
    public RawIslandBuilder(
        ChunkMap target,
        ChunkMap world,
        RawIslandRuntime islands,
        IReadOnlyList<RiftTypeFrames> frames,
        Func<uint> puzzlePieceType,
        RawIslandBuilderState state,
        Action<RawIslandBuilderAssertion>? diagnostic)
    {
        ArgumentNullException.ThrowIfNull(target);
        ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(puzzlePieceType);
        ArgumentNullException.ThrowIfNull(state);
        _target = target;
        _world = world;
        _islands = islands;
        _frames = frames;
        _puzzlePieceType = puzzlePieceType;
        _state = state;
        int expectedFrames = Pool().Edition == OriginalEdition.Patch1078 ? 188 : 171;
        if (_frames.Count != expectedFrames || diagnostic is null)
        {
            throw new ArgumentException("실제 IslandBuilder 판본/진단 연결 오류");
        }
        _diagnostic = diagnostic;
    }

    /// <summary>고정 puzzlePiece 타입 번호 값으로 빌더를 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="target">배치 대상 청크 지도.</param>
    /// <param name="world">실제 월드 전역 청크 지도.</param>
    /// <param name="islands">3204BYTE 섬 런타임.</param>
    /// <param name="frames">판본별 타입 프레임 표.</param>
    /// <param name="puzzlePieceType">puzzlePiece 타입 번호.</param>
    /// <param name="state">빌더 경계·소유자 상태.</param>
    /// <param name="diagnostic">원본 진단 콜백.</param>
    public RawIslandBuilder(
        ChunkMap target,
        ChunkMap world,
        RawIslandRuntime islands,
        IReadOnlyList<RiftTypeFrames> frames,
        uint puzzlePieceType,
        RawIslandBuilderState state,
        Action<RawIslandBuilderAssertion>? diagnostic)
        : this(target, world, islands, frames, () => puzzlePieceType, state, diagnostic)
    {
    }

    /// <summary>원본 inWorld처럼 대상 지도 참조와 world 전역 지도 참조의 동일성만 판단한다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>같은 지도 인스턴스이면 true.</returns>
    public bool InWorld() => ReferenceEquals(_target, _world);

    /// <summary>같은 pool의 실제 Runtime 소비자임을 월드 바인딩에서 확인한다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>섬 런타임이 참조하는 <see cref="RawSidPool"/>.</returns>
    public RawSidPool Pool() => _islands.Pool();

    /// <summary>0046dba0 / CD 00490750: record 6BYTE·위치 nibble·현재 유효 패턴·ordinal·같은 32바이트 Bounds를 기록한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="record">6바이트 Territory 레코드.</param>
    /// <param name="territory">영역 번호.</param>
    /// <param name="ignorePosition">위치 오프셋 무시 여부.</param>
    /// <returns>배치된 섬 또는 지역 번호.</returns>
    public uint Place(ReadOnlySpan<byte> record, uint territory, bool ignorePosition = false)
    {
        if (record.Length < RawTerritoryRecord)
        {
            throw new ArgumentException("Territory 레코드는 6바이트 이상이어야 합니다.", nameof(record));
        }
        ChunkRect currentBounds = _state.Bounds;
        if (currentBounds.Left == -1 || currentBounds.Top == -1 || currentBounds.Right == -1 || currentBounds.Bottom == -1)
        {
            _diagnostic(RawIslandBuilderAssertion.InvalidBounds);
        }

        uint offsetX = ignorePosition ? 0u : (uint)(record[2] & 15u);
        uint offsetY = ignorePosition ? 0u : (uint)(record[2] >> 4);
        uint region = territory;
        if (InWorld())
        {
            region = _islands.Add(_state.Owner, unchecked((byte)territory));
        }

        currentBounds = _state.Bounds;
        float x = Origin(currentBounds.Left, offsetX);
        float y = Origin(currentBounds.Top, offsetY);
        uint type = _puzzlePieceType();
        uint pattern = (uint)(record[0] & 63u);
        int direction = record[0] >> 6;
        if (type >= (uint)_frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(record), "실제 IslandBuilder 현재 puzzlePiece 타입 범위 오류");
        }
        if (Pool().Edition == OriginalEdition.Cd1072 && (direction & 1) != 0)
        {
            _diagnostic(RawIslandBuilderAssertion.OddDirection);
        }

        ReadOnlySpan<CanonPattern> patterns = CanonDecoder.TerritoryPatterns();
        if (pattern >= (uint)patterns.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(record), "실제 IslandBuilder 영역 패턴 범위 오류");
        }

        var decoder = new CanonDecoder(
            _frames[(int)type],
            patterns[(int)pattern],
            direction,
            x,
            y,
            () => _diagnostic(RawIslandBuilderAssertion.MissingFrame),
            Pool().Edition,
            _state.DecoderDiagnostics);

        int ordinal = 0;
        // 실제 decoder의 최초 유효 칸을 포함하고 프레임이 없는 칸은 완료된 Advance가 건너뛴다
        for (; decoder.Valid(); decoder.Advance(), ++ordinal)
        {
            ref ChunkEntry entry = ref ChunkAt(ToChunkCoordinate(decoder.X()), ToChunkCoordinate(decoder.Y()));
            entry.Island = unchecked((byte)region);
            entry.Record ??= new byte[RawTerritoryRecord];
            // 원본 record 6BYTE만 복사하며 방향·ordinal은 별도 필드에 기록한다
            for (int byteIndex = 0; byteIndex < RawTerritoryRecord; ++byteIndex)
            {
                entry.Record[byteIndex] = record[byteIndex];
            }
            entry.Ordinal = ordinal;
            entry.Side = unchecked((sbyte)decoder.Side());
        }

        if (InWorld())
        {
            int number = unchecked((int)region);
            if (_islands.Exists(number) == 0u)
            {
                _diagnostic(RawIslandBuilderAssertion.BoundsMissingIsland);
            }
            ChunkRect bounds = _target.Bounds(number);
            if (Pool().Edition == OriginalEdition.Cd1072 && bounds.Left == 16 && bounds.Top == 16)
            {
                _diagnostic(RawIslandBuilderAssertion.BoundsNotFound);
            }
            _islands.Bounds(
                number,
                new FortRandomRectangle(
                    CellBoundary(bounds.Left),
                    CellBoundary(bounds.Top),
                    CellBoundary(bounds.Right),
                    CellBoundary(bounds.Bottom)));
        }

        return region;
    }

    /// <summary>0046dd70 / CD 00490980: flag bit0/bit1을 현재 읽으며 [start, end) 순서로 Place를 호출한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="records">20개 Territory(120바이트) 스팬.</param>
    /// <param name="start">시작 영역 인덱스.</param>
    /// <param name="end">끝 영역 인덱스(배타).</param>
    public void PlaceAll(ReadOnlySpan<byte> records, int start = 0, int end = 20)
    {
        if (records.Length < RawTerritoryBytes || start < 0 || end < 0 || start > 20 || end > 20)
        {
            throw new ArgumentOutOfRangeException(nameof(records), "실제 IslandBuilder Territory 물리 20레코드 범위 오류");
        }
        if (start == end)
        {
            _diagnostic(RawIslandBuilderAssertion.EqualRange);
        }
        // 원본은 각 레코드의 현재 flag를 다시 읽고 이름·지역 순서로 정렬하지 않는다
        for (int territory = start; territory < end; ++territory)
        {
            ReadOnlySpan<byte> record = records.Slice(territory * RawTerritoryRecord, RawTerritoryRecord);
            if ((record[1] & 1) != 0 && (record[1] & 2) == 0)
            {
                Place(record, (uint)territory, false);
            }
        }
    }

    /// <summary>실제 ChunkMap getter의 범위 진단을 보고한 뒤 기존 clamp 몸체로 같은 저장소를 얻는다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="x">청크 X 좌표.</param>
    /// <param name="y">청크 Y 좌표.</param>
    /// <returns>대상 청크 참조.</returns>
    private ref ChunkEntry ChunkAt(int x, int y)
    {
        if (Pool().Edition == OriginalEdition.Patch1078 && (x < 0 || y < 0 || x >= _target.XLength() || y >= _target.YLength()))
        {
            _diagnostic(RawIslandBuilderAssertion.ChunkRange);
        }
        return ref _target.At(x, y);
    }

    /// <summary>원본 DWORD 감김 16배를 float로 저장하며 signed shift의 미정의 동작을 피한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="value">청크 경계 정수.</param>
    /// <returns>셀 경계 float 값.</returns>
    private static float CellBoundary(int value)
    {
        return (float)unchecked((int)(unchecked((uint)value) << 4));
    }

    /// <summary>원본 시작 좌표의 DWORD 감김 덧셈과 float32 저장이다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="value">기준 원점 정수.</param>
    /// <param name="offset">4비트 오프셋.</param>
    /// <returns>원점 float 값.</returns>
    private static float Origin(int value, uint offset)
    {
        return (float)unchecked((int)(unchecked((uint)value) + offset));
    }

    /// <summary>decoder float의 실제 _ftol 하위 DWORD를 사용하여 INT_MAX 부근의 float 반올림도 보존한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="value">디코더 float 좌표.</param>
    /// <returns>청크 정수 좌표.</returns>
    private static int ToChunkCoordinate(float value)
    {
        return unchecked((int)unchecked((uint)(long)value));
    }
}
