// Native 장기 Host 기본 삭제/전송 선택 장치와 실제 공통 분배기의 장기 수명 결선을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>기본 선택 설치·기본 삭제기·기본 훅·분배기 결합의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceProcessCurrentTests
{
    /// <summary>현재 TYPE/프레임/셰이프와 원문 생성자를 갖는 독립 장기 물리다.</summary>
    private static Netstorm.Core.Original.RawSurfaceStorage Storage()
    {
        var types = Enumerable.Range(0, 188).Select(index => new Netstorm.Core.Original.OriginalTypeRecord
        {
            ConstructorAddress = Netstorm.Core.Original.OriginalConstructors.Address(index),
            Flags1 = Netstorm.Core.Original.OriginalTypeRecord.HasHitPoints,
            MaxHitPoints = 1001,
            FootX = 1,
            FootY = 1,
        }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var storage = new Netstorm.Core.Original.RawSurfaceStorage(Netstorm.Core.Original.OriginalEdition.Patch1078, types, frames, shapes);
        storage.GameAux().SquidTransmission.Compress = 0;
        storage.GameAux().SquidTransmission.ClientSidAllowed = 1;
        // 기본 삭제기의 Unpop/표시 경로가 같은 장기 뷰를 읽으므로 독립 표시 장치를 먼저 연결한다.
        storage.BindDisplayDevices(new Netstorm.Core.Original.RawSurfaceDisplayBindings
        {
            View = () => new Netstorm.Core.Original.SquidDisplayView { Zoom = 65536, Viewport = new Netstorm.Core.Display.DisplayRect(0, 0, 1024, 768) },
            Suppressed = () => false,
            Invalidate = (_, _) => { },
        });
        return storage;
    }

    /// <summary>실제 방송 장치만 있는 Native 전송 묶음이다.</summary>
    private static Netstorm.Core.Original.RawSurfaceTransmissionDevices Transmission() => new()
    {
        Broadcast = _ => { },
        Connected = _ => false,
        Send = (_, _) => { },
    };

    /// <summary>실제 현재 선택/해제 부모만 있는 기본 선택 묶음이다.</summary>
    private static Netstorm.Core.Original.RawSurfaceProcessDevices Devices(uint selected = 0, Action? clear = null) => new()
    {
        Selected = () => selected,
        ClearSelection = clear ?? (() => { }),
    };

    /// <summary>실제 방송 장치 결합과 최초 소유자 상태에서만 기본 선택 설치를 허용한다.</summary>
    [Fact]
    public void BindRequiresNativeAndFirstOwners()
    {
        var storage = Storage();
        // 전송 장치 없이 기본 선택을 설치하면 거부한다.
        Assert.Throws<ArgumentException>(() => storage.BindProcessDevices(Devices()));
        storage.BindTransmissionDevices(Transmission());
        storage.BindProcessDevices(Devices());
        // 두 번째 설치는 같은 장기 수명을 바꾸지 않는다.
        Assert.Throws<ArgumentException>(() => storage.BindProcessDevices(Devices()));
        // 필수 항목이 하나라도 없으면 설치하지 않는다.
        var missing = Storage();
        missing.BindTransmissionDevices(Transmission());
        Assert.Throws<ArgumentException>(() => missing.BindProcessDevices(new Netstorm.Core.Original.RawSurfaceProcessDevices()));
        // 이미 Host 저장소를 만든 뒤에는 설치하지 않는다.
        var hosted = Storage();
        hosted.BindTransmissionDevices(Transmission());
        _ = hosted.Processes();
        Assert.Throws<ArgumentException>(() => hosted.BindProcessDevices(Devices()));
        // 이미 기본 삭제기를 만든 뒤에는 설치하지 않는다.
        var destroyed = Storage();
        destroyed.BindTransmissionDevices(Transmission());
        _ = destroyed.BaseDestroy();
        Assert.Throws<ArgumentException>(() => destroyed.BindProcessDevices(Devices()));
        // 빌린 물리에서는 설치하지 않는다.
        var borrowed = Storage();
        borrowed.BindTransmissionDevices(Transmission());
        using var lease = borrowed.AcquirePrepared();
        Assert.Throws<ArgumentException>(() => borrowed.BindProcessDevices(Devices()));
    }

    /// <summary>PATCH 진단 명시 설치 뒤에만 실제 Client 진단 경로를 기본 삭제기에 연결한다.</summary>
    [Fact]
    public void PatchDiagnosticsAndBaseDestroySharesPool()
    {
        var storage = Storage();
        storage.BindTransmissionDevices(Transmission());
        storage.BindProcessDevices(Devices());
        // 명시 설치 전 기본 삭제기는 독립 진단을 쓴다.
        Netstorm.Core.Original.RawSquidDestroy first = storage.BaseDestroy();
        Assert.False(first.HasPatchDiagnostics);
        Assert.Same(storage.BaseDestroy(), first);
        Assert.Same(storage.Pool, first.Pool);
        // CD 판본은 PATCH 진단 설치를 거부한다.
        var cd = new Netstorm.Core.Original.RawSurfaceStorage(Netstorm.Core.Original.OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => cd.EnablePatchDestroyDiagnostics());
        // PATCH 명시 설치 뒤 기본 삭제기는 같은 장기 진단 owner를 공유한다.
        var patched = Storage();
        patched.BindTransmissionDevices(Transmission());
        patched.BindProcessDevices(Devices());
        patched.EnablePatchDestroyDiagnostics();
        Netstorm.Core.Original.RawSquidDestroy second = patched.BaseDestroy();
        Assert.True(second.HasPatchDiagnostics);
        Assert.Same(patched.Pool, second.Pool);
    }

    /// <summary>현재 전체 선택 DWORD를 잘림 없이 다시 읽고 미연결은 명시 실패한다.</summary>
    [Fact]
    public void CurrentProcessSelectionPreservesFullDword()
    {
        var storage = Storage();
        // 선택 조회 장치 없이 읽으면 거부한다.
        Assert.Throws<InvalidOperationException>(() => storage.CurrentProcessSelection());
        storage.BindTransmissionDevices(Transmission());
        // 상위 비트를 포함한 전체 DWORD를 그대로 돌려준다.
        storage.BindProcessDevices(Devices(selected: 0xFEDCBA98u));
        Assert.Equal(0xFEDCBA98u, storage.CurrentProcessSelection());
    }

    /// <summary>기본 훅은 전송/선택 해제를 실제 장치로 보내고 그 밖은 파생 종단 명시 실패로 돌린다.</summary>
    [Fact]
    public void DefaultHooksRouteTransmitClearAndDedicated()
    {
        var storage = Storage();
        int broadcasts = 0;
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => broadcasts++,
            Connected = _ => false,
            Send = (_, _) => { },
        });
        int cleared = 0;
        storage.BindProcessDevices(Devices(selected: 7u, clear: () => cleared++));
        Netstorm.Core.Original.SquidDestroyHooks hooks = storage.DefaultProcessAssetHooks();
        // 선택 getter는 같은 장기 장치의 현재 값을 읽는다.
        Assert.Equal(7u, hooks.SelectedRaw!());
        // 선택 해제는 실제 현재 Client 부모를 호출한다.
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.ClearSelection, 7, 0, 0));
        Assert.Equal(1, cleared);
        // 전송은 같은 장기 writer의 실제 삭제 writer로 간다. 빈 슬롯은 writer 관문에서 거부된다.
        var transmitError = Assert.Throws<InvalidOperationException>(() => hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.Transmit, 0, 0, 0)));
        Assert.Contains("가상+58", transmitError.Message);
        Assert.Equal(1, cleared);
        // 그 밖의 자산 효과는 파생 종단 미연결로 명시 실패한다.
        var dedicated = Assert.Throws<InvalidOperationException>(() => hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, 0, 0, 0)));
        Assert.Contains("전용 삭제 몸체 미연결", dedicated.Message);
        // 공장 Unpop 경계는 같은 풀의 실제 공장을 쓴다.
        Assert.NotNull(hooks.UnpopForm);
    }

    /// <summary>분배기는 World 없이 같은 실제 Host와 의존/Gump Form을 결합하고 재사용한다.</summary>
    [Fact]
    public void ProcessDispatchBindsHostDependGumpAndReuses()
    {
        var storage = Storage();
        storage.BindTransmissionDevices(Transmission());
        storage.BindProcessDevices(Devices());
        Netstorm.Core.Original.RawSquidFormDispatch first = storage.ProcessFormDispatch();
        // 같은 장기 물리를 다음 World에서도 유지한다.
        Assert.Same(first, storage.ProcessFormDispatch());
        Assert.Same(storage.Pool, first.Pool);
        // 의존/Gump 본체는 같은 풀의 실제 장기 본체다.
        Assert.Same(storage.Pool, storage.DependForms().Pool);
        Assert.Same(storage.Pool, storage.GumpForms().Pool);
        // 같은 분배기를 두 번 요구해도 새로 만들지 않는다.
        Assert.Same(storage.DependForms(), storage.DependForms());
        Assert.Same(storage.GumpForms(), storage.GumpForms());
    }

    /// <summary>가상 삭제는 같은 장기 이웃/Graph를 빌리고 분배기는 같은 기본 삭제기를 공유한다.</summary>
    [Fact]
    public void VirtualDestroySharesPoolAndGraph()
    {
        var storage = Storage();
        storage.BindTransmissionDevices(Transmission());
        storage.BindProcessDevices(Devices());
        Netstorm.Core.Original.RawSquidDestroyDispatch virtualDestroy = storage.VirtualDestroy();
        Assert.Same(virtualDestroy, storage.VirtualDestroy());
        // 이웃/Graph 실행기와 같은 풀을 공유한다.
        Assert.Same(storage.Pool, storage.NeighborsExecutor().Pool);
        // 분배기의 자산 경계는 같은 장기 선택 장치를 읽는다.
        Netstorm.Core.Original.SquidDestroyHooks hooks = storage.DefaultProcessAssetHooks();
        Assert.NotNull(hooks.Emit);
        Assert.NotNull(hooks.SelectedRaw);
        Assert.NotNull(hooks.UnpopForm);
    }
}
