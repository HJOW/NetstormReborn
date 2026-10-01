using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 맵 뷰어의 배치 시험 모드: 게임 세션(BattleSession)에 명령을 넣어 플레이어 1 이 유닛을 놓거나 건물을 짓는다.
/// 에너지 공급 범위·필요 에너지·섬 소유권·빈 자리·Storm Power 와, 미션으로 열었을 때는 기술 허용 표·덱 등록·재충전·건설 시간까지
/// 세션 규칙 그대로 확인하는 개발용 기능이다. 빈 섬의 다리 연결은 세션이 다리 격자에서 계산한다(C 키로 강제 허용 가능).
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>공급 범위 원을 그릴 점 개수</summary>
    private const int RangeDotCount = 120;

    /// <summary>건설 중인 건물을 반투명하게 그리는 불투명도</summary>
    private const float UnderConstructionAlpha = 0.5f;

    /// <summary>건설 진행 막대의 화면 폭(논리 픽셀, 확대 배율 1 기준)</summary>
    private const int ProgressBarWidth = 48;

    /// <summary>배치 시험 모드 아래 안내 영역의 높이 (제목·판정·덱·알림·조작 키 5줄)</summary>
    private const int PlacementPanelHeight = 138;

    /// <summary>원본 Storm Power 숫자 색 (흰·노랑·빨강)</summary>
    private static readonly IReadOnlyDictionary<StormPowerColor, Color> StormPowerColors = new Dictionary<StormPowerColor, Color>
    {
        [StormPowerColor.White] = Color.White,
        [StormPowerColor.Yellow] = Color.Yellow,
        [StormPowerColor.Red] = new Color(255, 70, 60),
    };

    /// <summary>원소별 공급 범위 표시 색</summary>
    private static readonly IReadOnlyDictionary<Element, Color> ElementColors = new Dictionary<Element, Color>
    {
        [Element.Sun] = Color.Gold,
        [Element.Rain] = new Color(90, 170, 255),
        [Element.Wind] = new Color(170, 230, 140),
        [Element.Thunder] = new Color(220, 140, 255),
    };

    /// <summary>배치해 볼 수 있는 타입: 워크샵 생산 유닛(원소·레벨 순) 다음에 사제가 짓는 건물(템플·워크샵·알타·아웃포스트)</summary>
    private TypeInfo[] _candidates = [];

    /// <summary>배치 시험 모드가 켜졌는지</summary>
    private bool _placementMode;

    /// <summary>선택한 후보 번호</summary>
    private int _candidateIndex;

    /// <summary>커서 아래 칸의 최근 판정 결과</summary>
    private SessionPlacementCheck? _lastCheck;

    /// <summary>최근 Draw 에서 계산한 화면 중심 (입력 좌표 → 월드 좌표 변환용)</summary>
    private Vector2 _lastCenter;

    /// <summary>검증용: 커서 대신 판정할 칸 (--probe)</summary>
    private (int X, int Y)? _probeCell;

    /// <summary>
    /// 배치 시험 모드를 켜고 타입을 고른다 (명령줄 --placement). probe 가 있으면 커서 대신 그 칸을 판정하고
    /// 카메라를 그 칸으로 옮긴다 (스크린샷 검증용).
    /// </summary>
    /// <param name="typeName">유닛 또는 건물 타입 이름 (대소문자 무시)</param>
    /// <param name="probe">판정할 기준점 칸 (없으면 커서)</param>
    public void StartPlacement(string typeName, (int X, int Y)? probe)
    {
        int index = Array.FindIndex(_candidates, t => t.Name.Equals(typeName, StringComparison.OrdinalIgnoreCase));
        if (index < 0)
        {
            throw new ArgumentException($"배치 시험 대상이 아닌 타입입니다: {typeName} (워크샵 생산 유닛과 건물만 가능)");
        }
        _placementMode = true;
        _candidateIndex = index;
        _probeCell = probe;
        if (probe is (int x, int y))
        {
            _camera = WorldPixels(x, y);
        }
    }

    /// <summary>배치해 볼 후보 목록을 만든다 (규칙 상태는 세션이 이미 갖고 있다).</summary>
    private void InitializePlacement(TypeCatalog catalog)
    {
        TypeInfo[] units = [.. catalog.Types.Where(t => ProducibleUnit.FromType(t) != null)
            .OrderBy(t => Elements.FromTheme(t.Definition.GetString("theme")))
            .ThenBy(t => t.Definition.GetInt("level") ?? 0).ThenBy(t => t.Name, StringComparer.Ordinal)];
        // 건물은 종류(템플·워크샵·아웃포스트·알타) 순, 원소가 없는 미사용 타입(예: fireVortex)은 뺀다
        TypeInfo[] buildings = [.. catalog.Types.Where(IsBuildable)
            .OrderBy(t => ObjectKinds.Of(t)).ThenBy(t => Elements.FromTheme(t.Definition.GetString("theme"))).ThenBy(t => t.Name, StringComparer.Ordinal)];
        _candidates = [.. units, .. buildings];
    }

    /// <summary>사제가 짓는 건물 타입인지 (템플·워크샵은 원소가 있어야 하고, 비용이 없는 그림용 보조 타입은 제외한다)</summary>
    private static bool IsBuildable(TypeInfo type)
    {
        ObjectKind kind = ObjectKinds.Of(type);
        if (!ObjectKinds.IsBuilding(kind) || (type.Definition.GetInt("cost") ?? 0) <= 0)
        {
            return false;
        }
        return kind is ObjectKind.Outpost or ObjectKind.Altar || Elements.FromTheme(type.Definition.GetString("theme")) != null;
    }

    /// <summary>건물 타입인지 (건물은 Construct 명령, 유닛은 배치 명령을 쓴다)</summary>
    private static bool IsBuilding(TypeInfo type) => ObjectKinds.IsBuilding(ObjectKinds.Of(type));

    /// <summary>타입에 맞는 세션 판정을 한다 (건물 = 사제 Construct, 유닛 = 생산 창 배치).</summary>
    private SessionPlacementCheck Check(TypeInfo type, int x, int y) => IsBuilding(type)
        ? _session.CheckBuilding(TestPlayer, type.Name, x, y)
        : _session.CheckUnit(TestPlayer, type.Name, x, y);

    /// <summary>
    /// 배치 시험 키 입력과 클릭을 처리한다: P 모드, [ ] 타입 선택, C 빈 섬 연결 강제, F 지식 등록, Delete 회수, 좌클릭 배치·건설.
    /// </summary>
    private void UpdatePlacement(KeyboardState keyboard, MouseState mouse)
    {
        if (Pressed(keyboard, Keys.P))
        {
            _placementMode = !_placementMode;
            // 배치 시험과 다리 조각 시험은 같은 안내 영역을 쓰므로 하나만 켠다.
            if (_placementMode)
            {
                _bridgeMode = false;
            }
        }
        if (!_placementMode || _candidates.Length == 0)
        {
            _lastCheck = null;
            return;
        }
        if (Pressed(keyboard, Keys.OemCloseBrackets))
        {
            _candidateIndex = (_candidateIndex + 1) % _candidates.Length;
        }
        if (Pressed(keyboard, Keys.OemOpenBrackets))
        {
            _candidateIndex = (_candidateIndex + _candidates.Length - 1) % _candidates.Length;
        }
        if (Pressed(keyboard, Keys.C))
        {
            _session.AssumeConnected = !_session.AssumeConnected;
        }
        // 커서가 가리키는 칸을 기준점(오른쪽 아래 칸)으로 판정한다. 안내 영역 위에서는 판정하지 않는다.
        if (_probeCell == null && mouse.Y < HeaderHeight)
        {
            _lastCheck = null;
            return;
        }
        (int cellX, int cellY) = _probeCell ?? CellAt(new Vector2(mouse.X, mouse.Y));
        TypeInfo type = _candidates[_candidateIndex];
        _lastCheck = Check(type, cellX, cellY);
        if (Pressed(keyboard, Keys.F))
        {
            RegisterSelectedUnit(type);
        }
        if (Pressed(keyboard, Keys.Delete))
        {
            SalvageAt(cellX, cellY);
        }
        if (mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released)
        {
            if (_lastCheck.Allowed)
            {
                SubmitCommand(IsBuilding(type)
                    ? new ConstructBuildingCommand(TestPlayer, type.Name, cellX, cellY)
                    : new PlaceUnitCommand(TestPlayer, type.Name, cellX, cellY));
            }
            else
            {
                _notice = $"배치 불가: {_lastCheck.Describe()}";
            }
        }
    }

    /// <summary>
    /// 고른 유닛을 받을 수 있는 첫 워크샵에 지식으로 등록한다 ("Put Knowledge into Production"). 워크샵의 원소·빈 칸·중복은
    /// 여기서 걸러 첫 후보에 명령을 넣고, 지식이 없거나 기술 허용 표가 막은 경우는 세션이 명령을 거부하며 이유를 알린다.
    /// </summary>
    private void RegisterSelectedUnit(TypeInfo type)
    {
        ProducibleUnit? unit = ProducibleUnit.FromType(type);
        if (unit == null)
        {
            _notice = "건물은 워크샵에 등록하지 않는다 (유닛을 고르세요)";
            return;
        }
        PlayerState player = _session.Player(TestPlayer);
        // 완공된 내 워크샵 가운데 이 유닛을 받을 수 있고 빈 칸이 있는 첫 워크샵을 찾는다
        foreach (GameEntity workshop in _session.Entities.Where(e => e.Owner == TestPlayer && e.Kind == ObjectKind.Workshop && e.IsComplete))
        {
            Element workshopElement = Elements.FromTheme(workshop.Type.Definition.GetString("theme")) ?? Element.Sun;
            bool accepts = unit.Element == workshopElement || (workshopElement == Element.Sun && unit.IsGenerator);
            bool alreadyRegistered = player.Deck.Entries().Any(e => e.Kind == DeckEntryKind.Unit
                && e.TypeName.Equals(unit.Name, StringComparison.OrdinalIgnoreCase));
            if (accepts && !alreadyRegistered && player.Deck.FreeSlots(workshop.Id) > 0)
            {
                SubmitCommand(new RegisterKnowledgeCommand(TestPlayer, workshop.Id, unit.Name));
                return;
            }
        }
        _notice = "등록할 워크샵이 없음 (완공된 워크샵·맞는 원소·빈 칸 필요)";
    }

    /// <summary>커서 칸의 내 오브젝트를 회수한다 (비용의 25% 를 돌려받는다).</summary>
    private void SalvageAt(int x, int y)
    {
        GameEntity? entity = _session.EntityAt(x, y);
        if (entity == null || entity.Owner != TestPlayer)
        {
            _notice = "그 칸에 내 오브젝트가 없음";
            return;
        }
        SubmitCommand(new SalvageCommand(TestPlayer, entity.Id));
    }

    /// <summary>화면 좌표를 칸 좌표로 바꾼다 (Screen 의 역변환)</summary>
    private (int X, int Y) CellAt(Vector2 screen)
    {
        Vector2 world = (screen - _lastCenter) / _zoom + _camera;
        return ((int)Math.Floor(world.X / FortMap.CellPixelWidth), (int)Math.Floor(world.Y / FortMap.CellPixelHeight));
    }

    /// <summary>키를 이번 갱신에 새로 눌렀는지</summary>
    private bool Pressed(KeyboardState keyboard, Keys key) => keyboard.IsKeyDown(key) && !_previousKeyboard.IsKeyDown(key);

    /// <summary>
    /// 게임 중 놓거나 지은 오브젝트를 그린다 (저장 오브젝트 뒤, 안내 영역 앞). 맵에 저장된 오브젝트는 본 그리기가 그린다.
    /// 건설 중인 건물은 반투명하게 그리고 진행 막대를 붙인다.
    /// </summary>
    private void DrawPlacedUnits(SpriteBatch batch, Vector2 center)
    {
        // 세션 오브젝트 번호 순서로 그린다 (게임 중 새로 만든 것만)
        foreach (GameEntity entity in _session.Entities.Where(e => e.Kind != ObjectKind.Flyer && (e.Source == null || e.Kind == ObjectKind.Geyser && !_map.Objects.Contains(e.Source))))
        {
            Vector2 anchor = Screen(WorldPixels(entity.Footprint.AnchorX, entity.Footprint.AnchorY), center);
            if (entity.Kind == ObjectKind.Geyser && entity.Source != null)
            {
                // 미션 시작 때 생성된 연습 가이저는 저장 맵 지면에 없으므로 작은 받침도 함께 그린다.
                DrawSprite(batch, _supportBottomType.LoadIndex, MapSpriteFrames.BodyFrame(_supportBottomType.Definition, 0), anchor);
                DrawSprite(batch, _supportTopType.LoadIndex, MapSpriteFrames.BodyFrame(_supportTopType.Definition, 0), anchor);
            }
            DrawSprite(batch, entity.Type.LoadIndex, entity.Type.Definition.Frames.DefaultFrame, anchor,
                alpha: entity.IsComplete ? 1f : UnderConstructionAlpha);
            if (!entity.IsComplete)
            {
                DrawProgressBar(batch, anchor, _session.ConstructionProgress(entity));
            }
        }
    }

    /// <summary>건설 진행 막대를 오브젝트 기준점 위에 그린다.</summary>
    private void DrawProgressBar(SpriteBatch batch, Vector2 anchor, double progress)
    {
        int width = (int)(ProgressBarWidth * _zoom);
        var bar = new Rectangle((int)(anchor.X - width), (int)(anchor.Y - 10 * _zoom), width, Math.Max(3, (int)(4 * _zoom)));
        batch.Draw(_pixel, bar, new Color(30, 30, 30));
        batch.Draw(_pixel, new Rectangle(bar.X, bar.Y, (int)(bar.Width * progress), bar.Height), new Color(240, 200, 60));
    }

    /// <summary>공급 범위·발자국·판정 결과를 그린다. 모드가 꺼져 있으면 아무것도 그리지 않는다.</summary>
    private void DrawPlacementOverlay(SpriteBatch batch, SpriteFontBase font, Vector2 center, int width, int height)
    {
        _lastCenter = center;
        if (!_placementMode)
        {
            return;
        }
        BattleMap map = _session.Map;
        double radius = map.Options.GeneratorRadius;
        // 플레이어 1 의 공급원마다 반지름 원(화면에서는 16:11 타원)을 점으로 그린다.
        foreach (EnergySource source in map.Sources.Where(s => map.IsFriendly(TestPlayer, s.Owner)))
        {
            Color color = ElementColors[source.Element] * 0.8f;
            Vector2 origin = CellCenterScreen(source.CenterX, source.CenterY, center);
            // 원 둘레의 점을 같은 각도 간격으로 찍는다.
            for (int i = 0; i < RangeDotCount; i++)
            {
                double angle = i * 2 * Math.PI / RangeDotCount;
                var dot = origin + new Vector2((float)(Math.Cos(angle) * radius * FortMap.CellPixelWidth),
                    (float)(Math.Sin(angle) * radius * FortMap.CellPixelHeight)) * _zoom;
                batch.Draw(_pixel, new Rectangle((int)dot.X - 1, (int)dot.Y - 1, 3, 3), color);
            }
            batch.Draw(_pixel, new Rectangle((int)origin.X - 3, (int)origin.Y - 3, 7, 7), color);
        }
        if (_lastCheck is { Site: { } site } check)
        {
            Color fill = check.Allowed ? new Color(40, 220, 90) * 0.45f : new Color(230, 50, 40) * 0.45f;
            // 발자국의 각 칸을 반투명하게 칠한다.
            foreach ((int x, int y) in site.Footprint.Cells())
            {
                Vector2 topLeft = Screen(WorldPixels(x, y), center);
                batch.Draw(_pixel, new Rectangle((int)topLeft.X, (int)topLeft.Y,
                    (int)Math.Ceiling(FortMap.CellPixelWidth * _zoom), (int)Math.Ceiling(FortMap.CellPixelHeight * _zoom)), fill);
            }
            Vector2 target = CellCenterScreen(site.Footprint.CenterX, site.Footprint.CenterY, center);
            // 요구 글자에 배정된 공급원까지 선을 긋는다 (한 공급원 = 에너지 1개).
            foreach (EnergySource? source in site.Energy.Assigned)
            {
                if (source != null)
                {
                    Line(batch, target, CellCenterScreen(source.CenterX, source.CenterY, center), ElementColors[source.Element]);
                }
            }
        }
        DrawPlacementText(batch, font, width, height);
    }

    /// <summary>선택한 타입·판정 결과·덱 상태·조작 키 문구를 아래 안내 영역에 그린다.</summary>
    private void DrawPlacementText(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        TypeInfo type = _candidates[_candidateIndex];
        EnergyRequirement requirement = EnergyRequirement.ForType(type.Definition);
        string description = type.Definition.GetString("description") ?? type.Name;
        string head;
        if (IsBuilding(type))
        {
            head = $"건설 시험 [{_candidateIndex + 1}/{_candidates.Length}] {description} ({type.Name}) | 비용 {type.Definition.GetInt("cost") ?? 0} " +
                $"| 건설 {ConstructionTimes.Seconds(ObjectKinds.Of(type)):0}초 | 필요 에너지: {requirement.Describe()}";
        }
        else
        {
            head = $"배치 시험 [{_candidateIndex + 1}/{_candidates.Length}] {description} ({type.Name}, L{type.Definition.GetInt("level")}) " +
                $"| 비용 {type.Definition.GetInt("cost") ?? 0} | 필요 에너지: {requirement.Describe()} | 공급 반지름 {_session.Map.Options.GeneratorRadius}칸";
        }
        string result = _lastCheck == null ? "커서를 맵 위에 두세요"
            : _lastCheck.Site is { } site
                ? $"{_lastCheck.Describe()} | 섬: {IslandName(site.Island)} | 덮는 공급원 {site.Energy.Covering.Count}개"
                : _lastCheck.Describe();
        string keys = $"[ ]: 선택 · 좌클릭: 배치/건설 · F: 워크샵에 등록 · Del: 회수 · C: 빈 섬 연결 강제 {(_session.AssumeConnected ? "켜짐" : "꺼짐")} · " +
            "K: 생산 규칙 · Space: 정지 · P: 끄기";
        batch.Draw(_pixel, new Rectangle(0, height - PlacementPanelHeight, width, PlacementPanelHeight), new Color(18, 24, 38));
        batch.DrawString(font, head, new Vector2(16, height - PlacementPanelHeight + 4), Color.Gold);
        batch.DrawString(font, result, new Vector2(16, height - PlacementPanelHeight + 30), _lastCheck?.Allowed == true ? Color.LightGreen : new Color(255, 120, 110));
        batch.DrawString(font, DescribeDeck(), new Vector2(16, height - PlacementPanelHeight + 56), Color.Wheat);
        batch.DrawString(font, _notice, new Vector2(16, height - PlacementPanelHeight + 82), Color.LightGreen);
        batch.DrawString(font, keys, new Vector2(16, height - PlacementPanelHeight + 108), Color.LightGray);
    }

    /// <summary>섬 상태의 한국어 이름</summary>
    private static string IslandName(IslandState state) => state switch
    {
        IslandState.Mine => "내 섬",
        IslandState.Empty => "빈 섬",
        IslandState.Others => "남의 섬",
        _ => "섬 밖",
    };

    /// <summary>칸 좌표(실수, 칸 중심 기준)를 화면 좌표로 바꾼다</summary>
    private Vector2 CellCenterScreen(double cellX, double cellY, Vector2 center) =>
        Screen(new Vector2((float)((cellX + 0.5) * FortMap.CellPixelWidth), (float)((cellY + 0.5) * FortMap.CellPixelHeight)), center);

    /// <summary>1픽셀 텍스처를 회전·늘려 선을 긋는다</summary>
    private void Line(SpriteBatch batch, Vector2 from, Vector2 to, Color color)
    {
        Vector2 delta = to - from;
        batch.Draw(_pixel, from, null, color, (float)Math.Atan2(delta.Y, delta.X), Vector2.Zero,
            new Vector2(delta.Length(), 2f), SpriteEffects.None, 0f);
    }
}
