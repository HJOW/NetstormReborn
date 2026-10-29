using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 메뉴 입력을 실제 사건/캡처/프레임/트리/출력/수명에 조립한다.

/// <summary>예상 동작을 계산하지 않는 같은 실제 저장소와 외부 명령/소리 경계.</summary>
internal sealed class MenuInputScene
{
    /// <summary>같은 실제 메뉴 출력/기본 트리.</summary>
    public MenuDrawScene View { get; }
    /// <summary>같은 실제 메뉴/장식/제목/수명.</summary>
    public MenuWidgetScene Ui => View.Ui;
    /// <summary>현재 메뉴 사건 전역.</summary>
    public MenuGumpInputState State { get; } = new() { RequestEvent = 0x12390, ClosedEvent = 0x12391 };
    /// <summary>현재 실제 가상/외부 경계.</summary>
    public MenuGumpInputHooks Hooks { get; }
    /// <summary>실제 메뉴 입력 처리기.</summary>
    public MenuGumpInput Input { get; }
    /// <summary>실제 전체 등록표 통지/idle 처리기.</summary>
    public GumpFrame Frame { get; }
    /// <summary>명령 큐 도달 관측.</summary>
    public List<(uint Command, uint Id, uint Parameter)> Commands { get; } = [];
    /// <summary>소리 파일명/원본 나머지 인자 관측.</summary>
    public List<(string Name, int Loop, int Volume, int Pan, int Priority, int Limit)> Sounds { get; } = [];
    /// <summary>실제 행 변경 영역 관측.</summary>
    public List<ScreenRect> Invalidated { get; } = [];
    /// <summary>명령 콜백이 바꾸는 현재 선택 같은 명시 입력.</summary>
    public Action<uint, uint, uint>? OnCommand { get; set; }
    /// <summary>소리 콜백의 명시 현재 상태 변경.</summary>
    public Action<string>? OnSound { get; set; }
    /// <summary>행 변경 영역 콜백의 명시 현재 상태 변경.</summary>
    public Action<ScreenRect>? OnInvalidate { get; set; }
    /// <summary>상위 파생의 실제 동적 메뉴 요청 응답.</summary>
    public Func<uint, uint, uint, uint, uint>? Request { get; set; }
    /// <summary>같은 실제 사건/캡처/통지 분배와 명령·소리·변경 영역을 연결한다.</summary>
    public MenuInputScene(OriginalEdition edition = OriginalEdition.Patch1078)
    {
        View = new(edition); Hooks = new() { Node = Ui.Menu.MenuHooks.Node, Base = Ui.Menu.MenuHooks.BaseNode, Broadcast = (t, e, a, b) => Ui.Scene.FrameHooks.Broadcast!(t, e, a, b), Handle = (t, e, a, b) => Request?.Invoke(t, e, a, b) ?? Ui.Scene.EventHooks.Handle!(t, e, a, b), Push = t => Ui.Scene.TreeHooks.Push!(t), Attach = (t, x, y, p, m) => Ui.Scene.TreeHooks.Attach!(t, x, y, p, m), Redraw = (t, e) => Ui.Scene.FrameHooks.Redraw!(t, e), Command = (c, id, p) => { Commands.Add((c, id, p)); OnCommand?.Invoke(c, id, p); }, Invalidate = r => { Invalidated.Add(r); OnInvalidate?.Invoke(r); } }; MenuGumpInput.BindMenuGumpInputSound(Hooks, (name, loop, volume, pan, priority, limit) => { Sounds.Add((name, loop, volume, pan, priority, limit)); OnSound?.Invoke(name); }); Input = new(Ui.Scene.Create, State, Ui.Menu.Menu, Ui.Scene.Collection, Ui.Scene.Events, Ui.Scene.Lifetime, Ui.Scene.Tree, Hooks); MenuGumpInput.BindMenuGumpInput(Ui.Scene.EventHooks, Ui.Scene.InputHooks, Ui.Scene.FrameHooks, t => Ui.Scene.Nodes.GetValueOrDefault(t) is MenuGumpNode ? Input : null); Frame = new(Ui.Scene.Create, Ui.Scene.LifeState, Ui.Scene.FrameHooks);
    }
    /// <summary>실제 등록표 통지를 통해 마우스 선택을 갱신한다.</summary>
    public void Motion(int x, int y, uint flags = 0) => Frame.Broadcast(flags, unchecked((uint)x), unchecked((uint)y));
    /// <summary>실제 캡처/기본 사건 전달을 통해 왼쪽 누름을 전달한다.</summary>
    public uint Click(int x = 3, int y = 4, uint flags = 0) => Ui.Scene.Input.Route(0x10000 | flags, unchecked((uint)x), unchecked((uint)y));
}
