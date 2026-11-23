using System.Buffers.Binary;

// 같은 장기 Host의 현재 VT를 실제 정기 본체로 분배하고 큰 효과는 소비 위치에서 요구한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/client/RawSurfaceStorage.cpp의 기본 수명과 현재 VT 정기 분배 중 이식된 본체를 연결한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>보호 PE의 실제 +5c가00496e70인16개 VT만 원문 payload 반환을 허용한다.</summary>
    private static readonly uint[] BaseRegularVtables = [0x501dd0, 0x503360, 0x506d98, 0x506e50, 0x506f08, 0x5070c0, 0x5092d0, 0x509388,
        0x509b98, 0x50c3b0, 0x50c9f0, 0x50db78, 0x50eb98, 0x512098, 0x512110, 0x512188];
    /// <summary>실제 +5c가0044b900인14개 VT만 같은 Damageable 본문으로 보낸다.</summary>
    private static readonly uint[] DamageableRegularVtables = [0x501ea0, 0x509930, 0x5099e8, 0x509aa0, 0x50a1e8, 0x50a388, 0x50b6e8,
        0x50b7c0, 0x50b888, 0x50b9b8, 0x50bad0, 0x50e940, 0x50ea08, 0x50ead0];

    /// <summary>현재 raw VT로 실제 본체를 고르며 아직 없는 전용 본문을 기본 반환으로 대신하지 않는다.</summary>
    public float HandleDefaultRegular(int sid, uint @event, uint count, float payload)
    {
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid));
        if (table == 0x50b570) return DiscRegular(sid, @event, count, payload);
        if (table == 0x50cc08) return RawSpriteRegular.Run(Pool, Frames, FrameExecutor(), sid, payload);
        if (table == 0x50ee08) return PlayerRegular(sid, @event, count, payload);
        if (table == 0x50da88) return RawGeyserRegular.Run(Pool, Types, sid, @event, count, payload, new()
        {
            DisplayUpdate = (parent, flags) => DisplayExecutor().Update(Pool.Slot(parent), flags),
            Damageable = ResourceDamageableRegular,
        });
        if (table == 0x50e888) return IceTower().Handle(sid, @event, count, payload);
        if (table == 0x509470) return FenceRegular().Handle(sid, @event, count, payload);
        if (Array.IndexOf(DamageableRegularVtables, table) >= 0) return ResourceDamageableRegular(sid, @event, count, payload);
        if (Array.IndexOf(BaseRegularVtables, table) >= 0) return payload;
        throw new InvalidOperationException($"World 없는 현재 VT 0x{table:x8}의 전용 Regular 몸체 미설치");
    }

    /// <summary>현재 피해/건설 사건만 실제 큰 장치를 요구하며 일반 사건은 원문 기본 반환을 실행한다.</summary>
    public float ResourceDamageableRegular(int sid, uint @event, uint count, float payload)
    {
        if (@event == GameAux().DamageRegularEvent)
            return (_resourceDevices.DamageableRegular ?? throw new InvalidOperationException("장기 Damageable의 실제 피해 사건 미연결"))(sid, @event, count, payload);
        if (@event == GameAux().ConstructionRegularEvent)
        {
            if ((Pool.Slot(sid)[40] & 1) != 0)
            {
                var lookup = new RawContainedLookup(Pool, (parent, flags) =>
                    (_resourceDevices.Destroy ?? throw new InvalidOperationException("장기 Form의 현재 가상 삭제 미연결"))(parent, flags));
                if (lookup.Find(sid, GameAux().ConstructionProcessTypeBox.Value) == 0)
                    (_resourceDevices.StartConstruction ?? throw new InvalidOperationException("장기 Damageable의 실제 건설 재시도 미연결"))(sid);
            }
            return 0;
        }
        return payload;
    }
}
