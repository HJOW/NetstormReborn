using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

// Mana 공급원 수집·문자 대응·배치 허용 본체를 대조한다.
// 2026-11-26 1.054에서 SL-3a 전체 EnergySupply용으로 추가했다.
public sealed class RawEnergySupplyTests
{
    /// <summary>188TYPE·공급 목록·30칸 선택·9×9 동맹·상태·갈고리로 본체를 묶는다.</summary>
    /// <returns>풀·본체·공급 목록·선택 목록이다.</returns>
    private static (RawSidPool Pool, RawEnergySupply Supply, RawPostPopList Providers, RawPostPopList Selected) Scene(
        uint[]? alliances = null, EnergySupplyState? state = null, Func<uint, string>? requirement = null)
    {
        var pool = new RawSidPool(24000, false);
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        types[50].FootX = 2; types[50].FootY = 2; types[50].Theme = 0;
        var providers = new RawPostPopList { Entries = new uint[256] };
        var selected = new RawPostPopList { Entries = new uint[30] };
        var supply = new RawEnergySupply(pool, types, providers, selected,
            alliances ?? new uint[81], 256, state ?? new EnergySupplyState(),
            new EnergySupplyHooks
            {
                ProviderEntry = RawEnergySupply.EnergyProviderEntry,
                Requirement = requirement ?? (_ => ""),
            });
        return (pool, supply, providers, selected);
    }

