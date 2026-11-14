using System.Buffers.Binary;

// 현재 Carrier TYPE·지면 관문을 읽고 선택된 실제 Walker 낙하만 실행한다.
namespace Netstorm.Core.Original;

/// <summary>원본427720의 지면 확인과 사제/기타 Walker 낙하 호출이다.</summary>
public sealed class RawCarrierCheck
{
    /// <summary>원문 거의 올림용 단정도 배율이다.</summary>
    private const float CoordinateBias = 0.9999f;
    /// <summary>현재 물리 슬롯을 읽는 동일 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>매 호출 현재TYPE genus를 읽는 대여 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 원문 지면 비트를 읽는 같은 지도다.</summary>
    private readonly byte[] _spots;
    /// <summary>구성 때 복사한 실제 Walker 낙하 함수다.</summary>
    private readonly Func<int, bool> _fallWalker;
    /// <summary>판본별 물리 표/지도와 실제 낙하 함수만 구성 때 확인한다.</summary>
    public RawCarrierCheck(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, byte[] spots, Func<int, bool> fallWalker)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(spots); ArgumentNullException.ThrowIfNull(fallWalker);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || spots.Length != 65536) throw new ArgumentException("Carrier 지면 조회 연결 오류");
        _pool = pool; _types = types; _spots = spots; _fallWalker = fallWalker;
    }
    /// <summary>어댑터가 동일 풀을 확인할 실제 참조다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>double 합을 먼저0방향 절삭해 음수0도 원문0셀로 처리한다.</summary>
    private static int Cell(float coordinate)
    {
        double value = Math.Truncate((double)coordinate + CoordinateBias);
        if (!double.IsFinite(value) || value < 0 || value >= 256) throw new ArgumentOutOfRangeException(nameof(coordinate), "Carrier 지면 좌표 오류");
        return (int)value;
    }
    /// <summary>현재TYPE→제외 genus→현재 y/x→지면→실제 Walker 낙하 순서다.</summary>
    public bool Check(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid); byte type = raw[10];
        if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "Carrier 지면 조회 타입 오류");
        if ((_types[type].Flags2 & 0x320000) != 0) return false;
        int y = Cell(BinaryPrimitives.ReadSingleLittleEndian(raw[18..])), x = Cell(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]));
        if ((_spots[y * 256 + x] & 6) != 0) return false;
        return _fallWalker(sid);
    }
    /// <summary>같은 풀의 실제 Carrier 확인으로 postPop 효과 하나만 교체한다.</summary>
    public static CarrierPostPopHooks MakeCarrierCheckPostPopHooks(RawSidPool pool, RawCarrierCheck check, CarrierPostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(check); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, check.Pool)) throw new ArgumentException("Carrier postPop/조회 풀 불일치");
        return new CarrierPostPopHooks { CarrierCheck = sid => check.Check(sid), ClaimGround = hooks.ClaimGround, ValidateBase = hooks.ValidateBase, Base = hooks.Base };
    }
    /// <summary>같은 풀의 실제 Carrier 확인으로 생성 후 가상 효과 하나만 교체한다.</summary>
    public static PriestSpawnHooks MakeCarrierCheckSpawnHooks(RawSidPool pool, RawCarrierCheck check, PriestSpawnHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(check); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, check.Pool)) throw new ArgumentException("사제 생성/Carrier 조회 풀 불일치");
        PriestSpawnHooks result = hooks.Copy(); result.CheckCarrier = sid => check.Check(sid); return result;
    }
}
