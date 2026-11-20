using static Netstorm.Core.Original.RawMissionPacketFields;

// 원문 x87 비교와 실제 TYPE 비용/공급 범위/환불/밀도 쓰기를 실행한다.
namespace Netstorm.Core.Original;

/// <summary>같은 실제 옵션·TYPE·사용표와 현재 전역을 빌린다.</summary>
public sealed record MissionBattleOptionState
{
    /// <summary>0052f7e0의 현재87바이트다.</summary>
    public ReadOnlyMemory<byte> Options { get; init; }
    /// <summary>실제188TYPE의 같은 mutable 레코드를 빌린다.</summary>
    public ReadOnlyMemory<OriginalTypeRecord> Types { get; init; }
    /// <summary>005451f8의 같은 실제256바이트 사용표다.</summary>
    public Memory<byte> TypeUse { get; init; }
    /// <summary>005425f4의 실제3~4개 현재 float이며 기본 선택3은4개를 요구한다.</summary>
    public ReadOnlyMemory<float> RangeTable { get; init; }
    /// <summary>00541240의 현재 비용 변경 TYPE 번호를 쓰기 직전에 읽는다.</summary>
    public Func<uint>? SelectedType { get; init; }
    /// <summary>0052f488의 같은 실제 공급 반경이다.</summary>
    public FloatBox Range { get; init; } = new();
    /// <summary>0052f48c의 같은 실제 반경 제곱이다.</summary>
    public FloatBox RangeSquared { get; init; } = new();
    /// <summary>005424bc의 같은 실제 환불 비율이다.</summary>
    public UIntBox Value { get; init; } = new();
}

