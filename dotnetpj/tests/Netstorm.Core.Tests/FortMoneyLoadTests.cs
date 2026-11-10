using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort Money 로더(<see cref="RawFortMoneyLoad"/>)를 원본 x86 실행 기대값으로 대조한다.
/// cpppj <c>FortMoneyLoadTests</c>의 Money 전수 재생 2개와 선택·진단 직접 1개를 옮겼다.
/// 실제 월드 장치를 쓰는 공유 검사는 <c>RawSurfaceWorld</c>가 필요한 F3으로 넘긴다. (F2a, 2026-11-02)
/// </summary>
public sealed class FortMoneyLoadTests
{
    /// <summary>실제 SP·별도 CD 전역·9개 Player raw 배열·깊이와 모든 효과의 입력 저장소다.</summary>
    private sealed class MoneyScene
    {
        /// <summary>원본 Player stride·SP 배치를 선택하는 판본이다.</summary>
        public OriginalEdition Edition { get; }

        /// <summary>기존 SP·모드의 공유 상태다.</summary>
        public SquidRewardState Reward { get; } = new();

        /// <summary>현재 localOwner가 있는 원본 장부다.</summary>
        public RawPostPopState Players { get; } = new();

        /// <summary>현재 9개 Player의 실제 stride 바이트 배열이다.</summary>
        public byte[] Records { get; }

        /// <summary>같은 로딩 깊이 DWORD다.</summary>
        public uint Depth;

        /// <summary>현재 guard·client·시작 SP·99회 검사 전역이다.</summary>
        public FortMoneyState State { get; } = new();

        /// <summary>분산 SP가 실제로 소비하는 게임 난수다.</summary>
        public NetstormRandom Random { get; }

        /// <summary>현재 입력의 lazy·초기화 분산 SP 저장소다.</summary>
        public RawSpStore Store { get; }

        /// <summary>실제 assert·SP 표시의 순서다.</summary>
        public List<string> Events { get; } = [];

        /// <summary>동일 월드의 시작·끝 깊이 전달을 확인한다.</summary>
        public List<uint> Depths { get; } = [];

        /// <summary>원본 assert 보고 횟수다.</summary>
        public int Assertions;

        /// <summary>wrapper·패치 지정·스트림 본문 중 선택한 원본 진입점이다.</summary>
        public uint Kind;

        /// <summary>패치 직접 지정의 signed 입력이다.</summary>
        public int Owner;

        /// <summary>패치 직접 지정의 signed 금액이다.</summary>
        public int Amount;

        /// <summary>현재 원본 pos/end DWORD다.</summary>
        public FortMoneyCursor Stream { get; } = new();

        /// <summary>위 공유 자료만 참조하는 생산 Money 모듈이다.</summary>
        public RawFortMoneyLoad Money { get; }

