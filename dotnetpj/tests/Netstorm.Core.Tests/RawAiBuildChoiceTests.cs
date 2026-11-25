// 실제 본체/현재 TYPE로 건설 선택·비용 절삭·자금 분기와 선호 중복을 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 건설 선택을 검사하는 클래스다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.
// 근거: cpppj tests/AiBuildChoiceTests.cpp의 13건 전부다.
public sealed class RawAiBuildChoiceTests
{
    /// <summary>원본 입력과 현재 상태를 읽는 명시 하위 효과만 준비한다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    private sealed class AiChoiceScene
    {
        /// <summary>같은 실제 본체/핸들이다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x4e000000u, bytes => bytes.Fill(0));

        /// <summary>같은 실제 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>현재 TYPE와 전역 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>현재 로컬 소유자다.</summary>
        public uint Local = 1;

        /// <summary>현재 확정 여부다.</summary>
        public uint Known = 1;

        /// <summary>현재 신전 생산 TYPE다.</summary>
        public uint Collector = 133;

        /// <summary>현재 다이스 TYPE다.</summary>
        public uint Dais = 165;

        /// <summary>현재 0..3 워크샾 TYPE다.</summary>
        public readonly uint[] Workshops = [86, 87, 88, 85];

        /// <summary>현재 신전 3종이다.</summary>
        public readonly uint[] Temples = [123, 124, 125];

        /// <summary>현재 포획 여부다.</summary>
        public bool Captured;

        /// <summary>현재 사망 여부다.</summary>
        public bool Dead;

        /// <summary>현재 구매 가능 여부다.</summary>
        public bool Afford = true;

        /// <summary>원격 지갑 입력이다.</summary>
        public int Remote = 999;

        /// <summary>분산 SP 입력이다.</summary>
        public float Stored = 999.9f;

        /// <summary>선택 효과 장치다.</summary>
        public AiBuildChoiceHooks Hooks = new();

        /// <summary>실제 호출 순서다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>기본은 현재 신전/자금 600/수집자 2개분 여유/부족한 sun 워크샾이다.</summary>
        public AiChoiceScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Types[133].Cost = 100.9f;
            Types[165].Cost = 200;
            Types[123].Cost = 300.9f;
            Types[124].Cost = 400;
            Types[125].Cost = 500;
            Types[86].Cost = 50;
            Field(0x264, 2);
            Field(0x294, 1);
            Field(0x2d8, 600);
            Field(0x234, 2);
            Field(0x394, 86);
            Preferences([0]);
            Hooks.Captured = owner => { Trace.Add("captured:" + owner); return Captured; };
            Hooks.Dead = owner => { Trace.Add("dead:" + owner); return Dead; };
            Hooks.CanAfford = (brain, type) => { Assert.Equal(Token, brain); Trace.Add("afford:" + type); return Afford; };
            Hooks.Wallet = owner => { Trace.Add("wallet:" + owner); return Remote; };
            Hooks.StoredSp = owner => { Trace.Add("sp:" + owner); return Stored; };
            Hooks.Assertion = (text, line) =>
            {
                Assert.Equal(line == 0x1a ? "theIndex>=0 && theIndex<=(MAX_THEMES)" : "0", text);
                Trace.Add("assert:" + line);
            };
        }

        /// <summary>현재 본체 DWORD만 입력한다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Memory.Bytes(Token).AsSpan(at, 4), value);

        /// <summary>현재 TYPE별 실제 수량을 입력한다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
        /// <param name="type">쓸 TYPE 번호다.</param>
        /// <param name="value">쓸 수량이다.</param>
        public void Count(uint type, uint value) => Field(0x6d4 + (int)type * 4, value);

        /// <summary>같은 본체의 선호 순서를 입력한다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
        /// <param name="values">쓸 선호 순서다.</param>
        public void Preferences(uint[] values)
        {
            // 입력한 선호 순서만 쓰는 반복문이다.
            for (int index = 0; index < values.Length; index++)
            {
                Field(0x398 + index * 4, values[index]);
            }

            Field(0x3b0, (uint)values.Length);
        }

