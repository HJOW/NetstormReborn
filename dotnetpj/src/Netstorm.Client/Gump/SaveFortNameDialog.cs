namespace Netstorm.Client.Gump;

/// <summary>이름 입력 모달의 자식과 확인 모드를 공통 장식 노드에 보관한다.</summary>
public sealed class SaveFortNameDialogNode : GumpDecorationNode
{
    /// <summary>제목과 확인·취소 버튼, 편집 필드의 실제 토큰이다.</summary>
    public uint Title { get; set; }
    /// <summary>같은 창의 Ok 장치 또는 필드다.</summary>
    public uint Ok { get; set; }
    /// <summary>같은 창의 Cancel 장치 또는 필드다.</summary>
    public uint Cancel { get; set; }
    /// <summary>같은 창의 Entry 장치 또는 필드다.</summary>
    public uint Entry { get; set; }
    /// <summary>Fort 저장 여부와 취소 허용 여부다. 숫자 창에서는 FortMode가 대상이다.</summary>
    public uint FortMode { get; set; }
    /// <summary>같은 창의 CanCancel 장치 또는 필드다.</summary>
    public uint CanCancel { get; set; }
}

/// <summary>이름 창이 공유 Gump와 실제 확인 장치를 호출하는 경계다.</summary>
public sealed class SaveFortNameDialogHooks
{
    /// <summary>현재 파생 노드와 기본 노드 및 문자 입력 노드를 찾는다.</summary>
    public Func<uint, SaveFortNameDialogNode?>? Node { get; init; }
    /// <summary>현재 공통 노드를 찾는다.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; init; }
    /// <summary>현재 문자 입력 노드를 찾는다.</summary>
    public Func<uint, TextEntryGumpNode?>? Entry { get; init; }
    /// <summary>요청한 원본 크기의 자식 토큰을 할당한다.</summary>
    public Func<int, uint>? Allocate { get; init; }
    /// <summary>실제 장식과 테두리를 생성한다.</summary>
    public Action<uint, float, float>? Decoration { get; init; }
    /// <summary>같은 창의 Background 장치 또는 필드다.</summary>
    public Action<uint, float, float>? Background { get; init; }
    /// <summary>실제 입력란을 생성한다.</summary>
    public Action<uint, int, int, string>? TextEntry { get; init; }
    /// <summary>입력 금지 문자, 제목 및 돌 버튼을 설정한다.</summary>
    public Action<uint, string>? Excluded { get; init; }
    /// <summary>같은 창의 Title 장치 또는 필드다.</summary>
    public Action<uint, string>? Title { get; init; }
    /// <summary>같은 창의 Stone 장치 또는 필드다.</summary>
    public Action<uint, string>? Stone { get; init; }
    /// <summary>제목과 버튼 크기를 맞추고 자식을 분리한다.</summary>
    public Action<uint>? FitTitle { get; init; }
    /// <summary>같은 창의 FitStone 장치 또는 필드다.</summary>
    public Action<uint>? FitStone { get; init; }
    /// <summary>같은 창의 Push 장치 또는 필드다.</summary>
    public Action<uint>? Push { get; init; }
    /// <summary>공유 트리에 지정 위치와 정렬로 부착한다.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; init; }
    /// <summary>상대 깊이를 더한다.</summary>
    public Action<uint, int>? Depth { get; init; }
    /// <summary>모달 등록과 지연 삭제를 수행한다.</summary>
    public Action<uint>? Modal { get; init; }
    /// <summary>같은 창의 Kill 장치 또는 필드다.</summary>
    public Action<uint>? Kill { get; init; }
    /// <summary>현재 창의 확인 및 취소 결과를 반환한다.</summary>
    public Func<uint, uint>? Accept { get; init; }
    /// <summary>같은 창의 Cancel 장치 또는 필드다.</summary>
    public Func<uint, uint>? Cancel { get; init; }
    /// <summary>처리하지 않은 입력을 기본 부모에 전달한다.</summary>
    public Func<uint, uint, uint, uint, uint>? BaseHandle { get; init; }
    /// <summary>현재 언어의 문자열을 반환한다.</summary>
    public Func<string, string>? Translate { get; init; }
}

