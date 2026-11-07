// 편집기의 임시 청크 지도에서 영역 패턴을 놓을 수 있는지 원본 순서로 검사한다 (cpppj o/RawIslandPlacement.h/.cpp, 2026-11-07 F2c-12 이식).
using System;
using System.Collections.Generic;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 IslandBuilder의 공용 읽기 전용 지도 배치 가능 여부 검사(`0046de50` / CD `00490a50`)를 복원한 정적 클래스다 (cpppj <c>o/RawIslandPlacement.h</c>·<c>o/RawIslandPlacement.cpp</c>, 2026-11-07 F2c-12 이식).
/// </summary>
public static class RawIslandPlacement
{
    /// <summary>10.78 0046de50 / CD 00490a50: 현재 bounds·TYPE 프레임·청크를 읽고 쓰기 없이 배치 가능 여부를 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="edition">원본 판본.</param>
    /// <param name="target">검사할 청크 지도.</param>
    /// <param name="frames">판본별 타입 프레임 표.</param>
    /// <param name="puzzlePieceType">현재 puzzlePiece 타입 번호를 읽는 대리자.</param>
    /// <param name="state">빌더 경계·소유자 상태.</param>
    /// <param name="record">6바이트 Territory 레코드.</param>
    /// <param name="diagnostic">원본 진단 콜백.</param>
    /// <returns>놓을 수 있으면 true.</returns>
    public static bool CanPlaceIsland(
        OriginalEdition edition,
        ChunkMap target,
        IReadOnlyList<RiftTypeFrames> frames,
        Func<uint> puzzlePieceType,
        RawIslandBuilderState state,
        ReadOnlySpan<byte> record,
        Action<RawIslandBuilderAssertion>? diagnostic)
    {
        ArgumentNullException.ThrowIfNull(target);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(puzzlePieceType);
        ArgumentNullException.ThrowIfNull(state);
        if (record.Length < RawIslandBuilder.RawTerritoryRecord)
        {
            throw new ArgumentException("Territory 레코드는 6바이트 이상이어야 합니다.", nameof(record));
        }
        // 원문 stack의 값 전달이며 이후 진단이 외부 입력을 바꿔도 현재 인자는 유지한다
        Span<byte> value =
        [
            record[0],
            record[1],
            record[2],
            record[3],
            record[4],
            record[5],
        ];
        if (diagnostic is null)
        {
            throw new ArgumentException("섬 배치 검사의 원본 진단 장치 누락", nameof(diagnostic));
        }

        ChunkRect initialBounds = state.Bounds;
        if (initialBounds.Left == -1 || initialBounds.Top == -1 || initialBounds.Right == -1 || initialBounds.Bottom == -1)
        {
            diagnostic(RawIslandBuilderAssertion.InvalidBounds);
        }

        // 진단 복귀 뒤 현재 TYPE을 읽고 decoder 생성 중 같은 값을 유지한다
        uint type = puzzlePieceType();
        if (type >= (uint)frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(puzzlePieceType), "섬 배치 검사의 현재 puzzlePiece TYPE 범위");
        }

        int direction = value[0] >> 6;
        ChunkRect originBounds = state.Bounds;
        float x = PlacementOrigin(originBounds.Left, (uint)(value[2] & 15u));
        float y = PlacementOrigin(originBounds.Top, (uint)(value[2] >> 4));
        if (edition == OriginalEdition.Cd1072 && (direction & 1) != 0)
        {
            diagnostic(RawIslandBuilderAssertion.OddDirection);
        }

        var decoder = new CanonDecoder(
            frames[(int)type],
            CanonDecoder.TerritoryPatterns()[value[0] & 63],
            direction,
            x,
            y,
            () => diagnostic(RawIslandBuilderAssertion.MissingFrame),
            edition,
            state.DecoderDiagnostics);

        // 첫 유효 패턴 칸을 포함하여 현재 우측·하단과 실제 청크를 매번 다시 검사한다
        for (; decoder.Valid(); decoder.Advance())
        {
            int cx = PlacementCoordinate(decoder.X());
            int cy = PlacementCoordinate(decoder.Y());
            ChunkRect currentBounds = state.Bounds;
            if (currentBounds.Right <= cx || currentBounds.Bottom <= cy)
            {
                return false;
            }
            if (edition == OriginalEdition.Patch1078 && (cx < 0 || cy < 0 || cx >= target.XLength() || cy >= target.YLength()))
            {
                diagnostic(RawIslandBuilderAssertion.ChunkRange);
            }
            if (target.At(cx, cy).Island != ChunkMapShape.NoIsland)
            {
                return false;
            }
        }
        return true;
    }

    /// <summary>고정 puzzlePiece 타입 번호로 배치 가능 여부를 검사한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="edition">원본 판본.</param>
    /// <param name="target">검사할 청크 지도.</param>
    /// <param name="frames">판본별 타입 프레임 표.</param>
    /// <param name="puzzlePieceType">puzzlePiece 타입 번호.</param>
    /// <param name="state">빌더 경계·소유자 상태.</param>
    /// <param name="record">6바이트 Territory 레코드.</param>
    /// <param name="diagnostic">원본 진단 콜백.</param>
    /// <returns>놓을 수 있으면 true.</returns>
    public static bool CanPlaceIsland(
        OriginalEdition edition,
        ChunkMap target,
        IReadOnlyList<RiftTypeFrames> frames,
        uint puzzlePieceType,
        RawIslandBuilderState state,
        ReadOnlySpan<byte> record,
        Action<RawIslandBuilderAssertion>? diagnostic)
    {
        return CanPlaceIsland(edition, target, frames, () => puzzlePieceType, state, record, diagnostic);
    }

    /// <summary>signed 덧셈의 DWORD 감김 뒤 원본 float32 저장을 보존한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="origin">기준 원점 정수.</param>
    /// <param name="offset">4비트 오프셋.</param>
    /// <returns>원점 float 값.</returns>
    private static float PlacementOrigin(int origin, uint offset)
    {
        return (float)unchecked((int)(unchecked((uint)origin) + offset));
    }

    /// <summary>실제 decoder의 유한 청크 좌표를 원본 _ftol 하위 DWORD로 변환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="value">디코더 float 좌표.</param>
    /// <returns>청크 정수 좌표.</returns>
    private static int PlacementCoordinate(float value)
    {
        return unchecked((int)unchecked((uint)(long)value));
    }
}
