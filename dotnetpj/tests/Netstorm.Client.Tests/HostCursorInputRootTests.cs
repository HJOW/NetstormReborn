using System.Buffers.Binary;
using System.Reflection;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 입력 경로의 커서 취득·취소·조회·위치 호출자를 세션·메뉴 결선에서 검증한다.
// 2026-11-26 1.046에서 DI-3 커서 호출자용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>세션 커서 장치 조립과 실제 포인터·Esc·Take·Query 진입을 같은 장치에서 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>창·음향을 열지 않고 실제 메뉴 트리와 입력 분배기를 조립한다.</summary>
    /// <returns>화면·메뉴·입력 분배기와 변경 횟수다.</returns>
    private static (Screen Screen, HostMainMenu Menu, HostMenuInput Router, Func<int> Changes) CursorInputMenuScene()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        HostMenuInput? router = null; int changes = 0;
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        { Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 },
            action => router!.Post(action));
        var queue = new InputQueue(); var strings = new ScriptStringRing();
        var windows = new HostHtmlWindows(menu, queue, () => 1, _ => { });
        windows.BindParser(startup, raw => ScriptCommand.QueueScriptCommand(raw, queue, strings,
            new() { Lookup = DialogScriptCommands.Lookup, Unknown = _ => throw new InvalidOperationException() }));
        var text = new HostHtmlText(menu, windows, screen, _ => default, (_, _) => { }, _ => { }, _ => { });
        text.Bind(windows);
        var menus = new HostHtmlMenus(menu, windows, text, startup, screen, (_, _) => { });
        menus.BindInput(queue, _ => { }, (_, _) => { });
        var factory = new HostHtmlFactory(menu, windows, menus, startup, screen, queue, strings, () => 1, _ => 0, _ => { });
        router = new HostMenuInput(menu, windows, factory, queue, strings, () => changes++, action => throw new InvalidOperationException(action.ToString()));
        return (screen, menu, router, () => changes);
    }

    /// <summary>세션 커서 장치를 실제 지형 Pop·프로세스와 함께 연결한다.</summary>
    /// <param name="effects">호출자가 미리 묶는 수명 장치. 비어 있으면 세션 실제 장치와 R6 명시 거부로 채운다.</param>
    /// <param name="placement">호출자가 미리 묶는 배치 장치. 비어 있으면 세션 실제 장치와 R6 명시 거부로 채운다.</param>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) CursorDeviceScene(CursorLifecycleHooks? effects = null, CursorPlacementHooks? placement = null)
    {
        var (session, types) = Scene();
        session.BuildPlayerWorldHooks();
        session.BuildCursorDevices(types, effects, placement);
        return (session, types);
    }

    /// <summary>세션 커서 장치는 같은 지형 Pop TYPE 인스턴스에서 한 번만 연결되며 그 전 조회는 거부된다.</summary>
    [Fact]
    public void CursorDevicesRequireSameTerrainTypesAndSingleBinding()
    {
        var (session, types) = Scene();
        session.BuildPlayerWorldHooks();
        Assert.Throws<InvalidOperationException>(() => session.CursorLifecycle);
        Assert.Throws<InvalidOperationException>(() => session.CursorPlacement);
        // 같은 개수라도 다른 인스턴스이면 지형 Pop 장치와 어긋나므로 거부된다.
        Assert.Throws<ArgumentException>(() => session.BuildCursorDevices(types.ToList()));
        session.BuildCursorDevices(types);
        Assert.Same(session.Pool, session.CursorLifecycle.Pool);
        Assert.Throws<InvalidOperationException>(() => session.BuildCursorDevices(types));
    }

    /// <summary>실제 포인터 이동이 같은 세션 커서의 현재 위치에 반영되고 창 밖 이동은 무시된다.</summary>
    [Fact]
    public void PointerMovementReachesSessionCursor()
    {
        var (session, _) = Scene();
        var (_, _, router, _) = CursorInputMenuScene();
        // 제품 진입 경로와 같은 결선으로 실제 포인터 이동을 현재 커서 위치에 반영한다.
        router.CursorPosition = (x, y) => { session.Gameplay.Cursor.ScreenX = x; session.Gameplay.Cursor.ScreenY = y; };
        router.NotifyPointer(new(InputCode.Release, 100, 200));
        Assert.Equal(100, session.Gameplay.Cursor.ScreenX);
        Assert.Equal(200, session.Gameplay.Cursor.ScreenY);
        router.NotifyPointer(new(InputCode.Outside, 5, 6));
        Assert.Equal(100, session.Gameplay.Cursor.ScreenX);
        Assert.Equal(200, session.Gameplay.Cursor.ScreenY);
    }

    /// <summary>실제 Esc 누름이 잡은 커서가 있을 때만 실제 분배를 거쳐 놓는다.</summary>
    [Fact]
    public void EscPressCancelsArmedCursorThroughRealDispatch()
    {
        var events = new List<string>();
        var effects = new CursorLifecycleHooks
        {
            Highlight = value => events.Add("H" + value),
            SelectCursor = value => events.Add("S" + value),
            PrepareCursor = () => events.Add("P"),
            Destroy = (sid, flags) => events.Add($"D{sid},{flags}"),
        };
        var (session, _) = CursorDeviceScene(effects);
        int preview = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(preview).Fill(0);
        // 원문 Cancel의 실제 놓기 분기를 타도록 잠금 없이 미리보기와 0x100 비용 표시를 둔다.
        session.Gameplay.Cursor.Preview = unchecked((uint)preview);
        session.Gameplay.Cursor.LockDepth = 0;
        session.Gameplay.Cursor.CostFlags = 0x100;
        session.Gameplay.Cursor.InputFlags = 0;
        session.Gameplay.CursorGlobals.Lifecycle.PreviewCount = 0;
        var (_, _, router, changes) = CursorInputMenuScene();
        router.CursorCancel = session.TryCancelCursor;
        int before = changes();
        // Esc(누름, 떼지 않음)는 가상 키 0x1b을 위로 민 코드다. 근거: HostInputMapper.KeyCode.
        Assert.False(router.Dispatch(new(0x1B0000, 10, 20)));
        Assert.Equal(0u, session.Gameplay.Cursor.Preview);
        Assert.Equal(1u, session.Gameplay.Cursor.LockDepth);
        Assert.Equal(new[] { "H0", "P", "S1", $"D{preview},0", "P" }, events);
        Assert.True(changes() > before);
    }

    /// <summary>Esc 뗌과 잡은 커서가 없는 Esc 누름은 놓기를 실행하지 않는다.</summary>
    [Fact]
    public void EscReleaseAndUnarmedCursorDoNotCancel()
    {
        var events = new List<string>();
        var effects = new CursorLifecycleHooks
        {
            Highlight = value => events.Add("H" + value),
            SelectCursor = value => events.Add("S" + value),
            PrepareCursor = () => events.Add("P"),
            Destroy = (sid, flags) => events.Add($"D{sid},{flags}"),
        };
        var (session, _) = CursorDeviceScene(effects);
        int preview = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(preview).Fill(0);
        session.Gameplay.Cursor.Preview = unchecked((uint)preview);
        session.Gameplay.Cursor.LockDepth = 0;
        session.Gameplay.Cursor.CostFlags = 0x100;
        session.Gameplay.CursorGlobals.Lifecycle.PreviewCount = 0;
        var (_, _, router, _) = CursorInputMenuScene();
        router.CursorCancel = session.TryCancelCursor;
        // 뗌 사건에는 Release 비트가 붙으며 놓기 분기를 타지 않는다.
        Assert.False(router.Dispatch(new(0x1B0000 | InputCode.Release, 10, 20)));
        Assert.Equal(unchecked((uint)preview), session.Gameplay.Cursor.Preview);
        Assert.Empty(events);
        // 잡은 커서가 없으면 취소 갈고리가 거짓을 돌려주고 종전 흐름을 유지한다.
        session.Gameplay.Cursor.Preview = 0;
        Assert.False(router.Dispatch(new(0x1B0000, 10, 20)));
        Assert.Empty(events);
    }

    /// <summary>실제 Take가 같은 배치 조회에 닿고 Query·Prepare가 같은 배치 본체를 실행한다.</summary>
    [Fact]
    public void TakeAndQueryReachSessionPlacementEntries()
    {
        bool energyAvailable = true; // 같은 정지 위치에서 공급원 완료/소실만 바뀌는 입력이다.
        var calls = new List<string>();
        var invalidated = new List<(ScreenRect Rect, uint Flags)>();
        int preview = 0, source = 0;
        (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types)? scene = null;
        var effects = new CursorLifecycleHooks
        {
            Highlight = value => calls.Add("H" + value),
            SelectCursor = value => calls.Add("S" + value),
            PrepareCursor = () => calls.Add("P"),
            ClosePopups = () => calls.Add("C"),
            CreatePreviews = () =>
            {
                var life = scene!.Value.Session.Gameplay.CursorGlobals.Lifecycle;
                life.Previews[0] = unchecked((uint)preview); life.PreviewCount = 1; calls.Add("V");
            },
            Source = _ => source,
            SourceNotice = (_, _, _) => calls.Add("N"),
            Detach = (_, _) => calls.Add("T"),
            Unpop = (_, _) => calls.Add("U"),
            Destroy = (sid, flags) => calls.Add($"D{sid},{flags}"),
            CreateMana = (_, _) => 7u,
            CreateRange = (_, _) => 9u,
            TypeNumber = _ => 1u,
            HasRange = _ => false,
            Translate = value => value,
            Measure = (_, _) => (0, 0),
            CostImage = (_, _) => new CursorCostImage(9u, 4, 5, 1, 2),
            Mouse = () => (30, 40),
            UpdatePosition = (x, y) => calls.Add($"M{x},{y}"),
        };
        var placement = new CursorPlacementHooks
        {
            Describe = (ref string text, float _, float _, uint _, uint _) => { calls.Add("Qd"); text = energyAvailable ? "ok" : "에너지 부족"; return energyAvailable ? 1u : 0u; },
            Walk = (_, _) => calls.Add("Qw"),
            GumpAt = (_, _) => 0u,
            MayPlace = _ => 1u,
            Project = (_, _) => new ScreenPoint(7, 9),
            Invalidate = (rect, flags) => invalidated.Add((rect, flags)),
        };
        scene = CursorDeviceScene(effects, placement);
        var (session, types) = scene.Value;
        Assert.True(100 < types.Count);
        preview = session.Pool.Allocate(SidAllocation.Client);
        source = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(source).Fill(0);
        // 미리보기 SID의 원문 WORD(타입 18·인자 12·입력 표시 20)를 실제 값으로 채운다.
        Span<byte> raw = session.Pool.AllocatedBytes(preview);
        raw.Fill(0);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[18..], 100);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], 5);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[20..], 0);
        Assert.Throws<InvalidOperationException>(() => session.TakeProductionCursor(preview, 0));
        Assert.Throws<InvalidOperationException>(() => session.CancelProductionCursor());
        // 검사 장면에서만 적재 관문을 열어 실제 커서 수신을 검증한다.
        FieldInfo loaded = typeof(HostSession).GetField("_nativeLoaded", BindingFlags.NonPublic | BindingFlags.Instance)!;
        loaded.SetValue(session, true);
        try { session.TakeProductionCursor(preview, 0); }
        finally { loaded.SetValue(session, false); }
        Assert.Equal(unchecked((uint)preview), session.Gameplay.Cursor.Preview);
        Assert.Equal(100u, session.Gameplay.Cursor.Type);
        // 원문 Take는 Flags2 bit2가 있는 타입에서만 0x200을 덧붙인다.
        uint expectedFlags = (types[100].Flags2 & 4u) != 0 ? 0x200u : 0u;
        Assert.Equal(expectedFlags, session.Gameplay.Cursor.CostFlags);
        // 같은 세션의 기본 QueryCursor가 배치 Query에 닿았는지 순서로 확인한다.
        int describe = calls.IndexOf("Qd"), walk = calls.IndexOf("Qw");
        Assert.True(calls.Contains("C") && calls.Contains("V") && calls.Contains("N"));
        Assert.True(describe >= 0 && walk > describe);
        Assert.Equal("M30,40", calls[^1]);
        Assert.Equal(1u, session.Gameplay.Cursor.Legal);
        // 같은 배치 본체의 직접 조회·준비도 같은 갈고리에서 실행된다.
        session.QueryCursor();
        session.PrepareCursor();
        Assert.True(invalidated.Count >= 1);
        var previews = session.Gameplay.CursorGlobals.Lifecycle.Previews.ToArray(); // 재판정이 다시 만들면 안 되는 미리보기 목록이다.
        var position = (session.Gameplay.Cursor.X, session.Gameplay.Cursor.Y); // 공급원 상태만 바뀌고 포인터는 정지한다.
        energyAvailable = false; Assert.True(session.RefreshCursorPlacement()); Assert.Equal(0u, session.Gameplay.Cursor.Legal);
        energyAvailable = true; Assert.True(session.RefreshCursorPlacement()); Assert.Equal(1u, session.Gameplay.Cursor.Legal);
        Assert.Equal(previews, session.Gameplay.CursorGlobals.Lifecycle.Previews); Assert.Equal(position, (session.Gameplay.Cursor.X, session.Gameplay.Cursor.Y));
        session.Gameplay.Cursor.LockDepth = 1; Assert.False(session.RefreshCursorPlacement());
    }
}
