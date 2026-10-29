using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 돌 버튼의 생성/트리/수명/입력/글꼴/배경이 같은 노드를 공유하는 장면이다.

/// <summary>cpppj/tests/GumpTextTests.cpp의 StoneScene 조립이다. 창/장치를 만들지 않는다.</summary>
internal sealed class StoneButtonFixture
{
    /// <summary>실제 현재 등록/판본 전역.</summary>
    public GumpCreateState Create { get; } = new() { ScreenWidth = 640, ScreenHeight = 480 };
    /// <summary>실제 수명/idle/삭제 표.</summary>
    public GumpLifetimeState LifeState { get; } = new();
    /// <summary>실제 돌 버튼 생성 전역.</summary>
    public StoneButtonGumpState State { get; } = new() { Color = -7, Palette = new(9, 243, 57) };
    /// <summary>기본 루트 토큰1.</summary>
    public GumpCreateNode Root { get; } = new();
    /// <summary>돌 버튼 토큰2.</summary>
    public StoneButtonGumpNode Node { get; } = new();
    /// <summary>배경 자식 토큰3.</summary>
    public GumpBackgroundNode Background { get; } = new();
    /// <summary>실제 배경/부모 free 순서.</summary>
    public List<uint> Released { get; } = [];
    /// <summary>실제 부모 체인으로 도달한 사건/serial/값.</summary>
    public List<(uint Event, uint Serial, uint Value)> Notifications { get; } = [];
    /// <summary>독립 입력의 배경 할당 실패.</summary>
    public bool FailAllocation { get; set; }
    /// <summary>실제 기본 생성/등록.</summary>
    public GumpCreate Factory { get; }
    /// <summary>실제 위치/수집.</summary>
    public GumpCollection Collection { get; }
    /// <summary>실제 부모/자식 부착.</summary>
    public GumpTree Tree { get; }
    /// <summary>실제 지연 삭제와 자식 우선 파괴.</summary>
    public GumpLifetime Lifetime { get; }
    /// <summary>실제 캡처/입력 경로.</summary>
    public GumpInput Input { get; }
    /// <summary>실제 버튼 상태.</summary>
    public ButtonGump Buttons { get; }
    /// <summary>실제 파생 돌 버튼.</summary>
    public StoneButtonGump Stones { get; }
    /// <summary>측정 콜백을 독립 입력으로 바꿀 수 있는 경계.</summary>
    public StoneButtonGumpHooks StoneHooks { get; }
    /// <summary>같은 실제 기본/파생 노드를 조회한다.</summary>
    private GumpCreateNode? Find(uint token) => token == 1 ? Root : token == 2 ? Node : token == 3 ? Background : null;
    /// <summary>배경+b4의 실제 저장 필드를 반환한다. 원본 null 포인터는 명시 저장소 예외다.</summary>
    private ref uint BackgroundFlags(uint token) { if (token != 3) throw new InvalidOperationException("배경 토큰 없음"); return ref Background.BackgroundFlags; }

    /// <summary>실제 기본 루트부터 생성하고 순환 가상 분배는 실행 시 현재 처리기를 조회한다.</summary>
    public StoneButtonFixture(GumpText text, OriginalEdition edition = OriginalEdition.Patch1078, GumpBackgroundAssets? assets = null)
    {
        Create.Collection.Edition = edition; if (assets is not null) State.BackgroundType = 82; Factory = new(Create, new() { Node = Find, Allocate = size => { Assert.Equal(GumpCreateState.BaseSize, size); return 1; } }); Factory.InitializeRoot(); Collection = new(Create.Collection, Factory.CollectionHooks());
        var branch = new GumpTreeHooks { Node = Find, Redraw = (_, _) => { }, Push = t => Tree!.Push(t), Attach = (t, x, y, p, m) => Tree!.Attach(t, x, y, p, m), Kill = t => Lifetime!.Kill(t) };
        var life = new GumpLifetimeHooks { Node = Find, Push = branch.Push, DeletingDestructor = (t, flags) => Lifetime!.DeleteScalar(t, flags), Release = t => Released.Add(t) }; StoneButtonGump.BindStoneButtonGump(branch, life, t => t == 2 ? Stones : null);
        Tree = new(Create, new() { MapView = new(0, 0, 640, 480) }, branch); Lifetime = new(Create, LifeState, life);
        var e = new GumpEventHooks { Node = Find, Handle = (_, code, a, b) => { Notifications.Add((code, a, b)); return 1; } }; var i = new GumpInputHooks { Node = Find, Handle = e.Handle }; var f = new GumpFrameHooks { Node = Find }; ButtonGump.BindButtonGump(e, i, f, t => t == 2 ? Buttons : null);
        var events = new GumpEvents(Create, LifeState, new(), Collection, e); Input = new(Create, LifeState, new(), Collection, events, i);
        Buttons = new(Create, new() { ValueEvent = 0x12345, ClickEvent = 0x12346, RightEvent = 0x12347, RepeatEvent = 0x12348 }, Factory, Collection, events, Input, Lifetime, new() { Node = t => t == 2 ? Node : null, BaseNode = Find, BackgroundFlags = BackgroundFlags, Redraw = (_, _) => { }, Now = () => 1, Sound = () => { } });
        var bg = new GumpBackgroundCreateHooks { Node = t => t == 3 ? Background : null }; if (assets is not null) GumpBackgroundAssets.BindGumpBackgroundAssets(bg, new(), assets); var backgrounds = new GumpBackgroundCreate(Create, Factory, () => 0, bg);
        StoneHooks = new() { Node = t => t == 2 ? Node : null, Allocate = size => { Assert.Equal(0xb8, size); return FailAllocation ? 0U : 3; }, Push = branch.Push, Attach = branch.Attach }; GumpText.BindStoneButtonText(StoneHooks, text); Stones = new(Create, State, Factory, Buttons, backgrounds, Tree, Lifetime, StoneHooks);
    }
}
