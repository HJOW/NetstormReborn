using System.Diagnostics;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 이동형 유닛(사제·골렘 등 수송 유닛)의 화면 표현: 걷는 방향과 걷기 그림, 칸 사이를 부드럽게 이동하는 위치,
/// 그림 모양 그대로의 클릭 판정. 규칙은 <see cref="BattleSession"/> 이 칸 단위로 처리하고 여기는 그리기와 입력만 다룬다.
/// 근거는 2026-10-03 원본 자동 분석 녹화(docs/videos/auto-war-begins-20261003.md)다.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>
    /// 걷기 그림이 바뀌는 초당 횟수. 녹화에서 사제의 8프레임 걷기 주기가 약 0.68초(54.63→55.33→56.00초 같은 프레임 반복)로 약 11.8Hz였다.
    /// </summary>
    private const double WalkFramesPerSecond = 12;

    /// <summary>걷기 한 바퀴의 프레임 수 (.type 의 클러스터 번호 00~07).</summary>
    private const int WalkFrameCount = 8;

    /// <summary>
    /// 한 번이라도 걸은 뒤 멈춰 선 그림의 프레임 번호. 녹화에서 이동을 마친 사제·골렘이 모두 번호 1(방향 글자만 마지막 방향)이었다.
    /// 아직 움직이지 않은 유닛은 맵 저장 프레임(사제 A00)·타입 기본 프레임(골렘 A05)을 그대로 쓴다.
    /// </summary>
    private const int IdleFrameNumber = 1;

    /// <summary>클릭 판정에서 그림 사각형 둘레에 더하는 여유(논리 픽셀). 10×25 픽셀 크기의 작은 그림을 손쉽게 누르게 한다.</summary>
    private const int PickMargin = 4;

    /// <summary>
    /// 자동 입력 검사가 확인할 선택·이동형 유닛 상태. 예: <c>selected=priest;units=priest:C:moving,sunwalker:-:idle</c>
    /// (유닛마다 타입 이름:바라보는 방향 글자(움직인 적이 없으면 -):moving/idle, 내 유닛만). rotation은 들고 있는 캐논 방위다.
    /// sunposes·crossbowposes는 포대의 실제 그림 번호이며 shots·impacts는 탄·효과의 진단 개수다.
    /// sites는 아직 완공되지 않은 내 건물(타입 이름:waiting=사제가 오는 중/building=건설 중)이고 sp는 내 Storm Power다.
    /// </summary>
    public string UiDetail => $"selected={_session.Entity(_session.Player(TestPlayer).SelectedEntityId)?.Type.Name.ToLowerInvariant() ?? "none"};units="
        + string.Join(',', _session.Entities.Where(e => IsMobile(e) && e.Owner == TestPlayer).Select(e =>
            $"{e.Type.Name.ToLowerInvariant()}:{UnitHeading.Side(e.Heading)?.ToString() ?? "-"}:{(_session.IsMoving(e.Id) ? "moving" : "idle")}"))
        + $";rotation={_cannonRotation};camera=" + FormattableString.Invariant($"{_camera.X:0.##},{_camera.Y:0.##}")
        + ";workshops=" + string.Join(',', _session.Entities.Where(e => e.Owner == TestPlayer && e.Kind == ObjectKind.Workshop)
            .Select(e => $"{e.Type.Name.ToLowerInvariant()}:{WorkshopLevel(e, e.Source)}"))
        + $";placement={(_lastCheck?.Allowed == true ? "allowed" : _lastCheck?.Failure.ToString() ?? "none")}"
        + $";cursor={(_placementMode ? _candidates[_candidateIndex].Name.ToLowerInvariant() : "none")};range={RangeDetail}"
        + ";sunposes=" + string.Join(',', _session.Entities.Where(e => e.Type.Name.Equals("sunCannon", StringComparison.OrdinalIgnoreCase)).Select(e => e.SunCannonFrame))
        + ";crossbowposes=" + string.Join(',', _session.Entities.Where(e => e.Type.Name.Equals("windArcher", StringComparison.OrdinalIgnoreCase)).Select(e => e.CrossbowFrame))
        + $";shots={_session.Shots.Count};impacts={_session.Impacts.Count}"
        + ";sites=" + string.Join(',', _session.Entities.Where(e => e.Owner == TestPlayer && !e.IsComplete)
            .Select(e => $"{e.Type.Name.ToLowerInvariant()}:{(e.AwaitingBuilder ? "waiting" : "building")}"))
        + $";sp={_session.Player(TestPlayer).StormPower}";

    /// <summary>걷기 그림 시계(초). 게임 시간이 흐르는 동안에만 진행해 일시정지 중에는 걷는 자세가 멈춘다.</summary>
    private double _walkClock;

    /// <summary>(타입 번호, 방향 글자, 프레임 번호) → 클러스터 번호. 매 프레임 .type 코드 표를 훑지 않도록 기억한다.</summary>
    private readonly Dictionary<(int Type, char Side, int Number), int> _walkFrameCache = [];

    /// <summary>
    /// 진단용(<c>--dump-objects</c>): 맵에 저장된 오브젝트를 타입·월드 칸·영역·소유자·저장 프레임·상태 바이트와 함께 콘솔에 쓴다.
    /// 원본 화면과 클론 화면의 그림이 다를 때 어떤 타입·프레임인지 확인하는 데 쓴다. 다리와 투명 지면(noIsland)은 뺀다.
    /// </summary>
    public void DumpObjects()
    {
        // 화면 위에서 아래·왼쪽에서 오른쪽 순서로 쓴다.
        foreach (FortMapObject item in _map.Objects.OrderBy(o => o.Y).ThenBy(o => o.X))
        {
            FortObject o = item.Object;
            if (o.Type.Name is "noIsland" or "bridge") continue;
            Console.WriteLine($"[obj] {o.Type.Name} cell=({item.X},{item.Y}) terr={item.Territory?.ToString() ?? "-"} owner={o.Owner?.ToString() ?? "-"} "
                + $"frame={o.Frame?.ToString() ?? "-"} qa={o.QA?.ToString() ?? "-"} qb={o.QB?.ToString() ?? "-"} factory={o.FactoryState?.ToString() ?? "-"} contents={o.Contents.Count}");
        }
        // 맵에 없는데 세션이 만든 오브젝트(내용물에서 꺼낸 유닛 등)도 써서 화면에 더 그려지는 그림의 출처를 찾는다.
        foreach (GameEntity entity in _session.Entities.Where(e => e.Source == null))
            Console.WriteLine($"[session] {entity.Type.Name} id={entity.Id} cell=({entity.Footprint.AnchorX},{entity.Footprint.AnchorY}) owner={entity.Owner} kind={entity.Kind}");
        // 지면 영역마다 테마·소유자·타일 수를 써서 섬 테두리 색이 어떤 소유자로 계산됐는지 확인한다.
        foreach (var group in _terrain.Tiles.GroupBy(t => (t.Region, t.Theme, t.Owner)).OrderBy(g => g.Key.Region))
            Console.WriteLine($"[terrain] region={group.Key.Region} theme={group.Key.Theme} owner={group.Key.Owner} tiles={group.Count()} "
                + $"x={group.Min(t => t.X)}..{group.Max(t => t.X)} y={group.Min(t => t.Y)}..{group.Max(t => t.Y)}");
    }

    /// <summary>걷기 그림 시계를 진행한다 (게임 시간이 흐를 때만).</summary>
    /// <param name="seconds">지난 갱신 이후 경과 시간(초)</param>
    private void UpdateWalkClock(double seconds)
    {
        if (SimulationRunning && !TutorialDialogOpen) _walkClock += seconds;
    }

    /// <summary>사제·수송 유닛처럼 걸어 다니는 오브젝트인지 (비행 공격체는 제외).</summary>
    private static bool IsMobile(GameEntity entity) => entity.Kind is ObjectKind.Priest or ObjectKind.Transport;

    /// <summary>타입에 8방향 × 8프레임 걷기 그림(A00~H07)이 있는지.</summary>
    private bool HasWalkSet(TypeInfo type) =>
        WalkFrame(type, 'A', 0) >= 0 && WalkFrame(type, 'H', WalkFrameCount - 1) >= 0;

    /// <summary>방향 글자·번호의 클러스터 번호 (없으면 -1).</summary>
    private int WalkFrame(TypeInfo type, char side, int number)
    {
        var key = (type.LoadIndex, side, number);
        if (!_walkFrameCache.TryGetValue(key, out int frame))
            _walkFrameCache[key] = frame = type.Definition.Frames.Find(side, TypeFrameTable.DefaultVariant, number);
        return frame;
    }

    /// <summary>
    /// 이동형 유닛이 지금 그릴 걷기·정지 프레임. 걷는 그림이 없거나, 움직인 적이 없거나, 포획·기절·허공 상태이면 null이며
    /// 그때는 호출한 쪽이 저장 프레임이나 기본 프레임을 쓴다.
    /// </summary>
    /// <param name="entity">그릴 오브젝트</param>
    private int? MobileFrame(GameEntity entity)
    {
        if (entity.Heading == UnitHeading.None || entity.Captivity != PriestCaptivity.Free || entity.IsSuspended || entity.IsStunned) return null;
        if (!HasWalkSet(entity.Type)) return null;
        // 걷는 중에는 시계로 8프레임을 돌리고(유닛마다 시작을 어긋나게 해 로봇처럼 보이지 않게 한다) 서 있으면 정지 프레임이다.
        int number = _session.IsMoving(entity.Id)
            ? (int)(_walkClock * WalkFramesPerSecond + entity.Id * 3) % WalkFrameCount
            : IdleFrameNumber;
        int frame = WalkFrame(entity.Type, UnitHeading.Side(entity.Heading)!.Value, number);
        return frame >= 0 ? frame : null;
    }

    /// <summary>걷는 그림이 없을 때의 프레임: 맵에 저장된 오브젝트는 저장 프레임, 게임 중 만든 것은 타입 기본 프레임.</summary>
    private int RestFrame(GameEntity entity) => entity.Source != null
        ? MapSpriteFrames.BodyFrame(entity.Source, _terrain.TerritoryTheme(entity.Source.Territory))
        : entity.Type.Definition.Frames.DefaultFrame;

    /// <summary>칸 사이를 걷는 위치까지 반영한 월드 픽셀 좌표 (규칙상 칸 위치가 아니라 화면용 보간 위치).</summary>
    private Vector2 MobileWorldPixels(GameEntity entity)
    {
        (double x, double y) = _session.VisualCell(entity);
        return new Vector2((float)(x * FortMap.CellPixelWidth), (float)(y * FortMap.CellPixelHeight));
    }

    /// <summary>
    /// 오브젝트 중심의 화면 좌표. 이동형 유닛은 보간한 걷는 위치를 쓰고 나머지는 규칙상 중심이다.
    /// 체력 막대·기절 표시 같은 겹쳐 그리기가 그림을 따라 부드럽게 움직이게 한다.
    /// </summary>
    private Vector2 EntityCenterScreen(GameEntity entity, Vector2 center)
    {
        if (entity.Flight != null || !IsMobile(entity)) return CellCenterScreen(entity.WorldX, entity.WorldY, center);
        (double x, double y) = _session.VisualCell(entity);
        return CellCenterScreen(entity.WorldX + x - entity.Footprint.AnchorX, entity.WorldY + y - entity.Footprint.AnchorY, center);
    }

    /// <summary>이동형 유닛의 지금 그림(텍스처·기준점 오프셋)과 화면 기준점. 그릴 그림이 없으면 null.</summary>
    private (Texture2D Texture, Point Offset, Vector2 Anchor)? MobileSprite(GameEntity entity, Vector2 center)
    {
        // 화면에 그리는 것과 같은 그림·위치로 판정한다 (풍선의 기준점 이동, Sail Skater 의 방향 그림 포함).
        (TypeInfo type, StructureFrames frames, Vector2 shift) = ObjectSprite(entity.Type, entity, entity.Source);
        var sprite = GetTexture(type.LoadIndex, frames.Body, _playerColors.GetValueOrDefault(entity.Owner));
        return sprite is { } s ? (s.Texture, s.Offset, Screen(MobileWorldPixels(entity), center) + shift * _zoom) : null;
    }

    /// <summary>
    /// 화면 좌표에서 클릭한 이동형 유닛을 찾는다. 규칙상 칸(발자국)이 아니라 **그려진 그림 사각형**(+여유)으로 판정하므로
    /// 몸통·머리를 눌러도 선택된다. 여럿이 겹치면 그림 중심에 가까운 유닛을 고른다.
    /// 포획되어 수송 유닛 안에 있는 사제는 건너뛴다.
    /// </summary>
    /// <param name="screen">논리 화면 좌표</param>
    private GameEntity? PickMobileAt(Point screen)
    {
        GameEntity? best = null;
        float bestDistance = float.MaxValue;
        // 모든 이동형 유닛의 그림 사각형을 확인한다.
        foreach (GameEntity entity in _session.Entities)
        {
            if (!IsMobile(entity) || entity.Captivity == PriestCaptivity.Carried || !entity.IsComplete) continue;
            if (MobileSprite(entity, _lastCenter) is not { } sprite) continue;
            Vector2 topLeft = sprite.Anchor + sprite.Offset.ToVector2() * _zoom;
            var rect = new Rectangle((int)topLeft.X - PickMargin, (int)topLeft.Y - PickMargin,
                (int)(sprite.Texture.Width * _zoom) + 2 * PickMargin, (int)(sprite.Texture.Height * _zoom) + 2 * PickMargin);
            if (!rect.Contains(screen)) continue;
            float distance = Vector2.DistanceSquared(new Vector2(rect.Center.X, rect.Center.Y), screen.ToVector2());
            if (distance >= bestDistance) continue;
            best = entity;
            bestDistance = distance;
        }
        return best;
    }

    /// <summary>
    /// 화면 좌표의 오브젝트: 이동형 유닛은 그림 모양으로, 건물·가이저 같은 나머지는 차지한 칸으로 찾는다.
    /// 유닛은 건물 앞에 서 있으므로 같은 자리에서는 유닛이 먼저다.
    /// </summary>
    /// <param name="screen">논리 화면 좌표</param>
    private GameEntity? PickEntityAt(Point screen)
    {
        if (PickMobileAt(screen) is { } unit) return unit;
        (int x, int y) = CellAt(screen.ToVector2());
        return _session.EntityAt(x, y);
    }

    /// <summary>
    /// 미션을 열 때 한 번, 모든 지면·오브젝트 그림과 이동형 유닛의 걷기 그림을 텍스처로 만들어 둔다.
    /// 처음 보는 프레임을 그리는 순간 디코딩하면 화면 이동 때마다 끊겨 프레임이 떨어지기 때문이다
    /// (측정: 첫 4초간 그리기 평균 20ms·최대 60ms, 첫 프레임 330ms).
    /// </summary>
    private void WarmTextures()
    {
        long warmStart = Stopwatch.GetTimestamp();
        // 지면·절벽·가장자리 농지·받침 그림
        // 섬 소유자 색 표시(F7)를 켜고 끌 때 쓰는 두 가지 색 변형을 모두 만든다.
        foreach (FortTerrainTile tile in _terrain.Tiles)
        {
            int frame = MapSpriteFrames.BodyFrame(_terrainType.Definition, tile.Cluster);
            GetTexture(_terrainType.LoadIndex, frame);
            GetTexture(_terrainType.LoadIndex, frame, _playerColors.GetValueOrDefault(tile.Owner));
        }
        foreach (FortTerrainFringeSprite fringe in _fringes)
            GetTexture(_fringeType.LoadIndex, MapSpriteFrames.BodyFrame(_fringeType.Definition, fringe.Cluster));
        foreach (FortEdgeFarmTile tile in _edgeFarms)
        {
            int frame = MapSpriteFrames.BodyFrame(_edgeFarmType.Definition, tile.Cluster);
            GetTexture(_edgeFarmType.LoadIndex, frame);
            GetTexture(_edgeFarmType.LoadIndex, frame, _playerColors.GetValueOrDefault(tile.Owner));
        }
        foreach (int cluster in Enumerable.Range(0, 8))
        {
            GetTexture(_supportBottomType.LoadIndex, MapSpriteFrames.BodyFrame(_supportBottomType.Definition, Math.Min(cluster, _supportBottomType.Definition.Clusters.Count - 1)));
            GetTexture(_supportTopType.LoadIndex, MapSpriteFrames.BodyFrame(_supportTopType.Definition, Math.Min(cluster, _supportTopType.Definition.Clusters.Count - 1)));
        }
        // 저장된 오브젝트의 기본 그림
        foreach (FortMapObject item in _sorted)
        {
            if (item.Object.Type.Name == "noIsland") continue;
            GetTexture(item.Object.Type.LoadIndex, MapSpriteFrames.BodyFrame(item, _terrain.TerritoryTheme(item.Territory)));
        }
        // 제자리 애니메이션이 있는 타입(가이저 증기·워크샵 레벨·신전 회오리·풍선)은 모든 클러스터와 그 그림자를 만들어 둔다.
        foreach (TypeInfo type in _sorted.Select(o => o.Object.Type).DistinctBy(t => t.LoadIndex)
                     .Where(t => ObjectKinds.Of(t) is ObjectKind.Geyser or ObjectKind.Workshop or ObjectKind.Temple || t.Definition.HasFlag("balloon")))
        {
            // 클러스터 번호가 곧 본체 프레임 번호다.
            for (int frame = 0; frame < type.Definition.Clusters.Count; frame++)
            {
                GetTexture(type.LoadIndex, frame);
                if (ShadowStyleOf(type) != ShadowStyle.None) GetShadowTexture(type, frame, ShadowStyleOf(type) == ShadowStyle.Dithered);
            }
        }
        // 이동형 유닛 타입의 8방향 × 8프레임: 지도에 있는 것과 생산 후보 가운데 걷는 그림이 있는 것
        IEnumerable<TypeInfo> mobileTypes = _session.Entities.Where(IsMobile).Select(e => e.Type)
            .Concat(_candidates.Where(t => ObjectKinds.Of(t) == ObjectKind.Transport)).DistinctBy(t => t.LoadIndex);
        foreach (TypeInfo type in mobileTypes.Where(HasWalkSet))
        {
            for (int heading = 0; heading < UnitHeading.Count; heading++)
            {
                for (int number = 0; number < WalkFrameCount; number++)
                {
                    int frame = WalkFrame(type, UnitHeading.Side(heading)!.Value, number);
                    if (frame >= 0) GetTexture(type.LoadIndex, frame);
                }
            }
        }
        // 사격·성장 중 처음 등장하는 그림도 예열해 첫 교전의 디코딩 지연을 줄인다.
        IEnumerable<TypeInfo> combatTypes = _candidates.Where(CannonAnimation.IsCannon)
            .Concat(_session.Entities.Select(e => e.Type).Where(CannonAnimation.IsCannon))
            .Concat(new[] { "growingRainBlocker", "fenceShield", "rainCannonMissile", "thunderCannonMissile" }
                .Select(_knowledgeTypes.Find).OfType<TypeInfo>()).DistinctBy(t => t.LoadIndex);
        foreach (TypeInfo type in combatTypes)
        {
            // 클러스터 저장 순서가 셰이프 본체 프레임 순서이므로 중복 이름도 그대로 예열한다.
            for (int frame = 0; frame < type.Definition.Clusters.Count; frame++) GetTexture(type.LoadIndex, frame);
        }
        // 소유자 색이 바뀌는 타입은 현재 소유자별 모든 본체 그림도 예열해 걷기·흔들림 중 디코딩을 피한다.
        foreach (var group in _session.Entities.Where(e => _objectColors.AppliesTo(e.Type.LoadIndex))
                     .GroupBy(e => (e.Type.LoadIndex, Color: _playerColors.GetValueOrDefault(e.Owner))))
        {
            TypeInfo type = group.First().Type;
            // 클러스터 순서대로 같은 소유자 색의 프레임을 만든다.
            for (int frame = 0; frame < type.Definition.Clusters.Count; frame++)
                GetTexture(type.LoadIndex, frame, group.Key.Color);
        }
        if (PerfMeter.Current != null)
            Console.WriteLine($"[perf] 텍스처 예열 {_textures.Count}개 {Stopwatch.GetElapsedTime(warmStart).TotalMilliseconds:0}ms");
    }
}
