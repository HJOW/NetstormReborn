using System.Buffers.Binary;

// 실제352바이트 생산 Deck의 필드와 현재 시각 추첨을 복원한다.
namespace Netstorm.Core.Original;

/// <summary>원본 생산 Deck의 실제 크기와 필드 위치다.</summary>
public static class ProductionDeckLayout
{
    /// <summary>항목과 다섯 DWORD를 포함한 물리 크기다.</summary>
    public const int Bytes = 0x160;
    /// <summary>항목의 type/chance/power/remaining/order 폭이다.</summary>
    public const int EntryBytes = 5;
    /// <summary>물리 저장소에 들어가는 항목 수다.</summary>
    public const int Capacity = 66;
    /// <summary>signed 활성 항목 개수 위치다.</summary>
    public const int Count = 0x14c;
    /// <summary>DWORD 가중합 위치다.</summary>
    public const int Chance = 0x150;
    /// <summary>DWORD 추첨 누적 수 위치다.</summary>
    public const int Remaining = 0x154;
    /// <summary>Open이 저장하는 seed 위치다.</summary>
    public const int Seed = 0x158;
    /// <summary>bit0이 열림 여부인 DWORD 위치다.</summary>
    public const int Open = 0x15c;

    /// <summary>원본 고정 크기의 저장소만 허용한다.</summary>
    internal static void Check(int length)
    {
        if (length != Bytes) throw new ArgumentException("Deck의 실제352바이트 누락");
    }
    /// <summary>정렬과 무관하게 현재 DWORD를 읽는다.</summary>
    internal static uint Read(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw[offset..]);
    /// <summary>선택 DWORD만 쓰고 인접 항목과 꼬리는 보존한다.</summary>
    internal static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>signed 음수는 빈 순회이며 양수의 물리 초과만 거부한다.</summary>
    internal static int CurrentCount(ReadOnlySpan<byte> raw)
    {
        int count = unchecked((int)Read(raw, Count));
        return count > Capacity ? throw new ArgumentOutOfRangeException(nameof(raw), "Deck의66항목 물리 범위") : count;
    }
}

/// <summary>원본 Deck.cpp의 항목 추가 진단이다.</summary>
public enum ProductionDeckAssertion
{
    /// <summary>같은 type이 이미 존재한다.</summary>
    DuplicateType,
    /// <summary>항목 추가의 물리 용량을 벗어났다.</summary>
    Capacity,
    /// <summary>type DWORD가 BYTE로 잘렸다.</summary>
    TypeTruncated,
    /// <summary>chance signed DWORD가 BYTE로 잘렸다.</summary>
    ChanceTruncated,
    /// <summary>power 기록의 원문 축소 진단이다.</summary>
    PowerTruncated,
    /// <summary>remaining 기록의 원문 축소 진단이다.</summary>
    RemainingTruncated,
}

/// <summary>추첨의 현재 시각·진단·가중합 로그·선택적 power 분류만 공급한다.</summary>
public sealed class DeckDrawHooks
{
    /// <summary>호출 지점의 실제 double 벽시각이다.</summary>
    public Func<double>? WallSeconds { get; set; }
    /// <summary>원본 Deck.cpp의 진단 행 번호를 받는다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>현재 저장 가중합과 다시 계산한 합계의 종류1d 로그다.</summary>
    public Action<uint, uint>? Mismatch { get; set; }
    /// <summary>power 출력을 요청했을 때만 현재 네 제품 TYPE를 분류한다.</summary>
    public Func<uint, bool>? IsProduct { get; set; }
}

/// <summary>44f200 초기화·44f290 추가·44f250/280 열기/닫기·44f510 추첨 본체다.</summary>
/// <remarks>근거: cpppj/src/o/RawDeckDraw.cpp/.h. Draw는 Open의 저장 seed를 소비하지 않는다.</remarks>
public sealed class RawDeckDraw
{
    /// <summary>실제 소유자가 빌린 같은 물리 Deck이다.</summary>
    private readonly Memory<byte> _raw;
    /// <summary>Open의0seed 보충만 소비하는 전역 게임 난수다.</summary>
    private readonly GameRandom _global;
    /// <summary>현재 시각과 선택된 하위 효과다.</summary>
    private readonly DeckDrawHooks _hooks;

