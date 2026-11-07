// 실제 표면/다리의 섬 번호 지정과 조회 몸체를 같은 SID 풀과 현재 타입 표에 적용한다(cpppj o/RawSquidRegion.h/.cpp).
// 2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 이식했다.
namespace Netstorm.Core.Original;

/// <summary>
/// 패치판의 두 원본 assert 원인이다. 진단 뒤 원본처럼 region 쓰기를 생략한다
/// (cpppj <c>o/RawSquidRegion.h</c>의 <c>SquidRegionAssert</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public enum SquidRegionAssert
{
    /// <summary>island/bridge/island3x3 중 어느 genus도 없는 현재 타입이다.</summary>
    UnsupportedGenus,

    /// <summary>다리는 현재 noRegion DWORD만 받을 수 있으며 항상 기존 WORD를 보존한다.</summary>
    BridgeRegion,
}

/// <summary>
/// 패치 004acd20·004acda0 / CD 004aca80의 표면 섬 번호 읽기·쓰기 도우미다
/// (cpppj <c>o/RawSquidRegion.h</c>·<c>o/RawSquidRegion.cpp</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public static class RawSquidRegion
{
    /// <summary>원본 Squid의 타입 BYTE 위치다.</summary>
    private const int TypeOffset = 10;

    /// <summary>원본 Squid의 섬 WORD 위치다.</summary>
    private const int RegionOffset = 8;

    /// <summary>세 표면 genus 묶음 비트다.</summary>
    private const uint SurfaceGenus = 0x01000006u;

    /// <summary>다리 genus 비트다.</summary>
    private const uint BridgeGenus = 4u;

    /// <summary>
    /// 004acda0: 현재 genus 진단/다리의 noRegion 반환 뒤 실제 raw+8 WORD를 읽는다.
    /// </summary>
    /// <param name="pool">SID 풀이다.</param>
    /// <param name="types">현재 타입 레코드 표다.</param>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <param name="noRegion">지역 없음 반환 값이다.</param>
    /// <param name="diagnostic">패치 진단 콜백이다.</param>
    /// <returns>읽은 섬 번호 또는 noRegion이다.</returns>
    public static uint GetSquidRegion(
        RawSidPool pool,
        IReadOnlyList<OriginalTypeRecord> types,
        int sid,
        uint noRegion,
        Action<SquidRegionAssert>? diagnostic = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        byte typeIndex = raw[TypeOffset];
        if (typeIndex >= types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "섬 번호 getter의 현재 TYPE 범위");
        }
        uint genus = types[typeIndex].Flags2;
        if ((genus & SurfaceGenus) == 0)
        {
            if (diagnostic is null)
            {
                throw new InvalidOperationException("섬 번호 getter의 원문 genus 진단 미연결");
            }
            diagnostic(SquidRegionAssert.UnsupportedGenus);
            return noRegion;
        }
        return (genus & BridgeGenus) != 0
            ? noRegion
            : (uint)raw[RegionOffset] | ((uint)raw[RegionOffset + 1] << 8);
    }

    /// <summary>
    /// 패치 004acd20 / CD 004aca80: 허용 표면의 raw+8 WORD에 region 하위 16비트를 저장한다.
    /// 패치 진단 분기는 필수 callback을 호출하며 CD는 조용히 반환한다.
    /// </summary>
    /// <param name="pool">SID 풀이다.</param>
    /// <param name="types">현재 타입 레코드 표다.</param>
    /// <param name="sid">대상 슬롯 번호다.</param>
    /// <param name="region">저장할 지역 번호다.</param>
    /// <param name="noRegion">지역 없음 값이다.</param>
    /// <param name="diagnostic">패치 진단 콜백이다.</param>
    public static void SetSquidRegion(
        RawSidPool pool,
        IReadOnlyList<OriginalTypeRecord> types,
        int sid,
        uint region,
        uint noRegion,
        Action<SquidRegionAssert>? diagnostic = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        byte typeIndex = raw[TypeOffset];
        if (typeIndex >= types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "섬 번호 지정 타입 범위 오류");
        }
        uint genus = types[typeIndex].Flags2;
        if ((genus & SurfaceGenus) == 0 || (genus & BridgeGenus) != 0)
        {
            if (pool.Edition == OriginalEdition.Patch1078 && ((genus & SurfaceGenus) == 0 || region != noRegion))
            {
                if (diagnostic is null)
                {
                    throw new InvalidOperationException("섬 번호 지정 원본 진단 경계 누락");
                }
                diagnostic((genus & SurfaceGenus) == 0 ? SquidRegionAssert.UnsupportedGenus : SquidRegionAssert.BridgeRegion);
            }
            return;
        }
        Span<byte> target = pool.AllocatedBytes(sid);
        target[RegionOffset] = (byte)region;
        target[RegionOffset + 1] = (byte)(region >> 8);
    }
}
