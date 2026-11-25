using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 세 설치 값의 원문 순서 공개와 Editor 한정 동기 범위를 창 없이 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>RU-6 Load·Editor prefix·후반 UI 공개 부모의 순서·관문·플래그를 검증한다. 0.997에서 추가됐다.</summary>
public sealed class HostSessionLoadPublishTests
{
    /// <summary>설치 전 세션이다. 세 설치 값은 모두 거짓이다.</summary>
    private static HostSession Session() => new(new Kernel(), new GameRandom(1u), new CrtRandom());

    /// <summary>호출 순서를 기록하는 여덟 실제 하위 장치다.</summary>
    private static HostSessionLoadHooks RecordingHooks(List<string> order) => new()
    {
        Building = () => order.Add("building"),
        Motion = () => order.Add("motion"),
        RuntimeLoad = () => order.Add("runtime"),
        Devastation = () => order.Add("devastation"),
        CarrierDrop = () => order.Add("carrierDrop"),
        PriestPrayer = () => order.Add("priestPrayer"),
        Builder = () => order.Add("builder"),
        Placement = () => order.Add("placement"),
        Cursor = () => order.Add("cursor"),
    };

    /// <summary>여덟 하위가 원문 순서로 모두 정상 복귀한 뒤에만 loaded를 공개한다.</summary>
    [Fact]
    public void LoadPublishesAfterAllStepsReturnNormally()
    {
        var session = Session();
        Assert.False(session.NativeLoaded);
        var order = new List<string>();
        session.Load(RecordingHooks(order));
        Assert.Equal(["building", "motion", "runtime", "devastation", "carrierDrop", "priestPrayer", "builder", "placement", "cursor"], order);
        Assert.True(session.NativeLoaded);
        Assert.False(session.EditorUiInstalling);
        Assert.False(session.EditorPrefixUiInstalling);
    }

    /// <summary>중간 하위 실패는 공개하지 않고 앞선 호출만 남긴다.</summary>
    [Fact]
    public void LoadFailsWithoutPartialPublish()
    {
        var session = Session();
        var order = new List<string>();
        var hooks = RecordingHooks(order);
        hooks.Devastation = () => throw new InvalidOperationException("파편 연결 실패");
        var error = Assert.Throws<InvalidOperationException>(() => session.Load(hooks));
        Assert.Equal("파편 연결 실패", error.Message);
        Assert.Equal(["building", "motion", "runtime"], order);
        Assert.False(session.NativeLoaded);
    }

    /// <summary>장치 누락은 시작 전에 거부하고 공개하지 않는다.</summary>
    [Fact]
    public void LoadRejectsMissingDeviceBeforeAnyStep()
    {
        var session = Session();
        var order = new List<string>();
        var hooks = RecordingHooks(order);
        hooks.Cursor = null;
        Assert.Throws<InvalidOperationException>(() => session.Load(hooks));
        Assert.Empty(order);
        Assert.False(session.NativeLoaded);
    }

    /// <summary>이미 공개된 적재는 중복 실행하지 않는다.</summary>
    [Fact]
    public void LoadRejectsDuplicatePublish()
    {
        var session = Session();
        session.Load(RecordingHooks([]));
        Assert.True(session.NativeLoaded);
        var error = Assert.Throws<InvalidOperationException>(() => session.Load(RecordingHooks([])));
        Assert.Contains("중복", error.Message);
        Assert.True(session.NativeLoaded);
    }

    /// <summary>편집기 prefix 범위는 한 번만 열고 닫을 때 scope만 반납한다.</summary>
    [Fact]
    public void EditorPrefixScopeOpensAndClosesExactlyOnce()
    {
        var session = Session();
        bool inside = false;
        session.WithEditorPrefixUi(() => { inside = session.EditorPrefixUiInstalling; });
        Assert.True(inside);
        Assert.False(session.EditorPrefixUiInstalling);
        Assert.False(session.NativeLoaded);
        var error = Assert.Throws<InvalidOperationException>(() => session.WithEditorPrefixUi(() => { }));
        Assert.Contains("중복", error.Message);
        Assert.False(session.EditorPrefixUiInstalling);
    }

    /// <summary>후반 UI 범위는 정상 복귀만 tail 완료를 기록하고 실패해도 scope는 닫는다.</summary>
    [Fact]
    public void EditorInstallScopeRecordsTailOnlyOnSuccess()
    {
        var session = Session();
        session.WithEditorInstallUi(() => { Assert.True(session.EditorUiInstalling); });
        Assert.False(session.EditorUiInstalling);
        Assert.True(session.EditorUiTailReady);
        var tail = Session();
        Assert.Throws<InvalidOperationException>(() => tail.WithEditorInstallUi(() => throw new InvalidOperationException("후반 UI 실패")));
        Assert.False(tail.EditorUiInstalling);
        Assert.False(tail.EditorUiTailReady);
        Assert.False(tail.NativeLoaded);
    }

    /// <summary>공개 뒤 편집기 진입과 편집 중 캠페인 적재를 모두 거부한다.</summary>
    [Fact]
    public void PublishedAndEditingPathsExcludeEachOther()
    {
        var loaded = Session();
        loaded.Load(RecordingHooks([]));
        Assert.Throws<InvalidOperationException>(() => loaded.WithEditorPrefixUi(() => { }));
        Assert.Throws<InvalidOperationException>(() => loaded.WithEditorInstallUi(() => { }));
        var editing = Session();
        var order = new List<string>();
        editing.WithEditorPrefixUi(() => { });
        var error = Assert.Throws<InvalidOperationException>(() => editing.Load(RecordingHooks(order)));
        Assert.Contains("편집기", error.Message);
        Assert.Empty(order);
        Assert.False(editing.NativeLoaded);
    }
}
