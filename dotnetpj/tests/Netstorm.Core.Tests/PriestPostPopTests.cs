using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 사제 접두의 원본 전체 출력과 실제 HP·목록·프로세스 합성을 검사한다.
namespace Netstorm.Tests;

/// <summary>PATCH 사제 목록/회복 접두를 원본2392입력과 실제 부품으로 대조한다.</summary>
public sealed class PriestPostPopTests
{
    /// <summary>fixture의 원본 입력만 읽는다.</summary>
    private static uint Number(string text) => X86Fixture.UInt(text);
    /// <summary>현재 DWORD 비트를 주변 필드 변경 없이 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);

    /// <summary>실제 PATCH 풀·HP·원본 입력 TYPE·공유 상태를 소유하는 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>원본 입력 SID50의 같은 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>현재 HP 조회가 빌리는 동일188TYPE다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>목록과 소유자9DWORD다.</summary>
        public readonly PriestPostPopState State = new();
        /// <summary>현재 사제1/4 HP와 진단 전역이다.</summary>
        public readonly SquidRewardState HpState = new();
        /// <summary>HP 조회가 SP를 초기화하지 않는지 검사하는 실제 저장소다.</summary>
        public readonly RawSpStore Sp = new(new GameRandom(), () => 1u);
        /// <summary>현재 TYPE 참조와 같은 풀을 조회하는 실제 HP 모듈이다.</summary>
        public readonly RawSquidReward Hp;
        /// <summary>실제 도달 효과의 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>독립 원본 입력의 검색 결과다.</summary>
        public bool Found;
        /// <summary>독립 원본 입력의 할당 결과다.</summary>
        public bool Allocated;

        /// <summary>같은 HP 장치와 입력 슬롯을 준비한다.</summary>
        public Scene()
        {
            Hp = new RawSquidReward(Pool, Types, new RawPostPopState(), HpState, Sp);
            Types[158].Flags1 = 0x69012; Types[158].Flags2 = 0x210000;
        }

