using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 장기 공통 종속 해제의 현재 내용물 부모 통지 경로를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>Native 현재 대여와 독립 저장소의 파생 통지 경계를 확인한다.</summary>
public sealed class RawSurfaceContentDetachTests
{
    /// <summary>한 호출의 명시 대여가 정확히 한 번 반납되는지 검사한다.</summary>
    private sealed class Lease(Action released) : IRawRuntimeLease
    {
        /// <summary>중복 반납 여부다.</summary>
        private bool _disposed;
        /// <summary>현재 호출의 반납을 기록한다.</summary>
        public void Dispose() { Assert.False(_disposed); _disposed = true; released(); }
    }

    /// <summary>공통 저장소를 만들고 선택된 경우에만 실제 Native 전송 장치를 연결한다.</summary>
    private static RawSurfaceStorage CreateStorage(bool native)
    {
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        if (native) storage.BindTransmissionDevices(new() { Broadcast = _ => { }, Connected = _ => false, Send = (_, _) => { } });
        return storage;
    }

    /// <summary>실제 Form 링크와 워크샾 가상표를 가진 부모를 같은 풀에 배치한다.</summary>
    private static (int Parent, int Form) Attach(RawSurfaceStorage storage)
    {
        int parent = storage.Pool.Allocate(SidAllocation.Client);
        BinaryPrimitives.WriteUInt32LittleEndian(storage.Pool.RawSlot(parent), 0x509148u);
        storage.Pool.RawSlot(parent)[11] = 0;
        int form = new RawSquidForms(storage.Pool).Create(6, SidAllocation.Client);
        storage.Pool.RawSlot(form)[11] = 8;
        BinaryPrimitives.WriteUInt16LittleEndian(storage.Pool.RawSlot(form)[16..], (ushort)parent);
        BinaryPrimitives.WriteUInt16LittleEndian(storage.Pool.RawSlot(parent)[6..], (ushort)form);
        return (parent, form);
    }

    /// <summary>늦게 결합한 현재 Native 소비자를 선택하고 정상·예외 복귀 모두 호출 대여를 반납한다.</summary>
    [Fact]
    public void NativeDetachUsesCurrentRuntimeAndReleasesPin()
    {
        var storage = CreateStorage(true);
        var unpop = storage.ContainedUnpopExecutor();
        var missing = Attach(storage);
        Assert.Throws<InvalidOperationException>(() => unpop.Detach(missing.Form));
        int pins = 0, releases = 0, calls = 0;
        bool fail = false;
        var runtime = new RawSurfaceContentBindings
        {
            Pin = () => { pins++; return new Lease(() => releases++); },
            Notice = (parent, form, reason) =>
            {
                Assert.Equal(2u, reason);
                Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(storage.Pool.Slot(parent)[6..]));
                Assert.Equal(0, storage.Pool.Slot(form)[11] & 8);
                Assert.Equal(pins - 1, releases);
                calls++;
                if (fail) throw new InvalidOperationException("통지 검사 예외");
            },
        };
        storage.BindContentRuntime(runtime);
        var normal = Attach(storage);
        unpop.Detach(normal.Form);
        Assert.Equal((1, 1, 1), (pins, releases, calls));
        fail = true;
        var exceptional = Attach(storage);
        Assert.Throws<InvalidOperationException>(() => unpop.Detach(exceptional.Form));
        Assert.Equal((2, 2, 2), (pins, releases, calls));
        GC.KeepAlive(runtime);
    }

    /// <summary>독립 저장소는 파생 소비자를 임의로 성공시키지 않고 기존 진단을 유지한다.</summary>
    [Fact]
    public void IndependentDetachKeepsMissingDerivedNoticeExplicit()
    {
        var storage = CreateStorage(false);
        var attached = Attach(storage);
        Assert.Throws<InvalidOperationException>(() => storage.ContainedUnpopExecutor().Detach(attached.Form));
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(storage.Pool.Slot(attached.Parent)[6..]));
    }
}
