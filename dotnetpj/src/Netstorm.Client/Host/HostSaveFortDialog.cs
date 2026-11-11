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
    /// <summary>실제 편집 입력의 전체 본문을 읽는 문자 소유자다.</summary>
    private readonly HostTextEntry _entries;
    /// <summary>토큰별 Save as 시나리오·오류 문맥이다. 일반 창은 항목이 없다.</summary>
    private readonly Dictionary<uint, SaveFortDialogContext> _scenarios = [];
    /// <summary>Scenario 다중 줄 Enter의 원문 사건 코드다.</summary>
    private const uint EnterEvent = 0x02000038u;
    /// <summary>같은 화면·문자 편집기·번역 및 확인 장치로 창을 연결한다.</summary>
    public HostSaveFortDialog(HostMainMenu menu, HostHtmlWindows windows, HostHtmlText text, HostTextEntry entry,
        HostStartupScene startup, Screen screen, Func<uint, uint> accept, Action<string> tell)
    {
        _tree = menu.Tree; _windows = windows; _accept = accept; _tell = tell; _entries = entry;
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
            Kill = _tree.Lifetime.Kill, Accept = AcceptWithScenario,
            Cancel = CancelWithScenario,
            BaseHandle = (t, e, a, b) => previousEvent!(t, e, a, b), Translate = value => translation?.Translate(value) ?? value
        };
        _dialog = new SaveFortNameDialog(_tree.Create, hooks);
        _tree.EventHooks.Handle = (t, e, a, b) => IsScenarioEnter(t, e) ? 1 : (_tree.Node(t) is SaveFortNameDialogNode ? _dialog.Handle(t, e, a, b) : previousEvent!(t, e, a, b));
        _tree.InputHooks.Handle = (t, e, a, b) => IsScenarioEnter(t, e) ? 1 : (_tree.Node(t) is SaveFortNameDialogNode ? _dialog.Handle(t, e, a, b) : previousInput!(t, e, a, b));
        Action<uint> release = _tree.LifetimeHooks.Release!;
        _tree.LifetimeHooks.Release = t => { release(t); _roots.Remove(t); _scenarios.Remove(t); };
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

    /// <summary>토큰에 시나리오·오류 문맥을 묶는다. R7 Edit 조립에서 Scenario 입력창 생성 뒤 호출한다.</summary>
    /// <param name="token">이름 모달 부모 토큰이다.</param>
    /// <param name="context">시나리오 확정·취소·오류 문맥이다.</param>
    public void BindScenario(uint token, SaveFortDialogContext context)
    {
        ArgumentNullException.ThrowIfNull(context);
        _scenarios[token] = context;
    }

    /// <summary>Scenario 다중 줄 Enter인지 판정한다. 줄바꿈만 하며 실제 OK 버튼에서 확정한다.</summary>
    /// <param name="token">사건 대상 토큰이다.</param>
    /// <param name="code">원문 사건 코드다.</param>
    /// <returns>삼켜야 하는 Scenario Enter이면 참이다.</returns>
    private bool IsScenarioEnter(uint token, uint code)
    {
        return _tree.Node(token) is SaveFortNameDialogNode
            && _scenarios.TryGetValue(token, out SaveFortDialogContext? scenario)
            && scenario.EditorAccept != null
            && scenario.EditorMultiline
            && (code & 0x03ffffffu) == EnterEvent;
    }

    /// <summary>현재 창의 시나리오 확정 또는 일반 확인을 실행한다.</summary>
    /// <param name="token">이름 모달 부모 토큰이다.</param>
    /// <returns>원문 확정 결과다.</returns>
    private uint AcceptWithScenario(uint token)
    {
        // 시나리오 전용 확정이 있으면 실제 버퍼 본문으로 확정한다.
        if (_scenarios.TryGetValue(token, out SaveFortDialogContext? scenario) && scenario.EditorAccept != null)
        {
            var root = _tree.Node(token) as SaveFortNameDialogNode ?? throw new InvalidOperationException("Fort 이름 창 없음");
            string text = _entries.ReadEntryText(root.Entry);
            return scenario.EditorAccept(text) ? 1u : 0u;
        }

        // 일반 확인은 파일 실패 안내가 있으면 같은 창에 띄우고 0을 돌려준다.
        return SaveFortDialogContext.AcceptWithReport(scenario, () => _accept(token));
    }

    /// <summary>현재 창의 시나리오 취소 또는 일반 취소를 실행한다.</summary>
    /// <param name="token">이름 모달 부모 토큰이다.</param>
    /// <returns>원문 취소 결과다.</returns>
    private uint CancelWithScenario(uint token)
    {
        // 시나리오 취소는 보류 문서를 버리지 않고 현재 필드만 취소한다.
        if (SaveFortDialogContext.TryCancelScenario(_scenarios.GetValueOrDefault(token), out uint cancelled))
        {
            return cancelled;
        }

        var root = (SaveFortNameDialogNode)_tree.Node(token)!;
        return SaveFortName.Cancel(root.CanCancel, _tell);
    }
}
