using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 맵 뷰어와 게임 세션(<see cref="BattleSession"/>)의 연결: 규칙 상태는 모두 세션이 갖고, 뷰어는 입력을 명령으로 넣고
/// 세션이 진행한 결과(오브젝트·다리·Storm Power·이벤트)를 그린다. 세션은 배치 시험(P)·다리 시험(B) 모드가 켜져 있거나
/// 미션(<c>--mission</c>)으로 시작했을 때만 시간이 흐른다 — 맵만 볼 때 저장된 다리가 무너지지 않도록.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>시험·미션에서 조작하는 플레이어 번호</summary>
    private const int TestPlayer = 1;

    /// <summary>저장된 Money 섹션이 없을 때 쓰는 시작 Storm Power (미션 없이 맵만 열었을 때)</summary>
    private const int FallbackStormPower = 5000;

    /// <summary>게임 시각 표시를 넣을 오른쪽 위 상자의 폭</summary>
    private const int HudWidth = 420;

    /// <summary>규칙 상태·틱 루프를 가진 게임 세션</summary>
    private BattleSession _session = null!;

    /// <summary>미션 시작 조건으로 연 경우의 미션 (맵만 연 경우 null)</summary>
    private MissionStart? _mission;

    /// <summary>일시정지 (Space)</summary>
    private bool _simulationPaused;

    /// <summary>최근 세션 이벤트 문구 (배치 성공·거부·건설 완료 등)</summary>
    private string _notice = "";

    /// <summary>세션 시간이 흐르고 있는지: 일시정지가 아니고, 미션 또는 배치·다리·전투 시험이 켜져 있다.</summary>
    private bool SimulationRunning => !_simulationPaused && (_mission != null || _placementMode || _bridgeMode || _session.CombatEnabled);

    /// <summary>
    /// 맵에서 게임 세션을 만든다. 미션이 있으면 시작 Storm Power·지식·전투 옵션이 미션 값이고 생산 규칙(기술 허용 표·덱·재충전·회수 금지)을
    /// 켠다. 맵만 열면 시험 모드라 생산 규칙을 끄고 저장된 Money 로 시작한다.
    /// </summary>
    private void InitializeSession(FortFile fort, TypeCatalog catalog, MissionStart? mission)
    {
        _mission = mission;
        int? startStormPower = fort.Money is float money && money > 0 ? (int)money : FallbackStormPower;
        _session = BattleSessionFactory.Create(_map, _terrain.IslandCells, _edgeFarmCells, catalog, mission, TestPlayer, startStormPower);
        _session.EnforceProductionRules = mission != null;
        _session.CombatEnabled = mission != null;
    }

    /// <summary>
    /// 세션 입력과 시간 진행: Space 일시정지, K 생산 규칙 켜기/끄기, T 커서 칸의 오브젝트 선택/해제,
    /// 흐른 시간만큼 틱 진행, 이벤트 알림 갱신.
    /// </summary>
    /// <param name="seconds">지난 갱신 이후 경과 시간(초)</param>
    /// <param name="keyboard">현재 키 상태</param>
    /// <param name="mouse">현재 마우스 상태 (논리 화면 좌표)</param>
    private void UpdateSession(double seconds, KeyboardState keyboard, MouseState mouse)
    {
        if (Pressed(keyboard, Keys.Space))
        {
            _simulationPaused = !_simulationPaused;
        }
        if (!_playUi && Pressed(keyboard, Keys.K))
        {
            _session.EnforceProductionRules = !_session.EnforceProductionRules;
        }
        if (!_playUi && Pressed(keyboard, Keys.F3))
        {
            _session.CombatEnabled = !_session.CombatEnabled;
            _notice = _session.CombatEnabled ? "포대 전투 켜짐" : "포대 전투 꺼짐";
        }
        if (Pressed(keyboard, Keys.T) && mouse.Y >= HeaderHeight)
        {
            // 커서 칸에 오브젝트가 있으면 선택하고, 없으면 선택을 푼다 (튜토리얼 단계 C·F 가 선택한 템플을 본다)
            (int selectX, int selectY) = CellAt(new Vector2(mouse.X, mouse.Y));
            SubmitCommand(new SelectEntityCommand(TestPlayer, _session.EntityAt(selectX, selectY)?.Id ?? 0));
        }
        if (Pressed(keyboard, Keys.H) && mouse.Y >= HeaderHeight)
        {
            // 개발용 수확 입력: 커서가 가리키는 가이저로 플레이어 사제를 보낸다.
            (int harvestX, int harvestY) = CellAt(new Vector2(mouse.X, mouse.Y));
            SubmitCommand(new HarvestGeyserCommand(TestPlayer, _session.EntityAt(harvestX, harvestY)?.Id ?? 0));
        }
        if (!_playUi) UpdateSacrificeInput(keyboard, mouse);
        if (SimulationRunning)
        {
            _session.Advance(seconds);
        }
        // 세션이 알린 일을 알림 문구로 옮긴다 (1초마다 생기는 다리 조각 알림은 칸 패널에 보이므로 뺀다)
        foreach (SessionEvent sessionEvent in _session.DrainEvents())
        {
            QueueEventSound(sessionEvent);
            if (sessionEvent.Kind != SessionEventKind.BridgePieceAdded)
            {
                _notice = DescribeEvent(sessionEvent);
                if (sessionEvent.Kind is SessionEventKind.TutorialTell or SessionEventKind.MissionTell)
                {
                    OpenTutorialTell(sessionEvent.Text);
                }
            }
        }
    }

    /// <summary>
    /// 명령을 세션에 넣는다. 세션이 멈춰 있어도(일시정지 등) 눌러 본 결과가 보이도록 한 틱만 곧바로 진행한다.
    /// </summary>
    /// <param name="command">명령</param>
    private void SubmitCommand(GameCommand command)
    {
        _session.Submit(command);
        if (!SimulationRunning)
        {
            _session.RunTicks(1);
        }
    }

    /// <summary>
    /// 검증용 명령 스크립트 (--script): ';' 로 나눈 명령을 차례로 실행하고, 명령마다 세션이 알린 일을 콘솔에 출력한다.
    /// <list type="bullet">
    /// <item><description><c>construct 타입 x,y</c> — 사제가 건물을 짓기 시작 · <c>place 타입 x,y</c> — 유닛 배치 · <c>register 타입</c> — 워크샵에 지식 등록 · <c>salvage x,y</c> — 그 칸의 내 오브젝트 회수</description></item>
    /// <item><description><c>wait 초</c> — 게임 시간을 진행 · <c>rules 0|1</c> — 생산 규칙 끄기/켜기</description></item>
    /// <item><description><c>select x,y</c> — 그 칸의 오브젝트 선택 (없으면 선택 해제) · <c>select none</c> — 선택 해제 (튜토리얼 단계 C·F 는 선택한 템플을 본다)</description></item>
    /// <item><description><c>harvest x,y</c> — 그 칸의 가이저로 사제를 보내 결정을 반복 수확한다 · <c>home</c> — F4 화면 복귀</description></item>
    /// <item><description><c>capture tx,ty px,py [ax,ay]</c> — 수송 유닛을 사제에게 보내고 선택적으로 제단까지 운반 · <c>deliver tx,ty ax,ay</c> — 운반 사제를 제단에 묶기 · <c>altar ax,ay</c> — 내 사제를 제단으로 이동 · <c>drop tx,ty x,y</c> — 지정 칸에 사제 내려놓기</description></item>
    /// <item><description>단계 처리가 없는 미션에서 손으로 재현: <c>allow 타입</c>·<c>deny 타입</c> — 기술 허용 표 변경, <c>denysalvage 0|1</c> — 회수 금지 변경. 튜토리얼 2 는 세션의 단계 처리(<c>TutorialStages</c>)가 자동으로 바꾼다 (docs/exe/mission-header-flags.md)</description></item>
    /// </list>
    /// </summary>
    /// <param name="script">명령 스크립트</param>
    public void RunScript(string script)
    {
        // 명령을 하나씩 해석해 실행한다
        foreach (string raw in script.Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
        {
            string[] words = raw.Split(' ', StringSplitOptions.RemoveEmptyEntries);
            string verb = words[0].ToLowerInvariant();
            string Word(int index) => index < words.Length ? words[index]
                : throw new ArgumentException($"--script 명령에 값이 모자랍니다: {raw}");
            switch (verb)
            {
                case "wait":
                    _session.RunTicks((int)Math.Round(double.Parse(Word(1), System.Globalization.CultureInfo.InvariantCulture) * _session.TicksPerSecond));
                    break;
                case "construct":
                    (int cx, int cy) = ParseCell(Word(2));
                    SubmitCommand(new ConstructBuildingCommand(TestPlayer, Word(1), cx, cy));
                    break;
                case "place":
                    (int px, int py) = ParseCell(Word(2));
                    SubmitCommand(new PlaceUnitCommand(TestPlayer, Word(1), px, py));
                    break;
                case "register":
                    RegisterSelectedUnit(_candidates.FirstOrDefault(t => t.Name.Equals(Word(1), StringComparison.OrdinalIgnoreCase))
                        ?? throw new ArgumentException($"알 수 없는 타입입니다: {Word(1)}"));
                    break;
                case "salvage":
                    (int sx, int sy) = ParseCell(Word(1));
                    SalvageAt(sx, sy);
                    break;
                case "select":
                    // "none" 이거나 오브젝트가 없는 칸이면 선택을 푼다
                    int selectedId = 0;
                    if (!Word(1).Equals("none", StringComparison.OrdinalIgnoreCase))
                    {
                        (int ex, int ey) = ParseCell(Word(1));
                        selectedId = _session.EntityAt(ex, ey)?.Id ?? 0;
                    }
                    SubmitCommand(new SelectEntityCommand(TestPlayer, selectedId));
                    break;
                case "harvest":
                    (int hx, int hy) = ParseCell(Word(1));
                    SubmitCommand(new HarvestGeyserCommand(TestPlayer, _session.EntityAt(hx, hy)?.Id ?? 0));
                    break;
                case "capture":
                    (int tx, int ty) = ParseCell(Word(1));
                    (int captureX, int captureY) = ParseCell(Word(2));
                    GameEntity transport = _session.EntityAt(tx, ty) ?? throw new ArgumentException("capture 첫 좌표에 수송 유닛이 없습니다.");
                    GameEntity captive = _session.EntityAt(captureX, captureY) ?? throw new ArgumentException("capture 두 번째 좌표에 사제가 없습니다.");
                    int altarId = 0;
                    if (words.Length > 3)
                    {
                        (int ax, int ay) = ParseCell(Word(3));
                        altarId = _session.EntityAt(ax, ay)?.Id ?? throw new ArgumentException("capture 제단 좌표에 제단이 없습니다.");
                    }
                    SubmitCommand(new CapturePriestCommand(TestPlayer, transport.Id, captive.Id, altarId));
                    break;
                case "deliver":
                    (int dtx, int dty) = ParseCell(Word(1));
                    (int dax, int day) = ParseCell(Word(2));
                    GameEntity delivering = _session.EntityAt(dtx, dty) ?? throw new ArgumentException("deliver 좌표에 수송 유닛이 없습니다.");
                    GameEntity destinationAltar = _session.EntityAt(dax, day) ?? throw new ArgumentException("deliver 좌표에 제단이 없습니다.");
                    SubmitCommand(new DeliverPriestCommand(TestPlayer, delivering.Id, destinationAltar.Id));
                    break;
                case "altar":
                    (int max, int may) = ParseCell(Word(1));
                    GameEntity targetAltar = _session.EntityAt(max, may) ?? throw new ArgumentException("altar 좌표에 제단이 없습니다.");
                    SubmitCommand(new MovePriestToAltarCommand(TestPlayer, targetAltar.Id));
                    break;
                case "drop":
                    (int dpx, int dpy) = ParseCell(Word(1));
                    (int dx, int dy) = ParseCell(Word(2));
                    GameEntity dropping = _session.EntityAt(dpx, dpy) ?? throw new ArgumentException("drop 첫 좌표에 수송 유닛이 없습니다.");
                    SubmitCommand(new DropPriestCommand(TestPlayer, dropping.Id, dx, dy));
                    break;
                case "home":
                    SubmitCommand(new ReturnHomeCommand(TestPlayer));
                    break;
                case "rules":
                    _session.EnforceProductionRules = Word(1) != "0";
                    break;
                case "combat":
                    _session.CombatEnabled = Word(1) != "0";
                    break;
                case "allow":
                case "deny":
                    _session.Player(TestPlayer).Tech.Set(Word(1), verb == "allow");
                    break;
                case "denysalvage":
                    _session.DenySalvage = Word(1) != "0";
                    break;
                default:
                    throw new ArgumentException($"알 수 없는 --script 명령입니다: {raw}");
            }
            // 세션에 넣은 명령은 곧바로 한 틱 진행해 결과가 이 명령의 것으로 남게 한다
            if (verb is "construct" or "place" or "register" or "salvage" or "select" or "harvest" or "capture" or "deliver" or "altar" or "drop" or "home")
            {
                _session.RunTicks(1);
            }
            // 이 명령 뒤에 세션이 알린 일을 콘솔에 남긴다
            foreach (SessionEvent sessionEvent in _session.DrainEvents())
            {
                QueueEventSound(sessionEvent);
                if (sessionEvent.Kind != SessionEventKind.BridgePieceAdded)
                {
                    Console.WriteLine($"[{_session.Seconds,6:0.0}s] {raw} → {sessionEvent.Kind}: {sessionEvent.Text}");
                    _notice = DescribeEvent(sessionEvent);
                    if (sessionEvent.Kind is SessionEventKind.TutorialTell or SessionEventKind.MissionTell)
                    {
                        OpenTutorialTell(sessionEvent.Text);
                    }
                }
            }
        }
    }

    /// <summary>"x,y" 칸 좌표를 읽는다.</summary>
    private static (int X, int Y) ParseCell(string value)
    {
        string[] parts = value.Split(',');
        if (parts.Length != 2 || !int.TryParse(parts[0], out int x) || !int.TryParse(parts[1], out int y))
        {
            throw new ArgumentException($"칸 좌표는 x,y 형식이어야 합니다: {value}");
        }
        return (x, y);
    }

    /// <summary>맵에 저장되어 있던 오브젝트가 회수되어 세션에서 사라졌는지 (그리기에서 뺀다)</summary>
    private bool IsRemovedInitialObject(FortMapObject item) => _session.IsInitialObjectRemoved(item);

    /// <summary>저장 다리가 시험 중에 무너져 격자에서 빠졌는지 (그리기에서 뺀다)</summary>
    private bool IsCrumbledStoredBridge(FortMapObject item) => _session.IsStoredBridgeGone(item);

    /// <summary>오른쪽 위에 Storm Power(원본 색 규칙)·게임 시각·생산 규칙 상태·미션을 표시한다.</summary>
    private void DrawSessionHud(SpriteBatch batch, SpriteFontBase font, int width)
    {
        if (!SimulationRunning && !_simulationPaused && !_placementMode && !_bridgeMode)
        {
            return;
        }
        PlayerState player = _session.Player(TestPlayer);
        // 튜토리얼이면 단계·선택 줄이 하나 더 있다
        var box = new Rectangle(width - HudWidth - 10, HeaderHeight + 6, HudWidth, _session.Tutorial != null ? 112 : 88);
        batch.Draw(_pixel, box, new Color(18, 24, 38) * 0.85f);
        batch.DrawString(font, $"Storm Power {player.StormPower}", new Vector2(box.X + 10, box.Y + 3),
            StormPowerColors[StormPower.DisplayColor(player.StormPower)]);
        TimeSpan time = TimeSpan.FromSeconds(_session.Seconds);
        // 안내·브리핑 창이 열려 있으면 Update 가 세션 틱을 건너뛰므로 시간이 멈춘 이유를 함께 보여 준다.
        string state = TutorialDialogOpen ? " · 안내 창(시간 정지)" : _simulationPaused ? " · 일시정지" : SimulationRunning ? "" : " · 정지";
        batch.DrawString(font, $"게임 {(int)time.TotalMinutes:00}:{time.Seconds:00} (틱 {_session.Tick}){state}", new Vector2(box.X + 10, box.Y + 27), Color.LightGray);
        string rules = _session.EnforceProductionRules ? "생산 규칙 켜짐" : "생산 규칙 꺼짐(시험)";
        batch.DrawString(font, $"{rules} · 미션: {_mission?.Title ?? "없음"}", new Vector2(box.X + 10, box.Y + 51), Color.LightGray);
        if (_session.Tutorial is { } tutorial)
        {
            string selected = _session.Entity(player.SelectedEntityId)?.DisplayName ?? "없음";
            string stage = tutorial.Finished ? "완료" : tutorial.Stage.ToString();
            batch.DrawString(font, $"튜토리얼 단계 {stage} · 선택: {selected} (T)", new Vector2(box.X + 10, box.Y + 75), Color.LightGray);
        }
    }

    /// <summary>세션 이벤트를 화면 알림 문구로 바꾼다 (거부·튜토리얼 안내에는 머리말을 붙인다)</summary>
    private static string DescribeEvent(SessionEvent sessionEvent) => sessionEvent.Kind switch
    {
        SessionEventKind.CommandRejected => $"거부: {sessionEvent.Text}",
        SessionEventKind.TutorialTell => $"튜토리얼 안내 [{sessionEvent.Text}] (스크립트 섹션)",
        SessionEventKind.MissionTell => $"미션 안내 [{sessionEvent.Text}] (스크립트 섹션)",
        _ => sessionEvent.Text,
    };

    /// <summary>플레이어 생산 창(덱)을 요약한 문구: 다리 칸, 골렘, 워크샵별 등록 유닛과 재충전</summary>
    private string DescribeDeck()
    {
        PlayerState player = _session.Player(TestPlayer);
        var parts = new List<string>();
        // 덱 항목을 차례로 요약한다 (다리는 칸 수로 묶는다)
        foreach (DeckEntry entry in player.Deck.Entries())
        {
            if (entry.Kind == DeckEntryKind.Bridge)
            {
                parts.Add($"다리 {player.Tray.Pieces.Count}/{player.Tray.Capacity}");
                continue;
            }
            double wait = _session.SecondsUntilReady(TestPlayer, entry.TypeName);
            parts.Add(wait > 0 ? $"{entry.TypeName}({wait:0.0}초)" : entry.TypeName);
        }
        return parts.Count == 0 ? "덱 비어 있음" : "덱: " + string.Join(" · ", parts);
    }
}
