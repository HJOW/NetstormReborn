using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Core.Tests.RawPathProcessTests;

// 같은 실제 Host/Kernel에서 원문 혼합 목록 필터와 저장한 다음 노드를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>4b27a0의 실제 필터/삭제flags와 원문64관측을 검증한다.</summary>
public sealed class RawProcessRemovalTests
{
    /// <summary>실제 Host가 전달한 삭제 flags만 외부에서 관찰한다.</summary>
    private sealed class Probe : SquidProcess
    {
        /// <summary>검사 입력의 실제 소멸 경계다.</summary>
        private readonly Action<uint> _destroy;
        /// <summary>실제 생성/부착은 Host에 맡기고 관찰 함수만 보관한다.</summary>
        public Probe(Action<uint> destroy) => _destroy = destroy;
        /// <summary>가상 삭제 시의 원문 flags를 그대로 알린다.</summary>
        public override void OnFormDestroy(uint flags) => _destroy(flags);
    }
    /// <summary>원문 WORD 링크의 폭 밖 바이트를 보존한다.</summary>
    private static void Word(Span<byte> raw, int offset, int value) => BinaryPrimitives.WriteUInt16LittleEndian(raw[offset..], checked((ushort)value));
    /// <summary>내장/일반 자산 혼합64목록의 실제 삭제 순번과 flags가 같다.</summary>
    [Fact]
    public void NativeMixedFilter64UsesActualHostDeletion()
    {
        string[][] rows = Rows("process-removal-filter-x86.tsv"); Assert.Equal(64, rows.Length);
        // 각 원문 목록을 실제 별도 Host/Kernel에 구성한다.
        foreach (string[] row in rows)
        {
            using var scene = new Scene(OriginalEdition.Patch1078); scene.Host.Kill(scene.Process, 0x40); var trace = new List<string>(); var nodes = new List<int>(); uint[] types = row[0] == "-" ? [] : row[0].Split(',').Select(U).ToArray();
            // 프로세스는 실제 부착하고 나머지는 원문 입력의 raw Form/자산이다.
            for (int index = 0; index < types.Length; index++)
            {
                uint type = types[index]; int sequence = index;
                if (type >= 10 && type < 63) nodes.Add(scene.Host.Attach(new Probe(flags => trace.Add($"{sequence}:{flags}")), type, scene.Parent, 0x10)!.Form());
                else { int sid = scene.Pool.Allocate(SidAllocation.Client); Put(scene.Pool.AllocatedBytes(sid), 0, type < 70 ? 0x512208u : 0x501dd0u); scene.Pool.AllocatedBytes(sid)[10] = (byte)type; nodes.Add(sid); }
            }
            Word(scene.Pool.AllocatedBytes(scene.Parent), 6, nodes.Count == 0 ? 0 : nodes[0]);
            // 실제 부착순서와 구별해 원문 목록의 다음/이전/부모 링크를 공급한다.
            for (int index = 0; index < nodes.Count; index++) { Span<byte> raw = scene.Pool.AllocatedBytes(nodes[index]); Word(raw, 4, index + 1 < nodes.Count ? nodes[index + 1] : 0); raw[11] |= 8; Word(raw, raw[10] < 70 ? 14 : 16, index == 0 ? 0 : nodes[index - 1]); Word(raw, raw[10] < 70 ? 16 : 14, scene.Parent); }
            RawProcessRemoval.RemoveAttached(scene.Host, scene.Parent, U(row[1])); Assert.Equal(row[2], trace.Count == 0 ? "-" : string.Join(';', trace));
        }
    }
    /// <summary>삭제 도중 머리에 추가된 새 노드는 다시 순회하지 않는다.</summary>
    [Fact]
    public void SavedNextSkipsNewAttachmentAndForwardsFlag40()
    {
        using var scene = new Scene(OriginalEdition.Patch1078); uint oldPath = scene.Process.KernelId; RegularProcess regular = scene.Host.AddRegular(scene.Parent, 0, 1)!; uint regularId = regular.KernelId, added = 0; var flags = new List<uint>();
        SquidProcess probe = scene.Host.Attach(new Probe(value => { flags.Add(value); added = scene.Host.AddRegular(scene.Parent, 1, .05f)!.KernelId; }), 39, scene.Parent, 0x10)!; uint id = probe.KernelId;
        RawProcessRemoval.RemoveAttached(scene.Host, scene.Parent, 61); Assert.Same(probe, scene.Kernel.Get(id)); RawProcessRemoval.RemoveAttached(scene.Host, scene.Parent, 0);
        Assert.Equal(new uint[] { 0x40 }, flags); Assert.Null(scene.Kernel.Get(id)); Assert.Null(scene.Kernel.Get(regularId)); Assert.Null(scene.Kernel.Get(oldPath)); Assert.NotNull(scene.Kernel.Get(added));
    }
    /// <summary>원문 필터로 선택됐지만 실제 PID가 없으면 명시 실패한다.</summary>
    [Fact]
    public void MissingActualProcessIsRejectedAfterTypeFilter()
    {
        using var scene = new Scene(OriginalEdition.Patch1078); scene.Host.Kill(scene.Process, 0x40); int form = scene.Pool.Allocate(SidAllocation.Client); Span<byte> raw = scene.Pool.AllocatedBytes(form); raw.Clear(); raw[10] = 39; raw[11] = 8; Word(raw, 16, scene.Parent); Word(scene.Pool.AllocatedBytes(scene.Parent), 6, form);
        Assert.Throws<InvalidOperationException>(() => RawProcessRemoval.RemoveAttached(scene.Host, scene.Parent, 0));
    }
}
