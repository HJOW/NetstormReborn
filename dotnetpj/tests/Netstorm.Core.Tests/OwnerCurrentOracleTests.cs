using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 공통/사제 소유자의 독립 원본과 실제 목록·현재 TYPE·부분 효과를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>원본 owner72·priestowner7416·현재사제144입력과 실제 장부 결선 검사다.</summary>
public sealed class OwnerCurrentOracleTests
{
    /// <summary>범위 진단/알림/목록 자식의 원문 중단 입력이다.</summary>
    private sealed class Stop(string stage) : Exception(stage);
    /// <summary>새 TYPE를 판본의 물리 칸 수만큼 만든다.</summary>
    private static OriginalTypeRecord[] Types(OriginalEdition edition) => Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
    /// <summary>빈 껍데기 생성자 대신 실제 TYPE/프레임/모양을 소유하는 같은 저장소로 World를 만든다.</summary>
    private static RawSurfaceWorld World()
    {
        var types = Types(OriginalEdition.Patch1078); var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray(); var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        return new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97u, storage, frames);
    }
    /// <summary>원본 현재값 입력과 같은 전체 raw 패턴이다.</summary>
    private static void Pattern(RawSidPool pool, int sid)
    {
        // 기대 결과는 계산하지 않고 초기 payload만 동일하게 준비한다.
        for (int index = 0; index < RawSidPool.Stride; index++) pool.RawSlot(sid)[index] = unchecked((byte)(index * 29 + 7));
    }
    /// <summary>원문DWORD 필드만 비정렬로 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>원문WORD 필드만 쓴다.</summary>
    private static void Word(Span<byte> raw, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], value);
    /// <summary>현재 원문WORD를 읽는다.</summary>
    private static ushort Word(RawSidPool pool, int sid) => BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(sid)[12..]);
    /// <summary>원본PATCH50/CD36BYTE를 모두 대조하고 C# 고정 슬롯의 나머지 여백도 입력 그대로인지 확인한다.</summary>
    private static void CompareRaw(string hex, RawSidPool pool, int sid, byte[] before)
    {
        byte[] native = X86Fixture.Bytes(hex); Assert.Equal(pool.Edition == OriginalEdition.Patch1078 ? 50 : 36, native.Length);
        Assert.Equal(native, pool.Slot(sid)[..native.Length].ToArray()); Assert.Equal(before.AsSpan(native.Length).ToArray(), pool.Slot(sid)[native.Length..].ToArray());
    }
    /// <summary>같은 물리 raw/효과 순서를 공통 Owner의72독립 입력과 대조한다.</summary>
    [Fact]
    public void CommonOwnerMatchesNativeCurrent72Inputs()
    {
        string[][] rows = X86Fixture.Read("owner-current-local"); Assert.Equal(72, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            bool patch = row[0] == "originals"; uint[] args = row[1].Split(',').Select(X86Fixture.UInt).ToArray(); uint fort = args[0], player = args[1], profile = args[2], genus = 0x4000;
            var pool = new RawSidPool(32768, true, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072); Pattern(pool, 5); int ownerOffset = patch ? 34 : 32;
            pool.RawSlot(5)[10] = 85; pool.RawSlot(5)[11] = 0; pool.RawSlot(5)[ownerOffset] = 3; pool.RawSlot(5)[patch ? 40 : 35] = 0; var events = new List<string>(); string cause = "returned";
            var hooks = new CommonOwnerHooks
            {
                Fort = () => fort != 0, Battle = () => false, Challenge = () => false,
                Genus = type => { events.Add($"G{type}"); return genus; },
                Assertion = (expression, file, line) => { events.Add($"D{expression}|{file}|{line}"); if (profile == 1) { fort = 1; pool.RawSlot(5)[10] = 86; genus = 0; } },
                Remove = (owner, sid) => { events.Add($"M{owner},{sid}"); if (profile == 2) { pool.RawSlot(5)[10] = 86; genus = 0; } if (profile == 3) fort = 0; if (profile == 4) throw new Stop("remove"); },
                Append = (owner, sid) => { events.Add($"A{owner},{sid}"); if (profile == 5) throw new Stop("append"); },
            };
            byte[] before = pool.Slot(5).ToArray(); try { RawCommonOwner.SetCommonSquidOwner(pool, 5, player, hooks); } catch (Stop stop) { cause = stop.Message; }
            Assert.Equal(row[2], cause); Assert.Equal(row[3], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal(row[4], fort.ToString()); CompareRaw(row[5], pool, 5, before); return true;
        });
    }
    /// <summary>세 실제 PE의 전체 사제 소유자7416입력을 같은 실제 공통 Owner로 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078, 3528)]
    [InlineData("originalCD", OriginalEdition.Cd1072, 1944)]
    [InlineData("original1037", OriginalEdition.Cd1072, 1944)]
    public void PriestOwnerMatchesFullNativeBody(string editionName, OriginalEdition edition, int count)
    {
        string[][] rows = X86Fixture.Read("priestowner").Where(row => row[1] == editionName).ToArray(); Assert.Equal(count, rows.Length);
        var pool = new RawSidPool(24000, false, edition: edition); var types = Types(edition); types[158].Flags2 = 0x210000; var mode = new SquidOwnerMode(); var state = new PriestOwnerState(); var book = new RawPostPopState(); var common = new SquidOwner(pool, types, book, mode); var events = new List<string>(); int ownerOffset = edition == OriginalEdition.Patch1078 ? 34 : 32;
        // 원문 자료의 실제 client 할당 순서를 유지한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        // 알림과 부모 진입에서 실제 그 시점의WORD/소유자를 기록한다.
        string Fields(int sid) => $"{Word(pool, sid)}:{pool.Slot(sid)[ownerOffset]}";
        var priest = new RawPriestOwner(pool, mode, state, new PriestOwnerHooks
        {
            Base = (sid, owner) => { events.Add($"B:{sid}:{owner}:{Fields(sid)}"); common.Set(sid, owner); },
            Notify = sid => events.Add($"N:{sid}:{Fields(sid)}"),
        });
        X86Fixture.CheckRows(rows, row =>
        {
            mode.Fort = row[2] != "0"; mode.Battle = row[3] != "0"; mode.Challenge = false; state.LoadingDepth = X86Fixture.UInt(row[4]); state.FortPlayer = X86Fixture.UInt(row[5]); state.BattlePlayer = X86Fixture.UInt(row[6]); events.Clear(); Span<byte> raw = pool.RawSlot(50); raw.Clear();
            Put(raw, 0, edition == OriginalEdition.Patch1078 ? 0x0050f210u : 0x005003e0u); raw[10] = 158; raw[11] = (byte)X86Fixture.UInt(row[10]); Word(raw, (ushort)X86Fixture.UInt(row[7])); raw[ownerOffset] = (byte)X86Fixture.UInt(row[8]); raw[edition == OriginalEdition.Patch1078 ? 40 : 35] = (byte)X86Fixture.UInt(row[11]); Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
            byte[] before = pool.Slot(50).ToArray(); priest.Set(50, X86Fixture.UInt(row[9])); Assert.Equal(row[12], events.Count == 0 ? "-" : string.Join(';', events)); CompareRaw(row[13], pool, 50, before); return true;
        });
        Assert.Equal(0u, book.Providers.Count); Assert.Equal(0u, book.Factories.Count); Assert.Equal(0, book.TotalCost);
    }
    /// <summary>진단/알림/부모가 현재 모드·TYPE/free를 바꾸거나 중단하는144입력을 대조한다.</summary>
    [Fact]
    public void PriestOwnerMatchesNativeCurrent144Inputs()
    {
        string[][] rows = X86Fixture.Read("priest-owner-current-local"); Assert.Equal(144, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            bool patch = row[0] == "originals"; uint[] args = row[1].Split(',').Select(X86Fixture.UInt).ToArray(); uint player = args[2], profile = args[3]; var pool = new RawSidPool(32768, true, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072); Pattern(pool, 5); int ownerOffset = patch ? 34 : 32;
            Put(pool.RawSlot(5), 0, patch ? 0x0050f210u : 0x005003e0u); pool.RawSlot(5)[10] = 158; pool.RawSlot(5)[11] = 4; Word(pool.RawSlot(5), 0xab83); pool.RawSlot(5)[ownerOffset] = 3;
            var mode = new SquidOwnerMode { Fort = true, Battle = args[0] != 0 }; var state = new PriestOwnerState { LoadingDepth = args[1], FortPlayer = 2, BattlePlayer = 3 }; var events = new List<string>(); uint notices = 0; string cause = "returned";
            var hooks = new PriestOwnerHooks
            {
                Assertion = (expression, file, line) => { events.Add($"D{expression}|{file}|{line}"); if (profile == 1) { mode.Battle = true; mode.Fort = false; state.LoadingDepth = 0; state.BattlePlayer = 0; Word(pool.RawSlot(5), 0xcc81); } },
                Notify = sid => { events.Add($"N{sid},{Word(pool, 5)},{pool.Slot(5)[ownerOffset]}"); notices++; if (profile == 4) throw new Stop("notify"); if (profile == 2 && notices == 1) { mode.Battle = false; mode.Fort = false; state.LoadingDepth = 1; Word(pool.RawSlot(5), 0xbb07); } },
                Base = (_, owner) => { events.Add($"B{owner},{Word(pool, 5)},{pool.Slot(5)[ownerOffset]}"); if (profile == 5) throw new Stop("base"); pool.RawSlot(5)[ownerOffset] = (byte)owner; if (profile == 3) { pool.RawSlot(5)[11] |= 1; pool.RawSlot(5)[10] = 255; Word(pool.RawSlot(5), 0xcc81); mode.Battle = false; mode.Fort = true; state.LoadingDepth = 0; state.FortPlayer = 2; } },
            };
            var owner = new RawPriestOwner(pool, mode, state, hooks); byte[] before = pool.Slot(5).ToArray(); try { owner.Set(5, player); } catch (Stop stop) { cause = stop.Message; }
            Assert.Equal(row[2], cause); Assert.Equal(row[3], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal(row[4], $"{(mode.Fort ? 1 : 0)},{(mode.Battle ? 1 : 0)},{state.LoadingDepth},{state.FortPlayer},{state.BattlePlayer}"); CompareRaw(row[5], pool, 5, before); return true;
        });
    }
    /// <summary>실제 목록의 개수 쓰기가 바꾼 모드를 현재 Owner가 후단에서 다시 읽는다.</summary>
    [Fact]
    public void ActualCurrentListWritesChangeLaterOwnerModeAndKeepTail()
    {
        var pool = new RawSidPool(24000, false); int sid = pool.Allocate(SidAllocation.Client); pool.RawSlot(sid).Clear(); pool.RawSlot(sid)[10] = 85; pool.RawSlot(sid)[34] = 3;
        var types = Types(OriginalEdition.Patch1078); types[85].Flags2 = 0x4000; var book = new RawPostPopState(); var mode = new SquidOwnerMode { Fort = true }; var owner = new SquidOwner(pool, types, book, mode); uint[] previous = [(uint)sid, 77, (uint)sid]; uint used = 3; object identity = new();
        var binding = new PostPopListCurrentBinding { Storage = () => previous, Capacity = () => 3, ReadCount = () => used, WriteCount = value => { used = value; mode.Fort = false; }, SourceOwner = identity, SourceSlot = 3 };
        book.OwnerFactories[3].BindCurrent(binding); binding.WriteCount = _ => throw new InvalidOperationException("원래 함수 객체 변경은 영향 없음"); book.OwnerFactories[4].Entries = new uint[3]; owner.Set(sid, 4);
        Assert.Equal(1u, used); Assert.Equal(new uint[] { 77, 77, (uint)sid }, previous); Assert.Equal(0u, book.OwnerFactories[4].Count); Assert.Equal(4, pool.Slot(sid)[34]); Assert.False(mode.Fort); Assert.True(book.OwnerFactories[3].MatchesCurrentSource(identity, 3)); Assert.False(book.OwnerFactories[3].MatchesCurrentSource(null, 3)); Assert.False(book.OwnerFactories[3].MatchesCurrentSource(new object(), 3));
        var full = new RawPostPopList(); int storageReads = 0; full.BindCurrent(new PostPopListCurrentBinding { Storage = () => { storageReads++; throw new InvalidOperationException("가득 찬 목록의 저장소를 읽으면 안 된다"); }, Capacity = () => 0, ReadCount = () => 0, WriteCount = _ => throw new InvalidOperationException("가득 찬 목록의 개수를 쓰면 안 된다") }); full.Append(5); Assert.Equal(0, storageReads);
    }
    /// <summary>실제 현재 표 대여/사본·정상 복귀 진단·선택 목록의 물리 실패와 사제 후단을 확인한다.</summary>
    [Fact]
    public void LiveTypesReturningDiagnosticsAndPriestParentPreservePartialEffects()
    {
        var pool = new RawSidPool(24000, false); int sid = pool.Allocate(SidAllocation.Client); pool.RawSlot(sid).Clear(); pool.RawSlot(sid)[10] = 85; pool.RawSlot(sid)[34] = 3;
        var types = Types(OriginalEdition.Patch1078); var book = new RawPostPopState(); var mode = new SquidOwnerMode { Fort = true }; var owner = new SquidOwner(pool, types, book, mode); types[85].Flags2 = 0x4000; book.OwnerFactories[3].Entries = new uint[3]; owner.Set(sid, 3); Assert.Equal(0u, book.OwnerFactories[3].Count);
        int reports = 0; var diagnostics = new SquidOwnerDiagnostics { Types = types, Assertion = (_, _, _) => reports++ }; owner.BindDiagnostics(diagnostics); diagnostics.Assertion = (_, _, _) => throw new InvalidOperationException("입력 함수 객체를 재사용하면 안 된다"); owner.Set(sid, 3); Assert.Equal(1u, book.OwnerFactories[3].Count);
        Assert.Throws<ArgumentException>(() => owner.BindDiagnostics(new SquidOwnerDiagnostics { Types = types[..1] })); owner.Set(sid, 3); Assert.Equal(1u, book.OwnerFactories[3].Count);
        Assert.Throws<ArgumentOutOfRangeException>(() => owner.Set(sid, 9)); Assert.Equal(1, reports); Assert.Equal(9, pool.Slot(sid)[34]); Assert.Equal(0u, book.OwnerFactories[3].Count);
        mode.Fort = false; pool.RawSlot(sid)[10] = 255; pool.RawSlot(sid)[11] = 1; owner.Set(sid, 2); Assert.Equal(2, pool.Slot(sid)[34]);
        Put(pool.RawSlot(sid), 0, 0x0050f210); pool.RawSlot(sid)[10] = 158; pool.RawSlot(sid)[11] = 4; Word(pool.RawSlot(sid), 0xab83); mode.Fort = true; mode.Battle = true; var state = new PriestOwnerState { LoadingDepth = 1, FortPlayer = 2, BattlePlayer = 3 }; var events = new List<string>();
        var hooks = new PriestOwnerHooks { Base = (target, value) => { events.Add("base"); owner.Set(target, value); }, Notify = _ => events.Add("notify") }; var priest = new RawPriestOwner(pool, mode, state, hooks); hooks.Base = null; priest.Set(sid, 3); Assert.Equal(0xab83, Word(pool, sid)); Assert.Equal(new[] { "notify", "base", "notify" }, events);
        state.LoadingDepth = 0; events.Clear(); priest.Set(sid, 1); Assert.Equal(3, pool.Slot(sid)[34]); Assert.Equal(new[] { "base", "notify" }, events);
    }

    /// <summary>실제 RawSurfaceWorld도 구성 뒤 바뀐 같은 TYPE를 Owner에서 읽는다.</summary>
    [Fact]
    public void ActualWorldOwnerUsesCurrentStorageTypes()
    {
        var world = World(); RawSurfaceStorage storage = world.Storage();
        int sid = storage.Pool.Allocate(SidAllocation.Client); storage.Pool.RawSlot(sid).Clear(); storage.Pool.RawSlot(sid)[10] = 85; storage.OwnerMode.Fort = true;
        storage.Types[85].Flags2 = 0x4000; world.Bookkeeping().OwnerFactories[3].Entries = new uint[3]; world.SetAssetOwner(sid, 3);
        Assert.Equal(1u, world.Bookkeeping().OwnerFactories[3].Count); Assert.Equal((uint)sid, world.Bookkeeping().OwnerFactories[3].Entries[0]);
    }

    /// <summary>실제 World의 정상 복귀 진단 뒤 선택된 목록만 실패하며 앞의BYTE/제거는 남는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void ActualWorldDiagnosticKeepsEarlierOwnerEffects(bool listed)
    {
        var world = World(); RawSurfaceStorage storage = world.Storage(); int reports = 0;
        storage.BindDisplayDiagnostics(new StringBox(), (expression, file, line) => { reports++; Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0xa02u, line); Assert.Contains("newPlayerId", expression); });
        int sid = storage.Pool.Allocate(SidAllocation.Client); storage.Pool.RawSlot(sid).Clear(); storage.Pool.RawSlot(sid)[10] = 85; storage.Pool.RawSlot(sid)[34] = 3; storage.Types[85].Flags2 = listed ? 0x4000u : 0u; storage.OwnerMode.Fort = listed;
        RawPostPopList previous = world.Bookkeeping().OwnerFactories[3]; previous.Entries = [(uint)sid]; previous.Count = 1;
        if (listed) Assert.Throws<ArgumentOutOfRangeException>(() => world.SetAssetOwner(sid, 9)); else world.SetAssetOwner(sid, 9);
        Assert.Equal(1, reports); Assert.Equal(9, storage.Pool.Slot(sid)[34]); Assert.Equal(listed ? 0u : 1u, previous.Count);
    }
}
