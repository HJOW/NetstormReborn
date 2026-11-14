using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원본 생성/나선/현재 지면 자료를 실제 모듈의 부분 효과와 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>사제 생성 세판본1236·Carrier PATCH676입력과 실제 생성/소유자 결선 검사다.</summary>
public sealed class PriestCreateOracleTests
{
    /// <summary>판본의 현재TYPE 표를 만든다.</summary>
    private static OriginalTypeRecord[] Types(OriginalEdition edition) => Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
    /// <summary>정렬되지 않은DWORD 필드만 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>좌표의 원문 단정도 비트만 쓴다.</summary>
    private static void Position(Span<byte> raw, float x, float y) { Put(raw, 14, BitConverter.SingleToUInt32Bits(x)); Put(raw, 18, BitConverter.SingleToUInt32Bits(y)); }
    /// <summary>생성 본체의 실제 나선·캐시한spot·현재TYPE/WORD를 세판본의 원본과 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078, 408)]
    [InlineData("originalCD", OriginalEdition.Cd1072, 414)]
    [InlineData("original1037", OriginalEdition.Cd1072, 414)]
    public void SpawnMatchesNativeSpiralAndPartialEffects(string editionName, OriginalEdition edition, int expectedRows)
    {
        string[][] rows = X86Fixture.Read("priestspawn", editionName); Assert.Equal(expectedRows, rows.Length); var pool = new RawSidPool(24000, false, edition: edition); var types = Types(edition); var spots = new byte[65536]; var state = new PriestSpawnState();
        // 원본 대체 생성자의 번호100까지 실제 client 정책으로 확보한다.
        for (int sid = 5; sid <= 100; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        X86Fixture.CheckRows(rows, row =>
        {
            var events = new List<string>(); uint queries = 0, change = X86Fixture.UInt(row[7]), afterGenus = X86Fixture.UInt(row[8]); long pattern = X86Fixture.Long(row[6]); state.BridgeType = 82; state.CheckingPlacement.Set(false); types[158].Flags2 = types[82].Flags2 = 0x210000; Array.Fill(spots, (byte)X86Fixture.UInt(row[5])); pool.RawSlot(100).Fill(0xcd); byte[] before = pool.Slot(100).ToArray();
            var hooks = new PriestSpawnHooks
            {
                MayPlace = query =>
                {
                    queries++; uint result = pattern == 0xffffffffL ? uint.MaxValue : pattern == -2 ? queries % 2 : pattern > 0 && queries >= pattern ? 1u : 0u;
                    events.Add($"Q:{query.Type}:{BitConverter.SingleToUInt32Bits(query.X)}:{BitConverter.SingleToUInt32Bits(query.Y)}:{query.Flags}:{query.Owner}:{query.Mode}:{result}");
                    if (change == 1 && queries == 1) { int cell = (int)query.Y * 256 + (int)query.X; spots[cell] = spots[cell - 256] = 0; }
                    if (change == 2) state.BridgeType = query.Type; return result != 0;
                },
                Create = (type, flags) => { Assert.Equal(0u, flags); pool.RawSlot(100).Fill(0xcd); Put(pool.RawSlot(100), 0, 0x20000100); pool.RawSlot(100)[10] = (byte)type; pool.RawSlot(100)[11] = 0; events.Add($"N:{type}:{flags}:100"); if (change == 2) state.BridgeType = 82; return 100; },
                SetOwner = (sid, owner) => { Assert.Equal(100, sid); events.Add($"O:{sid}:{owner}"); },
                Pop = (sid, x, y, flags) => { Assert.Equal(100, sid); Assert.Equal(0u, flags); events.Add($"L:{sid}:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{flags}"); if (change == 3) { pool.RawSlot(100)[10] = 82; types[82].Flags2 = afterGenus; } },
                NotifySurface = sid => events.Add($"S:{sid}"), CheckCarrier = sid => events.Add($"K:{sid}"),
            };
            var spawn = new RawPriestSpawn(pool, types, spots, state, hooks); spawn.Spawn(X86Fixture.FloatBits(row[1]), X86Fixture.FloatBits(row[2]), X86Fixture.UInt(row[3]), (ushort)X86Fixture.UInt(row[4]));
            Assert.Equal(X86Fixture.UInt(row[9]), state.BridgeType); Assert.Equal(row[10], string.Join(';', events)); byte[] native = X86Fixture.Bytes(row[11]); Assert.Equal(edition == OriginalEdition.Patch1078 ? 50 : 36, native.Length); Assert.Equal(native, pool.Slot(100)[..native.Length].ToArray()); Assert.Equal(before.AsSpan(native.Length).ToArray(), pool.Slot(100)[native.Length..].ToArray()); Assert.Equal(X86Fixture.UInt(row[12]), X86Fixture.Adler32(spots)); return true;
        });
    }
    /// <summary>현재TYPE/지면 확인과 실제 낙하 본체를 원본 PATCH676입력에 연결한다.</summary>
    [Fact]
    public void CarrierCheckMatchesNativePatchWithActualFall()
    {
        string[][] rows = X86Fixture.Read("carriercheck", "originals"); Assert.Equal(676, rows.Length); var pool = new RawSidPool(24000, false); var types = Types(OriginalEdition.Patch1078); var spots = new byte[65536]; var sp = new RawSpStore(new GameRandom(), () => 1u); var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(); var choices = new PriestFallFrames[188]; var surfaces = new ushort[65536];
        types[158].MaxHitPoints = 200; types[158].Flags1 = 0x69012; types[158].Flags2 = 0x210000; types[158].FootX = types[158].FootY = 1; var hp = new RawSquidReward(pool, types, new RawPostPopState(), new SquidRewardState(), sp); var priest = new RawPriestState(pool, hp, types, spots);
        // 원문 대상50까지 실제 client 슬롯을 확보한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        X86Fixture.CheckRows(rows, row =>
        {
            var events = new List<string>(); types[158].Flags2 = X86Fixture.UInt(row[2]); var mode = new PriestFallState { Authority = row[6] != "0" }; bool allocated = row[7] != "0"; uint change = X86Fixture.UInt(row[8]), reply = X86Fixture.UInt(row[10]); frames[158] = new RiftTypeFrames([new RiftFrameCode((byte)X86Fixture.UInt(row[5]), 80, 1, 0)]); Array.Clear(spots); spots[21 * 256 + 20] = (byte)X86Fixture.UInt(row[3]); spots[22 * 256 + 21] = (byte)X86Fixture.UInt(row[4]); Span<byte> raw = pool.RawSlot(50); raw.Clear(); Put(raw, 0, row[9] != "0" ? 0x0050f210u : 0x11313000u); raw[10] = 158; raw[11] = (byte)X86Fixture.UInt(row[14]); raw[34] = 1; raw[40] = (byte)X86Fixture.UInt(row[13]); Put(raw, 26, 20); Put(raw, 14, X86Fixture.UInt(row[11])); Put(raw, 18, X86Fixture.UInt(row[12]));
            // 명시 입력의 하위 효과만 바꾸며 낙하/권한 소비 순서는 실제 모듈이 결정한다.
            void Change(string stage) { if ((stage == "shield" && change == 1) || (stage == "new" && change == 2)) mode.Authority = !mode.Authority; if (stage == "repop" && change == 3) Position(pool.RawSlot(50), 30.75f, 31.9f); }
            var hooks = new PriestFallHooks
            {
                EnsureShield = sid => { events.Add($"S:{sid}"); Change("shield"); }, ReserveRegular = () => { events.Add("A:40"); Change("new"); return allocated; },
                AddSharedRegular = (sid, ev, payload) => events.Add($"R:{sid}:{ev}:{BitConverter.SingleToUInt32Bits(payload)}"), Unpop = (sid, flags) => events.Add($"U:{sid}:{flags}"), Repop = (sid, flags) => { events.Add($"P:{sid}:{flags}"); Change("repop"); }, Sound = (sid, x, y) => events.Add($"W:{sid}:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}"),
                SetFrame = (_, _, _) => Assert.Fail("요청 단계에서 프레임 지정 미선택"), AdvanceFrame = (_, _, _) => Assert.Fail("요청 단계에서 프레임 진행 미선택"), ClearShield = _ => Assert.Fail("요청 단계에서 보호막 삭제 미선택"),
            };
            var fall = new RawPriestFall(pool, priest, frames, choices, surfaces, mode, hooks); var check = new RawCarrierCheck(pool, types, spots, RawPriestFall.MakePriestFallWalker(pool, fall, sid => { events.Add($"V:{sid}:{reply}"); return reply != 0; })); bool result = row[1] == "Try" ? fall.TryFall(50) : check.Check(50);
            Assert.Equal(row[15] != "0", result); Assert.Equal(row[16], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal(X86Fixture.Bytes(row[17]), pool.Slot(50).ToArray()); Assert.Equal(row[18] != "0", mode.Authority); return true;
        });
        Assert.False(sp.Initialized);
    }

    /// <summary>실제 종속 해방→나선→서버 생성자→사제/공통 소유자→현재 Carrier를 연결한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void ReleaseSpawnsThroughActualFactoryAndPriestOwner(bool native)
    {
        var pool = new RawSidPool(24000, true); uint free = pool.FreeCount; var types = Types(OriginalEdition.Patch1078);
        types[158].ConstructorAddress = OriginalConstructors.Address(158); types[158].Flags1 = 0x69012; types[158].Flags2 = 0x210000; types[158].MaxHitPoints = 200; types[158].FootX = types[158].FootY = 1;
        var factory = new RawSquidFactory(pool, types); int parent = factory.Create(158, SidAllocation.Client), contained = pool.Allocate(SidAllocation.Client); Position(pool.RawSlot(parent), 20.75f, 21.9f); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(parent)[6..], (ushort)contained);
        pool.RawSlot(contained).Clear(); pool.RawSlot(contained)[10] = 6; pool.RawSlot(contained)[11] = 8; BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(contained)[12..], 0x8101); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(contained)[18..], 158); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(contained)[20..], 0x20);
        var book = new RawPostPopState(); var mode = new SquidOwnerMode { Battle = true }; var commonOwner = new SquidOwner(pool, types, book, mode); int notices = 0, pops = 0, carrierChecks = 0, born = 0; var priestOwner = new RawPriestOwner(pool, mode, new PriestOwnerState { BattlePlayer = 1 }, new PriestOwnerHooks { Base = commonOwner.Set, Notify = _ => notices++ }); var spots = new byte[65536];
        var check = new RawCarrierCheck(pool, types, spots, _ => throw new InvalidOperationException("실제 사제 genus는 Walker 낙하를 선택하지 않는다"));
        var hooks = new PriestSpawnHooks
        {
            MayPlace = _ => throw new InvalidOperationException("빈 후보는 배치 검사를 선택하지 않는다"),
            Create = (type, flags) => { Assert.Equal(158u, type); Assert.Equal(0u, flags); born = factory.Create(type); return born; },
            SetOwner = RawPriestOwner.MakePriestOwnerDispatch(priestOwner, commonOwner.Set),
            Pop = (sid, x, y, flags) => { Assert.Equal(born, sid); Assert.Equal(21f, x); Assert.Equal(22f, y); Assert.Equal(0u, flags); Assert.NotEqual(0, pool.Slot(sid)[11] & 4); pops++; },
            NotifySurface = _ => throw new InvalidOperationException("사제 생성은 다리 알림을 선택하지 않는다"), CheckCarrier = _ => throw new InvalidOperationException("어댑터 설치 전 경계가 호출되면 안 된다"),
        };
        hooks = RawCarrierCheck.MakeCarrierCheckSpawnHooks(pool, check, hooks); Action<int> actualCheck = hooks.CheckCarrier!; hooks.CheckCarrier = sid => { carrierChecks++; actualCheck(sid); }; var spawn = new RawPriestSpawn(pool, types, spots, new PriestSpawnState(), hooks); hooks.Create = null;
        var releases = new DamageableReleaseHooks { Create = (_, _) => throw new InvalidOperationException("사제는 일반 해방 생성을 선택하지 않는다"), SetOwner = (_, _) => Assert.Fail("일반 해방 소유자 미선택"), Pop = (_, _, _, _) => Assert.Fail("일반 해방 Pop 미선택"), NotifySurface = _ => Assert.Fail("일반 해방 표면 알림 미선택") };
        DamageableReleaseHooks attached = RawPriestSpawn.MakePriestSpawnHooks(pool, spawn, releases); Assert.Null(releases.SpawnPriest); Assert.Same(releases.Create, attached.Create);
        var releaseState = new DamageableReleaseState { Battle = true };
        var release = new RawDamageableRelease(pool, types, spots, new ContainedFinderState(), releaseState, attached);
        if (native) release.ReleaseNative(parent, () => mode.Battle, () => releaseState.BridgeType); else release.Release(parent);
        Assert.Equal(15000, born); Assert.Equal(free - 3, pool.FreeCount); Assert.True(priestOwner.Handles(born)); Assert.Equal((ushort)0x8181, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(born)[12..])); Assert.Equal(1, pool.Slot(born)[34]); Assert.Equal((ushort)0x8101, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(contained)[12..])); Assert.Equal((1, 1, 1), (pops, carrierChecks, notices));
        var other = new RawSidPool(24000, false); Assert.Throws<ArgumentException>(() => RawPriestSpawn.MakePriestSpawnHooks(other, spawn, releases)); Assert.Throws<ArgumentException>(() => RawCarrierCheck.MakeCarrierCheckSpawnHooks(other, check, new PriestSpawnHooks())); Assert.Throws<ArgumentException>(() => RawCarrierCheck.MakeCarrierCheckPostPopHooks(other, check, new CarrierPostPopHooks()));
    }

    /// <summary>매 호출 현재TYPE와지면을 읽고 제외 genus 뒤에는 비유한 좌표도 읽지 않는다.</summary>
    [Fact]
    public void CurrentCarrierGenusAndGroundChooseOnlyReachedEffects()
    {
        var pool = new RawSidPool(24000, false); int sid = pool.Allocate(SidAllocation.Client); var types = Types(OriginalEdition.Patch1078); var spots = new byte[65536]; pool.RawSlot(sid).Clear(); pool.RawSlot(sid)[10] = 158; Position(pool.RawSlot(sid), 20.75f, 21.9f); int calls = 0;
        var check = new RawCarrierCheck(pool, types, spots, target => { Assert.Equal(sid, target); calls++; return true; }); Assert.True(check.Check(sid)); Assert.Equal(1, calls);
        spots[22 * 256 + 21] = 4; Assert.False(check.Check(sid)); Assert.Equal(1, calls); spots[22 * 256 + 21] = 0; Assert.True(check.Check(sid)); Assert.Equal(2, calls);
        types[158].Flags2 = 0x210000; Position(pool.RawSlot(sid), float.NaN, float.PositiveInfinity); Assert.False(check.Check(sid)); Assert.Equal(2, calls); types[158].Flags2 = 0; Assert.Throws<ArgumentOutOfRangeException>(() => check.Check(sid));
        Assert.Throws<ArgumentException>(() => new RawCarrierCheck(pool, types[..1], spots, _ => false)); Assert.Throws<ArgumentException>(() => new RawCarrierCheck(pool, types, spots[..1], _ => false)); Assert.Throws<ArgumentNullException>(() => new RawCarrierCheck(pool, types, spots, null!));
    }

    /// <summary>실패한132후보가 끝난 뒤 PATCH만 현재 checking 전체DWORD를 읽는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void CompleteSpiralThenReadCheckingAtOriginalEditionGate(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition); var types = Types(edition); var spots = Enumerable.Repeat((byte)6, 65536).ToArray(); int queries = 0, reads = 0; var state = new PriestSpawnState { CheckingPlacement = new RawCheckingFlag(() => { Assert.Equal(132, queries); reads++; return 0x100u; }) };
        var hooks = new PriestSpawnHooks { MayPlace = _ => { queries++; return false; }, Create = (_, _) => throw new InvalidOperationException("실패 나선은 생성을 선택하지 않는다"), SetOwner = (_, _) => Assert.Fail("실패 나선 소유자 미선택"), Pop = (_, _, _, _) => Assert.Fail("실패 나선 Pop 미선택"), NotifySurface = _ => Assert.Fail("실패 나선 알림 미선택"), CheckCarrier = _ => Assert.Fail("실패 나선 Carrier 미선택") }; var spawn = new RawPriestSpawn(pool, types, spots, state, hooks);
        if (edition == OriginalEdition.Patch1078) Assert.Throws<InvalidOperationException>(() => spawn.Spawn(20.75f, 21.9f, 158, 1)); else spawn.Spawn(20.75f, 21.9f, 158, 1);
        Assert.Equal(132, queries); Assert.Equal(edition == OriginalEdition.Patch1078 ? 1 : 0, reads);
    }
}
