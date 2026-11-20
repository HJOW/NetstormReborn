using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 실제 Player와 같은 Deck 저장소의 원문 바이트·진단·소유/반납 경계를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>생산 Deck의 독립 원본158입력과 현재 Player/장기 저장소/정상 반납 증빙 검사다.</summary>
public sealed class RawProductionDeckTests
{
    /// <summary>독립 원본 관찰기의 불투명 Deck 주소다.</summary>
    private const uint NativeToken = 0x11100000;
    /// <summary>같은 현재 Player와 실제 블록만 제공하는 검사 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>실제9개 Player의 같은 원문 배열이다.</summary>
        public readonly byte[] Records = new byte[9 * 180];
        /// <summary>두 소유자와 PlayerKnown의 현재 입력이다.</summary>
        public readonly PlayerWorldState State = new();
        /// <summary>실제 Player 몸체다. 선택하지 않은 생성/공간 효과는 제공하지 않는다.</summary>
        public readonly RawPlayerWorld Player;
        /// <summary>현재 할당/진단 호출 순서다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>같은 물리를 소유하는 실제 Deck 본체다.</summary>
        public readonly RawProductionDeck Deck;
        /// <summary>원본 입력만 준비하고 계산 결과는 주입하지 않는다.</summary>
        public Scene(uint owner = 1, byte pattern = 0, RawProductionDeckStorage? storage = null)
        {
            var pool = new RawSidPool(24000, true, null, OriginalEdition.Patch1078);
            OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
            State.TemplateOwner = owner;
            Player = new(pool, new RawPostPop(pool, types, new()), Records, State, new());
            Deck = new(Player, raw => { raw.Fill(pattern); Trace.Add("N:352"); },
                where => Trace.Add("A:" + AssertionLine(where)), NativeToken, storage);
        }
        /// <summary>Player가 살아 있는 동안 자기 Deck 소유권을 명시 반납한다.</summary>
        public void Dispose() => Deck.Dispose();
    }
    /// <summary>원문 진단 종류를 관측된 Deck.cpp 행 번호로 표시한다.</summary>
    private static uint AssertionLine(ProductionDeckAssertion where) => where switch
    {
        ProductionDeckAssertion.DuplicateType => 100, ProductionDeckAssertion.Capacity => 102,
        ProductionDeckAssertion.TypeTruncated => 45, ProductionDeckAssertion.ChanceTruncated => 46,
        ProductionDeckAssertion.PowerTruncated => 47, ProductionDeckAssertion.RemainingTruncated => 48,
        _ => throw new ArgumentOutOfRangeException(nameof(where)),
    };
    /// <summary>독립 관찰과 같은66개 물리 항목·두 패딩·다섯 DWORD를 입력으로 만든다.</summary>
    private static byte[] DeckInput(uint count, byte pattern)
    {
        var raw = new byte[ProductionDeckLayout.Bytes]; raw.AsSpan().Fill(pattern);
        byte[] kinds = [0, 71, 158, 255], powers = [255, 0, 1, 128, 127];
        // 개수 밖 물리 꼬리도 원문과 동일한 입력을 공급한다.
        for (int index = 0; index < ProductionDeckLayout.Capacity; index++)
        {
            int offset = index * 5; raw[offset] = kinds[index % 4]; raw[offset + 1] = (byte)(index % 7 + 1);
            raw[offset + 2] = powers[index % 5]; raw[offset + 3] = 37; raw[offset + 4] = (byte)index;
        }
        Put(raw, 0x14c, count); Put(raw, 0x150, 0x12345678); Put(raw, 0x154, 0x23456789);
        Put(raw, 0x158, 0x3456789a); Put(raw, 0x15c, 0x456789ab); return raw;
    }
    /// <summary>지정 DWORD만 little endian으로 입력한다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>바이트 전체를 원문 TSV의 소문자 hex로 비교한다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>원본248행 중 기술 표시90행을 제외한 Deck 본체158행을 기대값 재계산 없이 비교한다.</summary>
    [Theory]
    [InlineData("Initialize", 4)]
    [InlineData("Clear", 4)]
    [InlineData("Append", 25)]
    [InlineData("Activate", 25)]
    [InlineData("Compact", 4)]
    [InlineData("Contains", 84)]
    [InlineData("Get", 12)]
    public void OriginalDeck158(string mode, int expectedCount)
    {
        string[][] rows = X86Fixture.Read("production-deck").Where(row => row[0].StartsWith(mode + ',', StringComparison.Ordinal)).ToArray();
        Assert.Equal(expectedCount, rows.Length);
        // 각 원본 행을 새 실제 Player/Deck으로 실행해 이전 출력에 의존하지 않는다.
        foreach (string[] row in rows)
        {
            uint[] input = row[0].Split(',').Skip(1).Select(X86Fixture.UInt).ToArray();
            uint owner = input[0], type = input[1]; int chance = unchecked((int)input[2]);
            using var scene = new Scene(owner, unchecked((byte)input[4]));
            scene.State.BattleOwner = input[7]; scene.State.PlayerKnown = input[6]; Put(scene.Records, (int)input[7] * 180 + 0x9c, 50);
            if ((mode != "Get" && mode != "Contains") || input[5] != 0)
            {
                uint token = scene.Deck.Get(owner); Assert.Equal(NativeToken, token);
                DeckInput(input[3], unchecked((byte)input[4])).CopyTo(scene.Deck.Bytes(token).Span);
                if (input[5] == 0) Put(scene.Records, (int)owner * 180 + 0x68, 0);
            }
            scene.Trace.Clear(); string result = "-";
            switch (mode)
            {
                case "Get": result = scene.Deck.Get(owner).ToString(); break;
                case "Contains": result = scene.Deck.Contains(type) ? "1" : "0"; break;
                case "Initialize": scene.Deck.Initialize(NativeToken); break;
                case "Clear": scene.Deck.ClearEntries(NativeToken); break;
                case "Append": scene.Deck.Append(NativeToken, type, chance); break;
                case "Activate": scene.Deck.Activate(NativeToken, type, chance); break;
                case "Compact": scene.Deck.Compact(NativeToken); break;
            }
            Assert.Equal(row[1], result); Assert.Equal(row[2], Hex(scene.Deck.Bytes(NativeToken).Span));
            Assert.Equal(row[3], Hex(scene.Records)); Assert.Equal(row[4], scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace));
        }
    }

    /// <summary>현재 templateOwner·서로 다른 블록·외부 교체 포인터 보존과 반납 후 주소 전진을 확인한다.</summary>
    [Fact]
    public void CurrentPlayerAndSharedStorageOwnOneBlockPerPointer()
    {
        var storage = new RawProductionDeckStorage(NativeToken); using var scene = new Scene(storage: storage);
        uint first = scene.Deck.Get(1); scene.Deck.Append(first, 71, 1);
        scene.State.TemplateOwner = 2; Assert.False(scene.Deck.Contains(71)); uint second = scene.Deck.Get(2);
        Assert.NotEqual(first, second); scene.State.TemplateOwner = 1; Assert.True(scene.Deck.Contains(71));
        Put(scene.Records, 180 + 0x68, 0xdeadbeef); scene.Deck.Dispose();
        Assert.Equal(0xdeadbeefu, BinaryPrimitives.ReadUInt32LittleEndian(scene.Records.AsSpan(180 + 0x68)));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Records.AsSpan(360 + 0x68)));
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.Bytes(first)); Assert.Throws<ArgumentOutOfRangeException>(() => storage.Bytes(second));
        using var next = new Scene(storage: storage); Assert.Equal(second + ProductionDeckLayout.Bytes, next.Deck.Get(1));
    }

    /// <summary>실제 정리 정상 복귀와 세 필드/할당 반납 뒤에만 같은 증빙을 소비하고 모든 소비자를 닫는다.</summary>
    [Fact]
    public void ActualPlayerLinksPassClosesConsumersAndRejectsForeignOrConsumedPass()
    {
        using var scene = new Scene(); uint token = scene.Deck.Get(1);
        Put(scene.Records, 0x9c, 90); Put(scene.Records, 8, 10);
        RawProductionDeck.PlayerLinksPass pass = scene.Deck.CleanupPlayerLinksForState(() => scene.Records);
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Deck.Bytes(token)); Assert.False(scene.Deck.PlayerLinksReturnedForState);
        scene.Deck.ReturnPlayerLinksForState(pass); Assert.True(scene.Deck.PlayerLinksReturnedForState);
        Assert.Throws<InvalidOperationException>(() => scene.Deck.Get(1)); Assert.Throws<InvalidOperationException>(() => scene.Deck.Bytes(token));
        Assert.Throws<ArgumentException>(() => scene.Deck.ReturnPlayerLinksForState(pass));
        using var other = new Scene(); RawProductionDeck.PlayerLinksPass foreign = other.Deck.CleanupPlayerLinksForState(() => other.Records);
        Assert.Throws<ArgumentException>(() => scene.Deck.ReturnPlayerLinksForState(foreign));
        other.Deck.Dispose(); Assert.Throws<ObjectDisposedException>(() => other.Deck.ReturnPlayerLinksForState(foreign));
    }

    /// <summary>정리 중 소비와 실패한 정리의 완료 선언을 막고 실제 복구 후의 새 증빙만 소비한다.</summary>
    [Fact]
    public void LinksEffectsMustReturnAndFailuresPreservePrefix()
    {
        using var scene = new Scene(); RawProductionDeck.PlayerLinksPass old = scene.Deck.CleanupPlayerLinksForState(() => scene.Records);
        bool checkedActive = false;
        _ = scene.Deck.CleanupPlayerLinksForState(() =>
        {
            if (!checkedActive) { checkedActive = true; Assert.Throws<InvalidOperationException>(() => scene.Deck.ReturnPlayerLinksForState(old)); }
            return scene.Records;
        });
        Put(scene.Records, 0x9c, 123); Put(scene.Records, 8, 456); int reads = 0;
        Assert.Throws<IOException>(() => scene.Deck.CleanupPlayerLinksForState(() => ++reads == 2 ? throw new IOException("명시 입력 중단") : scene.Records));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Records.AsSpan(0x9c)));
        Assert.Equal(456u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Records.AsSpan(8)));
        Assert.False(scene.Deck.PlayerLinksReturnedForState);
        RawProductionDeck.PlayerLinksPass repaired = scene.Deck.CleanupPlayerLinksForState(() => scene.Records);
        scene.Deck.ReturnPlayerLinksForState(repaired); Assert.True(scene.Deck.PlayerLinksReturnedForState);
    }

    /// <summary>실제 블록 부재·foreign 포인터·손상 개수·할당 실패를 성공으로 숨기지 않는다.</summary>
    [Fact]
    public void StorageAndPhysicalBoundsAreExplicit()
    {
        var storage = new RawProductionDeckStorage(NativeToken); byte[] players = new byte[9 * 180];
        Assert.Throws<IOException>(() => storage.Get(players, 1, _ => throw new IOException("할당 바이트 중단")));
        Assert.Equal(NativeToken, storage.Get(players, 1, raw => raw.Fill(0xa5)));
        Put(players, 2 * 180 + 0x68, 0x12345678);
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.Get(players, 2, null));
        storage.Release(0); storage.Release(NativeToken); Assert.Throws<ArgumentOutOfRangeException>(() => storage.Release(NativeToken));
        byte[] raw = DeckInput(66, 0); var assertions = new List<ProductionDeckAssertion>();
        Assert.Throws<ArgumentOutOfRangeException>(() => RawDeckDraw.Append(raw, 77, 1, assertions.Add)); Assert.Contains(ProductionDeckAssertion.Capacity, assertions);
        Assert.Equal(66u, BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0x14c)));
        Assert.Throws<ArgumentException>(() => RawDeckDraw.Initialize(new byte[351]));
    }
}