    /// <summary>필드를 초기화하지 않고 같은 물리 Deck과 전역 난수를 빌린다.</summary>
    public RawDeckDraw(Memory<byte> raw, GameRandom global, DeckDrawHooks hooks)
    {
        ProductionDeckLayout.Check(raw.Length); ArgumentNullException.ThrowIfNull(global); ArgumentNullException.ThrowIfNull(hooks);
        _raw = raw; _global = global; _hooks = hooks;
    }
    /// <summary>생성자의 원문 순서대로 다섯 DWORD만 초기화한다.</summary>
    public static void Initialize(Span<byte> raw)
    {
        ProductionDeckLayout.Check(raw.Length);
        ProductionDeckLayout.Put(raw, ProductionDeckLayout.Count, 0); ProductionDeckLayout.Put(raw, ProductionDeckLayout.Seed, 0);
        ProductionDeckLayout.Put(raw, ProductionDeckLayout.Chance, 0); ProductionDeckLayout.Put(raw, ProductionDeckLayout.Remaining, 0);
        ProductionDeckLayout.Put(raw, ProductionDeckLayout.Open, 0);
    }
    /// <summary>중복 진단 후 초기5바이트를 쓰고 축소 진단과 가중합/개수 증가를 실행한다.</summary>
    public static void Append(Span<byte> raw, uint type, int chance, Action<ProductionDeckAssertion>? assertion)
    {
        ProductionDeckLayout.Check(raw.Length); int count = ProductionDeckLayout.CurrentCount(raw);
        // 진단이 복귀해도 최초 개수까지 순회하고 현재 항목의 type을 읽는다.
        for (int index = 0; index < count; index++)
            if ((uint)raw[index * ProductionDeckLayout.EntryBytes] == type) Need(assertion)(ProductionDeckAssertion.DuplicateType);
        if (count < 0 || count >= ProductionDeckLayout.Capacity)
        {
            Need(assertion)(ProductionDeckAssertion.Capacity);
            throw new ArgumentOutOfRangeException(nameof(raw), "Deck 추가의 물리 범위");
        }
        int offset = count * ProductionDeckLayout.EntryBytes;
        raw[offset + 2] = 1; raw[offset] = unchecked((byte)type); raw[offset + 1] = unchecked((byte)chance);
        raw[offset + 3] = 255; raw[offset + 4] = 0;
        if (type > 255) Need(assertion)(ProductionDeckAssertion.TypeTruncated);
        if ((uint)raw[offset + 1] != unchecked((uint)chance)) Need(assertion)(ProductionDeckAssertion.ChanceTruncated);
        ProductionDeckLayout.Put(raw, ProductionDeckLayout.Chance, unchecked(ProductionDeckLayout.Read(raw, ProductionDeckLayout.Chance) + (uint)chance));
        ProductionDeckLayout.Put(raw, ProductionDeckLayout.Count, unchecked((uint)count + 1));
    }
    /// <summary>seed0일 때만 전역 난수를 한 번 소비하고 현재 open의bit0을 켠다.</summary>
    public void Open(uint seed)
    {
        if (seed == 0) seed = _global.Next(10000);
        ProductionDeckLayout.Put(_raw.Span, ProductionDeckLayout.Seed, seed);
        ProductionDeckLayout.Put(_raw.Span, ProductionDeckLayout.Open, ProductionDeckLayout.Read(_raw.Span, ProductionDeckLayout.Open) | 1u);
    }
    /// <summary>저장 seed와 다른 open 비트를 보존하며 bit0만 지운다.</summary>
    public void Close() => ProductionDeckLayout.Put(_raw.Span, ProductionDeckLayout.Open, ProductionDeckLayout.Read(_raw.Span, ProductionDeckLayout.Open) & ~1u);
    /// <summary>1013배·x87 ftol 낮은DWORD·signed sar4와 감김 곱셈/합산을 복원한다.</summary>
    public static uint WallSeed(double seconds)
    {
        double scaled = seconds * 1013.0; uint low = 0;
        if (double.IsFinite(scaled) && scaled >= -9223372036854775808.0 && scaled < 9223372036854775808.0)
            low = unchecked((uint)(long)scaled);
        return unchecked(low * 33u + (uint)((int)low >> 4));
    }
    /// <summary>power 분류 장치를 호출하지 않는 원문 NULL 출력 추첨이다.</summary>
    public uint Draw() => DrawCore(false, out _);
    /// <summary>선택한 제품이면power0, 일반 종류면-1, 선택 실패면0을 기록한다.</summary>
    public uint Draw(out int power) => DrawCore(true, out power);
    /// <summary>진단 뒤 현재 합계와 시각 효과 뒤 현재 항목을 선택하며 독립 시각 난수만 소비한다.</summary>
    private uint DrawCore(bool writePower, out int power)
    {
        if ((_raw.Span[ProductionDeckLayout.Open] & 1) == 0) Need(_hooks.Assertion)(0xe5);
        if (Sum() != ProductionDeckLayout.Read(_raw.Span, ProductionDeckLayout.Chance))
        {
            uint stored = ProductionDeckLayout.Read(_raw.Span, ProductionDeckLayout.Chance), sum = Sum();
            Need(_hooks.Mismatch)(stored, sum);
            ProductionDeckLayout.Put(_raw.Span, ProductionDeckLayout.Chance, Sum());
        }
        uint chance = ProductionDeckLayout.Read(_raw.Span, ProductionDeckLayout.Chance);
        var local = new GameRandom(WallSeed(Need(_hooks.WallSeconds)()));
        uint remaining = unchecked((int)chance) > 0 ? local.Next(chance) : 0;
        int index = 0, limit = ProductionDeckLayout.CurrentCount(_raw.Span);
        // 각 항목의 현재 signed power/chance를 읽고 선택된 뒤의 항목은 읽지 않는다.
        while (index < limit)
        {
            int offset = index * ProductionDeckLayout.EntryBytes;
            if (unchecked((sbyte)_raw.Span[offset + 2]) > 0) remaining = unchecked(remaining - _raw.Span[offset + 1]);
            if (unchecked((int)remaining) < 0) break;
            index++;
        }
        if (index >= limit) { power = 0; return 0; }
        int selected = index * ProductionDeckLayout.EntryBytes; uint type = _raw.Span[selected];
        _raw.Span[selected + 4] = unchecked((byte)(_raw.Span[selected + 4] + 1));
        ProductionDeckLayout.Put(_raw.Span, ProductionDeckLayout.Remaining, unchecked(ProductionDeckLayout.Read(_raw.Span, ProductionDeckLayout.Remaining) + 1u));
        power = writePower ? Need(_hooks.IsProduct)(type) ? 0 : -1 : 0;
        return type;
    }
    /// <summary>현재 signed power가 양수인 항목의 BYTE chance만 합산한다.</summary>
    private uint Sum()
    {
        uint sum = 0; int count = ProductionDeckLayout.CurrentCount(_raw.Span);
        // 현재 활성 범위만 방문하며 power0/음수 항목은 제외한다.
        for (int index = 0; index < count; index++)
            if (unchecked((sbyte)_raw.Span[index * ProductionDeckLayout.EntryBytes + 2]) > 0)
                sum = unchecked(sum + _raw.Span[index * ProductionDeckLayout.EntryBytes + 1]);
        return sum;
    }
    /// <summary>선택된 미연결 효과를 명시 오류로 보고한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("Deck 추첨의 실제 효과 미연결");
}
