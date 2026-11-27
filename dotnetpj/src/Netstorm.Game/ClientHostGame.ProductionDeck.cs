// 미션 HUD 왼쪽 생산 덱 틀을 실제 세션과 같은 트리에 부착한다.
using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Host;

// R5-4c2 생산 덱 부착용 실제 호스트 장치다.
public sealed partial class ClientHostGame
{
    /// <summary>미션에 부착한 실제 생산 패널이다. 미션이 없으면 null이다.</summary>
    private HostProductionWindows? _missionProduction;
    /// <summary>미션 생산 패널이 공유하는 실제 장기 전역이다. 미션보다 오래 산다.</summary>
    private readonly ProductionMaintenanceGlobals _productionGlobals = new();
    /// <summary>생산 우클릭 About 메뉴의 실제 현재 본문/사건/주제다.</summary>
    private readonly GumpHelpState _productionHelpState = new();
    /// <summary>미션 생산 패널의 실제 우클릭 도움말 처리기다. 미션 진입 때 만든다.</summary>
    private GumpHelp? _productionHelp;
    /// <summary>실제 UI 등록표와 장기 GumpForm 통지 대여다.</summary>
    private HostGumpFormRuntime? _gumpFormRuntime;

    /// <summary>생산 파생의 부모 소실은 해당 패널에, 그 밖의 노드는 공통 수명에 전달한다.</summary>
    private void InitializeNativeGumpForms()
    {
        var tree = (_mainMenu ?? throw new InvalidOperationException("Gump Form의 실제 메뉴 트리 없음")).Tree;
        _gumpFormRuntime = new HostGumpFormRuntime(_surfaceGlobals.Storage, tree, (token, parent) =>
        {
            if (_missionProduction?.Node(token) is null) return false;
            _missionProduction.ParentGone(token, unchecked((uint)parent));
            return true;
        }, () => _menuDirty = true);
    }

