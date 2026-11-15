using System.Buffers.Binary;

// 실제 현재 VT의 생산 접두·중립 몸체와 기존 표면 소유자 분배를 합성한다.
namespace Netstorm.Core.Original;

/// <summary>PATCH/CD 자산 가상+74의 원문 쓰기 순서와 현재 카운터 주소를 보존한다.</summary>
public static class RawAssetOwner
{
    /// <summary>같은 풀/표면 전역/생산 DWORD와 실제 공통 부모를 빌려 현재 분배기를 만든다.</summary>
    public static Action<int, uint> MakeAssetOwnerDispatch(RawSidPool pool, BridgeConnectState state, UIntBox productionDirty, Action<int, uint> parent)
    {
        ArgumentNullException.ThrowIfNull(productionDirty);
        Action<int, uint> surfaces = RawBridgeConnect.MakeOwnerDispatch(pool, state, parent);
        return (sid, owner) =>
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid); bool patch = pool.Edition == OriginalEdition.Patch1078;
            uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
            if (table == (patch ? 0x50a8e8u : 0x500b10u)) { pool.RawSlot(sid)[patch ? 34 : 32] = 0; return; }
            if (table == (patch ? 0x509148u : 0x503298u) || table == (patch ? 0x509210u : 0x503358u))
                if ((raw[patch ? 40 : 35] & 9) == 0) productionDirty.Value = unchecked(productionDirty.Value + 1);
            surfaces(sid, owner);
        };
    }
}
