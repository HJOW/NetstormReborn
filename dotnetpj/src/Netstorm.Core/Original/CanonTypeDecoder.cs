// 원본 CanonDecoder 생성자의 타입 전역·패턴 번호 선택을 공통 반복자에 연결한다 (cpppj o/CanonTypeDecoder.h/.cpp 및 o/CanonTypePatterns.inc, 2026-11-07 F2c-12 이식).
using System;

namespace Netstorm.Core.Original;

/// <summary>
/// 타입별 Canon 패턴·방향·초기 좌표를 고르는 입력 값이다(cpppj <c>o/CanonTypeDecoder.h</c>의 <c>CanonTypeQuery</c>, 2026-11-07 F2c-12 이식).
/// </summary>
/// <param name="Type">현재 타입 번호(패턴 번호와 구별).</param>
/// <param name="Argument">패턴 번호 또는 명시 프레임.</param>
/// <param name="Direction">방향 값.</param>
/// <param name="X">생성 시 캡처하는 시작 X 좌표.</param>
/// <param name="Y">생성 시 캡처하는 시작 Y 좌표.</param>
/// <param name="ExplicitFrame">비패턴 경로에서만 Argument를 프레임으로 선택할지 여부.</param>
public readonly record struct CanonTypeQuery(
    uint Type,
    int Argument,
    int Direction,
    float X,
    float Y,
    bool ExplicitFrame = false);

/// <summary>
/// 원본 <c>00425c20</c> ↔ CD <c>0041fcf0</c>의 타입별 패턴 선택과 특수 패턴 표(섬·받침·공유 전투)를 복원한 정적 클래스다 (cpppj <c>o/CanonTypeDecoder.h</c>·<c>o/CanonTypeDecoder.cpp</c>·<c>o/CanonTypePatterns.inc</c>, 2026-11-07 F2c-12 이식).
/// </summary>
public static class CanonTypeDecoder
{
    /// <summary>패치 0x531410, CD/10.37 0x5164f8: 섬 패턴 2개 표다 (2026-11-07 F2c-12 이식).</summary>
    private static readonly CanonPattern[] IslandPatterns =
    [
        new(10, 1, 1, [new(0x31,0x50,0x00,0x00), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0)]),
        new(10, 1, 1, [new(0x33,0x50,0x00,0x00), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0)]),
    ];

    /// <summary>패치 0x5314a0, CD/10.37 0x516588: 받침(noIsland) 패턴 1개 표다 (2026-11-07 F2c-12 이식).</summary>
    private static readonly CanonPattern[] SupportPatterns =
    [
        new(10, 3, 3, [new(0x31,0x46,0x00,0x00), new(0x31,0x43,0x00,0x00), new(0x31,0x47,0x00,0x00), new(0x31,0x42,0x00,0x00), new(0x31,0x41,0x00,0x00), new(0x31,0x44,0x00,0x00), new(0x31,0x49,0x00,0x00), new(0x31,0x45,0x00,0x00), new(0x31,0x48,0x00,0x00), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0)]),
    ];

    /// <summary>패치 0x5314e8, CD/10.37 0x5165d0: 공유 전투 패턴 1개 표다 (2026-11-07 F2c-12 이식).</summary>
    private static readonly CanonPattern[] BattlePatterns =
    [
        new(10, 1, 1, [new(0x30,0x4c,0x00,0x00), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0), new(0,0,0,0)]),
    ];

    /// <summary>분석·검사에 복원된 특수 패턴 레코드 표를 돌려준다. 그룹은 island=0, noIsland=1, 공유 전투=2다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="group">특수 패턴 그룹 번호(0..2).</param>
    /// <returns>해당 그룹의 패턴 스팬.</returns>
    public static ReadOnlySpan<CanonPattern> CanonSpecialPatterns(int group)
    {
        return group switch
        {
            0 => IslandPatterns,
            1 => SupportPatterns,
            2 => BattlePatterns,
            _ => throw new ArgumentOutOfRangeException(nameof(group), "CanonDecoder 특수 패턴 그룹 오류"),
        };
    }

    /// <summary>
    /// 원본 00425c20 ↔ CD 0041fcf0: 앞 네 전역을 우선 비교하고 뒤 네 타입은 공유 표를 써서 <see cref="CanonDecoder"/>를 만든다 (2026-11-07 F2c-12 이식).
    /// 전역 순서: puzzlePiece, bridge, island, noIsland, thunderCannon, rainCannon, windArcher, windBlocker.
    /// </summary>
    /// <param name="frames">타입 프레임 코드 표.</param>
    /// <param name="defaultFrame">비패턴 기본 프레임.</param>
    /// <param name="patternTypes">8개 패턴 타입 전역 번호 스팬.</param>
    /// <param name="query">타입·인자·방향·좌표·명시 프레임 질의.</param>
    /// <param name="edition">원본 판본.</param>
    /// <param name="diagnostics">Checking·단언 진단 묶음.</param>
    /// <returns>초기화된 <see cref="CanonDecoder"/>.</returns>
    public static CanonDecoder DecodeCanonType(
        RiftTypeFrames frames,
        int defaultFrame,
        ReadOnlySpan<uint> patternTypes,
        CanonTypeQuery query,
        OriginalEdition edition,
        CanonDecoderDiagnostics? diagnostics = null)
    {
        ArgumentNullException.ThrowIfNull(frames);
        if (patternTypes.Length != 8)
        {
            throw new ArgumentException("patternTypes는 8개여야 합니다.", nameof(patternTypes));
        }
        if (!float.IsFinite(query.X) || !float.IsFinite(query.Y))
        {
            throw new ArgumentException("CanonDecoder 좌표 오류", nameof(query));
        }
        if (edition == OriginalEdition.Cd1072 && (query.Direction & 1) != 0)
        {
            throw new ArgumentOutOfRangeException(nameof(query), "CD CanonDecoder 홀수 방향 오류");
        }

        ReadOnlySpan<CanonPattern> patterns;
        if (query.Type == patternTypes[0])
        {
            patterns = CanonDecoder.TerritoryPatterns();
        }
        else if (query.Type == patternTypes[1])
        {
            patterns = CanonDecoder.BridgePatterns(edition);
        }
        else if (query.Type == patternTypes[2])
        {
            patterns = CanonSpecialPatterns(0);
        }
        else if (query.Type == patternTypes[3])
        {
            patterns = CanonSpecialPatterns(1);
        }
        else if (ContainsType(patternTypes.Slice(4), query.Type))
        {
            patterns = CanonSpecialPatterns(2);
        }
        else
        {
            return new CanonDecoder(
                frames,
                defaultFrame,
                query.Argument,
                query.Direction,
                query.X,
                query.Y,
                query.ExplicitFrame,
                edition,
                diagnostics);
        }

        if (query.Argument < 0 || query.Argument >= patterns.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(query), "CanonDecoder 패턴 번호 오류");
        }
        return new CanonDecoder(
            frames,
            patterns[query.Argument],
            query.Direction,
            query.X,
            query.Y,
            null,
            edition,
            diagnostics);
    }

    /// <summary>뒤 네 공유 전투 타입 전역 중 일치하는 번호가 있는지 찾는다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="types">검사할 타입 번호 스팬.</param>
    /// <param name="target">찾을 타입 번호.</param>
    /// <returns>포함되어 있으면 true.</returns>
    private static bool ContainsType(ReadOnlySpan<uint> types, uint target)
    {
        // 4개 공유 전투 타입 번호를 순서대로 비교한다
        for (int i = 0; i < types.Length; ++i)
        {
            if (types[i] == target)
            {
                return true;
            }
        }
        return false;
    }
}
