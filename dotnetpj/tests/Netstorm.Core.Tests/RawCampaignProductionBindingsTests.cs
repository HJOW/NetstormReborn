using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 실제 Player/내용물 공장과 같은 Deck/장부의 독립 원본 기술 표시·재계산을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>기술 표시90행·생산 재계산242행과 현재 표/모드의 공유 계약을 검사한다.</summary>
public sealed class RawCampaignProductionBindingsTests
{
    /// <summary>원본 관찰기의 불투명 Deck 주소다.</summary>
    private const uint NativeToken = 0x11100000;
    /// <summary>같은 실제 풀·생성기·Player·SP·내용물 정렬/통지·Deck을 제공하는 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>모든 실제 장치의 같은 PATCH 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, true, null, OriginalEdition.Patch1078);
        /// <summary>현재188개 TYPE와 원문 생성자 주소다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(index) }).ToArray();
        /// <summary>실제 Pop 장부의 세 공유 배열이다.</summary>
        public readonly RawTotalMadeStorage Counts = new();
        /// <summary>같은 현재 공장 목록/장부다.</summary>
        public readonly RawPostPopState Book;
        /// <summary>현재 생산 모드다.</summary>
        public readonly SquidOwnerMode Mode = new();
        /// <summary>현재9개 Player 원문이다.</summary>
        public readonly byte[] Records = new byte[9 * 180];
        /// <summary>현재 두 소유자와 PlayerKnown이다.</summary>
        public readonly PlayerWorldState State;
        /// <summary>실제 Player getter와 장부 몸체다.</summary>
        public readonly RawPlayerWorld Player;
        /// <summary>같은 풀의 실제 원문 생성기다.</summary>
        public readonly RawSquidFactory Factory;
        /// <summary>현재 내용물 검색 역할/권한이다.</summary>
        public readonly ContainedFinderState Finder = new();
        /// <summary>실제 종류/mask 검색 본체다.</summary>
        public readonly RawCarrierPreDestroy Carrier;
        /// <summary>실제 생성/정렬/Player 통지 사슬이다.</summary>
        public readonly RawContainedCreate Create;
        /// <summary>같은 Player+68의 실제 Deck이다.</summary>
        public readonly RawProductionDeck Deck;
        /// <summary>실제 할당/부착/설정 효과 순서다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>같은 금지 DWORD 표다.</summary>
        public readonly uint[] Restrictions = new uint[256];
        /// <summary>같은 실제 금지 표와 세 장부의 계산 뷰다.</summary>
        public readonly TotalMade Allowed;
        /// <summary>현재 미션 여부 입력이다.</summary>
        public bool Mission = true;
        /// <summary>현재 전체 Deck DWORD 입력이다.</summary>
        public uint Full;
        /// <summary>현재 무료 기술 역할이다.</summary>
        public uint FreeTechnology = 153;
        /// <summary>현재 신전 기본 생산 역할이다.</summary>
        public uint TempleProduct = 133;
        /// <summary>현재 무료 기술 설정 입력이다.</summary>
        public int Config;
        /// <summary>전체 동작 대신 실제 하위 장치와 원문에 제공한 TYPE/슬롯 입력만 준비한다.</summary>
        public Scene(uint owner = 1, byte fill = 167)
        {
            Types[85].Flags1 = Types[86].Flags1 = 0x04000000; Types[85].Flags2 = Types[86].Flags2 = 0x4000; Types[124].Flags2 = 0x200;
            Book = new(Counts); State = new() { TemplateOwner = owner, BattleOwner = owner, PlayerKnown = 1 };
            Put(Records, (int)owner * 180 + 0x9c, 50); Player = new(Pool, new RawPostPop(Pool, Types, Book), Records, State, new());
            Factory = new(Pool, Types); Carrier = new(Pool, Finder, new()
            {
                ValidateDamageable = (_, _) => throw new InvalidOperationException("검색 검사에서 사전 파괴 검증 불필요"),
                DamageablePre = (_, _) => throw new InvalidOperationException("검색 검사에서 사전 파괴 불필요"),
                DependentFound = () => throw new InvalidOperationException("검색 검사에서 종속 파괴 후처리 불필요"),
            });
            var sp = new RawSpStore(new GameRandom(1), () => 1); var tracking = new IntBox(); sp.BindTrackingOffset(tracking); sp.Initialize();
            var notice = new RawPlayerContentNotice(Pool, new()
            {
                CursorA = () => sp.OffsetCurrent, CursorB = () => unchecked((uint)tracking.Value), ProblemCode = new(), FallbackCode = () => 0,
                MoneyChecks = new(), BattleShown = () => State.PlayerKnown, LocalPlayer = () => State.BattleOwner,
                HasFirstSpPool = () => sp.FirstPoolCurrent is not null, FirstPermutation = () => sp.Permutation[0], GumpFormType = () => 3, ContentEvent = () => 0x02000031,
            }, new()
            {
                Money = player => BitConverter.UInt32BitsToSingle(sp.Get(player)),
                Gump = _ => throw new InvalidOperationException("검사에 없는 Gump 등록 조회"),
                Handle = (_, _, _, _) => throw new InvalidOperationException("검사에 없는 Gump 처리"),
            });
            var forms = new RawContentFormHost(Pool, Types, (parent, form, reason) => notice.Notify(parent, (uint)form, reason));
            var hooks = forms.BindCreate(Factory, new() { NotifySurface = _ => throw new InvalidOperationException("검사에 없는 표면 통지") });
            hooks.Attach = (form, parent, flags) =>
            {
                ReadOnlySpan<byte> raw = Pool.Slot(form);
                Trace.Add($"C:{parent},{Word(raw, 18)},{Word(raw, 12)},{Word(raw, 20)},{flags},{raw[30]}"); forms.Attach(form, parent, flags);
            };
            Create = new(Pool, new(), hooks);
            Deck = new(Player, raw => { raw.Fill(fill); Trace.Add("N:352"); }, _ => throw new InvalidOperationException("검사에 없는 Deck 진단"), NativeToken);
            Allowed = new(Types, Counts, Restrictions, _ => throw new InvalidOperationException("검사에 없는 금지 표 진단"));
        }
        /// <summary>현재 생산 모드/역할과 같은 실제 장치로 재계산 본체를 만든다.</summary>
        public RawCampaignProductionBindings Production(CampaignProductionEffects? effects = null) => new(Pool, Types, Book, Mode, Player, Carrier, Create, Deck, Allowed, Finder,
            new() { Mission = () => Mission, FullDeck = () => Full, FreeTechnologyType = () => FreeTechnology, TempleProduct = () => TempleProduct },
            effects ?? new() { ConfigInt = key => { Assert.Equal("getTeleportTechFree", key); Trace.Add("Q:" + key); return Config; }, ModeAssertion = () => Trace.Add("A:331") });
        /// <summary>현재 Player가 살아 있는 동안 실제 Deck을 반납한다.</summary>
        public void Dispose() => Deck.Dispose();
    }
    /// <summary>지정 폭만 읽는 원문 WORD 입력 조회다.</summary>
    private static uint Word(ReadOnlySpan<byte> raw, int at) => BinaryPrimitives.ReadUInt16LittleEndian(raw[at..]);
    /// <summary>지정 DWORD만 little endian으로 쓴다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>지정 WORD만 little endian으로 쓴다.</summary>
    private static void PutWord(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt16LittleEndian(raw[at..], unchecked((ushort)value));
    /// <summary>원본 관찰과 같은66개 항목/미정 꼬리·다섯 DWORD의 초기 입력이다.</summary>
    private static byte[] DeckInput(uint count, byte fill)
    {
        byte[] raw = new byte[352]; raw.AsSpan().Fill(fill); byte[] kinds = [0, 71, 158, 255], powers = [255, 0, 1, 128, 127];
        // 결과를 계산하지 않고 모든 물리 항목의 초기값을 그대로 공급한다.
        for (int index = 0; index < 66; index++)
        {
            raw[index * 5] = kinds[index % 4]; raw[index * 5 + 1] = (byte)(index % 7 + 1); raw[index * 5 + 2] = powers[index % 5]; raw[index * 5 + 3] = 37; raw[index * 5 + 4] = (byte)index;
        }
        Put(raw, 0x14c, count); Put(raw, 0x150, 0x12345678); Put(raw, 0x154, 0x23456789); Put(raw, 0x158, 0x3456789a); Put(raw, 0x15c, 0x456789ab); return raw;
    }
    /// <summary>원본에 제공한 실제 생성자/고정 SID/현재 슬롯 필드를 준비한다.</summary>
    private static void RefreshNodes(Scene scene, uint variant, uint level)
    {
        Assert.Equal(5, scene.Factory.Create(85, SidAllocation.Client)); Assert.Equal(6, scene.Factory.Create(124, SidAllocation.Client)); Assert.Equal(7, scene.Factory.Create(86, SidAllocation.Client));
        Assert.Equal(8, scene.Pool.Allocate(SidAllocation.Client)); Assert.Equal(9, scene.Pool.Allocate(SidAllocation.Client));
        // 종속은 실제 ContentForm 생성자를 사용하고 원본 관찰의 상태만 뒤에서 공급한다.
        for (int sid = 10; sid < 14; sid++) Assert.Equal(sid, scene.Factory.CreateContained(2));
        uint[][] nodes =
        [
            [5,85,(level << 5) | new uint[] {0,1,2,4}[variant % 4],1,10,0,0,0],
            [6,124,0,variant % 5 == 1 ? 2u : 1u,0,0,0,0], [7,86,0,variant % 5 == 2 ? 2u : 1u,13,0,0,0],
            [10,6,8,0,0,11,71,0x10], [11,6,1,0,0,12,variant % 4 == 2 ? 83u : 71u,variant % 3 != 0 ? 0x10u : 1u],
            [12,6,2,0,0,0,158,0x10], [13,6,0,0,0,0,83,0x10],
        ];
        // 실제 VT/TYPE는 보존하고 원본 관찰의 owner/head/next/kind/mask/state만 입력한다.
        foreach (uint[] input in nodes)
        {
            Span<byte> raw = scene.Pool.AllocatedBytes((int)input[0]); Assert.Equal(input[1], (uint)raw[10]); raw[34] = (byte)input[3];
            PutWord(raw, 6, input[4]); PutWord(raw, 4, input[5]);
            if (input[1] == 6) { PutWord(raw, 18, input[6]); PutWord(raw, 20, input[7]); }
            raw[11] = (byte)input[2];
        }
        scene.Book.Factories.Entries = [5, 6, 7]; scene.Book.Factories.Count = 3;
    }

    /// <summary>원본 기술 표시90행을 실제 생성·정렬·Player/SP 통지 사슬로 실행한다.</summary>
    [Fact]
    public void OriginalIndicateTechnology90()
    {
        string[][] rows = X86Fixture.Read("production-deck").Where(row => row[0].StartsWith("Indicate,", StringComparison.Ordinal)).ToArray(); Assert.Equal(90, rows.Length);
        // 각 원본 입력을 독립 실제 풀/Player로 실행한다.
        foreach (string[] row in rows)
        {
            uint[] values = row[0].Split(',').Skip(1).Select(X86Fixture.UInt).ToArray(); using var scene = new Scene(values[0], (byte)values[4]);
            scene.State.PlayerKnown = values[6]; scene.State.BattleOwner = values[7]; Put(scene.Records, (int)values[7] * 180 + 0x9c, 50);
            uint token = scene.Deck.Get(values[0]); DeckInput(values[3], (byte)values[4]).CopyTo(scene.Deck.Bytes(token).Span);
            if (values[5] == 0) Put(scene.Records, (int)values[0] * 180 + 0x68, 0);
            // 원본 Technology SID50과 기존 내용물 SID60을 실제 생성자로 확보한다.
            for (int sid = 5; sid < 50; sid++) Assert.Equal(sid, scene.Pool.Allocate(SidAllocation.Client));
            Assert.Equal(50, scene.Factory.Create(152, SidAllocation.Client));
            // 사이의 빈 번호도 원본과 같은 할당 순서를 따른다.
            for (int sid = 51; sid < 60; sid++) Assert.Equal(sid, scene.Pool.Allocate(SidAllocation.Client));
            Assert.Equal(60, scene.Factory.CreateContained(2)); Span<byte> content = scene.Pool.AllocatedBytes(60);
            PutWord(content, 18, values[3]); PutWord(content, 20, values[4]); PutWord(content, 16, 50); content[11] = 8;
            if (values[5] != 0) PutWord(scene.Pool.AllocatedBytes(50), 6, 60);
            scene.Trace.Clear(); var indicate = new RawIndicateTechnology(scene.Player, scene.Carrier, scene.Create);
            Exception? error = Record.Exception(() => indicate.Indicate(values[1]));
            Assert.Equal(values[1] >= 188, error is not null); Assert.Equal("-", row[1]);
            Assert.Equal(row[2], Convert.ToHexString(scene.Deck.Bytes(token).Span).ToLowerInvariant()); Assert.Equal(row[3], Convert.ToHexString(scene.Records).ToLowerInvariant());
            Assert.Equal(row[4], scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace));
        }
    }
    /// <summary>원문과 같은 입력으로242행의 반환·전체 Deck·효과 순서와 Pool/Player 불변을 대조한다.</summary>
    [Fact]
    public void OriginalFactoryAndRefresh242()
    {
        string[][] rows = X86Fixture.Read("production-refresh"); Assert.Equal(242, rows.Length);
        // 현재 상태와 원본 입력 Deck만 준비하고 기대값은 fixture에서 읽는다.
        foreach (string[] row in rows)
        {
            string[] cells = row[0].Split(','); uint[] values = cells.Skip(1).Select(X86Fixture.UInt).ToArray();
            using var scene = new Scene(values[0]); uint token = scene.Deck.Get(values[0]); RefreshNodes(scene, values[8], values[7]);
            Convert.FromHexString(row[1]).CopyTo(scene.Deck.Bytes(token).Span); scene.Mode.Fort = values[2] != 0; scene.Mode.Battle = values[3] != 0;
            scene.Mission = values[1] != 0; scene.Full = values[4]; scene.Config = (int)values[5]; scene.Restrictions[71] = values[6] & 1; scene.Restrictions[133] = values[6] & 2;
            scene.Trace.Clear(); byte[] records = scene.Records.ToArray(), pool = scene.Pool.Bytes.ToArray(); RawCampaignProductionBindings production = scene.Production();
            uint result = cells[0] == "Refresh" ? production.Refresh(values[0]) : unchecked((uint)production.AppendFactoryProducts(5, token));
            Assert.Equal(X86Fixture.UInt(row[2]), result); Assert.Equal(row[3], Convert.ToHexString(scene.Deck.Bytes(token).Span).ToLowerInvariant());
            Assert.Equal(row[4], scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace)); Assert.Equal(records, scene.Records); Assert.Equal(pool, scene.Pool.Bytes.ToArray());
            Assert.Equal(values[6] & 1, scene.Restrictions[71]); Assert.Equal(values[6] & 2, scene.Restrictions[133]); Assert.Same(scene.Pool, production.Pool); Assert.Same(scene.Deck, production.Deck);
        }
    }
    /// <summary>일반 공장 생산은 금지 조회 뒤에도 포함하며 신전 기본 생산만 현재 금지 표로 제외한다.</summary>
    [Fact]
    public void RestrictionsAndFactoryListAreCurrentSharedStorage()
    {
        using var scene = new Scene(); RefreshNodes(scene, 0, 0); uint token = scene.Deck.Get(1); scene.Mode.Battle = true;
        RawCampaignProductionBindings production = scene.Production(); production.RefreshCurrent(); Assert.True(production.Contains(71)); Assert.True(production.Contains(133));
        scene.Allowed.Allow(71, 0); scene.Allowed.Allow(133, 0); production.RefreshCurrent(); Assert.True(production.Contains(71)); Assert.False(production.Allowed(71)); Assert.False(production.Contains(133));
        scene.Book.Factories.Count = 0; Assert.True(production.Contains(71)); production.RefreshCurrent(); Assert.False(production.Contains(71));
        scene.Config = 1; production.RefreshCurrent(); Assert.True(production.Contains(153)); scene.State.TemplateOwner = 8; Assert.False(production.Contains(153));
        production.RefreshCurrent(); Assert.True(production.Contains(153)); Assert.Same(scene.Counts.LocalCounts, scene.Allowed.LocalCounts); Assert.Same(scene.Counts.LocalSecondaryCounts, scene.Allowed.LocalSecondaryCounts);
        Assert.Same(scene.Counts.GlobalCounts, scene.Allowed.GlobalCounts); Assert.Same(scene.Restrictions, scene.Allowed.Restrictions);
        scene.Counts.LocalCounts[71] = uint.MaxValue; Assert.Equal(uint.MaxValue, scene.Allowed.Local(71)); scene.Counts.LocalSecondaryCounts[71] = 3;
        scene.Types[71].Flags2 = 0x4000; Assert.Equal(3, scene.Allowed.BuiltForGenus(0x4000)); scene.Types[71].Flags2 = 0; Assert.Equal(0, scene.Allowed.BuiltForGenus(0x4000));
    }
    /// <summary>원문 모드 진단 효과 뒤의 현재 전체Deck DWORD가 재계산 생략을 선택한다.</summary>
    [Fact]
    public void FullDeckIsReadAfterModeDiagnostic()
    {
        using var scene = new Scene { Mission = false }; uint token = scene.Deck.Get(1); scene.Deck.Append(token, 71, 1); scene.Deck.Bytes(token).Span[2] = 126;
        byte[] before = scene.Deck.Bytes(token).ToArray(); int queries = 0, diagnostics = 0;
        var production = scene.Production(new() { ConfigInt = _ => { queries++; return 0; }, ModeAssertion = () => { diagnostics++; scene.Full = 0x80000000; } });
        production.RefreshCurrent(); Assert.Equal(1, diagnostics); Assert.Equal(0, queries); Assert.Equal(before, scene.Deck.Bytes(token).ToArray());
        scene.Mode.Battle = true; scene.Full = uint.MaxValue; production.RefreshCurrent(); Assert.Equal(0, queries);
        scene.Full = 0; production.RefreshCurrent(); Assert.Equal(1, queries); Assert.False(production.Contains(71));
    }
}