/// <summary>RawMissionBattleOptions.cpp의 현재 파생값·밀도/난수·타입 사용표 본체다.</summary>
public sealed partial class RawMissionBattleOptions
{
    /// <summary>실제 현재 배열과 변경 대상 주소다.</summary>
    private readonly MissionBattleOptionState _state;
    /// <summary>PATCH의 물리 크기를 확인하며 파생값이나 사용표를 초기화하지 않는다.</summary>
    public RawMissionBattleOptions(OriginalEdition edition, MissionBattleOptionState state)
    {
        ArgumentNullException.ThrowIfNull(state);
        if (edition != OriginalEdition.Patch1078 || state.Options.Length != 87 || state.Types.Length != 188 || state.TypeUse.Length != 256 || state.RangeTable.Length is not (3 or 4)) throw new ArgumentException("미션 옵션의 실제 PATCH 저장소 크기 오류"); _state = state;
    }
    /// <summary>실제 선택 위치에서 현재 BYTE만 읽고 전체 옵션을 복제하지 않는다.</summary>
    private static byte OptionByte(ReadOnlyMemory<byte> options, int at) => (byte)Get(options.Span, at, 1);
    /// <summary>실제 필요 위치의 float만 읽고 범위 밖이면 그 전 쓰기를 보존한다.</summary>
    private static float Choice(ReadOnlyMemory<float> choices, int at)
    {
        if ((uint)at >= choices.Length) throw new ArgumentOutOfRangeException(nameof(at), "현재 공급 반경 선택표 물리 범위"); return choices.Span[at];
    }
    /// <summary>원본 FLD/FSTP32의 signaling NaN quiet 쓰기와 나머지 비트를 보존한다.</summary>
    private static float StoreX87(float value)
    {
        uint bits = BitConverter.SingleToUInt32Bits(value); if ((bits & 0x7f800000) == 0x7f800000 && (bits & 0x007fffff) != 0) bits |= 0x00400000; return BitConverter.UInt32BitsToSingle(bits);
    }
    /// <summary>최초 선택을 먼저 쓰고 원문 ordered 최대/최소 비교와 float32 제곱을 수행한다.</summary>
    public static void UpdateSupplyRange(ReadOnlyMemory<byte> options, ReadOnlyMemory<float> choices, FloatBox range, FloatBox squared)
    {
        ArgumentNullException.ThrowIfNull(range); ArgumentNullException.ThrowIfNull(squared); int index = OptionByte(options, 4); range.Value = StoreX87(Choice(choices, index));
        float candidate = Choice(choices, 2) < range.Value ? Choice(choices, 2) : range.Value;
        if (Choice(choices, 0) > candidate) range.Value = StoreX87(Choice(choices, 0)); else if (Choice(choices, 2) < range.Value) range.Value = StoreX87(Choice(choices, 2)); squared.Value = range.Value * range.Value;
    }
    /// <summary>같은 현재 필드에 공급 반경 전체 부모를 적용한다.</summary>
    public void UpdateRange() => UpdateSupplyRange(_state.Options, _state.RangeTable, _state.Range, _state.RangeSquared);
    /// <summary>0041c8b0의 현재BYTE10h가 선택한1000/2000/3000/5000이다.</summary>
    public static int OptionTypeCost(ReadOnlyMemory<byte> options)
    {
        ReadOnlySpan<int> costs = [1000, 2000, 3000, 5000]; int index = OptionByte(options, 0x10); if (index >= costs.Length) throw new ArgumentOutOfRangeException(nameof(options), "미션 TYPE 비용 옵션 인덱스"); return costs[index];
    }
    /// <summary>현재 같은 옵션의 비용을 읽는다.</summary>
    public int TypeCost() => OptionTypeCost(_state.Options);
    /// <summary>0041ca70의 현재BYTE0fh가 선택한 환불 비율이다.</summary>
    public static uint OptionRefundPercent(ReadOnlyMemory<byte> options)
    {
        ReadOnlySpan<uint> values = [0, 25, 50, 75, 100, 150]; int index = OptionByte(options, 0xf); if (index >= values.Length) throw new ArgumentOutOfRangeException(nameof(options), "미션 파생값 옵션 인덱스"); return values[index];
    }
    /// <summary>현재 파생값을 캐시로 가리지 않고 같은 옵션에서 읽는다.</summary>
    public uint CurrentValue() => OptionRefundPercent(_state.Options);
    /// <summary>현재BYTE7이 선택한 최소 밀도10/15/20/25다.</summary>
    public int MinimumDensity() { ReadOnlySpan<int> choices = [10, 15, 20, 25]; int index = OptionByte(_state.Options, 7); if (index >= choices.Length) throw new ArgumentOutOfRangeException(nameof(index), "일반 전투 최소 밀도 옵션 범위"); return choices[index]; }
    /// <summary>하한 선택을 재사용하지 않고 현재BYTE7의 최대 밀도30/35/40/45를 읽는다.</summary>
    public int MaximumDensity() { ReadOnlySpan<int> choices = [30, 35, 40, 45]; int index = OptionByte(_state.Options, 7); if (index >= choices.Length) throw new ArgumentOutOfRangeException(nameof(index), "일반 전투 최대 밀도 옵션 범위"); return choices[index]; }
    /// <summary>현재 두 한계 쓰기 뒤 같은 게임 난수의 반값 범위 두 표본을 감김 합계로 쓴다.</summary>
    public void InitializeDensity(GameRandom random, IntBox minimum, IntBox maximum, UIntBox percent)
    {
        ArgumentNullException.ThrowIfNull(random); ArgumentNullException.ThrowIfNull(minimum); ArgumentNullException.ThrowIfNull(maximum); ArgumentNullException.ThrowIfNull(percent);
        minimum.Value = MinimumDensity(); maximum.Value = MaximumDensity();
        /// <summary>각 실제 난수 호출에서 같은 현재 두 한계를 다시 읽는다.</summary>
        uint Sample() { int low = minimum.Value / 2, high = maximum.Value / 2; return unchecked(random.Next(unchecked((uint)high - (uint)low)) + (uint)low); }
        uint first = Sample(), second = Sample(); percent.Value = unchecked(first + second);
    }
    /// <summary>현재 TYPE 번호를 쓰기 직전에 읽고 같은 mutable 비용 필드만 변경한다.</summary>
    public void SetSelectedTypeCost(float cost)
    {
        uint type = Need(_state.SelectedType)(); if (type >= _state.Types.Length) throw new ArgumentOutOfRangeException(nameof(type), "미션 비용 대상 TYPE 범위"); _state.Types.Span[(int)type].Cost = cost;
    }
    /// <summary>범위→비용 조회/현재TYPE쓰기→현재 환불 비율 쓰기를 원문 순서로 수행한다.</summary>
    public void ApplyDerived() { UpdateRange(); int cost = TypeCost(); SetSelectedTypeCost(cost); _state.Value.Value = CurrentValue(); }
    /// <summary>현재 사용표256바이트만0으로 만들며 양쪽 이웃은 보존한다.</summary>
    public void ClearTypes() => _state.TypeUse.Span.Clear();
}
