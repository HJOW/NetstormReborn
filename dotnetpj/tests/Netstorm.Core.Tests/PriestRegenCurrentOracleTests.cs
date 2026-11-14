using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 현재 HP·사제TYPE·SP 주소와 원문 getter 순서를 실제 부품에 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>현재 회복의 독립 원본과 장기 공유 상태 계약 검사다.</summary>
public sealed class PriestRegenCurrentOracleTests
{
    /// <summary>최대HP의 실제 저장소와 회복/공간 경계를 같은 풀에 조립한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 합성 입력 슬롯 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>실제 공유 게임 난수다.</summary>
        public readonly GameRandom Random = new(1);
        /// <summary>현재 HP getter가 읽을 타입 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord { MaxHitPoints = 200, Flags1 = 0x10 }).ToArray();
        /// <summary>회복이 읽고 쓰는 실제 상태다.</summary>
        public readonly PriestRegenState State = new() { NeutralBlocked = true };
        /// <summary>독립 권한·회복 허용 모드다.</summary>
        public readonly PriestHitPointMode Mode = new() { Authority = false };
        /// <summary>초기화 전 현재 주소도 조회할 실제 SP 저장소다.</summary>
        public readonly RawSpStore Store;
        /// <summary>실제 보상 HP getter다.</summary>
        public readonly RawSquidReward Hp;
        /// <summary>실제 사제 HP/공간 경계다.</summary>
        public readonly RawPriestState Priest;
        /// <summary>실제 입력 사제 번호다.</summary>
        public readonly int Sid;
        /// <summary>도달한 외부 효과만 기록한다.</summary>
        public readonly List<string> Events = [];
        /// <summary>실제 client 할당 뒤 원문 사제 raw를 준비한다.</summary>
        public Scene()
        {
            Store = new(Random, () => 1); Hp = new(Pool, Types, new(), new(), Store);
            Priest = new(Pool, Hp, Types, new byte[65536]); Priest.BindCurrentTypes(Types);
            Sid = Pool.Allocate(SidAllocation.Client); Span<byte> raw = Pool.RawSlot(Sid); raw.Clear();
            BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x50f210); raw[10] = 158;
            BinaryPrimitives.WriteInt32LittleEndian(raw[26..], 200);
        }
        /// <summary>순수 효과 기록이며 SP 회전은 개별 검사에서 명시 연결한다.</summary>
        public PriestRegenHooks Hooks() => new()
        { Refresh = _ => Events.Add("V"), Damageable = _ => Events.Add("D"), Shape = _ => 54,
            Occupied = (_, _) => false, NeutralAction = _ => Events.Add("N"), PatchAudit = () => { }, Measurement = () => 0 };
        /// <summary>현재 훅을 값 복사하는 실제 회복 처리기를 만든다.</summary>
        public RawPriestRegen Body(PriestRegenHooks hooks) => new(Pool, Hp, Priest, Mode,
            new() { Unpop = (_, _) => Events.Add("U"), Repop = (_, _) => Events.Add("R") }, Random, State, hooks);
    }

    /// <summary>양판본864입력의 반환·getter trace·자식 변경 후 HP/번호를 독립 원본과 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    public void HitPointGetterOrderMatchesOriginal(string name, OriginalEdition edition)
    {
        string[][] rows = X86Fixture.Read("priest-regen-hit-points-current", name); Assert.Equal(432, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            int[] input = row[1].Split(',').Select(X86Fixture.Int).ToArray(); bool patch = edition == OriginalEdition.Patch1078;
            int hp = patch ? input[0] : unchecked((short)input[0]); uint number = 74; int actorCalls = 0, typeCalls = 0;
            var trace = new List<string>();
            // getter의 명시 복귀 변경 입력만 구성하고 기대 반환/trace는 계산하지 않는다.
            void Mutate(bool actor, int count)
            {
                if (input[6] == 0) return;
                if (input[6] == 1) { hp = actor ? 90 - count * 17 : -13 + count * 40; number = (uint)((actor ? 80 : 90) + count); }
                else { hp = actor ? (patch ? unchecked(0x7ffffff0 + count) : 32767 - count * 100)
                        : (patch ? unchecked(int.MinValue + count) : -32768 + count); number = (uint)((actor ? 110 : 120) + count); }
                if (!patch) hp = unchecked((short)hp);
            }
            int? value = RawPriestRegenHitPoints.Read(edition, new()
            {
                Current = () => { trace.Add($"H{hp}"); return hp; }, PriestType = () => { trace.Add($"N{number}"); return number; },
                Maximum = () => { Assert.InRange(actorCalls, 0, 2); int selected = input[1 + actorCalls++]; trace.Add($"A{selected}"); Mutate(true, actorCalls); return selected; },
                TypeMaximum = selected => { Assert.InRange(typeCalls, 0, 1); int result = input[4 + typeCalls++]; trace.Add($"T{selected}:{result}"); Mutate(false, typeCalls); return result; },
            });
            Assert.Equal(row[2], value?.ToString(System.Globalization.CultureInfo.InvariantCulture) ?? "-");
            Assert.Equal(row[3], string.Join(';', trace)); Assert.Equal(X86Fixture.Int(row[4]), hp); Assert.Equal(X86Fixture.UInt(row[5]), number); return true;
        });
    }

    /// <summary>회복 없는 반환은 TYPE 장치를 요구하지 않고 누락된 첫 선택 위치만 진단한다.</summary>
    [Fact]
    public void UnselectedHitPointDevicesAreNotRequired()
    {
        string trace = ""; var hooks = new PriestRegenHitPointHooks { Current = () => { trace += "H"; return 50; }, Maximum = () => { trace += "A"; return 40; } };
        Assert.Null(RawPriestRegenHitPoints.Read(OriginalEdition.Patch1078, hooks)); Assert.Equal("HA", trace);
        trace = ""; hooks.Maximum = () => { trace += "A"; return 100; };
        Assert.Throws<InvalidOperationException>(() => RawPriestRegenHitPoints.Read(OriginalEdition.Patch1078, hooks)); Assert.Equal("HA", trace);
    }

    /// <summary>SP의 실제 추적 owner만 연결하고 초기화·회전이 같은 주소와 비트를 보존한다.</summary>
    [Fact]
    public void SharedStartupTrackingIsNotCopiedIntoSession()
    {
        var scene = new Scene(); Assert.Throws<InvalidOperationException>(() => RawPriestRegen.BindPriestRegenSpStore(scene.Store));
        Assert.Null(scene.Store.FirstPoolCurrent); Assert.All(scene.Store.Permutation.ToArray(), value => Assert.Equal((short)0, value));
        var tracking = new IntBox(); scene.Store.BindTrackingOffset(tracking); scene.Store.BindTrackingOffset(tracking);
        Assert.Throws<InvalidOperationException>(() => scene.Store.BindTrackingOffset(new IntBox()));
        Assert.Throws<ArgumentException>(() => RawPriestRegen.BindPriestRegenSpStore(scene.Store, scene.State.Patch));
        scene.Store.Initialize(); Assert.Same(tracking, scene.Store.TrackingOffsetCurrent); Assert.NotNull(scene.Store.FirstPoolCurrent);
        scene.Store.FirstPoolCurrent![0] = 123; Assert.Equal(123u, scene.Store.Pools[0]);
        scene.Store.Set(1, BitConverter.SingleToUInt32Bits(1234.5f)); int before = tracking.Value;
        scene.State.Patch.Sequence = -300; scene.State.Patch.ChangedSample = 19;
        PriestRegenHooks hooks = scene.Hooks(); hooks.PatchAudit = scene.Store.Rotate; hooks.SpReads = RawPriestRegen.BindPriestRegenSpStore(scene.Store);
        scene.Body(hooks).Handle(scene.Sid, 602, uint.MaxValue, 8.25f);
        Assert.Equal(before + 3, tracking.Value); Assert.Equal((uint)tracking.Value, scene.Store.OffsetCurrent);
        Assert.Equal(-300, scene.State.Patch.Sequence); Assert.Equal(19u, scene.State.Patch.ChangedSample);
        Assert.Equal(BitConverter.SingleToUInt32Bits(1234.5f), scene.Store.Get(1));
        Assert.Throws<InvalidOperationException>(() => scene.Store.BindTrackingOffset(tracking));
    }

    /// <summary>측정 안의 지연 초기화 뒤 실제 첫 배열·순열을 읽고 중첩 함수 사본을 유지한다.</summary>
    [Fact]
    public void StoreReadsFollowMeasurementAndKeepFunctionSnapshots()
    {
        var scene = new Scene(); scene.State.Patch.Sequence = -3; scene.State.Patch.ChangedSample = 19; scene.State.Patch.Remaining = 1;
        scene.Store.BindTrackingOffset(scene.State.Patch.SequenceBox); var reads = RawPriestRegen.BindPriestRegenSpStore(scene.Store, scene.State.Patch); string order = "";
        var hooks = scene.Hooks(); hooks.Measurement = () => { order += "M"; scene.Store.Initialize(); return 7000; };
        hooks.SpReads = new() { ObservedSequence = () => { order += "O"; return reads.ObservedSequence!(); },
            FirstPool = () => { order += "P"; return reads.FirstPool!(); }, FirstPermutation = () => { order += "S"; return reads.FirstPermutation!(); } };
        var body = scene.Body(hooks); hooks.SpReads.ObservedSequence = null; hooks.Measurement = null;
        body.Handle(scene.Sid, 602, 0, 8.25f); Assert.Equal("OMPS", order); Assert.True(scene.Store.Initialized);
        Assert.Equal(0u, scene.State.Patch.Remaining); Assert.Equal(19u, scene.State.Patch.ChangedSample);
    }

    /// <summary>첫 배열 null도 뒤 WORD 조회를 생략하지 않으며 각 store는 현재 표본·진단 주소만 쓴다.</summary>
    [Fact]
    public void IndependentPointerAndWordReadsUseCurrentSample()
    {
        var scene = new Scene(); string order = ""; var sample = new UIntBox(); var changed = new UIntBox();
        scene.State.Patch.Sample = 12345; scene.State.Patch.ChangedSample = 6789;
        scene.State.Patch.SharedSample = sample; scene.State.Patch.SharedChangedSample = changed;
        var hooks = scene.Hooks(); hooks.SpReads = new()
        {
            ObservedSequence = () => { order += "O"; return scene.State.Patch.Sequence; },
            FirstPool = () => { order += "P"; sample.Value = 77; return null; },
            FirstPermutation = () => { order += "S"; Assert.Equal(77u, changed.Value); sample.Value = 88; return -1; },
        };
        scene.Body(hooks).Handle(scene.Sid, 602, 0, 0); Assert.Equal("OPS", order); Assert.Equal(88u, changed.Value);
        Assert.Equal((12345u, 6789u), (scene.State.Patch.Sample, scene.State.Patch.ChangedSample));
        hooks.SpReads = new(); Assert.Throws<ArgumentException>(() => scene.Body(hooks));
    }

    /// <summary>네 독립 추적 조건과 조건 없는 반환이 장기 진단 주소에만 쓰는지 구별한다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(3)] [InlineData(4)]
    public void EachDiagnosticConditionUsesSharedAddress(int condition)
    {
        var scene = new Scene(); var sample = new UIntBox(); var diagnostic = new UIntBox { Value = 0x13579bdf };
        scene.State.Patch.SharedSample = sample; scene.State.Patch.SharedChangedSample = diagnostic;
        scene.State.Patch.Sample = 12345; scene.State.Patch.ChangedSample = 99; scene.State.Patch.ObservedSequence = 3;
        scene.State.Patch.Gate = 1; var hooks = scene.Hooks();
        if (condition == 0) scene.State.Patch.ObservedSequence = 4;
        if (condition == 1) { scene.State.Patch.Remaining = 1; hooks.Measurement = () => 7001; }
        if (condition == 2) scene.State.Patch.Gate = 0;
        if (condition == 3) scene.State.Patch.Sentinel = -1;
        scene.Body(hooks).Handle(scene.Sid, 602, 0, 0);
        Assert.Equal(condition == 4 ? 0x13579bdf : sample.Value, diagnostic.Value);
        Assert.Equal((12345u, 99u), (scene.State.Patch.Sample, scene.State.Patch.ChangedSample));
    }

    /// <summary>Actor의 현재 TYPE와 증분용 사제TYPE를 구별하고 실제 VT만으로 회복을 분배한다.</summary>
    [Fact]
    public void CurrentPriestTypeAndVtableAreIndependent()
    {
        var scene = new Scene(); scene.Types[159].MaxHitPoints = 60; scene.State.SharedPriestType = new() { Value = 159 };
        BinaryPrimitives.WriteInt32LittleEndian(scene.Pool.RawSlot(scene.Sid)[26..], 10);
        var body = scene.Body(scene.Hooks()); var handler = RawPriestRegen.MakePriestRegenHandler(scene.Pool, body);
        Assert.Equal(9f, handler(scene.Sid, 602, 0, 9)); Assert.Equal(20, scene.Priest.CurrentHitPoints(scene.Sid)); Assert.Equal(["V", "D"], scene.Events);
        scene.Events.Clear(); scene.Pool.RawSlot(scene.Sid)[10] = 160;
        Assert.Throws<InvalidOperationException>(() => handler(scene.Sid, 602, 0, 9)); Assert.Empty(scene.Events);
        BinaryPrimitives.WriteInt32LittleEndian(scene.Pool.RawSlot(scene.Sid)[26..], 200);
        Assert.Equal(9f, handler(scene.Sid, 602, 0, 9));
        Assert.Throws<ArgumentException>(() => handler(scene.Sid, 604, 0, 9));
        scene.Pool.RawSlot(scene.Sid)[0] = 0; Assert.Equal(9f, handler(scene.Sid, 604, 0, 9));
        Assert.Equal(7f, RawPriestRegen.MakePriestRegenHandler(scene.Pool, body, (_, _, _, _) => 7)(scene.Sid, 604, 0, 9));
    }
    /// <summary>첫 할당 정상 복귀 뒤에만 실제 배열 주소를 발행하고 재초기화 실패도 기존 주소를 지우지 않는다.</summary>
    [Fact]
    public void FirstArrayAddressPreservesPartialInitialization()
    {
        int calls = 0, failAt = 0; var store = new RawSpStore(new GameRandom(1), () => 1, bytes =>
        { int index = calls++; if (index == failAt) throw new InvalidOperationException("명시 할당 실패"); bytes.Fill(0x12); });
        Assert.Throws<InvalidOperationException>(store.Initialize); Assert.Null(store.FirstPoolCurrent); Assert.False(store.Initialized);
        calls = 0; failAt = 1; Assert.Throws<InvalidOperationException>(store.Initialize);
        uint[] first = Assert.IsType<uint[]>(store.FirstPoolCurrent); Assert.Equal(0x12121212u, first[0]); Assert.Equal(first[0], store.Pools[0]); Assert.False(store.Initialized);
        calls = 0; failAt = 0; Assert.Throws<InvalidOperationException>(store.Initialize); Assert.Same(first, store.FirstPoolCurrent);
    }

}