        /// <summary>
        /// 독립 입력의 동일 seed·fill·raw 모드를 구성한다. 반환 기대값은 계산하지 않는다.
        /// </summary>
        /// <param name="edition">선택한 판본이다.</param>
        /// <param name="row">fixture 행이다.</param>
        public MoneyScene(OriginalEdition edition, string[] row)
        {
            Edition = edition;
            uint stride = edition == OriginalEdition.Patch1078 ? 0xb4u : 0xacu;
            Records = new byte[9 * stride];
            string[] values = row[1].Split(',');
            Assert.Equal(19, values.Length);
            Kind = uint.Parse(values[0], CultureInfo.InvariantCulture);
            Reward.FixedSpA = values[1] != "0";
            Reward.HalfHealthMode = values[2] != "0";
            State.EconomyBlocked = values[3] != "0";
            State.ClientActive = values[4] != "0";
            Reward.FixedSpB = values[5] != "0";
            Players.LocalOwner = (byte)uint.Parse(values[6], CultureInfo.InvariantCulture);
            State.Difficulty = (byte)uint.Parse(values[7], CultureInfo.InvariantCulture);
            State.CdStartingSp = int.Parse(values[8], CultureInfo.InvariantCulture);
            Depth = uint.Parse(values[10], CultureInfo.InvariantCulture);
            Stream.Set(uint.Parse(values[11], CultureInfo.InvariantCulture), uint.Parse(values[12], CultureInfo.InvariantCulture));
            State.TemplatePresent = values[13] != "0";
            State.SpCheckCount = 17;
            Owner = unchecked((int)uint.Parse(values[17], CultureInfo.InvariantCulture));
            Amount = unchecked((int)uint.Parse(values[18], CultureInfo.InvariantCulture));
            uint mask = uint.Parse(values[9], CultureInfo.InvariantCulture);
            // 9개 Player의 존재 여부는 추정하지 않고 원본 flags BYTE 입력만 쓴다
            for (int index = 0; index < 9; index++)
            {
                Records[index * stride + 0x50] = (mask & (1u << index)) != 0 ? (byte)0x20 : (byte)0;
            }
            Reward.CdLocalSp = 123.5f;
            Random = new NetstormRandom();
            uint seed = uint.Parse(values[14], CultureInfo.InvariantCulture);
            uint fill = uint.Parse(values[15], CultureInfo.InvariantCulture);
            Store = new RawSpStore(Random, () => seed, fill);
            if (edition == OriginalEdition.Patch1078 && values[16] != "0")
            {
                Store.Initialize();
            }
            List<string> events = Events;
            List<uint> depths = Depths;
            Money = new RawFortMoneyLoad(edition, Reward, Players, Records, () => Depth, value => Depth = value, State,
                edition == OriginalEdition.Patch1078 ? Store : null,
                new FortMoneyHooks
                {
                    Assertion = code =>
                    {
                        Assertions++;
                        events.Add(code == FortMoneyAssertion.MissingTemplate ? "A:T" : "A:P");
                    },
                    LoadingDepthChanged = value => depths.Add(value),
                    RefreshSp = () => events.Add("U"),
                });
        }
    }

