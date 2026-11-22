using System.Buffers.Binary;
using System.Security.Cryptography;
using Netstorm.Core.Original;

// 실제 AI 지갑과 원본 두 함수의 독립 명령 관측을 같은 전체 메모리로 비교한다.
namespace Netstorm.Core.Tests;

/// <summary>현재 base·signed 가산·호출 대여·부분 효과와 실제 Native2520관측을 검사한다.</summary>
public sealed class AiWalletTests
{
    /// <summary>한 호출에만 발급한 실제 대여의 결정적 반납 횟수다.</summary>
    private sealed class Lease : IRawRuntimeLease
    {
        /// <summary>명시 반납된 횟수이며 일반 owner와 구별한다.</summary>
        public int Returns { get; private set; }
        /// <summary>호출 말미의 단일 반납을 관측한다.</summary>
        public void Dispose() => Returns++;
    }
    /// <summary>현재 입력의 DWORD만 쓴다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[at..], value);
    /// <summary>현재 관측의 DWORD를 읽는다.</summary>
    private static uint Get(ReadOnlySpan<byte> bytes, int at) => BinaryPrimitives.ReadUInt32LittleEndian(bytes[at..]);
    /// <summary>전체 바이트 관측을 원본 자료의 소문자 SHA256과 비교한다.</summary>
    private static string Hash(ReadOnlySpan<byte> bytes) => Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant();

    /// <summary>설치·비활성·signed 비양수·NULL brain은 실제 메모리와 데모를 요구하지 않는다.</summary>
    [Fact]
    public void DeferredReadersPreserveEarlyReturnsAndFollowCurrentPlayerBase()
    {
        var memory = new RawAiBrainMemory(0x10000000, bytes => bytes.Clear()); uint a = memory.Allocate(0xb08), b = memory.Allocate(0xb08);
        byte[] first = new byte[1620], next = new byte[1620]; Put(first, 180+0xb0, a); Put(next, 180+0xb0, b); Put(memory.Bytes(a), 0x2d8, 7); Put(memory.Bytes(b), 0x2d8, 100);
        bool disabled = true, second = false; int players = 0, accesses = 0, demos = 0;
        var wallet = new RawAiWallet(() => { accesses++; return new(memory, null); }, () => { players++; return second ? next : first; }, new()
        { Disabled = () => disabled, Demo = () => { demos++; return false; } });
        wallet.Credit(1, 4); Assert.Equal(0, wallet.Balance(1)); Assert.Equal(0, players); Assert.Equal(0, accesses);
        disabled = false;
        // 원문 signed 비교가 반환하는 세 DWORD를 별도 Player 주소로 해석하지 않는다.
        foreach (uint owner in new[] { 0u, 0xffffffffu, 0x80000000u }) { wallet.Credit(owner, 4); Assert.Equal(0, wallet.Balance(owner)); }
        Assert.Equal(0, players); wallet.Credit(8, 4); Assert.Equal(0, accesses);
        wallet.Credit(1, 4); second = true; wallet.Credit(1, 5); Assert.Equal(105, wallet.Balance(1));
        Assert.Equal(11u, Get(memory.Bytes(a), 0x2d8)); Assert.Equal(4, players); Assert.Equal(3, accesses); Assert.Equal(2, demos);
    }

    /// <summary>없는 필수 장치·Player9 밖·짧은 현재뷰는 메모리/데모를 선택하기 전에 거부한다.</summary>
    [Fact]
    public void RejectsMissingReadersAndPhysicalPlayerBounds()
    {
        var memory = new RawAiBrainMemory(0x10000000, bytes => bytes.Clear()); var reads = new AiWalletReads { Disabled = () => false, Demo = () => false };
        Assert.Throws<ArgumentException>(() => new RawAiWallet(memory, new byte[1619], reads));
        Assert.Throws<ArgumentException>(() => new RawAiWallet(memory, new byte[1620], new()));
        var wallet = new RawAiWallet(() => throw new IOException("물리 관문 뒤 메모리 금지"), () => new byte[1619], reads);
        Assert.Throws<ArgumentOutOfRangeException>(() => wallet.Credit(1, 1)); Assert.Throws<ArgumentOutOfRangeException>(() => wallet.Balance(1));
        Assert.Throws<ArgumentOutOfRangeException>(() => wallet.Credit(9, 1)); Assert.Throws<ArgumentOutOfRangeException>(() => wallet.Balance(9));
    }

    /// <summary>NULL 메모리/짧은 몸체/해제 토큰 실패도 선택한 대여를 한 번 반납하며 가산하지 않는다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)]
    public void InvalidBodyReturnsSelectedLeaseWithoutWrites(int invalid)
    {
        var memory = new RawAiBrainMemory(0x10000000, bytes => bytes.Clear()); uint token = memory.Allocate(invalid == 1 ? 0x2dc : 0xb08);
        byte[] players = new byte[1620]; Put(players, 180+0xb0, token); if (invalid == 2) memory.Free(token); var lease = new Lease();
        var wallet = new RawAiWallet(() => new(invalid == 0 ? null : memory, lease), players, new() { Disabled = () => false, Demo = () => throw new IOException("몸체 실패 뒤 데모 금지") });
        if (invalid == 0) Assert.Throws<InvalidOperationException>(() => wallet.Credit(1, 5));
        else Assert.ThrowsAny<ArgumentException>(() => wallet.Credit(1, 5));
        Assert.Equal(1, lease.Returns); if (invalid == 1) Assert.Equal(0u, Get(memory.Bytes(token), 0x2d8));
    }

