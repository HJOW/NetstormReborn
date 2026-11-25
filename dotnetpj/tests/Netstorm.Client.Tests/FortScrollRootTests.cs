using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Client.Scripting;

// Edit 목록의 이전·다음 창 이동 행 소비를 입력 분배와 실제 선택기에서 검증한다.
// 2026-11-26 1.043에서 WP-5c Edit 목록 스크롤 확인용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>스크롤 이동 행의 창 이동·캐시 유지·재표시를 대조한다.</summary>
public sealed class FortScrollRootTests
{
    /// <summary>이동 행 인자를 돌려주는 해석 장치와 기록한 이동량을 만든다.</summary>
    private static (FortSelectionHooks Hooks, List<int> Deltas) ScrollHooks(string resolved)
    {
        var deltas = new List<int>();
        return (new FortSelectionHooks { ResolveString = _ => resolved, ScrollMapList = delta => deltas.Add(delta) }, deltas);
    }

    /// <summary>이전·다음 이동량은 목록 창에 그대로 전달된다.</summary>
    [Theory]
    [InlineData("-50", -50)]
    [InlineData("50", 50)]
    public void ScrollMapListForwardsDeltaToListWindow(string text, int expected)
    {
        var (hooks, deltas) = ScrollHooks(text);
        Assert.True(FortSelection.Dispatch(new(FortSelection.ScrollMapList, 0, 7), new FortSelectionGlobals(), hooks));
        Assert.Equal([expected], deltas);
    }

    /// <summary>숫자가 아닌 이동 인자와 떼기 입력은 소비하지 않는다.</summary>
    [Fact]
    public void ScrollMapListRejectsNonNumericAndRelease()
    {
        var (hooks, deltas) = ScrollHooks("다음");
        Assert.False(FortSelection.Dispatch(new(FortSelection.ScrollMapList, 0, 7), new FortSelectionGlobals(), hooks));
        Assert.False(FortSelection.Dispatch(new(FortSelection.ScrollMapList | InputCode.Release, 0, 7), new FortSelectionGlobals(), hooks));
        Assert.Empty(deltas);
    }

    /// <summary>이동 효과 미연결은 명시 실패한다.</summary>
    [Fact]
    public void ScrollMapListWithoutHookFailsExplicitly()
    {
        var hooks = new FortSelectionHooks { ResolveString = _ => "50" };
        Assert.Throws<InvalidOperationException>(() => FortSelection.Dispatch(new(FortSelection.ScrollMapList, 0, 7), new FortSelectionGlobals(), hooks));
    }

    /// <summary>실제 선택기의 이동 행은 목록 창을 옮기고 같은 캐시로 다시 표시한다.</summary>
    [Fact]
    public void EditorScrollMovesWindowAndKeepsCache()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        using var temp = new EditorTempRoot();
        Directory.CreateDirectory(temp.Source);
        var customMaps = Path.Combine(temp.Root, "CustomMaps");
        using var workspace = new EditorCatalogWorkspace(new GameFileSystem(temp.Source, []), customMaps);
        var startup = new HostStartupScene(data);
        var strings = new ScriptStringRing();
        var tells = new List<string>();
        using var selection = new HostEditorSelection(startup, strings, tells.Add, workspace: workspace);
        uint forward = strings.Store("50");
        Assert.True(selection.Dispatch(new(FortSelection.ScrollMapList, 0, unchecked((int)forward))));
        Assert.Equal(50, selection.Catalog.State.WindowFirst);
        Assert.True(selection.Catalog.State.KeepCache);
        Assert.Equal(["LoadEditable"], tells);
        uint backward = strings.Store("-50");
        Assert.True(selection.Dispatch(new(FortSelection.ScrollMapList, 0, unchecked((int)backward))));
        Assert.Equal(0, selection.Catalog.State.WindowFirst);
        Assert.True(selection.Catalog.State.KeepCache);
        Assert.Equal(["LoadEditable", "LoadEditable"], tells);
        // 0 아래로는 내려가지 않는다.
        uint underflow = strings.Store("-50");
        Assert.True(selection.Dispatch(new(FortSelection.ScrollMapList, 0, unchecked((int)underflow))));
        Assert.Equal(0, selection.Catalog.State.WindowFirst);
    }
}
