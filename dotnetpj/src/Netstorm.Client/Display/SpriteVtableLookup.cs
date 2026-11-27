using System.Buffers.Binary;
using Netstorm.Core.Original;

// 10.78 객체의 실제 가상 표시 함수를 TYPE 추측 없이 조회한다.
namespace Netstorm.Client.Display;

/// <summary>현재 객체 가상표의 프레임·추가 그림·픽셀 선택 함수 주소다.</summary>
/// <param name="Table">실제 객체 가상표 주소다.</param>
/// <param name="Frame">가상 +78 프레임 함수 주소다.</param>
/// <param name="Additional">가상 +a4 추가 그림 함수 주소다.</param>
/// <param name="Hit">가상 +b0 픽셀 선택 함수 주소다.</param>
public readonly record struct SpriteVtableEntry(uint Table, uint Frame, uint Additional, uint Hit);

/// <summary>cpppj의 10.78 실제 객체 가상표 83개를 같은 주소로 보존한다.</summary>
public static class SpriteVtableLookup
{
    /// <summary>보호된 원본에서 확인한 가상표와 세 표시 함수의 대응이다.</summary>
    private static readonly SpriteVtableEntry[] Entries =
    [
        new(0x00501dd0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00501ea0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00502370u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00502448u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00502510u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x005025d8u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x005026a0u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00502778u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00502840u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00502908u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x005029d0u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00502aa8u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00502b70u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00502c38u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00502cf0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00502db0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00502e68u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00502f30u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00502ff8u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x005030c0u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00503188u, 0x004acfe0u, 0x0041e3d0u, 0x00498000u),
        new(0x00503360u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x005034c8u, 0x004222f0u, 0x004223c0u, 0x00498000u),
        new(0x00503d80u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00503e50u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00503f20u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00503ff0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x005040c0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00504198u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00506d98u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00506e50u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00506f08u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x005070c0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x005075d8u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x005076b8u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00507770u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00507950u, 0x004acfe0u, 0x004484b0u, 0x00498000u),
        new(0x00508cc8u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00509148u, 0x00454cd0u, 0x00454cf0u, 0x00498000u),
        new(0x00509210u, 0x00454cd0u, 0x00454cf0u, 0x00498000u),
        new(0x005092d0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00509388u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00509470u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x005096a0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00509758u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00509810u, 0x004acfe0u, 0x0045ac40u, 0x00498000u),
        new(0x00509930u, 0x0045b530u, 0x0045b560u, 0x00498000u),
        new(0x005099e8u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00509aa0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00509b98u, 0x004acfe0u, 0x0045bcd0u, 0x00498000u),
        new(0x0050a050u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050a1e8u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050a388u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050a748u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050a820u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050a8e8u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050b570u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050b6e8u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050b7c0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050b888u, 0x004acfe0u, 0x0046a6e0u, 0x00498000u),
        new(0x0050b9b8u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050bad0u, 0x0046ae10u, 0x0046ae30u, 0x00498000u),
        new(0x0050c3b0u, 0x004acfe0u, 0x00473710u, 0x00498000u),
        new(0x0050c9f0u, 0x004acfe0u, 0x00478d10u, 0x00498000u),
        new(0x0050cc08u, 0x004acfe0u, 0x00479340u, 0x00498000u),
        new(0x0050da88u, 0x004acfe0u, 0x00485480u, 0x00498000u),
        new(0x0050db78u, 0x004acfe0u, 0x00485f90u, 0x00498000u),
        new(0x0050dc30u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050e568u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050e668u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050e738u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050e888u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050e940u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050ea08u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050ead0u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050eb98u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050ee08u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050f210u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x0050f540u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x005196f0u, 0x004acfe0u, 0x004dcd00u, 0x00498000u),
        new(0x005197a8u, 0x004acfe0u, 0x0041c100u, 0x00498000u),
        new(0x00519860u, 0x004acfe0u, 0x004dcf10u, 0x00498000u),
        new(0x00519918u, 0x004acfe0u, 0x004dd0f0u, 0x00498000u),
    ];

    /// <summary>현재 풀 슬롯의 가상표를 읽고 등록된 표시 함수 주소를 반환한다.</summary>
    /// <param name="pool">실제 10.78 SID 풀이다.</param>
    /// <param name="sid">현재 객체의 SID다.</param>
    /// <returns>같은 가상표의 세 함수 주소다.</returns>
    /// <remarks>cpppj ObjectHitVtables.inc와 RawCampaignFactoryDisplay의 HitVtable에 대응한다. 확인되지 않은 표는 임의 기본 그림으로 대체하지 않는다.</remarks>
    public static SpriteVtableEntry Read(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("객체 표시 가상표는 10.78 전용이다", nameof(pool));
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid));
        // 현재 객체의 실제 가상표와 일치하는 항목만 선택한다.
        foreach (SpriteVtableEntry entry in Entries)
            if (entry.Table == table) return entry;
        throw new InvalidOperationException($"객체 표시 가상표 미복원: 0x{table:X8}");
    }
}