        /// <summary>현재 전역은 참조/조회기로 빌리며 TYPE 값을 생성시 복제하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
        /// <returns>검사할 건설 선택 몸체다.</returns>
        public RawAiBuildChoice Make() => new(Memory, Types, new AiBuildChoiceGlobals
        {
            LocalOwner = () => Local,
            PlayerKnown = () => Known,
            CollectorType = () => Collector,
            DaisType = () => Dais,
            Workshops = Workshops,
            Temple = index => { Trace.Add("temple:" + index); return Temples[index]; },
        }, Hooks);
    }

    /// <summary>수집자 cost는 곱하기 전에 절삭하며 하위 DWORD 곱 감김과 현재 전역 TYPE를 사용한다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CollectorReserveTruncationWrapAndCurrentType()
    {
        var input = new AiChoiceScene();
        input.Field(0x2d8, 200);
        Assert.True(input.Make().ReserveForCollectors(input.Token));
        input.Field(0x2d8, 199);
        Assert.False(input.Make().ReserveForCollectors(input.Token));
        input.Types[133].Cost = 0.9f;
        input.Field(0x234, 1000);
        input.Field(0x2d8, 0);
        Assert.True(input.Make().ReserveForCollectors(input.Token));
        input.Types[133].Cost = 2147483648.0f;
        input.Field(0x234, 2);
        Assert.True(input.Make().ReserveForCollectors(input.Token));
        var choice = input.Make();
        input.Collector = 134;
        input.Types[134].Cost = 20.9f;
        input.Field(0x2d8, 40);
        Assert.True(choice.ReserveForCollectors(input.Token));
    }

    /// <summary>로컬 확정 SP는 현재 float 절삭/미확정은 0이며 원격은 signed 실제 지갑을 쓴다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void EnemyLocalSpRemoteWalletAndUnknownPlayer()
    {
        var input = new AiChoiceScene();
        Assert.True(input.Make().EnemyLowFunds(input.Token));
        Assert.Equal(["wallet:2"], input.Trace);
        input.Remote = 1000;
        Assert.False(input.Make().EnemyLowFunds(input.Token));
        input.Field(0x264, 1);
        input.Trace.Clear();
        Assert.True(input.Make().EnemyLowFunds(input.Token));
        Assert.Equal(["sp:1"], input.Trace);
        input.Stored = 1000.9f;
        Assert.False(input.Make().EnemyLowFunds(input.Token));
        input.Stored = float.NaN;
        Assert.True(input.Make().EnemyLowFunds(input.Token));
        input.Known = 0;
        input.Trace.Clear();
        input.Hooks.StoredSp = null;
        input.Hooks.Wallet = null;
        Assert.True(input.Make().EnemyLowFunds(input.Token));
        Assert.Empty(input.Trace);
    }

    /// <summary>신전이 있으면 적 자금 관문 없이 포획 다이스를 선택하고 뒤의 잘못된 후보 TYPE를 읽지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void DaisPrioritySkipsLaterFundsAndCandidate()
    {
        var input = new AiChoiceScene();
        input.Field(0x248, 0x200);
        input.Field(0x394, 200);
        input.Captured = true;
        input.Hooks.Wallet = null;
        input.Hooks.StoredSp = null;
        Assert.Equal(165u, input.Make().Select(input.Token));
        Assert.Equal(["captured:2", "dead:2", "afford:165"], input.Trace);
    }

    /// <summary>각 사제/비용 효과 뒤 현재 적과 다이스를 다시 읽으며 이미 확인한 관문은 반복하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void DaisEffectsReadCurrentEnemyAndReturnType()
    {
        var input = new AiChoiceScene();
        input.Field(0x248, 0x200);
        input.Field(0x294, 0);
        input.Hooks.Captured = owner => { Assert.Equal(2u, owner); input.Trace.Add("captured"); input.Field(0x264, 3); input.Field(0x2a4, 1); return true; };
        input.Hooks.Dead = owner => { Assert.Equal(3u, owner); input.Trace.Add("dead"); return false; };
        input.Hooks.CanAfford = (brain, type) => { Assert.Equal(input.Token, brain); Assert.Equal(165u, type); input.Dais = 166; input.Trace.Add("afford"); return true; };
        input.Hooks.Wallet = owner => { Assert.Equal(3u, owner); input.Dais = 167; input.Trace.Add("wallet"); return 999; };
        Assert.Equal(167u, input.Make().Select(input.Token));
        Assert.Equal(["captured", "dead", "afford", "wallet"], input.Trace);
    }

    /// <summary>이미 다이스가 있거나 포획/생존 관문 탈락이면 후속 효과를 선택하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void DaisShortCircuitAndFallback()
    {
        var input = new AiChoiceScene();
        input.Field(0x248, 0x200);
        input.Field(0x2a4, 1);
        input.Hooks.Captured = null;
        Assert.Equal(86u, input.Make().Select(input.Token));
        Assert.Empty(input.Trace);
        input.Field(0x2a4, 0);
        input.Hooks.Captured = _ => { input.Trace.Add("captured"); return false; };
        input.Hooks.Dead = null;
        Assert.Equal(86u, input.Make().Select(input.Token));
        Assert.Equal(["captured"], input.Trace);
        input.Trace.Clear();
        input.Captured = true;
        input.Hooks.Captured = _ => { input.Trace.Add("captured"); return true; };
        input.Hooks.Dead = _ => { input.Trace.Add("dead"); return true; };
        input.Hooks.CanAfford = null;
        Assert.Equal(86u, input.Make().Select(input.Token));
        Assert.Equal(["captured", "dead"], input.Trace);
    }

    /// <summary>신전 없는 다이스는 적 자금 1000 미만이 필요하며 실패하면 현재 원소 신전으로 간다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void DaisFundsFallbackToTempleAndCostBoundary()
    {
        var input = new AiChoiceScene();
        input.Field(0x248, 0x200);
        input.Field(0x294, 0);
        input.Captured = true;
        Assert.Equal(165u, input.Make().Select(input.Token));
        input.Remote = 1000;
        input.Trace.Clear();
        Assert.Equal(123u, input.Make().Select(input.Token));
        Assert.Equal("temple:0", input.Trace[^1]);
        input.Field(0x248, 0);
        input.Field(0x2d8, 300);
        Assert.Equal(123u, input.Make().Select(input.Token));
        input.Field(0x2d8, 299);
        Assert.Equal(0u, input.Make().Select(input.Token));
    }

    /// <summary>실제 getter의 0..3/4·5/진단과 signed %5를 구별하며 5를 별도 신전으로 해석하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentThemeMappingsAndSignedRemainder()
    {
        var input = new AiChoiceScene();
        var choice = input.Make();
        Assert.Equal(86u, choice.WorkshopType(0));
        Assert.Equal(0u, choice.WorkshopType(4));
        Assert.Equal(0u, choice.WorkshopType(5));
        Assert.Equal(0u, choice.WorkshopType(-1));
        Assert.Equal("assert:26", input.Trace[^1]);
        input.Workshops[0] = 87;
        Assert.Equal(87u, choice.WorkshopType(0));
        input.Temples[0] = 126;
        Assert.Equal(126u, choice.TempleType(5));
        Assert.Equal(126u, choice.TempleType(-5));
        Assert.Equal(124u, choice.TempleType(6));
        Assert.Equal(125u, choice.TempleType(7));
        Assert.Equal(0u, choice.TempleType(8));
        Assert.Equal("assert:71", input.Trace[^1]);
        Assert.Equal(0u, choice.TempleType(-1));
        Assert.Equal("assert:71", input.Trace[^1]);
    }

    /// <summary>theme 3의 무신전은 신전 getter를 건너뛰고 workshop 후보 0은 비용/수집자 TYPE도 읽지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CommonThemeAndNullCandidateGates()
    {
        var input = new AiChoiceScene();
        input.Field(0x294, 0);
        input.Field(0x390, 3);
        input.Field(0x394, 0);
        input.Collector = 200;
        Assert.Equal(0u, input.Make().Select(input.Token));
        Assert.Empty(input.Trace);
        input.Field(0x294, 1);
        input.Field(0x394, 86);
        input.Field(0x2d8, 49);
        Assert.Equal(0u, input.Make().Select(input.Token));
        input.Field(0x2d8, 50);
        Assert.ThrowsAny<Exception>(() => input.Make().Select(input.Token));
    }

    /// <summary>현재 지갑이 각각의 비용 관문을 통과하면 되며 건설 비용을 여유 비용에 추가하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void WorkshopCostAndReserveAreSeparateGates()
    {
        var input = new AiChoiceScene();
        input.Types[86].Cost = 150;
        input.Field(0x2d8, 200);
        Assert.Equal(86u, input.Make().Select(input.Token));
        input.Field(0x2d8, 199);
        Assert.Equal(0u, input.Make().Select(input.Token));
    }

    /// <summary>중복 선호마다 같은 지역 복사 수량을 감소시키고 실제 본체 수량은 바꾸지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void DuplicatePreferencesAndReadOnlyCounts()
    {
        var input = new AiChoiceScene();
        input.Count(86, 1);
        input.Count(87, 1);
        input.Preferences([0, 1, 0]);
        byte[] raw = input.Memory.Bytes(input.Token);
        byte[] before = (byte[])raw.Clone();
        Assert.Equal(86u, input.Make().Select(input.Token));
        Assert.True(before.SequenceEqual(raw));
        input.Preferences([0, 1]);
        Assert.Equal(0u, input.Make().Select(input.Token));
        input.Preferences([1, 2]);
        Assert.Equal(88u, input.Make().Select(input.Token));
    }

    /// <summary>다섯번째 수량은 TYPE 0이며 음수 감김도 원본 DWORD 감소 뒤 signed 판정을 따른다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void FifthThemeAndSignedCounterWrap()
    {
        var input = new AiChoiceScene();
        input.Preferences([4, 0]);
        Assert.Equal(0u, input.Make().Select(input.Token));
        input.Count(0, 1);
        Assert.Equal(86u, input.Make().Select(input.Token));
        input.Preferences([0]);
        input.Count(86, 0x80000000u);
        Assert.Equal(0u, input.Make().Select(input.Token));
        input.Count(86, 0xffffffffu);
        Assert.Equal(86u, input.Make().Select(input.Token));
    }

    /// <summary>선호 count 0이어도 현재 다섯 수량을 먼저 읽으며 손상된 임시 인덱스는 가짜 TYPE로 보정하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CountCopyOrderAndPreferenceBounds()
    {
        var input = new AiChoiceScene();
        input.Preferences([]);
        input.Workshops[0] = 0xffffffffu;
        Assert.ThrowsAny<Exception>(() => input.Make().Select(input.Token));
        input.Workshops[0] = 86;
        input.Preferences([5]);
        Assert.ThrowsAny<Exception>(() => input.Make().Select(input.Token));
    }

    /// <summary>선택된 다이스 자금 하위 누락은 이미 호출한 사제/비용 효과를 보존한다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectedMissingFundsKeepsPrefix()
    {
        var input = new AiChoiceScene();
        input.Field(0x248, 0x200);
        input.Field(0x294, 0);
        input.Captured = true;
        input.Hooks.Wallet = null;
        Assert.ThrowsAny<Exception>(() => input.Make().Select(input.Token));
        Assert.Equal(["captured:2", "dead:2", "afford:165"], input.Trace);
    }
}
