// 일반 지형 소비자의 분기·범위·타입 연결 계약을 공유한다.
namespace Netstorm.Core.Original;

// 일반지형 바인딩 계약 모듈을 담는 네임스페이스다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.

// 군도와 미복원 부모를 정상 지형으로 대체하지 않는 검사다. 패턴 개수 잎까지 포함하며 생성·도장·군집 본체는 후속 단위에서 잇는다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐고 0.202에서 패턴 개수를 이었다.
public static class RawNormalTerrainChecks
{
    /// <summary>일반 분기가 아니면 거부한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="request">확인한 분기 요청이다.</param>
    public static void RequireBranch(NormalTerrainRequest request)
    {
        if (request.Branch != NormalTerrainBranch.Normal)
        {
            throw new InvalidOperationException("군도·전체 부모를 일반 지형으로 실행할 수 없습니다.");
        }
    }

    /// <summary>유효 내부 청크 범위를 요구한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="request">검사할 요청이다.</param>
    public static void RequireBounds(NormalTerrainRequest request)
    {
        ChunkRect bounds = request.Bounds;
        if (bounds.Left < 1 || bounds.Top < 1 || bounds.Right > 15 || bounds.Bottom > 15 ||
            bounds.Left >= bounds.Right || bounds.Top >= bounds.Bottom)
        {
            throw new ArgumentOutOfRangeException(nameof(request), "일반 지형 소비자의 유효 내부 청크 범위가 필요합니다.");
        }
    }

    /// <summary>군도와 서로 다른 원본 전역을 묵시적인 정상 지형·지역 기본값으로 바꾸지 않는다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="farm">군집 상태다.</param>
    /// <param name="globals">문자 mapping 전역이다.</param>
    /// <param name="placement">매 실제 배치가 읽는 타입 전역이다.</param>
    /// <param name="stamp">수명이 긴 초기 캐시 상태다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="pool">같은 실제 풀이다.</param>
    public static void RequireNormal(FarmClusterState farm, TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, IReadOnlyList<OriginalTypeRecord> types, RawSidPool pool)
    {
        ArgumentNullException.ThrowIfNull(farm);
        if (farm.CurrentArchipelago())
        {
            throw new InvalidOperationException("일반 전용 지형 요청에 군도 상태를 사용할 수 없습니다.");
        }

        RequireStorage(globals, placement, stamp, types, pool);
    }

    /// <summary>현재 원본 모드의 선택과 분리하여 같은 실제 TYPE·지역·생성자 연결을 확인한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="globals">문자 mapping 전역이다.</param>
    /// <param name="placement">매 실제 배치가 읽는 타입 전역이다.</param>
    /// <param name="stamp">수명이 긴 초기 캐시 상태다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="pool">같은 실제 풀이다.</param>
    public static void RequireStorage(TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, IReadOnlyList<OriginalTypeRecord> types, RawSidPool pool)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(placement);
        ArgumentNullException.ThrowIfNull(stamp);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(pool);
        if (globals.IslandType != placement.IslandType || globals.IslandType != stamp.IslandType ||
            globals.EdgeType != stamp.EdgeType || globals.NoRegion != stamp.NoRegion)
        {
            throw new ArgumentException("일반 지형 같은 타입·지역 전역 연결 오류");
        }

        // 지면 97의 실제 0 슬롯·base 계약은 보존하며 121의 실제 생성자를 임의 0 슬롯으로 대신하지 않는다
        foreach (uint type in new[] { globals.IslandType, placement.EdgeFarmType })
        {
            if (type >= (uint)types.Count ||
                types[(int)type].ConstructorAddress != OriginalConstructors.Address(pool.Edition, (int)type))
            {
                throw new ArgumentException("일반 지형 실제 타입 생성자 연결 누락");
            }
        }
    }

    /// <summary>6바이트 청크 기록의 패턴 번호·회전으로 영역 디코더를 돌려 유효 칸 수를 센다. 2026-11-07 F2d 지형 패턴 개수 이식에서 추가됐다.</summary>
    /// <param name="frames">같은 실제 월드의 타입별 프레임 표다.</param>
    /// <param name="puzzlePieceType">패턴을 펼칠 때 쓰는 현재 조각 타입 번호다.</param>
    /// <param name="record">셀·회전을 담은 6바이트 기록이다.</param>
    /// <param name="edition">디코더 판본이다.</param>
    /// <param name="decoderDiagnostics">디코더 진단 묶음이다. null이면 금 간 검사를 건너뛴다.</param>
    /// <returns>빈·누락 프레임을 건너뛰고 센 유효 칸 수다.</returns>
    public static uint PatternCount(IReadOnlyList<RiftTypeFrames> frames, uint puzzlePieceType, ReadOnlySpan<byte> record, OriginalEdition edition, CanonDecoderDiagnostics? decoderDiagnostics)
    {
        ArgumentNullException.ThrowIfNull(frames);
        if (puzzlePieceType >= (uint)frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(puzzlePieceType), "일반 지형 현재 puzzlePiece 타입 범위 오류");
        }

        if (record.Length != 6)
        {
            throw new ArgumentException("일반 지형 패턴 기록은 6바이트여야 합니다.", nameof(record));
        }

        ReadOnlySpan<CanonPattern> patterns = CanonDecoder.TerritoryPatterns();
        int pattern = record[0] & 0x3f;
        if (pattern >= patterns.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(record), "일반 지형 패턴 번호 범위 오류");
        }

        var decoder = new CanonDecoder(frames[(int)puzzlePieceType], patterns[pattern], (record[0] >> 6) * 2, 0f, 0f, null, edition, decoderDiagnostics);
        uint count = 0u;
        // 원본 생성자의 최초 유효 칸을 포함하고 끝날 때까지 센다
        for (; decoder.Valid(); decoder.Advance())
        {
            ++count;
        }

        return count;
    }
}
