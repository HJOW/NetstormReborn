using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Game;

/// <summary>
/// 맵 뷰어의 배치 시험 모드: Core 규칙(BattleMap)으로 플레이어 1 이 유닛을 놓을 수 있는지 판정하고 실제로 놓아 본다.
/// 에너지 공급 범위·필요 에너지·섬 소유권·빈 자리·Storm Power 를 확인하는 개발용 기능이다.
/// 다리 연결과 다리 끝 판정은 다리 규칙 분석 전이라 근사(C 키로 빈 섬 연결 가정, 다리 칸에 붙은 섬 밖 칸 = 다리 끝)를 쓴다.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>배치 시험을 하는 플레이어 번호</summary>
    private const int TestPlayer = 1;

    /// <summary>공급 범위 원을 그릴 점 개수</summary>
    private const int RangeDotCount = 120;

    /// <summary>저장된 Money 섹션이 없을 때 쓰는 시작 Storm Power</summary>
    private const int FallbackStormPower = 5000;

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

    /// <summary>규칙 상태 (섬 소유권·공급원·점유 칸)</summary>
    private BattleMap _battle = null!;

    /// <summary>배치해 볼 수 있는 유닛 타입 (워크샵 생산 대상, 원소·레벨 순)</summary>
    private TypeInfo[] _candidates = [];

    /// <summary>다리 조각이 있는 칸 → 소유자</summary>
    private Dictionary<(int X, int Y), int> _bridgeOwners = [];

    /// <summary>시험 중 놓은 유닛 (그리기용)</summary>
    private readonly List<(TypeInfo Type, int X, int Y)> _placed = [];

    /// <summary>배치 시험 모드가 켜졌는지</summary>
    private bool _placementMode;

    /// <summary>빈 섬이 내 섬과 다리로 연결되었다고 가정할지 (다리 연결 판정 전의 근사)</summary>
    private bool _assumeConnected;

    /// <summary>선택한 후보 번호</summary>
    private int _candidateIndex;

    /// <summary>플레이어 1 의 현재 Storm Power</summary>
    private int _stormPower;

    /// <summary>커서 아래 칸의 최근 판정 결과</summary>
    private PlacementCheck? _lastCheck;

    /// <summary>최근 Draw 에서 계산한 화면 중심 (입력 좌표 → 월드 좌표 변환용)</summary>
    private Vector2 _lastCenter;

    /// <summary>최근 알림 문구 (배치 성공 등)</summary>
    private string _placementNotice = "";

    /// <summary>검증용: 커서 대신 판정할 칸 (--probe)</summary>
    private (int X, int Y)? _probeCell;

    /// <summary>
    /// 배치 시험 모드를 켜고 유닛을 고른다 (명령줄 --placement). probe 가 있으면 커서 대신 그 칸을 판정하고
    /// 카메라를 그 칸으로 옮긴다 (스크린샷 검증용).
    /// </summary>
    /// <param name="typeName">유닛 타입 이름 (대소문자 무시)</param>
    /// <param name="probe">판정할 기준점 칸 (없으면 커서)</param>
    public void StartPlacement(string typeName, (int X, int Y)? probe)
    {
        int index = Array.FindIndex(_candidates, t => t.Name.Equals(typeName, StringComparison.OrdinalIgnoreCase));
        if (index < 0)
        {
            throw new ArgumentException($"배치 시험 대상이 아닌 타입입니다: {typeName} (워크샵 생산 유닛만 가능)");
        }
        _placementMode = true;
        _candidateIndex = index;
        _probeCell = probe;
        if (probe is (int x, int y))
        {
            _camera = WorldPixels(x, y);
        }
    }

    /// <summary>맵에서 규칙 상태와 후보 유닛을 만든다.</summary>
    private void InitializePlacement(FortFile fort, TypeCatalog catalog)
    {
        // 본섬 칸(영역 번호 0 이상)만 섬으로 본다. 작은 받침·발판은 영역 밖으로 둔다.
        var territories = new Dictionary<(int X, int Y), int>();
        foreach (FortTerrainCell cell in _terrain.IslandCells.Where(c => c.Region >= 0))
        {
            territories[(cell.X, cell.Y)] = cell.Region;
        }
        _battle = new BattleMap(_map.Objects, (x, y) => territories.TryGetValue((x, y), out int t) ? t : null);
        _bridgeOwners = _map.Objects.Where(o => ObjectKinds.Of(o.Object.Type) == ObjectKind.Bridge)
            .GroupBy(o => (o.X, o.Y)).ToDictionary(g => g.Key, g => g.First().Object.Owner ?? 0);
        _candidates = catalog.Types.Where(t => ProducibleUnit.FromType(t) != null)
            .OrderBy(t => Elements.FromTheme(t.Definition.GetString("theme")))
            .ThenBy(t => t.Definition.GetInt("level") ?? 0).ThenBy(t => t.Name, StringComparer.Ordinal).ToArray();
        _stormPower = fort.Money is float money && money > 0 ? (int)money : FallbackStormPower;
    }

    /// <summary>배치 시험 키 입력과 클릭을 처리한다 (P 모드, [ ] 유닛 선택, C 연결 가정, 좌클릭 배치).</summary>
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
            _assumeConnected = !_assumeConnected;
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
        if (mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released)
        {
            if (_lastCheck.Allowed)
            {
                _battle.PlaceUnit(type, _lastCheck, TestPlayer);
                _placed.Add((type, cellX, cellY));
                _stormPower -= _lastCheck.Cost;
                _placementNotice = $"{type.Definition.GetString("description")} 배치 (−{_lastCheck.Cost})";
                _lastCheck = Check(type, cellX, cellY);
            }
            else
            {
                _placementNotice = $"배치 불가: {PlacementRules.Describe(_lastCheck.Problem)}";
            }
        }
    }

    /// <summary>한 위치의 판정. 다리 끝은 발자국 둘레에 플레이어 다리 칸이 있는 섬 밖 위치로 근사한다.</summary>
    private PlacementCheck Check(TypeInfo type, int x, int y)
    {
        Footprint foot = Footprint.ForType(type.Definition, x, y);
        bool bridgeEnd = BorderCells(foot).Any(c => _bridgeOwners.TryGetValue(c, out int owner) && owner == TestPlayer);
        return _battle.CheckUnit(type, x, y, TestPlayer, _stormPower, _assumeConnected, bridgeEnd);
    }

    /// <summary>발자국 바로 바깥 둘레의 네 방향 이웃 칸</summary>
    private static IEnumerable<(int X, int Y)> BorderCells(Footprint foot)
    {
        // 위·아래 변 바깥 칸
        for (int x = foot.Left; x <= foot.AnchorX; x++)
        {
            yield return (x, foot.Top - 1);
            yield return (x, foot.AnchorY + 1);
        }
        // 왼쪽·오른쪽 변 바깥 칸
        for (int y = foot.Top; y <= foot.AnchorY; y++)
        {
            yield return (foot.Left - 1, y);
            yield return (foot.AnchorX + 1, y);
        }
    }

    /// <summary>화면 좌표를 칸 좌표로 바꾼다 (Screen 의 역변환)</summary>
    private (int X, int Y) CellAt(Vector2 screen)
    {
        Vector2 world = (screen - _lastCenter) / _zoom + _camera;
        return ((int)Math.Floor(world.X / FortMap.CellPixelWidth), (int)Math.Floor(world.Y / FortMap.CellPixelHeight));
    }

    /// <summary>키를 이번 갱신에 새로 눌렀는지</summary>
    private bool Pressed(KeyboardState keyboard, Keys key) => keyboard.IsKeyDown(key) && !_previousKeyboard.IsKeyDown(key);

    /// <summary>시험 중 놓은 유닛을 그린다 (저장 오브젝트 뒤, 안내 영역 앞).</summary>
    private void DrawPlacedUnits(SpriteBatch batch, Vector2 center)
    {
        // 놓은 순서대로 기본 프레임을 기준점에 그린다.
        foreach ((TypeInfo type, int x, int y) in _placed)
        {
            DrawSprite(batch, type.LoadIndex, type.Definition.Frames.DefaultFrame, Screen(WorldPixels(x, y), center));
        }
    }

    /// <summary>공급 범위·발자국·판정 결과를 그린다. 모드가 꺼져 있으면 아무것도 그리지 않는다.</summary>
    private void DrawPlacementOverlay(SpriteBatch batch, SpriteFontBase font, Vector2 center, int width, int height)
    {
        _lastCenter = center;
        if (!_placementMode)
        {
            return;
        }
        double radius = _battle.Options.GeneratorRadius;
        // 플레이어 1 의 공급원마다 반지름 원(화면에서는 16:11 타원)을 점으로 그린다.
        foreach (EnergySource source in _battle.Sources.Where(s => _battle.IsFriendly(TestPlayer, s.Owner)))
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
        if (_lastCheck is { } check)
        {
            Color fill = check.Allowed ? new Color(40, 220, 90) * 0.45f : new Color(230, 50, 40) * 0.45f;
            // 발자국의 각 칸을 반투명하게 칠한다.
            foreach ((int x, int y) in check.Footprint.Cells())
            {
                Vector2 topLeft = Screen(WorldPixels(x, y), center);
                batch.Draw(_pixel, new Rectangle((int)topLeft.X, (int)topLeft.Y,
                    (int)Math.Ceiling(FortMap.CellPixelWidth * _zoom), (int)Math.Ceiling(FortMap.CellPixelHeight * _zoom)), fill);
            }
            Vector2 target = CellCenterScreen(check.Footprint.CenterX, check.Footprint.CenterY, center);
            // 요구 글자에 배정된 공급원까지 선을 긋는다 (한 공급원 = 에너지 1개).
            foreach (EnergySource? source in check.Energy.Assigned)
            {
                if (source != null)
                {
                    Line(batch, target, CellCenterScreen(source.CenterX, source.CenterY, center), ElementColors[source.Element]);
                }
            }
        }
        DrawPlacementText(batch, font, width, height);
    }

    /// <summary>Storm Power(원본 색 규칙)·선택 유닛·판정 결과 문구를 그린다.</summary>
    private void DrawPlacementText(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        string sp = $"Storm Power {_stormPower}";
        Vector2 spSize = font.MeasureString(sp);
        batch.Draw(_pixel, new Rectangle(width - (int)spSize.X - 28, HeaderHeight + 6, (int)spSize.X + 20, 30), new Color(18, 24, 38) * 0.85f);
        batch.DrawString(font, sp, new Vector2(width - spSize.X - 18, HeaderHeight + 9), StormPowerColors[StormPower.DisplayColor(_stormPower)]);

        TypeInfo type = _candidates[_candidateIndex];
        EnergyRequirement requirement = EnergyRequirement.ForType(type.Definition);
        string head = $"배치 시험 [{_candidateIndex + 1}/{_candidates.Length}] {type.Definition.GetString("description")} ({type.Name}, L{type.Definition.GetInt("level")}) " +
            $"| 비용 {type.Definition.GetInt("cost") ?? 0} | 필요 에너지: {requirement.Describe()} | 공급 반지름 {_battle.Options.GeneratorRadius}칸";
        string result = _lastCheck == null ? "커서를 맵 위에 두세요"
            : $"{PlacementRules.Describe(_lastCheck.Problem)} | 섬: {IslandName(_lastCheck.Island)} | 덮는 공급원 {_lastCheck.Energy.Covering.Count}개";
        string keys = $"[ ]: 유닛 선택 · 좌클릭: 배치 · C: 빈 섬 연결 가정 {(_assumeConnected ? "켜짐" : "꺼짐")} · P: 모드 끄기";
        batch.Draw(_pixel, new Rectangle(0, height - 94, width, 94), new Color(18, 24, 38));
        batch.DrawString(font, head, new Vector2(16, height - 90), Color.Gold);
        batch.DrawString(font, result, new Vector2(16, height - 62), _lastCheck?.Allowed == true ? Color.LightGreen : new Color(255, 120, 110));
        batch.DrawString(font, _placementNotice.Length == 0 ? keys : $"{keys} | {_placementNotice}", new Vector2(16, height - 34), Color.LightGray);
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