    /// <summary>
    /// fixture 행들을 같은 진입점으로 실행해 전체 물리 SP·진단·깊이를 대조한다.
    /// </summary>
    /// <param name="editionName">판본 이름(originals·originalCD).</param>
    private static void ReplayMoney(string editionName)
    {
        OriginalEdition edition = editionName == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        string[][] rows = X86Fixture.Read("fortmoney", editionName);
        int count = 0;
        // 각 원본 입력을 생산 모듈의 동일 진입점으로 실행한다
        foreach (string[] row in rows)
        {
            Assert.Equal(12, row.Length);
            MoneyScene scene = new(edition, row);
            byte[] before = (byte[])scene.Records.Clone();
            if (scene.Kind == 1)
            {
                scene.Money.SetPlayerSp(scene.Owner, scene.Amount);
            }
            else if (scene.Kind == 2)
            {
                scene.Money.ReadMoney(scene.Stream);
            }
            else
            {
                scene.Money.Load(scene.Stream);
            }
            string values = "-";
            string pools = "-";
            if (edition == OriginalEdition.Patch1078 && scene.Store.Initialized)
            {
                List<string> got = [];
                // 각 owner의 실제 SP Get을 읽으며 이미 초기화된 저장소의 난수는 소비하지 않는다
                for (uint owner = 0; owner < 9; owner++)
                {
                    got.Add(scene.Store.Get(owner).ToString(CultureInfo.InvariantCulture));
                }
                values = string.Join(",", got);
                // 원본 288 DWORD 순서대로 현재 전체 물리 배열을 읽는다
                byte[] raw = new byte[32 * 9 * 4];
                for (int index = 0; index < scene.Store.Pools.Length; index++)
                {
                    BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(index * 4), scene.Store.Pools[index]);
                }
                pools = Convert.ToHexString(raw).ToLowerInvariant();
            }
            else if (edition == OriginalEdition.Cd1072)
            {
                values = BitConverter.SingleToUInt32Bits(scene.Reward.CdLocalSp).ToString(CultureInfo.InvariantCulture);
            }
            // 실제 효과 목록은 원본의 순서와 함께 비교한다
            string events = scene.Events.Count == 0 ? "-" : string.Join(";", scene.Events);
            bool same = scene.Depth == uint.Parse(row[2], CultureInfo.InvariantCulture)
                && scene.Stream.Position == uint.Parse(row[3], CultureInfo.InvariantCulture)
                && scene.State.SpCheckCount == uint.Parse(row[4], CultureInfo.InvariantCulture)
                && scene.Store.Initialized == (row[5] != "0")
                && scene.Store.Offset == uint.Parse(row[6], CultureInfo.InvariantCulture)
                && scene.Random.State == uint.Parse(row[7], CultureInfo.InvariantCulture)
                && values == row[8] && pools == row[9] && events == row[10]
                && scene.Assertions == int.Parse(row[11], CultureInfo.InvariantCulture);
            Assert.True(same);
            Assert.Equal(before, scene.Records);
            count++;
        }
        Assert.Equal(edition == OriginalEdition.Patch1078 ? 244 : 164, count);
    }

    /// <summary>패치 wrapper·8플레이어 순서·직접 SP 지정·분산 배열·공유 난수를 독립 입력과 전수 대조한다.</summary>
    [Fact]
    public void ReplaysPatchNative() => ReplayMoney("originals");

    /// <summary>CD 저장 4바이트 소비·시작 SP·로컬 flags 정책과 별도 float 전역을 전수 대조한다.</summary>
    [Fact]
    public void ReplaysCdNative() => ReplayMoney("originalCD");

    /// <summary>
    /// 진단에서 현재 섹션을 바꾸면 새 getter는 증가한 깊이에서 변경된 cursor를 소비하고 실패 때 깊이를 복구한다.
    /// cpppj <c>FortMoneyLoad_SelectsAfterDepthAndTemplateDiagnostic</c>을 옮겼다.
    /// </summary>
    [Fact]
    public void SelectsAfterDepthAndTemplateDiagnostic()
    {
        SquidRewardState reward = new() { FixedSpA = true };
        RawPostPopState players = new() { LocalOwner = 1 };
        FortMoneyState state = new() { TemplatePresent = false };
        NetstormRandom random = new(12345);
        RawSpStore store = new(random, () => 12345u);
        byte[] records = new byte[9 * 0xb4];
        uint depth = 3;
        FortMoneyCursor first = new() { Position = 17, End = 4 };
        FortMoneyCursor second = new() { Position = 19, End = 4 };
        FortMoneyCursor? current = first;
        List<string> effects = [];
        RawFortMoneyLoad money = new(OriginalEdition.Patch1078, reward, players, records, () => depth, value => depth = value, state, store,
            new FortMoneyHooks
            {
                Assertion = reason =>
                {
                    Assert.Equal(FortMoneyAssertion.MissingTemplate, reason);
                    Assert.Equal(4u, depth);
                    effects.Add("A");
                    current = second;
                    state.TemplatePresent = true;
                },
                LoadingDepthChanged = value =>
                {
                    Assert.Equal(depth, value);
                    effects.Add("L" + value.ToString(CultureInfo.InvariantCulture));
                },
                RefreshSp = () => throw new InvalidOperationException("고정 Fort SP의 예상 밖 표시"),
            });
        money.LoadSelected(() =>
        {
            Assert.Equal(4u, depth);
            Assert.Same(second, current);
            effects.Add("S4");
            current!.Position = 0;
            return current;
        });
        Assert.Equal(["L4", "A", "S4", "L3"], effects);
        Assert.Equal(17u, first.Position);
        Assert.Equal(4u, second.Position);
        Assert.Equal(3u, depth);
        Assert.Equal(BitConverter.SingleToUInt32Bits(194162.0f), store.Get(1));
        effects.Clear();
        Assert.ThrowsAny<Exception>(() => money.LoadSelected(() =>
        {
            Assert.Equal(4u, depth);
            effects.Add("SF");
            throw new InvalidOperationException("선택 파일 실패 입력");
        }));
        Assert.Equal(["L4", "SF", "L3"], effects);
        Assert.Equal(3u, depth);
        effects.Clear();
        Assert.ThrowsAny<Exception>(() => money.LoadSelected(null!));
        Assert.Empty(effects);
        Assert.Equal(3u, depth);
    }

    /// <summary>
    /// 원본 범위를 벗어난 difficulty·owner 0 입력의 계약을 직접 고정한다.
    /// difficulty 3은 배열 범위를 벗어나 거부하고, owner 0 조회는 저장소를 읽지 않고 0을 돌려준다.
    /// </summary>
    [Fact]
    public void DifficultyAndOwnerZeroBoundaries()
    {
        SquidRewardState reward = new() { HalfHealthMode = true };
        RawPostPopState players = new() { LocalOwner = 1 };
        FortMoneyState state = new();
        NetstormRandom random = new(42);
        RawSpStore store = new(random, () => 42u, 0xffffffffu);
        byte[] records = new byte[9 * 0xb4];
        uint depth = 3;
        List<string> effects = [];
        RawFortMoneyLoad money = new(OriginalEdition.Patch1078, reward, players, records, () => depth, value => depth = value, state, store,
            new FortMoneyHooks
            {
                Assertion = _ => effects.Add("A"),
                LoadingDepthChanged = value => effects.Add("L" + value.ToString(CultureInfo.InvariantCulture)),
                RefreshSp = () => effects.Add("U"),
            });
        // difficulty 3은 원본 표(0..2) 밖이라 거부한다
        state.Difficulty = 3;
        FortMoneyCursor stream = new() { Position = 0, End = 4 };
        Assert.Throws<ArgumentOutOfRangeException>(() => money.ReadMoney(stream));
        // owner 0 조회는 저장소를 읽지 않고 0을 돌려준다(저장소는 1 비트로 채워져 있다)
        PlayerSpSetReads reads = new() { ClientActive = () => 0u, LocalOwner = () => 1u, Editor = () => 0u };
        Assert.Equal(0.0f, RawFortMoneyLoad.ReadPlayerSpAmount(0, store, reads));
    }

    /// <summary>
    /// 실제 월드의 장부와 같은 저장소·난수를 참조해 현재 소유자·모드가 즉시 반영되는지 검증한다.
    /// cpppj <c>FortMoneyLoad_SharesCurrentWorldOwnerStoreAndRandom</c>을 옮겼다. F3 연기 검사였다.
    /// 2026-11-10 F3 미뤄 둔 전수 재생(0.300)에서 추가됐다.
    /// </summary>
    [Fact]
    public void SharesCurrentWorldOwnerStoreAndRandom()
    {
        // Money가 읽지 않는 월드 형식의 타입표이며 실제 자산 대조로 세지 않는다.
        List<OriginalTypeRecord> types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        List<RiftTypeFrames> frames = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new RiftTypeFrames([])).ToList();
        List<SquidDisplayShape> shapes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, storage.Links.IslandType, storage, frames);
        RawWorldCore core = world.Core();
        var reward = new SquidRewardState();
        var state = new FortMoneyState { ClientActive = true };
        var random = new NetstormRandom();
        var store = new RawSpStore(random, () => 42u);
        byte[] records = new byte[9 * 0xb4];
        world.SetLoadingDepth(3u);
        int refreshes = 0;
        core.Bookkeeping.LocalOwner = 1;
        var money = new RawFortMoneyLoad(OriginalEdition.Patch1078, reward, core.Bookkeeping, records,
            world.LoadingDepth, world.SetLoadingDepth, state, store,
            new FortMoneyHooks
            {
                Assertion = _ => throw new InvalidOperationException("정상 Money 소비자의 진단"),
                LoadingDepthChanged = value => world.SetLoadingDepth(value),
                RefreshSp = () => refreshes++,
            });
        money.SetPlayerSp(1, 7000);
        Assert.Equal(BitConverter.SingleToUInt32Bits(7000f), store.Get(1));
        Assert.Equal(1, refreshes);
        // 같은 장부의 현재 소유자를 바꾸면 다음 지정에 즉시 반영된다.
        core.Bookkeeping.LocalOwner = 2;
        reward.FixedSpB = true;
        money.SetPlayerSp(2, 3000);
        Assert.Equal(BitConverter.SingleToUInt32Bits(194162f), store.Get(2));
        Assert.Equal(2, refreshes);
        Assert.NotEqual(0u, random.State);
        Assert.Equal(3u, world.LoadingDepth());
    }
}
