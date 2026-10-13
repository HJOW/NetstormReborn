using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Display;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>지면·저장 객체·새 객체·월드 효과를 한 목록으로 정렬하고 그 목록을 선택 판정에도 사용한다.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>한 프레임의 월드 그림. Order는 원본 깊이·좌표, Paint는 실제 그리기, Entity와 Stored는 선택·진단 대상이다.</summary>
    /// <param name="Order">월드 출력의 깊이·동률 순서를 비교할 정렬 키.</param>
    /// <param name="Paint">정렬된 순서에 따라 SpriteBatch에 출력하는 콜백.</param>
    /// <param name="Entity">이 출력 항목과 대응하는 엔티티. 없으면 null.</param>
    /// <param name="Stored">이 출력 항목과 대응하는 저장 맵 객체. 없으면 null.</param>
    private sealed record WorldDisplayItem(DrawOrder Order, Action<SpriteBatch> Paint,
        GameEntity? Entity = null, FortMapObject? Stored = null);

    /// <summary>마지막 화면에 실제로 그린 순서. 입력 판정은 이 목록을 뒤에서부터 검사한다.</summary>
    private IReadOnlyList<WorldDisplayItem> _worldDisplay = [];

    /// <summary>표시 목록을 수집한 카메라·중심·배율·숨김 상태. 다음 그리기 전에 입력이 바뀐 화면을 조회하면 재수집한다.</summary>
    private (Vector2 Camera, Vector2 Center, float Zoom, bool HideBuildings) _worldDisplayView;

    /// <summary>그림의 투명 부분을 클릭할 때 뒤쪽 객체를 찾기 위한 본체 화소 캐시.</summary>
    private readonly Dictionary<Texture2D, Color[]> _pickPixels = [];

    /// <summary>자동 입력 검사에서 실제 그린 엔티티 순서를 확인한다. 이름 뒤 좌표는 보간한 월드 칸이다.</summary>
    private string WorldOrderDetail => string.Join(',', _worldDisplay.Where(i => i.Entity != null).Select(i =>
        FormattableString.Invariant($"{i.Entity!.Type.Name.ToLowerInvariant()}@{i.Order.X:0.##}/{i.Order.Y:0.##}")));

    /// <summary>원본 타입 깊이를 부호 있는 16비트로 제한하고 월드 픽셀을 정렬용 칸 좌표로 바꾼다.</summary>
    private static DrawOrder WorldOrder(TypeInfo type, Vector2 world) => new(
        world.X / FortMap.CellPixelWidth, world.Y / FortMap.CellPixelHeight,
        (short)Math.Clamp(type.ZOrder, short.MinValue, short.MaxValue));

    /// <summary>현재 화면과 큰 그림 여유 안에 있는 월드 기준점인지 확인한다.</summary>
    private bool WorldVisible(Vector2 world, Vector2 center, float margin)
    {
        Vector2 point = Screen(world, center);
        margin *= _zoom;
        return point.X > -margin && point.X < _viewSize.X + margin
            && point.Y > -margin && point.Y < _viewSize.Y + margin;
    }

    /// <summary>지면·받침·다리 등의 단일 원본 그림을 타입 깊이와 기준점으로 목록에 넣는다.</summary>
    private void AddWorldSprite(List<WorldDisplayItem> items, TypeInfo type, int frame,
        Vector2 world, Vector2 center, int color = 0, float alpha = 1)
    {
        items.Add(new WorldDisplayItem(WorldOrder(type, world),
            batch => DrawSprite(batch, type.LoadIndex, frame, Screen(world, center), color, alpha: alpha)));
    }

    /// <summary>받침의 하단과 윗면을 각각 원본 깊이로 등록한다. 하단은 절벽처럼 윗면보다 먼저 그린다.</summary>
    private void AddWorldSupport(List<WorldDisplayItem> items, Vector2 world, Vector2 center, int owner, int? cluster = null)
    {
        if (!WorldVisible(world, center, ObjectCullMargin)) return;
        int frame = cluster ?? FortIslandSupports.ColorCluster(owner, _playerColors);
        int color = _islandColors ? _playerColors.GetValueOrDefault(owner) : 0;
        AddWorldSprite(items, _supportBottomType, MapSpriteFrames.BodyFrame(_supportBottomType.Definition, frame), world, center, color);
        AddWorldSprite(items, _supportTopType, MapSpriteFrames.BodyFrame(_supportTopType.Definition, frame), world, center, color);
    }

    /// <summary>보간한 이동 위치 또는 고정 건물의 발자국 기준점으로 세션 객체를 등록한다.</summary>
    private void AddWorldEntity(List<WorldDisplayItem> items, GameEntity entity, Vector2 center, FortMapObject? stored = null)
    {
        if (HiddenByBuildingToggle(entity.Type) || entity.Captivity == PriestCaptivity.Carried
            || entity.Production is { AwaitingDeliveries: true }) return;
        Vector2 world = entity.Kind == ObjectKind.Flyer
            ? new Vector2((float)entity.WorldX * FortMap.CellPixelWidth, (float)entity.WorldY * FortMap.CellPixelHeight)
            : IsMobile(entity) ? MobileWorldPixels(entity) : WorldPixels(entity.Footprint.AnchorX, entity.Footprint.AnchorY);
        if (!WorldVisible(world, center, ObjectCullMargin)) return;
        if (entity.Kind == ObjectKind.Flyer)
        {
            TypeDefinition type = entity.Type.Definition;
            int count = type.Clusters.Count(c => c.Name.StartsWith("A", StringComparison.Ordinal));
            int frame = count == 0 ? type.Frames.DefaultFrame
                : type.Frames.Find('A', TypeFrameTable.DefaultVariant, (int)(_session.Tick / 2 % count));
            if (frame < 0) frame = type.Frames.DefaultFrame;
            int body = frame;
            // 비행체 그림자는 본체와 독립된 원본 zoFLYER_SHADOWS 깊이로 그린다.
            DrawOrder shadowOrder = WorldOrder(entity.Type, world) with { Depth = (short)TypeCatalog.ParseZOrder("zoFLYER_SHADOWS") };
            items.Add(new WorldDisplayItem(shadowOrder, batch => DrawShadow(batch, entity.Type, body, Screen(world, center))));
            items.Add(new WorldDisplayItem(WorldOrder(entity.Type, world), batch =>
                DrawSprite(batch, entity.Type.LoadIndex, body, Screen(world, center), _playerColors.GetValueOrDefault(entity.Owner)), entity, stored));
            return;
        }
        items.Add(new WorldDisplayItem(WorldOrder(entity.Type, world), batch =>
        {
            if (!entity.IsComplete && entity.Production == null && ObjectKinds.IsBuilding(entity.Kind))
            {
                DrawConstruction(batch, entity, center, entity.AwaitingBuilder);
                return;
            }
            (TypeInfo drawn, StructureFrames frames, Vector2 shift) = ObjectSprite(entity.Type, entity, stored);
            Vector2 anchor = Screen(world, center);
            bool painted = DrawObjectSprite(batch, drawn, frames, anchor + shift * _zoom,
                entity.Production != null ? ProductionOpacity(entity) : 1, _playerColors.GetValueOrDefault(entity.Owner));
            if (!painted && stored != null)
                batch.Draw(_pixel, new Rectangle((int)anchor.X - 3, (int)anchor.Y - 3, 6, 6),
                    entity.Owner == 2 ? Color.OrangeRed : Color.Turquoise);
        }, entity, stored));
    }

    /// <summary>각 분류의 현재 표시 대상을 모아 원본 zorder 내림차순·y·x 오름차순으로 한 번 정렬한다.</summary>
    private IReadOnlyList<WorldDisplayItem> CollectWorldDisplay(Vector2 center)
    {
        RefreshTerritoryAppearance();
        var items = new List<WorldDisplayItem>();
        // 사라진 개발용 지지 타일과 edgeFarm으로 대체되는 지면을 제외한다.
        foreach (FortTerrainTile tile in _terrain.Tiles)
        {
            Vector2 world = WorldPixels(tile.X, tile.Y);
            if (!WorldVisible(world, center, TileCullMargin) || _edgeFarmCells.Contains((tile.X, tile.Y))
                || tile.Region < 0 && !_session.Bridges.IsIsland(tile.X, tile.Y)) continue;
            AddWorldSprite(items, _terrainType, MapSpriteFrames.BodyFrame(_terrainType.Definition, tile.Cluster), world, center,
                _islandColors ? _playerColors.GetValueOrDefault(tile.Owner) : 0);
        }
        // 절벽은 지면으로부터 이동한 표시 기준점과 원본 zoSTALAG 깊이를 사용한다.
        foreach (FortTerrainFringeSprite fringe in _fringes)
        {
            Vector2 world = WorldPixels(fringe.X, fringe.Y);
            if (!WorldVisible(world, center, TileCullMargin)
                || !_session.Bridges.IsIsland(fringe.X, fringe.Y - FortTerrainFringe.OffsetY)) continue;
            AddWorldSprite(items, _fringeType, MapSpriteFrames.BodyFrame(_fringeType.Definition, fringe.Cluster), world, center);
        }
        // 가장자리 농장은 대체한 지면과 같은 목록에서 좌표 순서를 따른다.
        foreach (FortEdgeFarmTile tile in _edgeFarms)
        {
            Vector2 world = WorldPixels(tile.X, tile.Y);
            if (!WorldVisible(world, center, TileCullMargin)) continue;
            AddWorldSprite(items, _edgeFarmType, MapSpriteFrames.BodyFrame(_edgeFarmType.Definition, tile.Cluster), world, center,
                _islandColors ? _playerColors.GetValueOrDefault(tile.Owner) : 0);
        }
        // 저장 받침은 지지를 잃은 즉시 제외한다.
        foreach (FortIslandSupport support in _terrain.Supports)
            if (_session.Bridges.IsIsland(support.X, support.Y))
                AddWorldSupport(items, WorldPixels(support.X, support.Y), center, support.Owner);
        // 새 건물 받침과 미션에서 생성한 연습 가이저 받침도 같은 깊이로 등록한다.
        foreach (GameEntity entity in _session.Entities)
        {
            bool practice = entity.Kind == ObjectKind.Geyser && entity.Source != null && !_map.Objects.Contains(entity.Source);
            bool created = entity.Source == null && entity.IsComplete && entity.Type.Definition.HasFlag("createsisland")
                && _session.Map.TerritoryAt(entity.Footprint.AnchorX, entity.Footprint.AnchorY) == null
                && !_terrain.Supports.Any(s => s.X == entity.Footprint.AnchorX && s.Y == entity.Footprint.AnchorY);
            if (practice || created)
                AddWorldSupport(items, WorldPixels(entity.Footprint.AnchorX, entity.Footprint.AnchorY), center, entity.Owner,
                    practice ? 0 : null);
        }
        // 저장 다리는 아래의 실제 다리 상태 목록에서 한 번만 그린다. 비엔티티 장식도 깊이 정렬에 참여한다.
        foreach (FortMapObject stored in _map.Objects)
        {
            TypeInfo type = stored.Object.Type;
            if (type.Name is "noIsland" or "bridge" || IsRemovedInitialObject(stored)) continue;
            if (_session.EntityForInitial(stored) is { } entity)
            {
                AddWorldEntity(items, entity, center, stored);
                continue;
            }
            Vector2 world = WorldPixels(stored.X, stored.Y);
            if (HiddenByBuildingToggle(type) || !WorldVisible(world, center, ObjectCullMargin)) continue;
            (TypeInfo drawn, StructureFrames frames, Vector2 shift) = ObjectSprite(type, null, stored);
            items.Add(new WorldDisplayItem(WorldOrder(type, world), paint =>
                DrawObjectSprite(paint, drawn, frames, Screen(world, center) + shift * _zoom,
                    color: _playerColors.GetValueOrDefault(stored.Object.LoadOwner)), Stored: stored));
        }
        // 새로 생성한 유닛·건물은 저장 여부와 무관하게 같은 원본 비교 규칙을 따른다.
        foreach (GameEntity entity in _session.Entities.Where(e => e.Source == null || !_map.Objects.Contains(e.Source)))
            AddWorldEntity(items, entity, center);
        // 저장·추가 다리를 모두 현재 금감·강화 상태로 한 번씩 그린다.
        foreach (BridgeCellState cell in _session.Bridges.Cells)
        {
            Vector2 world = WorldPixels(cell.X, cell.Y);
            if (!WorldVisible(world, center, TileCullMargin)) continue;
            AddWorldSprite(items, _bridgeType, BridgeFrames.Find(_bridgeType.Definition.Frames, cell.Cell, cell.Condition), world, center);
        }
        // 취소 공사장은 선택되지 않는 그림으로 남되 살아 있는 건물과 같은 깊이에 놓는다.
        foreach (CancelledSiteVisual cancelled in _cancelledSites)
        {
            if (cancelled.Age >= CancelledSiteSeconds) continue;
            Vector2 world = WorldPixels(cancelled.Site.Footprint.AnchorX, cancelled.Site.Footprint.AnchorY);
            if (!WorldVisible(world, center, ObjectCullMargin)) continue;
            items.Add(new WorldDisplayItem(WorldOrder(cancelled.Site.Type, world),
                paint => DrawConstruction(paint, cancelled.Site, center, true)));
        }
        // 낙하체는 원본 zoFALLING 깊이와 현재 낙하 위치로 지면 뒤에 그린다.
        foreach (FallingSprite falling in _falling)
        {
            float age = (float)falling.Age;
            Vector2 world = falling.World + new Vector2(0, FallStartSpeed * age + 0.5f * FallAcceleration * age * age);
            if (!WorldVisible(world, center, ObjectCullMargin)) continue;
            float alpha = (float)Math.Clamp((falling.Duration - falling.Age) / FallFadeSeconds, 0, 1);
            var order = new DrawOrder(world.X / FortMap.CellPixelWidth, world.Y / FortMap.CellPixelHeight,
                (short)TypeCatalog.ParseZOrder("zoFALLING"));
            items.Add(new WorldDisplayItem(order, paint =>
                DrawSprite(paint, falling.TypeIndex, falling.Frame, Screen(world, center), falling.Color, alpha: alpha)));
        }
        AddSunForceFields(items, center);
        AddProductionDeliveries(items, center);
        AddCombatEffects(items, center);
        _worldDisplayView = (_camera, center, _zoom, _hideBuildings);
        return DrawOrder.Sort(items, item => item.Order);
    }

    /// <summary>카메라·배율·숨김 변경 뒤 첫 그리기보다 입력이 먼저 오면 동일한 수집기로 현재 화면 대상을 갱신한다.</summary>
    private void EnsureWorldDisplayView()
    {
        if (_worldDisplay.Count == 0 || _worldDisplayView != (_camera, _lastCenter, _zoom, _hideBuildings))
            _worldDisplay = CollectWorldDisplay(_lastCenter);
    }

    /// <summary>공통 수집기가 만든 원본 정렬 목록을 화면과 선택 판정의 기준으로 보존하고 그린다.</summary>
    private void DrawWorldDisplay(SpriteBatch batch, Vector2 center)
    {
        _worldDisplay = CollectWorldDisplay(center);
        // 실제 그리기와 다음 입력 판정이 같은 정렬 결과를 공유한다.
        foreach (WorldDisplayItem item in _worldDisplay) item.Paint(batch);
    }
}