        /// <summary>입력19칸 중 순수 입력만 실제 상태에 공급하고 용량 밖 목록 꼬리를 별도 보존한다.</summary>
        public uint[] Prepare(string[] row)
        {
            Assert.Equal(19, row.Length); Events.Clear();
            Found = Number(row[10]) != 0; Allocated = Number(row[11]) != 0;
            HpState.PriestQuarterHp = Number(row[9]) != 0; Types[158].MaxHitPoints = X86Fixture.Int(row[8]);
            uint[] storage = row[14].Split(',').Select(Number).ToArray();
            State.Priests.Entries = storage.Take(checked((int)Number(row[12]))).ToArray(); State.Priests.Count = Number(row[13]);
            // 원문 입력의 서로 다른9DWORD를 매번 초기화한다.
            for (int owner = 0; owner < 9; owner++) State.OwnerState[owner] = 0xabc000u + (uint)owner;
            Span<byte> raw = Pool.RawSlot(50); raw.Clear(); Put(raw, 0, 0x50f210);
            raw[10] = 158; raw[11] = (byte)Number(row[4]); raw[34] = (byte)Number(row[6]); raw[40] = (byte)Number(row[5]);
            BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], (ushort)Number(row[7]));
            Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f)); Put(raw, 26, 42);
            return storage;
        }

        /// <summary>원본 검색/할당/생성 경계의 실제 호출 인자와 당시 목록 개수를 관측한다.</summary>
        public PriestPostPopHooks Hooks() => new()
        {
            FindRegular = (sid, ev) => { Events.Add($"F:{sid}:{ev}:{State.GetPriests().Count}"); return Found; },
            ReserveRegular = () => { Events.Add("A:40"); return Allocated; },
            AddRegular = (sid, ev, payload) => Events.Add($"R:{sid}:{ev}:{BitConverter.SingleToUInt32Bits(payload)}"),
        };
    }

    /// <summary>원본 두x87정밀도의2392입력에서 목록·소유자·효과순서·50바이트를 전수 비교한다.</summary>
    [Fact]
    public void NativePatch2392()
    {
        string[][] all = X86Fixture.Read("priestpostpop"); Assert.Equal(7176, all.Length);
        string[][] rows = all.Where(row => row[1] == "originals").ToArray(); Assert.Equal(2392, rows.Length);
        var scene = new Scene(); var body = new RawPriestPostPop(scene.Pool, scene.Hp, scene.State, scene.Hooks());
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] storage = scene.Prepare(row); body.Prefix(50, Number(row[3]));
            Array.Copy(scene.State.Priests.Entries, storage, scene.State.Priests.Entries.Length);
            return $"{scene.State.Priests.Count}:{string.Join(',', storage)}" == row[15]
                && string.Join(',', scene.State.OwnerState) == row[16]
                && (scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)) == row[17]
                && scene.Pool.Slot(50).SequenceEqual(Convert.FromHexString(row[18]));
        });
        Assert.False(scene.Sp.Initialized);
    }

    /// <summary>할당 실패·기존 예약에서 HP 조회를 생략하고 실제 공유표를 콜백 뒤 재선택하는지 확인한다.</summary>
    [Fact]
    public void LazyHpAndCurrentSharedOwnerState()
    {
        var scene = new Scene(); string[] row = X86Fixture.Read("priestpostpop")[0].ToArray();
        row[3] = "1"; row[5] = "0"; row[6] = "1"; row[7] = "43905";
        row[10] = "0"; row[11] = "0"; row[12] = "4"; row[13] = "0"; scene.Prepare(row);
        scene.HpState.DebugAsserts = true; scene.Types[158].Flags1 = 0;
        Assert.Throws<InvalidOperationException>(() => scene.Hp.TypeHitPoints(158));
        var body = new RawPriestPostPop(scene.Pool, scene.Hp, scene.State, scene.Hooks());
        body.Prefix(50, 1); Assert.Equal(new[] { "F:50:602:1", "A:40" }, scene.Events);
        Assert.Equal(0u, scene.State.OwnerState[1]); Assert.Equal(50u, scene.State.Priests.Entries[0]);
        scene.Allocated = true; PriestPostPopHooks absent = scene.Hooks(); absent.AddRegular = null;
        var noAdd = new RawPriestPostPop(scene.Pool, scene.Hp, scene.State, absent);
        Assert.Contains("보상 HP", Assert.Throws<InvalidOperationException>(() => noAdd.Prefix(50, 1)).Message);
        uint[] first = Enumerable.Repeat(31u, 9).ToArray(), second = Enumerable.Repeat(41u, 9).ToArray();
        scene.State.SharedOwnerState = first; var hooks = scene.Hooks();
        hooks.FindRegular = (_, _) => { scene.State.SharedOwnerState = second; return true; };
        body = new RawPriestPostPop(scene.Pool, scene.Hp, scene.State, hooks); body.Prefix(50, 1);
        Assert.All(first, value => Assert.Equal(31u, value)); Assert.Equal(0u, second[1]); Assert.Equal(41u, second[0]);
        Assert.Equal(1u, scene.State.Priests.Count);
        Assert.Throws<ArgumentException>(() => body.Validate(0));
        scene.State.Priests.Count = 0xffffffff; Assert.Throws<ArgumentException>(() => body.Validate(50));
        Assert.Throws<ArgumentOutOfRangeException>(() => body.Prefix(50, 1));
    }

    /// <summary>현재 사제TYPE 분자와 객체HP 분모를 별도로 읽고 원본 한DWORD 초기화를 보존한다.</summary>
    [Fact]
    public void CurrentTypeAndDataOnlyOwnerReset()
    {
        var scene = new Scene(); string[] row = X86Fixture.Read("priestpostpop")[0].ToArray();
        row[5] = "0"; row[6] = "1"; row[7] = "43905"; row[12] = "4"; row[13] = "0"; scene.Prepare(row);
        scene.Types[159].Flags1 = 0x10; scene.Types[159].MaxHitPoints = 600;
        float observed = 0; uint current = 158;
        var body = new RawPriestPostPop(scene.Pool, scene.Hp, scene.State, new PriestPostPopHooks
        {
            FindRegular = (_, _) => { current = 159; return false; }, ReserveRegular = () => true,
            AddRegular = (_, _, value) => observed = value,
        }, () => current);
        body.Prefix(50, 1); Assert.Equal(25f, observed);
        uint[] states = Enumerable.Range(0, 9).Select(index => (uint)(0x1000 + index)).ToArray();
        RawPriestPostPop.ResetPriestOwnerState(states, 1); RawPriestPostPop.ResetPriestOwnerState(states, 8);
        Assert.Equal(0x1000u, states[0]); Assert.Equal(0u, states[1]); Assert.Equal(0u, states[8]);
        var diagnosed = new List<uint>(); RawPriestPostPop.ResetPriestOwnerState(states, 9, diagnosed.Add);
        Assert.Equal(new[] { 9u }, diagnosed); Assert.Throws<InvalidOperationException>(() => RawPriestPostPop.ResetPriestOwnerState(states, 0));
    }

    /// <summary>실제 생성자·HP·목록·조회·Form·커널에서 중복 예약 억제와 회복 재예약을 확인한다.</summary>
    [Fact]
    public void ActualFactoryHpLookupAndProcessHost()
    {
        var scene = new Scene();
        scene.Types[158].ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, 158);
        scene.Types[158].MaxHitPoints = 200;
        scene.Types[2].ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, 2);
        var factory = new RawSquidFactory(scene.Pool, scene.Types);
        int priest = factory.Create(158, SidAllocation.Client);
        // 원본 공간 등록/소유자 접두 뒤의 입력만 준비하며 전체 사제 Pop으로 주장하지 않는다.
        Span<byte> raw = scene.Pool.RawSlot(priest); raw[11] = 0; raw[12] = 1; raw[34] = 1; raw[40] = 0;
        scene.State.SharedPriests = new RawPostPopList { Entries = new uint[2] };
        uint[] ownerState = Enumerable.Repeat(17u, 9).ToArray(); scene.State.SharedOwnerState = ownerState;
        var hash = new RawSquidHash(); var spots = new byte[65536];
        var destroy = new RawSquidDestroy(scene.Pool, new RawUnpop(scene.Pool, hash, spots), scene.Types);
        var kernel = new Kernel(); var processState = new SquidProcessState { Now = 11.5 };
        var payloads = new List<float>();
        var host = new SquidProcessHost(scene.Pool, scene.Types, kernel, destroy, processState,
            (_, _, _, payload) => { payloads.Add(payload); return 2f; });
        host.BindFormFactory(factory);
        var body = new RawPriestPostPop(scene.Pool, scene.Hp, scene.State, RawPriestPostPop.MakePriestPostPopProcessHooks(host));
        var lookup = new RawPriestLookup(scene.Pool, scene.State);
        uint free = scene.Pool.FreeCount;
        body.Prefix(priest, 1);
        RegularProcess regen = Assert.IsType<RegularProcess>(host.FindEvent(priest, 602));
        Assert.Equal(8.25f, regen.Payload()); Assert.Equal(11.5, regen.Time());
        Assert.Equal(priest, regen.Parent()); Assert.Equal(1, kernel.Size()); Assert.Equal(free - 1, scene.Pool.FreeCount);
        Assert.Equal((uint)priest, lookup.Find(1)); Assert.Equal(0u, ownerState[1]); Assert.Equal(17u, ownerState[0]);
        ownerState[1] = 99; body.Prefix(priest, 0x801);
        Assert.Same(regen, host.FindEvent(priest, 602)); Assert.Equal(1u, scene.State.GetPriests().Count); Assert.Equal(0u, ownerState[1]);
        kernel.RunFrame(); Assert.Equal(new[] { 8.25f }, payloads); Assert.Equal(1u, regen.Count()); Assert.Equal(13.5, regen.Time());
        kernel.RunFrame(); Assert.Single(payloads);
        scene.HpState.PriestQuarterHp = true; body.Prefix(priest, 1); Assert.Equal(2f, regen.Payload());
        host.Kill(regen, 0x10); Assert.Equal(0, kernel.Size());
        body.Prefix(priest, 1); regen = Assert.IsType<RegularProcess>(host.FindEvent(priest, 602)); Assert.Equal(8f, regen.Payload());
        host.Kill(regen, 0x10); Assert.Equal(0, kernel.Size()); Assert.Equal(free, scene.Pool.FreeCount); Assert.False(scene.Sp.Initialized);
    }
}
