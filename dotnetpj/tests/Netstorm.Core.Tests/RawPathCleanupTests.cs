using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Core.Tests.RawPathProcessTests;

// 두 원본 정리 본체의 실제 finder/contained/Graph를 같은 입력에서 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>실제 플레이 재현 없이 독립 PE의 바이트/효과 관측을 소비한다.</summary>
public sealed class RawPathCleanupTests
{
    /// <summary>일반 좌표와 두 거의올림 경계의 원문 float 입력이다.</summary>
    private static readonly RawPathPoint[] Coordinates = [new(40.75f, 41.25f), new(40.00004959106445f, 41.00004959106445f), new(40.000003814697266f, 41.000003814697266f)];
    /// <summary>기존 contained와 Graph 처리기에 실제 raw/해시를 공급하는 입력 장면이다.</summary>
    private sealed class CleanupScene
    {
        /// <summary>모든 raw 조회/변이가 공유하는 풀이다.</summary>
        internal readonly RawSidPool Pool;
        /// <summary>일반 finder/Graph 조회의 실제 공간이다.</summary>
        internal readonly RawSquidHash Hash = new();
        /// <summary>현재 TYPE/부류/발자국 입력이다.</summary>
        internal readonly OriginalTypeRecord[] Types;
        /// <summary>helper 외 전체 이미지가 보존되는 실제 경로다.</summary>
        internal readonly RawPathRuntime Runtime;
        /// <summary>효과 뒤 현재 boss를 다시 읽는 상태다.</summary>
        internal readonly PathCleanupState State = new();
        /// <summary>원본 관측 순서의 외부 효과다.</summary>
        internal readonly List<string> Effects = [];
        /// <summary>실제 정리 실행기다.</summary>
        internal readonly RawPathCleanup Cleanup;
        /// <summary>기대 분기를 만들지 않고 원문 입력만 구성한다.</summary>
        internal CleanupScene(OriginalEdition edition, uint[] input)
        {
            Pool = new(24000, edition: edition); Types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(i => new OriginalTypeRecord { ConstructorAddress = i is 1 or 2 or 3 or 6 ? OriginalConstructors.Address(edition, i) : 0 }).ToArray(); Runtime = new(edition, true, 0xaa);
            // 명시 입력 슬롯은 free list에서 실제로 확보한다.
            for (int sid = 5; sid <= 11; sid++) { Assert.Equal(sid, Pool.Allocate(SidAllocation.Client)); Pool.RawSlot(sid).Clear(); }
            uint p = input[0]; int coordinate = (int)input[7]; RawPathPoint point = Coordinates[coordinate]; bool helper = p is 1 or 2 or 3 or 4 or 5 or 6 or 7 or 14 or 15 or 16, near = p is 1 or 6 or 7 or 16;
            if (helper) Add(6, 128, p is 7 or 5 ? 1u : 0u, (p == 3 ? 0u : 1u) | (p == 6 ? 8u : 0u), near ? point : new(80.25f, 81.25f), 1, p == 4 ? 0u : 0x404200u, 1, 7);
            bool place = p is 8 or 9 or 10 or 11 or 14 or 16 or 17;
            if (place) Add(7, p is 8 or 14 or 17 ? 126 : 85, p == 10 ? 2u : 0u, p == 11 ? 1u : 0u, point, 1, p is 8 or 14 or 17 ? 0u : 0x200u, p == 9 ? 2u : 1u, p == 17 ? 3u : 7u);
            if (p is 12 or 13 or 14 or 15 or 16) Add(9, 165, 0, p is 13 or 15 ? 8u : 0u, point, 1, 0, 1, 7);
            Add(11, 97, 0, 0, point, 0, 2, 1, 7); if (p == 17) Types[126].Flags1 = 0x800; Hash.Cell(0, coordinate == 0 ? 41 : 40, coordinate == 0 ? 42 : 41) = 11;
            Span<byte> parent = Pool.RawSlot(5); Put(parent, 0, 0x11010000); parent[10] = input[3] != 0 ? (byte)158 : (byte)133; parent[11] = input[2] != 0 ? (byte)2 : (byte)0;
            Put(parent, 14, BitConverter.SingleToUInt32Bits(point.X)); Put(parent, 18, BitConverter.SingleToUInt32Bits(point.Y)); parent[edition == OriginalEdition.Patch1078 ? 34 : 32] = 1; BinaryPrimitives.WriteUInt16LittleEndian(parent[6..], 10);
            Pool.RawSlot(10)[10] = 54; BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(10)[4..], place ? (ushort)8 : (ushort)0); Put(Pool.RawSlot(8), 0, 0x11010000); Pool.RawSlot(8)[10] = 6; BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(8)[18..], 154); BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(8)[20..], 32);
            Runtime.SetAttachment(10, 5); Runtime.SetCommandFields(helper ? 6u : 0u, 0x87654321, 0xabcdef01, new(17, 19)); State.Boss = input[1] != 0; State.Checking.Set(input[5]);
            var containedState = new ContainedFinderState(); var carrier = new RawCarrierPreDestroy(Pool, containedState, new() { ValidateDamageable = (_, _) => throw new InvalidOperationException("독립 조회만 실행"), DamageablePre = (_, _) => throw new InvalidOperationException("독립 조회만 실행"), DependentFound = () => throw new InvalidOperationException("독립 조회만 실행") });
            var anchor = new RawPlayerPlacementAnchor(Pool, Types, Hash.BorrowEntries(0), new(), new(), containedState, new() { GraphReady = 1 });
            Cleanup = new(Pool, Hash, Types, State, new()
            {
                ReserveShared = () => { Effects.Add("N:40"); return input[4] != 0; },
                AddShared = (sid, ev, payload) => { Assert.Equal(6, sid); Assert.Equal(0x1a2cu, ev); Assert.Equal(1f, payload); Effects.Add($"S:{ev},{sid},{BitConverter.SingleToUInt32Bits(payload)}"); if (input[6] == 1) State.Boss = input[1] == 0; },
                Contained = (sid, mask, kind, index) => { containedState.Boss = State.Boss; containedState.CheckingDead = State.Checking; return carrier.FindContained(sid, mask, kind, index); },
                GraphFor = sid => anchor.GraphFor((uint)sid),
                Ritual = (temple, sid, flags) => { Assert.Equal(9, temple); Assert.Equal(5, sid); Assert.Equal(0u, flags); Effects.Add("R:9,5,0"); if (input[6] == 2) State.Boss = !State.Boss; },
                Refund = (type, owner) => Effects.Add($"F:{type},{owner}"),
                Destroy = (sid, flags) => { Assert.Contains(sid, new[] { 6, 8 }); Assert.Equal(0u, flags); Effects.Add($"D:{sid},0"); Pool.RawSlot(sid)[11] |= 2; },
            });
        }
        /// <summary>실제 raw/type/공간 머리와 next를 입력 값 그대로 등록한다.</summary>
        private void Add(int sid, int type, uint state, uint extra, RawPathPoint point, int level, uint genus, uint owner, uint graph)
        {
            Span<byte> raw = Pool.RawSlot(sid); bool patch = Pool.Edition == OriginalEdition.Patch1078; Put(raw, 0, 0x11010000); raw[10] = (byte)type; raw[11] = (byte)state; raw[patch ? 40 : 35] = (byte)extra;
            Put(raw, 14, BitConverter.SingleToUInt32Bits(point.X)); Put(raw, 18, BitConverter.SingleToUInt32Bits(point.Y)); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], Hash.Bucket(level, point.X, point.Y)); Hash.Bucket(level, point.X, point.Y) = (ushort)sid; raw[patch ? 34 : 32] = (byte)owner; raw[patch ? 30 : 28] = (byte)graph;
            Types[type].Flags2 = genus; Types[type].Flags1 = type == 97 ? 0x800u : 0u; Types[type].FootX = Types[type].FootY = 1;
        }
        /// <summary>PE의 일곱 전체 슬롯을 주입 vtable DWORD만 제외하고 대조한다.</summary>
        internal void CheckSlots(string expected)
        {
            string[] images = expected.Split(';'); Assert.Equal(7, images.Length);
            // CD의 관측된 원문36BYTE와 PATCH50BYTE를 각각 비교한다.
            for (int sid = 5; sid <= 11; sid++) { byte[] bytes = Convert.FromHexString(images[sid - 5]); Assert.Equal(bytes.AsSpan(4).ToArray(), Pool.Slot(sid).Slice(4, bytes.Length - 4).ToArray()); }
        }
    }
    /// <summary>두 판본 전체 정리/질의의 효과와 helper 외 미사용 이미지까지 대조한다.</summary>
    [Fact]
    public void NativeCleanupBodyAndQueriesPreserveAllObservedBytes()
    {
        int count = 0, queries = 0;
        // 기대 효과는 기존 독립 PE 자료만 사용한다.
        foreach (string[] row in Rows("pathcleanup-x86.tsv"))
        {
            string[] values = row[1].Split(',');
            if (values[0] == "Q")
            {
                uint[] input = values.Skip(1).Select(U).ToArray(); var scene = new CleanupScene(Edition(row[0]), [input[0], 1, 0, 1, 1, 0, 0, input[1]]); RawPathPoint point = Coordinates[input[1]];
                Assert.Equal(row[3], $"Q:{scene.Cleanup.ReturnPlaceAt(point.X, point.Y, unchecked((int)input[2]))},{scene.Cleanup.NearbyType(point.X, point.Y, 2, input[3])}"); scene.CheckSlots(row[6]); queries++; continue;
            }
            var body = new CleanupScene(Edition(row[0]), values.Select(U).ToArray()); byte[] before = body.Runtime.Bytes.ToArray();
            if (row[3] == "A" || row[3].EndsWith(";A", StringComparison.Ordinal)) { Assert.ThrowsAny<Exception>(() => body.Cleanup.AfterSpatial(body.Runtime, 5)); body.Effects.Add("A"); }
            else body.Cleanup.AfterSpatial(body.Runtime, 5);
            Assert.Equal(row[3], body.Effects.Count == 0 ? "-" : string.Join(';', body.Effects)); Assert.Equal(U(row[4]), body.Runtime.Helper); Assert.Equal(U(row[5]) != 0, body.State.Boss); body.CheckSlots(row[6]);
            int helper = body.Runtime.Layout.Helper; Assert.Equal(before.AsSpan(0, helper).ToArray(), body.Runtime.Bytes.Span[..helper].ToArray()); Assert.Equal(before.AsSpan(helper + 4).ToArray(), body.Runtime.Bytes.Span[(helper + 4)..].ToArray()); count++;
        }
        Assert.True(count > 500); Assert.True(queries > 250);
    }
}