    /// <summary>실제 풀 슬롯에 공급원 DWORD(가상 표·타입·좌표·소유자)를 쓴다.</summary>
    /// <returns>쓴 SID다.</returns>
    private static int PlaceProvider(RawSidPool pool, int sid, uint vtable, int type, float x, float y, uint owner)
    {
        Span<byte> raw = pool.RawSlot(sid);
        raw.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(raw, vtable);
        raw[10] = unchecked((byte)type);
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(14), x);
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(18), y);
        raw[34] = unchecked((byte)owner);
        return sid;
    }

    /// <summary>알려진 83 가상 표를 나누고 표 밖을 거부하며 계약 위반을 거부한다.</summary>
    [Fact]
    public void ProviderEntryMapsKnownTablesAndRejectsUnknown()
    {
        Assert.Equal(0x0041c170u, RawEnergySupply.EnergyProviderEntry(0x00501ea0u));
        Assert.Equal(0x0041c0f0u, RawEnergySupply.EnergyProviderEntry(0x00501dd0u));
        Assert.Equal(0x0041c170u, RawEnergySupply.EnergyProviderEntry(0x00519918u));
        Assert.Throws<InvalidOperationException>(() => RawEnergySupply.EnergyProviderEntry(0x12345678u));
        var (pool, _, providers, selected) = Scene();
        var types = Enumerable.Range(0, 10).Select(_ => new OriginalTypeRecord()).ToArray();
        Assert.Throws<ArgumentException>(() => new RawEnergySupply(pool, types, providers, selected, new uint[81], 256, new(), new() { ProviderEntry = _ => 0u, Requirement = _ => "" }));
        Assert.Throws<ArgumentException>(() => new RawEnergySupply(pool, Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(), providers, selected, new uint[9], 256, new(), new() { ProviderEntry = _ => 0u, Requirement = _ => "" }));
    }

    /// <summary>덮는 소유·동맹 공급원만 모아 원소 문자를 만들고 선택 목록에 남긴다.</summary>
    [Fact]
    public void CollectGathersCoveringSuppliersAndBuildsLetters()
    {
        uint[] alliances = new uint[81]; alliances[2 * 9 + 1] = 1;
        var (pool, supply, providers, selected) = Scene(alliances);
        PlaceProvider(pool, 100, 0x00501ea0u, 50, 60f, 60f, 1);
        PlaceProvider(pool, 102, 0x00501ea0u, 50, 60f, 60f, 2);
        PlaceProvider(pool, 101, 0x00501ea0u, 50, 60f, 60f, 3);
        PlaceProvider(pool, 103, 0x00501ea0u, 50, 200f, 200f, 1);
        providers.Append(100); providers.Append(102); providers.Append(101); providers.Append(103);
        // 100(자신)·102(동맹)만 덮고 101(비동맹)·103(원거리)은 빠진다.
        Assert.Equal("ww", supply.Collect(60f, 60f, 50, 1));
        Assert.Equal([100u, 102u], selected.Items().ToArray());
        // 특정 원소 요청은 그것을 먼저 쓰고 나머지를 범용s로 둔다. 0번 원소 2개면 "ww"가 된다.
        Assert.Equal("ww", supply.Collect(60f, 60f, 50, 1, 0));
        // 1번 원소를 달라고 하면 0번 2개가 범용s가 된다.
        Assert.Equal("ss", supply.Collect(60f, 60f, 50, 1, 1));
    }

    /// <summary>같은 문자는 대문자화하고 s는 비점→대체 순서로 소비하며 우회를 따른다.</summary>
    [Fact]
    public void MatchConsumesLettersAndHonorsOverride()
    {
        var (_, supply, _, _) = Scene();
        string available = "wwr"; string required = "wr";
        Assert.Equal(1u, supply.Match(ref available, required, out string remainder));
        Assert.Equal("WR", available);
        Assert.Equal(".w.", remainder);
        available = "w";
        Assert.Equal(1u, supply.Match(ref available, "s", out remainder));
        Assert.Equal("S", available);
        Assert.Equal(".", remainder);
        available = "w";
        Assert.Equal(0u, supply.Match(ref available, "t", out remainder));
        Assert.Equal("t", available);
        Assert.Equal("w", remainder);
        Assert.Throws<InvalidOperationException>(() => supply.Match(ref available, "x", out _));
        // 우회가 켜지면 대응 실패도 성공한다.
        var state = new EnergySupplyState { Special = 1, Ignore = 1 };
        var (_, overriding, _, _) = Scene(state: state);
        available = "w";
        Assert.Equal(1u, overriding.Match(ref available, "t", out _));
        Assert.Equal("t", available);
    }

    /// <summary>빈 요구는 1이며 요구 대응 성공 시 수집 길이를 돌려준다.</summary>
    [Fact]
    public void EvaluateReturnsLengthOrZero()
    {
        var (pool, supply, providers, _) = Scene(requirement: _ => "wr");
        PlaceProvider(pool, 100, 0x00501ea0u, 50, 60f, 60f, 1);
        providers.Append(100);
        // 수집 "w"로는 요구 "wr"를 못 채우므로 0과 "Wr"이 된다.
        Assert.Equal(0u, supply.Evaluate(60f, 60f, 50, 1, out string output));
        Assert.Equal("Wr", output);
        var (_, empty, _, _) = Scene(requirement: _ => "");
        Assert.Equal(1u, empty.Evaluate(60f, 60f, 50, 1, out output));
        Assert.Equal("", output);
        var (_, nulled, _, _) = Scene(requirement: _ => "\0wr");
        Assert.Equal(1u, nulled.Evaluate(60f, 60f, 50, 1, out output));
        Assert.Equal("", output);
    }

    /// <summary>비공급 가상 표는 거짓이며 표 밖은 거부하고 반경 밖은 거짓이다.</summary>
    [Fact]
    public void CoversRejectsNonProviderAndChecksRadius()
    {
        var (pool, supply, _, _) = Scene();
        PlaceProvider(pool, 100, 0x00501dd0u, 50, 60f, 60f, 1);
        Assert.False(supply.Covers(100, 59.5f, 59.5f, 0.5f));
        PlaceProvider(pool, 101, 0x12345678u, 50, 60f, 60f, 1);
        Assert.Throws<InvalidOperationException>(() => supply.Covers(101, 59.5f, 59.5f, 0.5f));
        Assert.False(supply.Covers(100, 200f, 200f, 0.5f));
        Assert.Throws<ArgumentOutOfRangeException>(() => supply.Covers(0, 59.5f, 59.5f, 0.5f));
    }

    /// <summary>가까운 순으로 다시 보관하고 앞에서부터 소비하며 진단을 보고한다.</summary>
    [Fact]
    public void TakeSourceAndSortSourcesOrderByDistance()
    {
        var (pool, supply, providers, selected) = Scene();
        PlaceProvider(pool, 100, 0x00501ea0u, 50, 60f, 60f, 1);
        PlaceProvider(pool, 101, 0x00501ea0u, 50, 70f, 60f, 1);
        providers.Append(100); providers.Append(101);
        Assert.Equal("ww", supply.Collect(65f, 60f, 50, 1));
        supply.SortSources("ww", 69f, 60f);
        Assert.Equal([101u, 100u], selected.Items().ToArray());
        Assert.Equal(101u, supply.TakeSource());
        Assert.Equal([0u, 100u], selected.Items().ToArray());
        Assert.Equal(100u, supply.TakeSource());
        Assert.Equal(0u, supply.TakeSource());
        // 유효하지 않은 공급원은 진단 뒤 건너뛴다.
        var diagnostics = new List<EnergySortDiagnostic>();
        var (_, reporting, _, reportingSelected) = Scene();
        reportingSelected.Storage[0] = 99999; reportingSelected.Count = 1;
        Assert.Throws<InvalidOperationException>(() =>
        {
            var noHook = new RawEnergySupply(pool,
                Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(),
                providers, reportingSelected, new uint[81], 256, new EnergySupplyState(),
                new EnergySupplyHooks { ProviderEntry = _ => 0u, Requirement = _ => "" });
            noHook.SortSources("w", 0f, 0f);
        });
        var hooked = new RawEnergySupply(pool,
            Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(),
            providers, reportingSelected, new uint[81], 256, new EnergySupplyState(),
            new EnergySupplyHooks { ProviderEntry = _ => 0u, Requirement = _ => "", SortDiagnostic = diagnostics.Add });
        hooked.SortSources("w", 0f, 0f);
        Assert.Equal([EnergySortDiagnostic.InvalidSource], diagnostics);
    }
}
