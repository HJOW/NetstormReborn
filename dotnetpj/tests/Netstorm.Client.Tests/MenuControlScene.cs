using Netstorm.Assets;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 메뉴와 실제 제목 위젯이 같은 트리·소유 조각·픽셀을 쓰는 검사 입력이다.

/// <summary>원본 기대값을 생성하지 않는 실제 조립.설정 읽기/현재 칸 변경은 명시 입력이다.</summary>
internal sealed class MenuControlScene
{
    /// <summary>같은 실제 다중 줄 제목/버퍼/Slider/트리.</summary>
    public ScrollingWidgetScene Title { get; }
    /// <summary>같은 기본 트리/소유 조각을 공유하는 메뉴.</summary>
    public MenuLayoutScene Menu { get; }
    /// <summary>기존 실제 설정 층/치환.</summary>
    public ConfigStore Config { get; } = new();
    /// <summary>현재 메뉴/설정 읽기.</summary>
    public MenuGumpControlHooks Hooks { get; }
    /// <summary>같은 대괄호 처리기.</summary>
    public StyleTextMarkup Markup { get; }
    /// <summary>실제 controlled 메뉴 작성기.</summary>
    public MenuGumpControls Controls { get; }
    /// <summary>실제 설정 조회 순서.</summary>
    public List<string> Reads { get; } = [];
    /// <summary>실제 대괄호 적용 직후 문맥.뒤의 짧은~E가 다시 쓰기 전 상태를 관측한다.</summary>
    public List<(string Command, uint[] Context)> Brackets { get; } = [];
    /// <summary>기존 생성자/문맥/팔레트/설정/파서를 같은 실제 노드에 연결한다.</summary>
    public MenuControlScene(OriginalEdition edition = OriginalEdition.Patch1078)
    {
        Title = new(edition, true); Title.Construct(40, 40); Menu = new(edition, Title); Title.Node.Metadata = 9;
        var pictures = new Markup(Menu.Fragments, new(), new(), edition); Markup = new(Menu.Fragments, pictures, new(), new() { Black = () => Menu.Palette.Basic[0] }, edition); Markup.Hooks(Menu.StyleHooks);
        var bracket = Menu.StyleHooks.Bracket; Menu.StyleHooks.Bracket = (ReadOnlySpan<byte> command, uint[] context, ref uint head, ref uint metadata) => { bracket!(command, context, ref head, ref metadata); Brackets.Add((System.Text.Encoding.Latin1.GetString(command), (uint[])context.Clone())); };
        var parser = new ScrollingTextGumpMarkup(Title.Text, Menu.Style, pictures, Markup, new() { Node = t => Title.Scene.Nodes.GetValueOrDefault(t) as ScrollingTextGumpNode, Base = t => Title.Scene.Nodes.GetValueOrDefault(t) });
        Config.Push(new("MenuColor=123\nMenuBGColor=124\nMenuCheckedColor=131\nMenuCheckedBGColor=132\nMenuTitleColor=151\nMenuTitleBGColor=152\nMenuHeaderColor=161\nMenuHeaderBGColor=162\nMenuSubColor=141\nMenuSubBGColor=142\n")); Hooks = new() { Node = Menu.MenuHooks.Node, Base = Menu.MenuHooks.BaseNode }; MenuGumpControls.BindMenuGumpControlConfig(Hooks, Config); var read = Hooks.ReadInt; Hooks.ReadInt = (string key, ref int value) => { Reads.Add(key); read!(key, ref value); }; Controls = new(Title.Scene.Create, Menu.State, Menu.Menu, Menu.Layout, parser, Hooks);
    }
    /// <summary>같은 실제 기본 생성/등록을 거친 추가 메뉴.검사 뒤에도 객체 주소를 유지한다.</summary>
    public MenuGumpNode AddMenu(uint token)
    { var node = new MenuGumpNode { MenuFlags = 0x80000 }; Title.Scene.Nodes.Add(token, node); Title.Scene.Factory.Construct(token, 0, 0, 0, 1); Menu.Node.TextContext.CopyTo(node.TextContext, 0); return node; }
    /// <summary>실제 소유 체인의 비어 있지 않은 글자 조각만 관측한다.</summary>
    public StyleTextFragment[] ItemText(MenuGumpNode node, int index) => StyleTextTestSupport.Chain(Menu.Fragments, node.Items[index].Styled).Select(t => Menu.Fragments.Fragments[t]).Where(f => StyleTextTestSupport.Text(f).Length != 0).ToArray();
}
