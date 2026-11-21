using System.Buffers.Binary;
using System.Text.Json;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Core.Tests.RawPathProcessTests;

// 독립 PE의 실제 사제 정지/공통 도착 관측을 기존HP·공간·내용물에 재생한다.
namespace Netstorm.Core.Tests;

/// <summary>현재 PATCH 부모635관측과 Source의 중립 사제 wrapper 계약을 구분해 검사한다.</summary>
public sealed class RawPathParentBodyTests
{
    /// <summary>선택하면 안 되는 큰 하위 효과를 빈 성공으로 대신하지 않는다.</summary>
    internal static InvalidOperationException Forbidden() => new("미선택 실제 하위 효과가 호출됐습니다");
    /// <summary>기록된 입력 JSON만 읽으며 예상 분기를 계산하지 않는다.</summary>
    private static uint Value(JsonElement input, string name) => input.GetProperty(name).GetUInt32();
    /// <summary>독립 PE의 같은 슬롯·TYPE·일반 해시와 기존HP 모듈을 만든다.</summary>
    internal sealed class ParentScene
    {
        /// <summary>현재 부모/후보/내용물의 실제 풀이다.</summary>
        internal readonly RawSidPool Pool = new(24000);
        /// <summary>현재 일반finder와 지면 지도다.</summary>
        internal readonly RawSquidHash Hash = new();
        /// <summary>현재 실제 TYPE 표다.</summary>
        internal readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>현재 실제 지면 BYTE다.</summary>
        internal readonly byte[] Spots = new byte[65536];
        /// <summary>기존 실제 보상HP 본체다.</summary>
        internal readonly RawSquidReward Reward;
        /// <summary>기존 현재 사제HP/지면불가 본체다.</summary>
        internal readonly RawPriestState Priest;
        /// <summary>현재 내용물 커서/권한 입력이다.</summary>
        internal readonly ContainedFinderState Finder = new();
        /// <summary>기존 실제 내용물 조회 본체다.</summary>
        internal readonly RawCarrierPreDestroy Carrier;
        /// <summary>실제 귀환장소/근처TYPE getter다.</summary>
        internal readonly RawPathCleanup Cleanup;
        /// <summary>실제 현재Graph getter다.</summary>
        internal readonly RawPlayerPlacementAnchor Anchor;
        /// <summary>실제 내용물 생성 본체이며 선택하지 않은 외부 공장은 실패한다.</summary>
        internal RawContainedCreate Create;
        /// <summary>실제 발자국/중심 getter다.</summary>
        internal readonly RawSquidNeighbors Footprint;
        /// <summary>기존 실제 위치/부류 getter 소유자다.</summary>
        internal readonly RawDamageablePostDestroy Post;
        /// <summary>관측된 현재 경로 입력이다. 실제Host 부착 검사와 구별한다.</summary>
        internal readonly RawPathRuntime Runtime = new(OriginalEdition.Patch1078, true);
        /// <summary>현재 도착 전역이다.</summary>
        internal readonly CarrierArrivalState State = new();
        /// <summary>실제 부모가 요청한 외부 효과 순서다.</summary>
        internal readonly List<string> Events = [];
        /// <summary>기존 실제 helper를 구성하되 상위 결과는 만들지 않는다.</summary>
        internal ParentScene()
        {
            // 원본 관측의 부모50/후보60을 같은client free list에서 확보한다.
            for (int sid = 5; sid <= 60; sid++) { Assert.Equal(sid, Pool.Allocate(SidAllocation.Client)); Pool.RawSlot(sid).Clear(); }
            Types[158].MaxHitPoints = 200; Types[158].Flags2 = 0x210000;
            Reward = new(Pool, Types, new(), new(), new RawSpStore(new GameRandom(), () => 1)); Priest = new(Pool, Reward, Types, Spots); Priest.BindCurrentTypes(Types);
            Carrier = new(Pool, Finder, new() { ValidateDamageable = (_, _) => throw Forbidden(), DamageablePre = (_, _) => throw Forbidden(), DependentFound = () => throw Forbidden() });
            Cleanup = new(Pool, Hash, Types, new(), new() { ReserveShared = () => throw Forbidden(), AddShared = (_, _, _) => throw Forbidden(), Contained = (_, _, _, _) => throw Forbidden(), GraphFor = _ => throw Forbidden(), Ritual = (_, _, _) => throw Forbidden(), Refund = (_, _) => throw Forbidden(), Destroy = (_, _) => throw Forbidden() });
            Anchor = new(Pool, Types, Hash.BorrowEntries(0), new(), new(), Finder, new());
            Create = new(Pool, new(), new() { Create = (_, _) => throw Forbidden(), NotifySurface = _ => throw Forbidden(), Attach = (_, _, _) => throw Forbidden() });
            var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
            Footprint = new(Pool, Hash, Spots, Types, new RiftFrameCodeTables(frames)); Footprint.BindCurrentTypes(Types);
            Post = new(Pool, Hash, Types, frames, Reward, Footprint, new(), new() { DropDude = (_, _, _) => throw Forbidden(), DropIsland = (_, _, _, _) => throw Forbidden(), Regular = (_, _, _, _) => throw Forbidden(), HasBridgeFall = _ => throw Forbidden(), NotifySurface = _ => throw Forbidden(), DisplayUpdate = _ => throw Forbidden(), Destroy = (_, _) => throw Forbidden(), Base = (_, _) => throw Forbidden() });
        }
        /// <summary>원본 입력 슬롯의 실제 좌표/부류/해시를 등록한다.</summary>
        internal void Add(int sid, int type, float x, float y, int width = 1)
        {
            Span<byte> raw = Pool.RawSlot(sid); Put(raw, 0, 0x11018000); raw[10] = (byte)type; Put(raw, 14, BitConverter.SingleToUInt32Bits(x)); Put(raw, 18, BitConverter.SingleToUInt32Bits(y)); Types[type].FootX = Types[type].FootY = width;
            ref ushort head = ref Hash.Bucket(1, x, y); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)sid;
        }
        /// <summary>실제 부류getter 외의 큰 효과는 명시 경계로 두고 선택된 회피만 관측한다.</summary>
        internal CarrierArrivalHooks Hooks(bool path = true, uint reply = 1001) => new()
        {
            Path = _ => path ? Runtime : null, GenusAt = Post.GenusAt, CdPriestPickupPriest = () => throw Forbidden(), Pickup = _ => throw Forbidden(),
            Escape = (sid, delay, ev) => { Events.Add($"E:{sid}:{BitConverter.SingleToUInt32Bits(delay)}:{ev}:{reply}"); return reply; },
            ChooseReturn = _ => throw Forbidden(), ChooseGeyser = _ => throw Forbidden(), GoReturn = (_, _) => throw Forbidden(), GoGeyser = (_, _) => throw Forbidden(), MinedPacket = _ => throw Forbidden(), Destroy = (_, _) => throw Forbidden(), CdAddSp = (_, _) => throw Forbidden(), CdOwnerPacket = _ => throw Forbidden(), PlayAt = (_, _, _, _) => throw Forbidden(), Robust = _ => throw Forbidden(),
        };
        /// <summary>같은 실제 getter/내용물에 공통 도착을 연결한다.</summary>
        internal RawCarrierArrival Arrival(CarrierArrivalHooks? hooks = null) => new(Pool, Hash, Types, Carrier, Cleanup, Priest, Anchor, Create, Footprint, State, hooks ?? Hooks());
        /// <summary>같은 현재 지면/섬/내용물에 중립 사제 wrapper를 연결한다.</summary>
        internal RawPriestArrival PriestArrival(RawCarrierArrival carrier, PriestArrivalState state, PriestArrivalHooks hooks)
        {
            var plain = Enumerable.Range(0, 188).Select(_ => new PriestPlainCanonType()).ToArray();
            var terrain = new RawCanonPlacementTerrain(Pool, Types, plain, Hash.BorrowEntries(0), new(), new() { CandidatePermission = (_, _) => throw Forbidden(), ShapePermission = (_, _) => throw Forbidden(), FinishRelations = (_, _) => throw Forbidden() });
            var islands = new RawIslandRuntime(Pool, Hash, new byte[RawIslandStorage.Bytes], new(), new(), () => 0, _ => throw Forbidden(), () => 0, _ => throw Forbidden(), new() { SetResidenceOwner = (_, _) => throw Forbidden(), Assertion = _ => throw Forbidden() });
            return new(Pool, Priest, Reward, terrain, islands, Footprint, Create, carrier, state, hooks);
        }
    }
    /// <summary>PATCH432관측의최초좌표/정지·공통finder/낙하·반환·전체raw를 대조한다.</summary>
    [Fact]
    public void NativePriestHaltPatch432PreservesEffectsAndBytes()
    {
        int count = 0;
        // CD432관측은 기존HP171TYPE 하위 지원 전에는 검증 완료로 집계하지 않는다.
        foreach (string[] row in Rows("priesthalt-x86.tsv").Where(row => row[0] == "originals"))
        {
            using JsonDocument json = JsonDocument.Parse(row[1]); JsonElement input = json.RootElement; var scene = new ParentScene();
            float x = BitConverter.UInt32BitsToSingle(Value(input, "x")), y = BitConverter.UInt32BitsToSingle(Value(input, "y"));
            scene.Add(50, 158, x, y); uint neighbor = Value(input, "neighbor"); if (neighbor != 0) scene.Add(60, (int)neighbor, 20.2f, 21.3f);
            Put(scene.Pool.RawSlot(50), 26, Value(input, "hp")); scene.Types[158].Flags2 = Value(input, "genus"); scene.Spots[(int)((double)y + .9999f) * 256 + (int)((double)x + .9999f)] = (byte)Value(input, "ground");
            var check = new RawCarrierCheck(scene.Pool, scene.Types, scene.Spots, sid => { Assert.Equal(50, sid); scene.Events.Add($"F:50:{Value(input, "fall")}"); return Value(input, "fall") != 0; });
            var halt = new RawPriestHalt(scene.Pool, scene.Hash, scene.Types, scene.Spots, check, scene.Priest, scene.Reward, new() { Battle = Value(input, "battle") != 0, Boss = Value(input, "boss") != 0 }, new()
            {
                Unpop = (sid, flags) => { Assert.Equal(50, sid); Assert.Equal(0u, flags); scene.Events.Add("U:50:0"); if (Value(input, "change") != 0) { Put(scene.Pool.RawSlot(sid), 14, BitConverter.SingleToUInt32Bits(30.75f)); Put(scene.Pool.RawSlot(sid), 18, BitConverter.SingleToUInt32Bits(31.9f)); } },
                Pop = (sid, px, py, flags) => { Assert.Equal(50, sid); Assert.Equal(0u, flags); scene.Events.Add($"P:50:{BitConverter.SingleToUInt32Bits(px)}:{BitConverter.SingleToUInt32Bits(py)}:0"); Put(scene.Pool.RawSlot(sid), 14, BitConverter.SingleToUInt32Bits(px)); Put(scene.Pool.RawSlot(sid), 18, BitConverter.SingleToUInt32Bits(py)); },
                CarrierPickup = sid => { Assert.Equal(60, sid); scene.Events.Add($"C:60:{Value(input, "reply")}"); return Value(input, "reply"); },
                NuggetTransfer = sid => { Assert.Equal(60, sid); scene.Events.Add($"N:60:{Value(input, "reply")}"); return Value(input, "reply"); },
            });
            Assert.Equal(U(row[2]), input.GetProperty("kind").GetString() == "Halt" ? halt.Halt(50) : halt.CommonHalt(50));
            Assert.Equal(row[3], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); Assert.Equal(row[4], Convert.ToHexString(scene.Pool.Slot(50)).ToLowerInvariant()); count++;
        }
        Assert.Equal(432, count);
    }
    /// <summary>PATCH203관측을 실제 내용물/지면/Graph와 전체현재경로 입력에서 대조한다.</summary>
    [Fact]
    public void NativeCarrierArrivalPatch203PreservesEffectsAndBytes()
    {
        int count = 0;
        // Native 자료는 큰 포획/경제 하위를 고립시킨 실제 부모 입력이다.
        foreach (string[] row in Rows("carrierarrival-x86.tsv").Where(row => row[0] == "originals"))
        {
            using JsonDocument json = JsonDocument.Parse(row[1]); JsonElement input = json.RootElement; var scene = new ParentScene(); scene.Add(50, 158, 20.75f, 21.9f);
            uint profile = Value(input, "scene"); if (profile is >= 1 and <= 4) scene.Add(60, profile == 2 ? 84 : 150, 20.2f, profile == 4 ? 23.2f : 21.3f); else if (profile == 5) scene.Add(60, 165, 23.75f, 24.9f, 3);
            Put(scene.Pool.RawSlot(50), 26, 200); scene.Pool.RawSlot(50)[34] = (byte)Value(input, "owner"); scene.Types[158].Flags2 = Value(input, "genus");
            if (profile is >= 1 and <= 4) scene.Types[profile == 2 ? 84 : 150].Flags2 = new uint[] { 0, 0x10000, 0, 0x4200, 0x200 }[profile]; else if (profile == 5) scene.Types[165].Flags2 = 0x400000;
            scene.Spots[22 * 256 + 21] = scene.Spots[21 * 256 + 20] = 6; scene.State.Boss = Value(input, "boss") != 0; scene.State.ControlledOwners[Value(input, "owner")] = Value(input, "controlled") != 0;
            scene.Finder.Boss = scene.State.Boss; scene.Runtime.MoveAwayDepth = unchecked((int)Value(input, "depth")); scene.Runtime.SetCommandFields(Value(input, "helper"), 0, 0, new());
            var arrival = scene.Arrival(scene.Hooks(Value(input, "path") != 0, Value(input, "reply"))); Assert.Equal(U(row[2]), arrival.Arrival(50, 0, 0));
            Assert.Equal(row[3], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); Assert.Equal(row[4], Convert.ToHexString(scene.Pool.Slot(50)).ToLowerInvariant()); count++;
        }
        Assert.Equal(203, count);
    }
    /// <summary>새 독립 PE 자료가 없는 중립 wrapper의 Source 분기와 현재 억제/권한을 명시 대조한다.</summary>
    [Fact]
    public void PriestArrivalNeutralReturnAndSuppressionUseCurrentState()
    {
        var scene = new ParentScene(); scene.Add(50, 158, 20, 21); Put(scene.Pool.RawSlot(50), 26, 150);
        bool suppress = false, boss = true; int returned = 0; var state = new PriestArrivalState { SharedSuppressNeutralReturn = () => suppress, SharedBoss = () => boss };
        var arrival = scene.PriestArrival(scene.Arrival(), state, new() { Destroy = (_, _) => throw Forbidden(), NeutralReturn = sid => { Assert.Equal(50, sid); returned++; } });
        Assert.Equal(0, arrival.TempleAt(20, 21)); Assert.Equal(1001u, arrival.Arrival(50, 0, 0)); Assert.Equal(1, returned);
        suppress = true; Assert.Equal(1000u, arrival.Arrival(50, 0, 0)); suppress = false; boss = false; Assert.Equal(1000u, arrival.Arrival(50, 0, 0)); Assert.Equal(1, returned);
        boss = true; state.Loading = true; Assert.Equal(1000u, arrival.Arrival(50, 0, 0)); Assert.Equal(1, returned);
        Assert.Throws<ArgumentOutOfRangeException>(() => arrival.TempleAt(float.NaN, 0));
    }
    /// <summary>같은현재Player의AI 존재 변화와현재Robust 진단은사본표로고정되지않는다.</summary>
    [Fact]
    public void CarrierArrivalReadsCurrentControlAndRobustAtConsumption()
    {
        var scene = new ParentScene(); scene.Add(50, 158, 20.75f, 21.9f); scene.Add(60, 84, 20.2f, 21.3f); scene.Pool.RawSlot(50)[34] = 1;
        scene.Types[158].Flags2 = 0x10000; scene.Runtime.MoveAwayDepth = 1; bool controlled = false;
        RawCarrierArrival arrival = scene.Arrival(scene.Hooks() with { CurrentControlled = owner => { Assert.Equal(1u, owner); return controlled; }, Robust = _ => false });
        Assert.Equal(1001u, arrival.Arrival(50, 0, 0)); Assert.Single(scene.Events);
        controlled = true; Assert.Equal(1000u, arrival.Arrival(50, 0, 0)); Assert.Single(scene.Events);
        scene.State.Debug.Set(true); Assert.Throws<InvalidOperationException>(() => arrival.Arrival(50, 0, 0));
    }
}