    /// <summary>데모 조회 실패 전 가산은 보존하고 마지막 예외 복귀에서 같은 대여를 반납한다.</summary>
    [Fact]
    public void DemoFailureKeepsPrefixAndReleasesLease()
    {
        var memory = new RawAiBrainMemory(0x10000000, bytes => bytes.Clear()); uint token = memory.Allocate(0xb08); byte[] players = new byte[1620]; Put(players, 180+0xb0, token); Put(memory.Bytes(token), 0x2d8, 7);
        var lease = new Lease(); var failure = new IOException("데모 조회 실패");
        var wallet = new RawAiWallet(() => new(memory, lease), players, new() { Disabled = () => false, Demo = () => { Assert.Equal(0, lease.Returns); Assert.Equal(8u, Get(memory.Bytes(token), 0x2d8)); throw failure; } });
        Assert.Same(failure, Assert.Throws<IOException>(() => wallet.Credit(1, 1))); Assert.Equal(1, lease.Returns); Assert.Equal(8u, Get(memory.Bytes(token), 0x2d8));
    }

    /// <summary>실제 SP 부모가 float 전액·지갑 정수 절삭·화면 갱신 순서를 지키며 legacy 지갑은 건드리지 않는다.</summary>
    [Fact]
    public void RewardCreditsActualBodyAfterSpBeforeRefresh()
    {
        var memory = new RawAiBrainMemory(0x10000000, bytes => bytes.Clear()); uint token = memory.Allocate(0xb08); byte[] players = new byte[1620]; Put(players, 180+0xb0, token); Put(memory.Bytes(token), 0x2d8, 777);
        var wallet = new RawAiWallet(memory, players, new() { Disabled = () => false, Demo = () => false });
        var random = new GameRandom(123); var store = new RawSpStore(random, () => 100); store.Set(1, BitConverter.SingleToUInt32Bits(1000));
        var state = new SquidRewardState(); state.AiWallet[1] = 9000; int refresh = 0;
        var reward = new RawSquidReward(new RawSidPool(32768, true), Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(), new(), state, store, new()
        { RefreshSp = () => { refresh++; Assert.Equal(877, wallet.Balance(1)); Assert.Equal(1100.75f, BitConverter.UInt32BitsToSingle(store.Get(1))); } });
        reward.BindPatchWallet(wallet.Credit); reward.AddSp(1, 100.75f); Assert.Equal(1, refresh); Assert.Equal(9000, state.AiWallet[1]);
        uint before = random.State(); reward.CreditWallet(1, -4); Assert.Equal(873, wallet.Balance(1)); Assert.Equal(before, random.State()); Assert.Equal(1, refresh);
    }

    /// <summary>REA 원본 Credit2400/Balance120 전체 배열 해시·반환·데모 호출 수가 이식 본문과 같다.</summary>
    [Fact]
    public void MatchesAllNativeCreditAndBalanceObservations()
    {
        string[] rows = File.ReadAllLines(Path.Combine(AppContext.BaseDirectory, "X86", "ai-wallet-x86.tsv")); Assert.Equal(2521, rows.Length);
        var memory = new RawAiBrainMemory(0x20000000, bytes => bytes.Clear()); uint first = memory.Allocate(0xb08); _ = memory.Allocate(0x10000-0xb08); uint second = memory.Allocate(0xb08); Assert.Equal(0x20010000u, second);
        byte[] players = new byte[1620]; int credits = 0, balances = 0;
        // 각 행은 원본 입력을 다시 설치하고 실제 C# 실행 뒤 전체 바이트를 비교한다.
        foreach (string row in rows.Skip(1))
        {
            string[] f = row.Split('\t'); uint disabled = uint.Parse(f[1]), owner = uint.Parse(f[2]), brain = uint.Parse(f[3]), initial = uint.Parse(f[4]), amount = uint.Parse(f[5]); bool demo = f[6] == "1", variant = f[7] == "1";
            Array.Clear(players); memory.Bytes(first).AsSpan().Fill(0xa7); memory.Bytes(second).AsSpan().Fill(0x5b); Put(memory.Bytes(first), 0x2d8, initial); Put(memory.Bytes(second), 0x2d8, 5000);
            if (owner is 1 or 8) Put(players, (int)owner*180+0xb0, brain != 0 ? first : 0);
            int calls = 0; var wallet = new RawAiWallet(memory, players, new() { Disabled = () => disabled != 0, Demo = () =>
            {
                calls++; if (variant) { Put(players, (int)owner*180+0xb0, second); Put(memory.Bytes(first), 0x2d8, initial ^ 0x80000000); } return demo;
            } });
            if (f[0] == "Credit") { credits++; wallet.Credit(owner, unchecked((int)amount)); }
            else { balances++; Assert.Equal(uint.Parse(f[8]), unchecked((uint)wallet.Balance(owner))); }
            Assert.Equal(int.Parse(f[9]), calls); Assert.Equal(f[10], Hash(players)); Assert.Equal(f[11], Hash(memory.Bytes(first))); Assert.Equal(f[12], Hash(memory.Bytes(second)));
        }
        Assert.Equal(2400, credits); Assert.Equal(120, balances);
    }
}