    /// <summary>미션 진입에 생산 덱 틀을 만들고 실제 HUD 왼쪽에 부착해 그린다.</summary>
    /// <remarks>2026-11-26 1.079에서 R5-4c2용으로 추가했다. 근거: cpppj RawProductionControls.cpp 184~186행의 예산/격자/배경 생성과 UberRootGump.cpp 86행의 생산 공장 부착, HudDock.cpp의 왼쪽 도킹이다. 격자 내용물 populatation·유지·입력·음향·동기화 본체는 R6/R11에서 잇는다. DN-MP-01 우클릭 메뉴 표시는 이 틀의 HELP 포인터로 검증한다.</remarks>
    private void EnsureMissionProductionDeck()
    {
        if (_missionProduction is not null)
        {
            int height = _driver.Screen.Height;
            if (height > 80)
            {
                _missionProduction.Resize(height);
            }

            return;
        }

        var menu = _mainMenu ?? throw new InvalidOperationException("생산 덱의 실제 공통 트리 없음");
        var text = _htmlText ?? throw new InvalidOperationException("생산 덱의 실제 문자 owner 없음");
        var menus = _htmlMenus ?? throw new InvalidOperationException("생산 덱의 실제 메뉴 열 없음");
        var startup = _startup ?? throw new InvalidOperationException("생산 덱의 실제 시작 장면 없음");
        var tree = menu.Tree;
        var context = NativeProductionContext;
        _productionGlobals.BackgroundType = menu.BackgroundType;
        var help = new GumpHelp(tree.Create, _productionHelpState, new GumpHelpHooks
        {
            Node = tree.Node,
            Text = token => tree.ReadOwnedText(token),
            BeginMenu = _ => menus.Widgets.BeginVertical(0),
            Title = token => menus.Controls.AddTitle(Encoding.Latin1.GetBytes(tree.ReadOwnedText(token))),
            Entry = (_, _, topic, _, _, _) => menus.Controls.AddText(Encoding.Latin1.GetBytes(topic), 0, 0),
            AttachMenu = (x, y, parent) => { uint current = menus.Menu.TakeCurrent(); menus.Widgets.Attach(current, x, y, parent, 0); _menuDirty = true; },
            Sound = name => _buttonSound?.PlayByName(name),
        });
        _productionHelp = help;
        var devices = new HostProductionDevices
        {
            Now = () => _clock.NowSeconds,
            Sound = name => _buttonSound?.PlayByName(name),
            Tell = target => _menuInput?.ShowTell(target),
            ConfigInt = (key, fallback) => { int value = fallback; startup.Configuration.ReadInt(key, ref value); return value; },
            Translate = value => TranslateOuter(value),
            Changed = () => _menuDirty = true,
            BaseHandle = (token, code, first, second) => help.Handle(token, code, first, second),
            Redraw = (_, _) => _menuDirty = true,
            ClosePopups = () => _htmlMenus?.RequestClosePopups(() => _menuDirty = true),
            MenuInstances = () => _htmlMenus?.State.Instances ?? throw new InvalidOperationException("생산 구분선 실제 메뉴 수 미연결"),
            BindForm = (_, _) => throw new InvalidOperationException("생산 Form 실제 바인딩은 R6/R11에서 잇는다"),
            Synchronize = () => throw new InvalidOperationException("생산 동기화 실제 본체는 R6에서 잇는다"),
            UpdateContents = () => throw new InvalidOperationException("생산 내용물 실제 갱신은 R6에서 잇는다"),
            Maintenance = (_, _) => throw new InvalidOperationException("생산 유지 실제 본체는 R6에서 잇는다"),
            CancelCursor = () => throw new InvalidOperationException("생산 커서 실제 취소는 R6 입력에서 잇는다"),
            Take = (_, _) => throw new InvalidOperationException("생산 내용물 실제 취득은 R6 입력에서 잇는다"),
            About = (_, _, _) => throw new InvalidOperationException("생산 항목 실제 정보 메뉴는 R6에서 잇는다"),
            Poll = _ => throw new InvalidOperationException("생산 구분선 실제 폴링은 R5-5/R6 입력에서 잇는다"),
            Cursor = _ => throw new InvalidOperationException("생산 구분선 실제 커서는 R5-5/R6 입력에서 잇는다"),
        };
        var panel = new HostProductionWindows(tree, text, context, _productionGlobals, devices);
        uint token = panel.Construct();
        // R6에서 실제 내용물 갱신·유지를 연결하기 전까지 루트 유지 Idle을 끈다. 예산 Idle은 그대로 둬 SP 표시를 갱신한다.
        // 근거: 루트 Idle은 Technology가 0이 아닐 때 UpdateContents·Maintenance를 요구하는데, 두 본체는 R6/R11에서 잇는다.
        tree.Lifetime.SetIdle(token, false);
        panel.SetPalette(_driver.Screen.LogicalPalette);
        var root = panel.Node(token) as ProductionRootNode ?? throw new InvalidOperationException("생산 덱 실제 부모 없음");
        root.BaseWidth = root.Width = 80;
        float deckHeight = _driver.Screen.Height - 80;
        root.BaseHeight = root.Height = deckHeight;
        panel.Attach(token, 0, 0, tree.Create.Entries[0], 0);
        panel.Idle(root.Budget);
        _missionProduction = panel;
        _menuDirty = true;
    }

    /// <summary>공통 xlat 표의 같은 현재 언어 번역이다. 없으면 원문을 보존한다.</summary>
    private string TranslateOuter(string value)
    {
        var startup = _startup ?? throw new InvalidOperationException("번역의 실제 시작 장면 없음");
        byte[]? bytes = startup.Files.TryReadAllBytes("d/xlat." + (startup.Configuration.Configuration.Get("currentLanguage") ?? "english"));
        var table = bytes is null ? null : Netstorm.Assets.XlatTable.FromFileBytes(bytes);
        return table?.Translate(value) ?? value;
    }
}
