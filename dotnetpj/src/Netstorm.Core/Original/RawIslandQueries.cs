// 실제 mutable 섬 목록에서 현재 owner·원래 owner·territory를 읽는 세 검색 몸체다 (cpppj o/RawIslandQueries.h/.cpp, 2026-11-07 F2c-12 이식).
using System;

namespace Netstorm.Core.Original;

/// <summary>
/// 패치 <c>0046f380</c>/<c>0046f3f0</c>/<c>0046f430</c>와 CD <c>004bdaa0</c>/<c>004bdb10</c>/<c>004bdb60</c>의 읽기 전용 섬 검색 몸체다(cpppj <c>o/RawIslandQueries</c>, 2026-11-07 F2c-12 이식).
/// 같은 CoreData의 3204BYTE 기록과 현재 fort 모드를 참조하며 별도 섬 벡터를 만들지 않는다.
/// </summary>
public sealed class RawIslandQueries
{
    /// <summary>원본 두 검색 함수가 아무 섬도 찾지 못했을 때 반환하는 고정 번호(127)다 (2026-11-07 F2c-12 이식).</summary>
    public const uint NotFound = 127u;

    /// <summary>100개 섬 기록의 원본 stride(32바이트)다 (2026-11-07 F2c-12 이식).</summary>
    private const int IslandStride = 32;

    /// <summary>실제 32바이트 기록 안의 territory BYTE 위치(12)다 (2026-11-07 F2c-12 이식).</summary>
    private const int TerritoryOffset = 12;

    /// <summary>실제 32바이트 기록 안의 원래 owner BYTE 위치(13)다 (2026-11-07 F2c-12 이식).</summary>
    private const int OriginalOwnerOffset = 13;

    /// <summary>실제 currentCount/Exists/현재 owner와 3204BYTE를 가진 같은 월드의 섬 런타임이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly RawIslandRuntime _islands;

    /// <summary>owner/territory 검색의 현재 fort 플래그 상태다 (2026-11-07 F2c-12 이식).</summary>
    private readonly FortFinishOwnerState _modes;

    /// <summary>같은 CoreData의 3204BYTE 기록과 현재 fort 모드를 참조하는 섬 검색 객체를 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="islands">섬 런타임.</param>
    /// <param name="modes">FortFinishOwner 모드 상태.</param>
    public RawIslandQueries(RawIslandRuntime islands, FortFinishOwnerState modes)
    {
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(modes);
        _islands = islands;
        _modes = modes;
    }

    /// <summary>일반 모드는 signed 원래 owner BYTE와 현재 owner DWORD를 모두 비교하고, fort 모드는 현재 owner 0도 허용해 일치하는 첫 섬 번호를 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="owner">찾을 소유자 번호.</param>
    /// <param name="territory">찾을 영역 번호.</param>
    /// <returns>일치하는 첫 섬 번호(없으면 <see cref="NotFound"/>).</returns>
    public uint FindOwnerTerritory(int owner, int territory)
    {
        bool patch = _islands.Pool().Edition == OriginalEdition.Patch1078;
        bool initialFort = _modes.Fort;
        int initialCount = SignedCount(_islands.Count());
        // 실제 번호 0부터 순서대로 존재·두 owner·지역을 확인해 최초 일치를 반환한다
        for (int island = 0; island < (patch ? initialCount : SignedCount(_islands.Count())); ++island)
        {
            if (_islands.Exists(island) == 0u)
            {
                continue;
            }
            uint current = _islands.Owner(island);
            bool fort = patch ? initialFort : _modes.Fort;
            if (!fort)
            {
                if (SignedField(island, OriginalOwnerOffset) != owner || current != unchecked((uint)owner))
                {
                    continue;
                }
            }
            else if (current != 0u && current != unchecked((uint)owner))
            {
                continue;
            }
            if (SignedField(island, TerritoryOffset) == territory)
            {
                return (uint)island;
            }
        }
        return NotFound;
    }

    /// <summary>존재하는 기록 중 signed territory BYTE가 같은 첫 번호를 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="territory">찾을 영역 번호.</param>
    /// <returns>일치하는 첫 섬 번호(없으면 <see cref="NotFound"/>).</returns>
    public uint FindTerritory(int territory)
    {
        bool patch = _islands.Pool().Edition == OriginalEdition.Patch1078;
        int initialCount = SignedCount(_islands.Count());
        // 존재 DWORD가 0이 아닌 첫 기록의 signed 지역 BYTE를 비교한다
        for (int island = 0; island < (patch ? initialCount : SignedCount(_islands.Count())); ++island)
        {
            if (_islands.Exists(island) != 0u && SignedField(island, TerritoryOffset) == territory)
            {
                return (uint)island;
            }
        }
        return NotFound;
    }

    /// <summary>존재하는 기록 중 현재 owner DWORD가 같은 개수를 반환한다. owner 0도 비교 대상이다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="owner">비교할 현재 소유자 번호.</param>
    /// <returns>소유 섬 개수.</returns>
    public uint CountOwned(uint owner)
    {
        uint count = 0u;
        // count가 삭제 뒤 줄지 않는 원본 정책을 유지하며 빈 기록을 건너뛴다
        for (int island = 0; island < SignedCount(_islands.Count()); ++island)
        {
            if (_islands.Exists(island) != 0u && _islands.Owner(island) == owner)
            {
                ++count;
            }
        }
        return count;
    }

    /// <summary>실제 기록의 BYTE를 MSVC signed char처럼 32비트 부호 확장해 읽는다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="island">섬 번호.</param>
    /// <param name="field">기록 안 바이트 오프셋.</param>
    /// <returns>부호 확장된 32비트 정수.</returns>
    private int SignedField(int island, int field)
    {
        byte value = _islands.Bytes()[4 + island * IslandStride + field];
        return unchecked((sbyte)value);
    }

    /// <summary>원본의 signed count 비교를 위해 부호 없는 count 비트를 그대로 signed 정수로 변환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="count">부호 없는 count 값.</param>
    /// <returns>부호 있는 32비트 정수.</returns>
    private static int SignedCount(uint count) => unchecked((int)count);
}
