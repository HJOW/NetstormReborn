using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests; // 장기 GumpForm의 현재 UI 및 World 통지 수명을 검사한다.

/// <summary>같은 실제 SID와 장기 GumpForm이 현재 통지 대여를 소비하는지 확인한다.</summary>
public sealed class RawSurfaceGumpCurrentTests
{
    /// <summary>현재 타입과 Native 전송 장치를 가진 장기 물리를 만든다.</summary>
    private static RawSurfaceStorage Storage()
    {
        var types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        storage.BindTransmissionDevices(new() { Broadcast = _ => { }, Connected = _ => false, Send = (_, _) => { } });
        storage.BindDisplayDevices(new() { View = () => new() { Zoom = 65536 }, Suppressed = () => false, Invalidate = (_, _) => { } });
        return storage;
    }

    /// <summary>통지 호출에만 사용하는 명시 수명 대여다.</summary>
    private sealed class Lease(Action release) : IRawRuntimeLease
    {
        /// <summary>통지 완료 또는 실패 뒤 호출 대여를 반납한다.</summary>
        public void Dispose() => release();
    }

    /// <summary>같은 Form은 현재 World를 우선하며 없으면 현재 UI를 호출하고 두 수명을 반납한다.</summary>
    [Fact]
    public void ActualFormNoticeSelectsCurrentRuntimeAndReleasesPins()
    {
        var storage = Storage(); var effects = new List<string>();
        var ui = new RawSurfaceGumpBindings { Notice = (owner, parent) => effects.Add($"ui:{owner}:{parent}"), Pin = () => { effects.Add("pin-ui"); return new Lease(() => effects.Add("release-ui")); } };
        var world = new RawSurfaceGumpBindings { Notice = (owner, parent) => effects.Add($"world:{owner}:{parent}"), Pin = () => { effects.Add("pin-world"); return new Lease(() => effects.Add("release-world")); } };
        storage.BindGumpUiRuntime(ui); storage.BindGumpRuntime(world);
        var forms = storage.GumpForms(); int form = storage.Pool.Allocate(); Span<byte> raw = storage.Pool.AllocatedBytes(form); raw.Clear(); raw[10] = 3;
        BinaryPrimitives.WriteUInt16LittleEndian(raw[16..], 31); forms.SetOwner(form, 42); forms.Notify(form);
        Assert.Equal(new[] { "pin-world", "pin-ui", "world:42:31", "release-ui", "release-world" }, effects);
        effects.Clear(); storage.ReleaseGumpRuntime(world); forms.Notify(form);
        Assert.Equal(new[] { "pin-ui", "ui:42:31", "release-ui" }, effects);
        effects.Clear(); world.Notice = null; storage.BindGumpRuntime(world); forms.Notify(form);
        Assert.Equal(new[] { "pin-ui", "ui:42:31", "release-ui" }, effects);
        storage.ReleaseGumpRuntime(world); storage.ReleaseGumpUiRuntime(ui);
        Assert.Throws<InvalidOperationException>(() => forms.Notify(form));
    }

    /// <summary>중복·다른 소유자 반납을 거부하고 실패 시에도 호출 수명을 반납한다.</summary>
    [Fact]
    public void DuplicateAndForeignLeaseAreRejectedAndFailureReleasesPins()
    {
        var storage = Storage(); var effects = new List<string>();
        var ui = new RawSurfaceGumpBindings { Notice = (_, _) => throw new InvalidOperationException("통지 실패"), Pin = () => new Lease(() => effects.Add("release")) };
        Assert.Throws<ArgumentException>(() => storage.BindGumpUiRuntime(new())); storage.BindGumpUiRuntime(ui);
        Assert.Throws<ArgumentException>(() => storage.BindGumpUiRuntime(ui));
        Assert.Throws<ArgumentException>(() => storage.ReleaseGumpUiRuntime(new()));
        var forms = storage.GumpForms(); int form = storage.Pool.Allocate(); Span<byte> raw = storage.Pool.AllocatedBytes(form); raw.Clear(); raw[10] = 3; forms.SetOwner(form, 1);
        Assert.Throws<InvalidOperationException>(() => forms.Notify(form)); Assert.Equal(new[] { "release" }, effects);
        storage.ReleaseGumpUiRuntime(ui);
    }

    /// <summary>장기 저장소의 실제 Form이 같은 TYPE 주소와 공장을 사용하고 중복 설치를 거부한다.</summary>
    [Fact]
    public void StorageGumpFormsBorrowsCurrentTypeAndVirtualDelete()
    {
        var storage = Storage(); var type = new UIntBox { Value = 3 };
        storage.BindGumpType(type); Assert.Throws<ArgumentException>(() => storage.BindGumpType(type));
        storage.Types[4].ConstructorAddress = OriginalConstructors.Address(3);
        var forms = storage.GumpForms(); Assert.Same(forms, storage.GumpForms());
        int first = forms.Create(); Assert.Equal(3, storage.Pool.Slot(first)[10]);
        type.Value = 4; int second = forms.Create(); Assert.Equal(4, storage.Pool.Slot(second)[10]); Assert.True(forms.IsGumpForm(second));
        Assert.Throws<ArgumentException>(() => storage.BindGumpType(new()));
        // 실제 월드 삭제 장치가 없으면 가상 삭제 경로는 성공을 꾸미지 않는다.
        Assert.Throws<InvalidOperationException>(() => forms.Destroy(second));
    }

    /// <summary>기존 종속 해제기가 이전 TYPE 주소를 빌린 뒤에는 다른 주소 설치를 거부한다.</summary>
    [Fact]
    public void GumpTypeCannotBeInstalledAfterContainedTerminal()
    {
        var storage = Storage(); var terminal = storage.ContainedUnpopExecutor(); var type = new UIntBox { Value = 4 };
        Assert.Throws<ArgumentException>(() => storage.BindGumpType(type));
        Assert.Same(terminal, storage.ContainedUnpopExecutor());
        Assert.Equal(3u, storage.ContainedGumpType.Value);
    }
}
