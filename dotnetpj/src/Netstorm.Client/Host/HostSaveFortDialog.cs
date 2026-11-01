using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Host;

/// <summary>이름 입력 모달을 실제 공통 Gump·문자·글꼴·모달 장치에 조립한다.</summary>
public sealed class HostSaveFortDialog
{
    /// <summary>현재 메뉴와 같은 노드 저장소다.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>부분 생성 자식까지 보관하는 창 소유자다.</summary>
    private readonly HostHtmlWindows _windows;
    /// <summary>실제 제목 출력 및 소유 조각 처리기다.</summary>
    private readonly TextGump _title;
    /// <summary>같은 창의 원본 배치·입력 본체다.</summary>
    private readonly SaveFortNameDialog _dialog;
    /// <summary>생성 중 현재 제목의 부모를 찾는 실제 창 목록이다.</summary>
    private readonly HashSet<uint> _roots = [];
    /// <summary>실제 모달의 확인 처리를 소유자가 제공한다.</summary>
    private readonly Func<uint, uint> _accept;
    /// <summary>현재 창의 취소가 불가능할 때 실제 안내를 표시한다.</summary>
    private readonly Action<string> _tell;
    /// <summary>같은 화면·문자 편집기·번역 및 확인 장치로 창을 연결한다.</summary>
    public HostSaveFortDialog(HostMainMenu menu, HostHtmlWindows windows, HostHtmlText text, HostTextEntry entry,
        HostStartupScene startup, Screen screen, Func<uint, uint> accept, Action<string> tell)
    {
        _tree = menu.Tree; _windows = windows; _accept = accept; _tell = tell;
        var palette = screen.Colors();
        string language = startup.Configuration.Configuration.Get("currentLanguage") ?? "english";
        byte[]? bytes = startup.Files.TryReadAllBytes("d/xlat." + language);
        var translation = bytes == null ? null : XlatTable.FromFileBytes(bytes);
        _title = new TextGump(_tree.Create, new(), _tree.Factory, _tree.Lifetime, _tree.Collection, text.Clip, text.Style,
            new() { Node = t => _tree.Node(t) as TextGumpNode, BaseNode = _tree.Node });
        TextGump.BindTextGumpLifetime(_tree.LifetimeHooks, t => _tree.Node(t) is TextGumpNode ? _title : null);
        var previousEvent = _tree.EventHooks.Handle; var previousInput = _tree.InputHooks.Handle;
        var hooks = new SaveFortNameDialogHooks
        {
            Node = t => _tree.Node(t) as SaveFortNameDialogNode, Base = _tree.Node, Entry = t => _tree.Node(t) as TextEntryGumpNode,
            Allocate = size => size == 0x110 ? entry.AllocateEntry() : Allocate(size),
            Decoration = (t, w, h) => menu.Decoration.Construct(t, w, h),
            TextEntry = (t, w, h, value) => entry.Entry.Construct(t, w, h, Encoding.Latin1.GetBytes(value), 0, 0, palette.Basic[5]),
            Excluded = (t, value) => entry.Entry.SetExcluded(t, Encoding.Latin1.GetBytes(value)),
            Background = (t, w, h) => _tree.Backgrounds.Construct(t, w, h, 0x03e40000, 1, menu.BackgroundType, 0),
            Title = (t, value) =>
            {
                uint parent = _roots.Single(root => (_tree.Node(root) as SaveFortNameDialogNode)?.Title == t);
                int width = (int)((_tree.Node(parent) ?? throw new InvalidOperationException("제목 부모 없음")).BaseWidth * .8f);
                _title.Construct(t, width, 20, 0, 0); byte[] label = Encoding.Latin1.GetBytes(value);
                _title.AppendText(t, label, [2, 3, palette.Basic[7], 0, 0, 0, 1, 1], label.Length);
            },
            Stone = (t, value) => _tree.Stones.Construct(t, 70, 18, Encoding.Latin1.GetBytes(value + '\0'), palette.Named[8].Index),
            FitTitle = t => _title.Fit(t), FitStone = _tree.Stones.FitToText, Push = _tree.Tree.Push,
            Attach = (t, x, y, parent, mode) => _tree.TreeHooks.Attach!(t, x, y, parent, mode),
            Depth = (t, value) => _tree.Tree.AddDepth(t, unchecked((uint)value)), Modal = t => _tree.Modal.SetModal(t, true),
            Kill = _tree.Lifetime.Kill, Accept = t => _accept(t),
            Cancel = t => SaveFortName.Cancel(((SaveFortNameDialogNode)_tree.Node(t)!).CanCancel, _tell),
            BaseHandle = (t, e, a, b) => previousEvent!(t, e, a, b), Translate = value => translation?.Translate(value) ?? value
        };
        _dialog = new SaveFortNameDialog(_tree.Create, hooks);
        _tree.EventHooks.Handle = (t, e, a, b) => _tree.Node(t) is SaveFortNameDialogNode ? _dialog.Handle(t, e, a, b) : previousEvent!(t, e, a, b);
        _tree.InputHooks.Handle = (t, e, a, b) => _tree.Node(t) is SaveFortNameDialogNode ? _dialog.Handle(t, e, a, b) : previousInput!(t, e, a, b);
        Action<uint> release = _tree.LifetimeHooks.Release!;
        _tree.LifetimeHooks.Release = t => { release(t); _roots.Remove(t); };
    }
    /// <summary>원본 요청 크기의 부모·제목·테두리·버튼을 공통 저장소에 할당한다.</summary>
    private uint Allocate(int size)
    {
        GumpCreateNode node = size switch { 0xdc or 0xd8 => new SaveFortNameDialogNode(), 0xa4 => new TextGumpNode(),
            0xb8 => new GumpBackgroundNode(), 0x170 => new StoneButtonGumpNode(), _ => throw new ArgumentOutOfRangeException(nameof(size)) };
        uint token = _tree.AllocateNode(node); _windows.TrackChild(token); if (node is SaveFortNameDialogNode) _roots.Add(token); return token;
    }
    /// <summary>현재 Fort 이름으로 이름 모달을 열고 실제 부모 토큰을 돌려준다.</summary>
    public uint Show(uint mode, uint canCancel, string current) => _dialog.Show(mode, canCancel, current);
}
