using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 보호막 조회·생성의 원본 전체 관측과 실제 공장/소유자/사제 후반 연결을 대조한다.
namespace Netstorm.Tests;

/// <summary>실제 finder·소유자·사전 파괴를 원본 보호막 관측과 비교한다.</summary>
public sealed class PriestShieldTests
{
    /// <summary>원본 입력의 십진수DWORD를 읽는다.</summary>
    private static uint Number(string text) => X86Fixture.UInt(text);
    /// <summary>주변 바이트를 보존하고 원문DWORD만 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>원본 관측과 같은 소문자16진수 바이트를 만든다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexStringLower(raw);
    /// <summary>단순 요청을 판본별 원본 raw·현재TYPE·해시에 연결하는 입력 소유자다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 판본의 동일 물리 풀이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>실제 일반 finder가 조회하는 해시다.</summary>
        public readonly RawSquidHash Hash = new();
        /// <summary>현재 입력TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types;
        /// <summary>같은 현재 보호막 역할DWORD다.</summary>
        public readonly PriestForcefieldState Role = new();
        /// <summary>효과 뒤 다시 읽는 현재 로컬 조건이다.</summary>
        public readonly PriestShieldState ShieldState = new();
        /// <summary>사전 파괴와 공유하는 목록 상태다.</summary>
        public readonly PriestPostPopState Priests = new();
        /// <summary>원문 하위 효과의 인자·순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>PATCH면 원본50바이트, CD면36바이트를 관측한다.</summary>
        public readonly bool Patch;
        /// <summary>판본별 실제 owner 바이트다.</summary>
        public int OwnerOffset => Patch ? 34 : 32;
        /// <summary>판본별 실제 extra 바이트다.</summary>
        public int ExtraOffset => Patch ? 40 : 35;
        /// <summary>같은 해시·TYPE 표를 장면보다 오래 소유한다.</summary>
        public Scene(OriginalEdition edition)
        {
            Pool = new RawSidPool(24000, false, edition: edition); Patch = edition == OriginalEdition.Patch1078;
            Types = Enumerable.Range(0, Patch ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
        }
        /// <summary>원본 입력의 node10필드만 쓰고 요청된 단계에만 등록한다.</summary>
        public void Node(string[] fields, bool registered)
        {
            Assert.Equal(10, fields.Length); int sid = checked((int)Number(fields[0])); int type = checked((int)Number(fields[1]));
            Span<byte> raw = Pool.RawSlot(sid); raw.Clear();
            Put(raw, 0, sid == 50 ? (Patch ? 0x50f210u : 0x5003e0u) : 0x11010000u);
            raw[10] = (byte)type; raw[11] = (byte)Number(fields[2]); raw[ExtraOffset] = (byte)Number(fields[3]);
            raw[OwnerOffset] = (byte)Number(fields[8]); Put(raw, 14, Number(fields[4])); Put(raw, 18, Number(fields[5]));
            Types[type].FootX = X86Fixture.Int(fields[6]); Types[type].FootY = X86Fixture.Int(fields[7]);
            if (registered)
            {
                ref ushort head = ref Hash.Bucket(X86Fixture.Int(fields[9]), BitConverter.UInt32BitsToSingle(Number(fields[4])), BitConverter.UInt32BitsToSingle(Number(fields[5])));
                BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)sid;
            }
        }
        /// <summary>조회·사전 파괴 관측의 Scene 입력만 복원한다.</summary>
        public void PrepareForcefield(string[] input)
        {
            Hash.Reset(); Events.Clear();
            // 관측 대상7개 슬롯만 지워 앞 입력의 체인을 남기지 않는다.
            foreach (int sid in new[] { 50, 60, 61, 62, 63, 64, 65 }) Pool.RawSlot(sid).Clear();
            Node(["50", "158", input[3], input[4], input[0], input[1], "1", "1", input[2], "0"], Number(input[6]) != 0);
            // 원문 등록 순서를 복원하고 첫 선택은 실제 finder에 맡긴다.
            foreach (string node in input[7].Split(';')) Node(node.Split(','), true);
            Role.Type = Number(input[5]); Priests.Priests.Entries = [50, 60, 50, 61, 50, 62]; Priests.Priests.Count = 6;
        }
        /// <summary>판본별 실제 슬롯 폭만 원본과 같은 순서로 관측한다.</summary>
        public string Slot(int sid) => Hex(Pool.Slot(sid)[..(Patch ? 50 : 36)]);
    }

    /// <summary>원본 Scene1488입력과 PATCH 조회/사전파괴2976관측을 실제 모듈로 재생한다.</summary>
    [Fact]
    public void ForcefieldPatchNative2976()
    {
        string[][] all = X86Fixture.Read("priestforcefield"); Assert.Equal(10416, all.Length);
        Dictionary<string, string[]> inputs = all.Where(row => row[0] == "Scene").ToDictionary(row => row[1], row => row[2..]);
        Assert.Equal(1488, inputs.Count); string[][] rows = all.Where(row => row[0] != "Scene" && row[1] == "originals").ToArray(); Assert.Equal(2976, rows.Length);
        var scene = new Scene(OriginalEdition.Patch1078);
        X86Fixture.CheckRows(rows, row =>
        {
            scene.PrepareForcefield(inputs[row[2]]); var lookup = new RawPriestForcefield(scene.Pool, scene.Hash, scene.Types, scene.Role);
            bool same = true;
            if (row[0] == "Find") same = lookup.Find(50) == Number(row[4]);
            else
            {
                var hooks = new PriestPreDestroyHooks
                {
                    DestroyForcefield = (sid, flags) => scene.Events.Add($"D:{sid}:{flags}"), ValidateCarrier = (_, _) => { },
                    CarrierPre = (sid, flags) => scene.Events.Add($"C:{sid}:{flags}"),
                };
                new RawPriestPreDestroy(scene.Pool, scene.Priests, RawPriestForcefield.MakePriestForcefieldHooks(scene.Pool, lookup, hooks)).PreDestroy(50, Number(row[3]));
            }
            return same && scene.Priests.Priests.Count == Number(row[5]) && string.Join(',', scene.Priests.Priests.Entries) == row[6]
                && (scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)) == row[7]
                && string.Join(';', new[] { 50, 60, 61, 62, 63, 64, 65 }.Select(sid => $"{sid}:{scene.Slot(sid)}")) == row[8];
        });
    }

    /// <summary>CD 배치의 직접 조회1488입력을 별도PE마다 독립 원본과 비교한다.</summary>
    [Theory]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void ForcefieldCdFindNative1488(string edition)
    {
        string[][] all = X86Fixture.Read("priestforcefield");
        Dictionary<string, string[]> inputs = all.Where(row => row[0] == "Scene").ToDictionary(row => row[1], row => row[2..]);
        string[][] rows = all.Where(row => row[0] == "Find" && row[1] == edition).ToArray(); Assert.Equal(1488, rows.Length);
        var scene = new Scene(OriginalEdition.Cd1072);
        X86Fixture.CheckRows(rows, row => { scene.PrepareForcefield(inputs[row[2]]); return new RawPriestForcefield(scene.Pool, scene.Hash, scene.Types, scene.Role).Find(50) == Number(row[4]); });
    }

    /// <summary>원본 NaN payload·부호 있는0을 포함한4가지 double 입력 비트다.</summary>
    private static readonly ulong[] Clocks = [0, 0x8000000000000000ul, 0x3ff0000000000000ul, 0x7ff8000000001234ul];

    /// <summary>보호막 생성/소유자/Pop·소리/안내 뒤 실제 현재 조건의 변이를 원본 입력대로 공급한다.</summary>
    private static void Change(Scene scene, string[] row, uint stage)
    {
        if (Number(row[7]) != stage) return; Span<byte> raw = scene.Pool.RawSlot(50);
        if (stage == 1) { Put(raw, 14, 0x7fc12345); Put(raw, 18, 0x80000000); raw[scene.OwnerOffset] = (byte)((Number(row[2]) + 1) % 9); }
        else if (stage == 2) { Put(raw, 14, BitConverter.SingleToUInt32Bits(99.5f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(98.5f)); }
        else if (stage == 3) { raw[scene.OwnerOffset] = 8; scene.ShieldState.LocalPlayer = 8; scene.ShieldState.LoadingDepth = 0; scene.ShieldState.NoticeClock = 0; }
        else if (stage == 4) scene.ShieldState.NoticeClock = BitConverter.UInt64BitsToDouble(Clocks[3]);
    }

    /// <summary>판본별832입력의 실제 lookup/공통owner와 요청 순서·전체 raw/전역을 원본과 비교한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    [InlineData("original1037", OriginalEdition.Cd1072)]
    public void ShieldNative832(string edition, OriginalEdition version)
    {
        string[][] all = X86Fixture.Read("priestshield"); Assert.Equal(2496, all.Length);
        string[][] rows = all.Where(row => row[0] == edition).ToArray(); Assert.Equal(832, rows.Length);
        var scene = new Scene(version);
        X86Fixture.CheckRows(rows, row =>
        {
            int index = (int)Number(row[1]); uint owner = Number(row[2]); float x = index % 2 == 0 ? 20.75f : 254.9f, y = index % 2 == 0 ? 21.9f : 254.25f;
            byte[] statuses = [0, 2, 4, 6], extras = [0, 1, 8, 9]; scene.Hash.Reset(); scene.Events.Clear();
            // 각 관측 입력의 독립 TYPE 표를 다시 준비한다.
            for (int type = 0; type < scene.Types.Length; type++) scene.Types[type] = new OriginalTypeRecord();
            scene.Node(["50", "158", statuses[index % 4].ToString(), extras[index % 4].ToString(), BitConverter.SingleToUInt32Bits(x).ToString(), BitConverter.SingleToUInt32Bits(y).ToString(), "1", "1", owner.ToString(), "0"], false);
            if (index != 0)
            {
                // Scene7만 역순이며 그 밖은 원문 후보 등록 순서다.
                foreach (int candidate in index == 7 ? new[] { 1, 0 } : new[] { 0, 1 })
                    scene.Node([(60 + candidate).ToString(), (index == 3 ? 166 : 167).ToString(), statuses[(index + candidate) % 4].ToString(), (index == 4 ? 8 : 0).ToString(),
                        BitConverter.SingleToUInt32Bits(index == 6 ? x - 1 : x).ToString(), BitConverter.SingleToUInt32Bits(y).ToString(), "1", "1", (index == 2 ? (owner + 1) % 9 : owner).ToString(), ((index + candidate) % 4).ToString()], true);
            }
            scene.Role.Type = index == 5 ? 168u : 167u; scene.ShieldState.LocalPlayer = Number(row[6]); scene.ShieldState.LoadingDepth = Number(row[5]);
            scene.ShieldState.NoticeClock = BitConverter.UInt64BitsToDouble(Clocks[Number(row[4])]); scene.Pool.RawSlot(100).Fill(0xab);
            var lookup = new RawPriestForcefield(scene.Pool, scene.Hash, scene.Types, scene.Role);
            var commonOwner = new SquidOwner(scene.Pool, scene.Types, new RawPostPopState(), new SquidOwnerMode());
            var shield = new RawPriestShield(scene.Pool, lookup, scene.ShieldState, new PriestShieldHooks
            {
                Create = (type, flags) => { scene.Events.Add($"C:{type}:{flags}"); Span<byte> raw = scene.Pool.RawSlot(100); raw.Clear(); Put(raw, 0, 0x11010000); raw[10] = (byte)type; raw[11] = 4; Change(scene, row, 1); return 100; },
                SetOwner = (sid, player) => { scene.Events.Add($"O:{sid}:{player}"); Change(scene, row, 2); commonOwner.Set(sid, player); },
                Pop = (sid, px, py, flags) => { scene.Events.Add($"P:{sid}:{BitConverter.SingleToUInt32Bits(px)}:{BitConverter.SingleToUInt32Bits(py)}:{flags}"); Span<byte> raw = scene.Pool.RawSlot(sid); Put(raw, 14, BitConverter.SingleToUInt32Bits(px)); Put(raw, 18, BitConverter.SingleToUInt32Bits(py)); raw[11] = 0; Change(scene, row, 3); },
                ReserveSound = () => { scene.Events.Add($"A:40:{row[3]}"); return Number(row[3]) != 0; },
                LoadSound = file => { scene.Events.Add($"L:{file}"); Change(scene, row, 4); return 0x12345678; },
                AttachSound = (sid, sound, first, second) => scene.Events.Add($"S:{sid}:{sound}:{first}:{second}"),
                NoticeSound = notice => scene.Events.Add($"N:{notice.File}:{string.Join(':', notice.Arguments)}"),
                Translate = key => { scene.Events.Add($"T:{key}"); return "notice-token"; }, Tell = text => scene.Events.Add($"D:{text}"),
            });
            shield.Ensure(50);
            return (scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)) == row[8] && scene.Slot(50) == row[9] && scene.Slot(100) == row[10]
                && scene.ShieldState.LocalPlayer == Number(row[11]) && scene.ShieldState.LoadingDepth == Number(row[12]) && BitConverter.DoubleToUInt64Bits(scene.ShieldState.NoticeClock) == X86Fixture.ULong(row[13]);
        });
    }

    /// <summary>Native 조회의 현재 역할/TYPE 대여·typed 관문 구별과 원본0×0 교차식을 확인한다.</summary>
    [Fact]
    public void NativeCurrentRoleAndZeroFootprintKeepPhysicalContract()
    {
        var scene = new Scene(OriginalEdition.Patch1078);
        scene.Node(["50", "75", "9", "9", "1101398016", "1102000947", "1", "1", "3", "0"], false);
        scene.Node(["60", "70", "9", "0", "1101398016", "1102000947", "0", "0", "3", "0"], true);
        uint role = 70; var native = new RawPriestForcefield(scene.Pool, scene.Hash, scene.Types, () => role);
        Assert.Equal(60, native.Find(50)); role = 326; Assert.Equal(0, native.Find(50));
        role = 70; scene.Types[70].FootX = -1; Assert.Throws<ArgumentOutOfRangeException>(() => native.Find(50));
        scene.Types[70].FootX = 0; scene.Role.Type = 70;
        Assert.Throws<ArgumentException>(() => new RawPriestForcefield(scene.Pool, scene.Hash, scene.Types, scene.Role).Find(50));
        Put(scene.Pool.RawSlot(50), 14, 0x7fc12345); Assert.Throws<ArgumentOutOfRangeException>(() => native.Find(50));
        Assert.Throws<ArgumentException>(() => RawPriestForcefield.MakePriestForcefieldHooks(new RawSidPool(24000), native, new PriestPreDestroyHooks()));
    }

    /// <summary>실제 사제 접두·HP·Form/커널·장부·공장/owner·보호막 조회와 후반 어댑터를 함께 실행한다.</summary>
    [Fact]
    public void ActualTailProcessBooksFactoryAndShieldLifetime()
    {
        var scene = new Scene(OriginalEdition.Patch1078); var books = new RawPostPopState { LocalOwner = 1 };
        // 실제 생성자와 읽을 TYPE 필드만 합성 입력으로 공급한다.
        foreach (int type in new[] { 2, 158, 167 }) scene.Types[type].ConstructorAddress = OriginalConstructors.Address(type);
        scene.Types[158].Flags1 = 0x69012; scene.Types[158].Flags2 = 0x210000;
        scene.Types[158].MaxHitPoints = 200; scene.Types[158].Cost = 123;
        scene.Types[158].FootX = scene.Types[158].FootY = scene.Types[167].FootX = scene.Types[167].FootY = 1;
        var factory = new RawSquidFactory(scene.Pool, scene.Types); int priest = factory.Create(158, SidAllocation.Client);
        Span<byte> raw = scene.Pool.RawSlot(priest); raw[11] = 0; raw[12] = 1; raw[34] = 1; raw[40] = 0;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f)); Put(raw, 26, 20);
        var spots = new byte[65536]; var sp = new RawSpStore(new GameRandom(), () => 1u);
        var hp = new RawSquidReward(scene.Pool, scene.Types, books, new SquidRewardState(), sp);
        var destroy = new RawSquidDestroy(scene.Pool, new RawUnpop(scene.Pool, scene.Hash, spots), scene.Types);
        var kernel = new Kernel(); var host = new SquidProcessHost(scene.Pool, scene.Types, kernel, destroy, new SquidProcessState { Now = 12.5 });
        host.BindFormFactory(factory); scene.Priests.SharedPriests = new RawPostPopList { Entries = new uint[2] };
        var prefix = new RawPriestPostPop(scene.Pool, hp, scene.Priests, RawPriestPostPop.MakePriestPostPopProcessHooks(host));
        var common = new RawPostPop(scene.Pool, scene.Types, books);
        var carrier = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState(),
            RawCarrierPostPop.MakeCarrierPostPopHooks(scene.Pool, common, _ => { }, (_, _, _) => { }));
        carrier.BindCurrentTypes(scene.Types);
        var state = new RawPriestState(scene.Pool, hp, scene.Types, spots);
        var lookup = new RawPriestForcefield(scene.Pool, scene.Hash, scene.Types, scene.Role);
        scene.ShieldState.LocalPlayer = 1; scene.ShieldState.LoadingDepth = 1;
        int created = 0, pops = 0, falls = 0, deletes = 0;
        PriestShieldHooks effects = RawPriestShield.MakePriestShieldCreationHooks(scene.Pool, factory,
            new SquidOwner(scene.Pool, scene.Types, books, new SquidOwnerMode()), new PriestShieldHooks
            {
                Pop = (sid, x, y, flags) =>
                {
                    Assert.Equal(0u, flags); created = sid; pops++; Span<byte> shieldRaw = scene.Pool.RawSlot(sid);
                    // 아직 미연결인 전체 포스필드 Pop 대신 cpp 합성 검사와 같은 외부 공간 입력만 공급한다.
                    Put(shieldRaw, 14, BitConverter.SingleToUInt32Bits(x)); Put(shieldRaw, 18, BitConverter.SingleToUInt32Bits(y)); shieldRaw[11] = 0;
                    ref ushort head = ref scene.Hash.Bucket(0, x, y); BinaryPrimitives.WriteUInt16LittleEndian(shieldRaw[4..], head); head = (ushort)sid;
                },
                ReserveSound = () => false, LoadSound = _ => throw new InvalidOperationException("확보 실패 뒤 미도달 소리 조회"),
                AttachSound = (_, _, _, _) => throw new InvalidOperationException("미도달 소리 부착"),
                NoticeSound = _ => throw new InvalidOperationException("로딩 중 미도달 안내"),
                Translate = _ => throw new InvalidOperationException("로딩 중 미도달 문구"), Tell = _ => throw new InvalidOperationException("로딩 중 미도달 Tell"),
            });
        var shield = new RawPriestShield(scene.Pool, lookup, scene.ShieldState, effects);
        PriestPostPopTailHooks hooks = RawPriestPostPopTail.MakePriestPostPopTailShieldHooks(scene.Pool, shield, lookup,
            (sid, flags) => { Assert.Equal(created, sid); Assert.Equal(0u, flags); deletes++;
                scene.Hash.Bucket(0, 20.75f, 21.9f) = 0; scene.Pool.RawSlot(sid)[11] = 4; scene.Pool.Release(sid); },
            sid => { Assert.Equal(priest, sid); falls++; });
        RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[158] = new RiftTypeFrames([new RiftFrameCode(65, 80, 1, 0)]);
        var tail = new RawPriestPostPopTail(scene.Pool, prefix, carrier, state, frames, spots, hooks);
        uint free = scene.Pool.FreeCount; books.Depth = 1; tail.PostPop(priest, 1);
        Assert.Equal(1, kernel.Size()); Assert.Equal(8.25f, host.FindEvent(priest, 602)!.Payload());
        Assert.Equal(1u, scene.Priests.GetPriests().Count); Assert.Equal(0u, books.Depth); Assert.Equal(123, books.TotalCost); Assert.Equal(1u, books.GlobalCounts[158]);
        Assert.Equal(created, lookup.Find(priest)); Assert.Equal(1, falls); Assert.Equal(1, pops);
        books.Depth = 1; tail.PostPop(priest, 0x800); Assert.Equal(1, falls); Assert.Equal(1, pops); Assert.Equal(1, kernel.Size());
        Put(scene.Pool.RawSlot(priest), 26, 150); Array.Fill(spots, (byte)6);
        books.Depth = 1; tail.PostPop(priest, 0); Assert.Equal(1, deletes); Assert.Equal(0, lookup.Find(priest));
        scene.Pool.RawSlot(priest)[40] = 32; books.Depth = 1; tail.PostPop(priest, 0x800);
        Assert.Equal(2, pops); Assert.Equal(1, falls); Assert.Equal(created, lookup.Find(priest));
        host.Kill(host.FindEvent(priest, 602)!, 0x10); hooks.ClearShield!(priest);
        Assert.Equal(0, kernel.Size()); Assert.Equal(free, scene.Pool.FreeCount); Assert.False(sp.Initialized);
    }

    /// <summary>생성 전 필수 효과/같은 풀을 검사하고 기존 조회 성공에서도 누락 효과를 감추지 않는다.</summary>
    [Fact]
    public void ConstructorGuardsRunBeforeAnyCreation()
    {
        var scene = new Scene(OriginalEdition.Patch1078);
        scene.Node(["50", "158", "0", "0", "1101398016", "1102000947", "1", "1", "1", "0"], false);
        var lookup = new RawPriestForcefield(scene.Pool, scene.Hash, scene.Types, scene.Role); int creates = 0;
        var hooks = new PriestShieldHooks
        {
            Create = (_, _) => { creates++; return 0; }, SetOwner = (_, _) => { }, Pop = (_, _, _, _) => { },
            ReserveSound = () => false, LoadSound = _ => 0, AttachSound = (_, _, _, _) => { },
            NoticeSound = _ => { }, Translate = _ => "", Tell = _ => { },
        };
        var missing = hooks.Copy(); missing.Pop = null;
        Assert.Throws<ArgumentException>(() => new RawPriestShield(scene.Pool, lookup, scene.ShieldState, missing));
        missing = hooks.Copy(); missing.Tell = null;
        Assert.Throws<ArgumentException>(() => new RawPriestShield(scene.Pool, lookup, scene.ShieldState, missing));
        Assert.Throws<ArgumentException>(() => new RawPriestShield(new RawSidPool(24000), lookup, scene.ShieldState, hooks));
        Assert.Equal(0, creates);
        var shield = new RawPriestShield(scene.Pool, lookup, scene.ShieldState, hooks);
        Assert.Throws<ArgumentException>(() => shield.Ensure(50)); Assert.Equal(1, creates);
        Assert.Throws<ArgumentException>(() => RawPriestPostPopTail.MakePriestPostPopTailShieldHooks(new RawSidPool(24000), shield, lookup, (_, _) => { }, _ => { }));
    }
}