/// <summary>원본 SaveFortNameDialog의 생성과 버튼·Enter 처리를 공유 장치에서 실행한다.</summary>
public sealed class SaveFortNameDialog
{
    /// <summary>현재 화면 크기와 루트 등록표다.</summary>
    private readonly GumpCreateState _state;
    /// <summary>동일한 공통 저장소와 하위 장치다.</summary>
    private readonly SaveFortNameDialogHooks _hooks;
    /// <summary>도달한 장치가 없으면 해당 경계를 명시한다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("이름 창 실제 장치 미연결");
    /// <summary>화면 전역과 공유 장치를 빌린다.</summary>
    public SaveFortNameDialog(GumpCreateState state, SaveFortNameDialogHooks hooks) { _state = state; _hooks = hooks; }
    /// <summary>파생 부모를 조회한다.</summary>
    private SaveFortNameDialogNode Node(uint token) => Need(_hooks.Node)(token) ?? throw new InvalidOperationException("이름 창 부모 없음");
    /// <summary>현재 자식의 공통 필드를 조회한다.</summary>
    private GumpCreateNode Base(uint token) => Need(_hooks.Base)(token) ?? throw new InvalidOperationException("이름 창 자식 없음");
    /// <summary>두 실제 돌 버튼을 생성하고 원본 간격으로 배치한다.</summary>
    private void Buttons(uint token, float y)
    {
        var root = Node(token); root.Ok = Need(_hooks.Allocate)(0x170); Need(_hooks.Stone)(root.Ok, Need(_hooks.Translate)("OK"));
        root.Cancel = _hooks.Allocate!(0x170); _hooks.Stone!(root.Cancel, _hooks.Translate!("Cancel"));
        if ((Base(root.Ok).Flags & 8) == 0) Need(_hooks.Push)(root.Ok);
        if ((Base(root.Cancel).Flags & 8) == 0) Need(_hooks.Push)(root.Cancel);
        Need(_hooks.FitStone)(root.Ok); _hooks.FitStone!(root.Cancel);
        float total = Base(root.Cancel).BaseWidth + Base(root.Ok).BaseWidth + 10;
        Need(_hooks.Attach)(root.Ok, (root.BaseWidth - total) * .5f, y, token, 0);
        _hooks.Attach!(root.Cancel, (root.BaseWidth - total) * .5f + Base(root.Ok).BaseWidth + 10, y, token, 0);
    }
    /// <summary>이름 또는 숫자 입력의 공통 장식·입력·테두리·제목을 만든다.</summary>
    private uint Create(uint mode, uint canCancel, string initial, bool money)
    {
        uint token = Need(_hooks.Allocate)(money ? 0xd8 : 0xdc); Need(_hooks.Decoration)(token, 200, 100);
        var root = Node(token); root.Vtable = money ? 0x00517ca0u : 0x00517bc8u; root.Command = money ? 68u : 32u; root.FortMode = mode; root.CanCancel = canCancel;
        root.Entry = _hooks.Allocate!(0x110); Need(_hooks.TextEntry)(root.Entry, (int)(root.BaseWidth * .8f), 18, initial);
        string excluded = money ? new string(Enumerable.Range(1, 255).Where(c => c < '0' || c > '9').Select(c => (char)c).ToArray()) : ".?=()[]~\"`;:^&%$#@!*-+'{}|\\/<>,";
        if (!money) Need(_hooks.Excluded)(root.Entry, excluded);
        var entry = Need(_hooks.Entry)(root.Entry) ?? throw new InvalidOperationException("이름 창 입력 노드 없음"); entry.MaxLength = money ? 10 : 0x28; entry.Multiline = 0;
        if (money) Need(_hooks.Excluded)(root.Entry, excluded);
        Need(_hooks.Attach)(root.Entry, (root.BaseWidth - Base(root.Entry).BaseWidth) * .5f, 45, token, 0);
        uint border = _hooks.Allocate!(0xb8); Need(_hooks.Background)(border, Base(root.Entry).BaseWidth + 8, Base(root.Entry).BaseHeight + 4);
        _hooks.Attach!(border, Base(root.Entry).X - 4, Base(root.Entry).Y - 2, token, 0);
        string title = Need(_hooks.Translate)(money ? "Send X Storm Power" : mode != 0 ? "Save Fortress As?" : "New Save-Game Name?");
        root.Title = _hooks.Allocate!(0xa4); Need(_hooks.Title)(root.Title, title);
        if (!money) Base(root.Title).Flags |= 4;
        _hooks.Attach!(root.Title, Base(root.Entry).X, 10, token, 0); Need(_hooks.FitTitle)(root.Title);
        if (money) { Need(_hooks.Push)(root.Title); Base(root.Title).AlignFlags = 4; _hooks.Attach!(root.Title, Base(root.Title).X, Base(root.Title).Y, token, 2); }
        else { root.BaseWidth = Base(root.Title).BaseWidth * 1.25f; root.Width = root.BaseWidth * root.Scale; }
        Buttons(token, Base(root.Entry).Y + Base(root.Entry).BaseHeight + 15); root.BaseHeight += 4; root.Height = root.BaseHeight * root.Scale;
        if (money) { root.BaseWidth = Math.Max(root.BaseWidth, Base(root.Title).BaseWidth + 32); root.Width = root.BaseWidth * root.Scale; }
        _hooks.Attach!(token, (_state.ScreenWidth - root.BaseWidth) * .5f, (_state.ScreenHeight - root.BaseHeight) * .5f, _state.Entries[0], 0);
        Need(_hooks.Depth)(token, unchecked((int)root.Depth - 76)); Need(_hooks.Modal)(token); return token;
    }
    /// <summary>현재 또는 명시된 이름으로 저장 이름 모달을 연다.</summary>
    public uint Show(uint fortMode, uint canCancel, string current, string? initial = null) => Create(fortMode, canCancel, initial ?? current, false);
    /// <summary>대상을 보유하는 숫자 입력 모달을 연다.</summary>
    public uint ShowMoney(uint target) => Create(target, 1, "0", true);
    /// <summary>확인·취소 버튼 serial과 Enter를 판정하고 성공한 창을 지연 삭제한다.</summary>
    public uint Handle(uint token, uint input, uint first, uint second)
    {
        uint code = input & 0x03ffffff; var root = Node(token);
        if ((code == 0x0200002c && first == Base(root.Ok).Serial) || code == 0x02000038)
        { if (Need(_hooks.Accept)(token) != 0) { Need(_hooks.Kill)(token); return 1; } }
        else { if (code != 0x0200002c || first != Base(root.Cancel).Serial) return Need(_hooks.BaseHandle)(token, input, first, second); if (Need(_hooks.Cancel)(token) != 0) Need(_hooks.Kill)(token); }
        return 1;
    }
}
