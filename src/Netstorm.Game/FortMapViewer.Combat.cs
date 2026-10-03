using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>전투 상태의 체력·보호막·탄 표시와 신전 소유권 변경 후 지면 갱신.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>체력 막대의 논리 화면 폭.</summary>
    private const int HealthBarWidth = 36;

    /// <summary>직전 지면 갱신에 사용한 완공 신전 번호 목록.</summary>
    private string? _terrainTempleSignature;

    /// <summary>비행체를 지상·다리 위에 원본 A00~A07 회전 그림으로 그린다. 12fps는 임시 재생 속도다.</summary>
    private void DrawFlyers(SpriteBatch batch, Vector2 center)
    {
        // 연속 비행 좌표로 그려 칸 경계에서 튀지 않게 한다. 지상 깊이 정렬과 분리한다.
        foreach (GameEntity entity in _session.Entities.Where(e => e.Kind == ObjectKind.Flyer))
        {
            TypeDefinition type = entity.Type.Definition;
            int count = type.Clusters.Count(c => c.Name.StartsWith("A", StringComparison.Ordinal));
            int frame = count == 0 ? type.Frames.DefaultFrame :
                type.Frames.Find('A', TypeFrameTable.DefaultVariant, (int)(_session.Tick / 2 % count));
            if (frame < 0) frame = type.Frames.DefaultFrame;
            Vector2 anchor = Screen(new Vector2((float)(entity.WorldX * FortMap.CellPixelWidth),
                (float)(entity.WorldY * FortMap.CellPixelHeight)), center);
            DrawShadow(batch, entity.Type, frame, anchor);
            DrawSprite(batch, entity.Type.LoadIndex, frame, anchor, _playerColors.GetValueOrDefault(entity.Owner));
        }
    }

    /// <summary>완공 신전 목록이나 섬 테마 숨기기(Shift+F3)가 바뀔 때 지면 테마·소유자색을 다시 만든다. 섬 마스크는 그대로다.</summary>
    private void RefreshTerritoryAppearance()
    {
        GameEntity[] temples = _session.Entities.Where(e => e.Kind == ObjectKind.Temple && e.IsComplete).ToArray();
        string signature = string.Join(',', temples.Select(e => e.Id)) + (_hideIslandThemes ? "|plain" : "");
        if (signature == _terrainTempleSignature) return;
        _terrainTempleSignature = signature;
        // 섬 테마를 숨기면(Shift+F3) 소유자 색은 그대로 두고 지면만 신전이 없는 섬과 같은 기본(초록) 테마로 그린다
        _terrain = new FortTerrainPreview(_map, _terrainType.Definition, territory =>
        {
            GameEntity? temple = temples.FirstOrDefault(e => e.Territory == territory);
            return (temple?.Owner ?? 0, _hideIslandThemes ? "sun" : temple?.Type.Definition.GetString("theme") ?? "sun");
        });
        _fringes = FortTerrainFringe.Create(_terrain, _terrainType.Definition, _fringeType.Definition);
        var tiles = _terrain.Tiles.ToDictionary(tile => (tile.X, tile.Y));
        // 장식 위치는 다리 시작 금지 칸과 공유하므로 유지하고, 새 지면의 프레임과 소유자색만 바꾼다.
        _edgeFarms = _edgeFarms.Select(old => tiles.TryGetValue((old.X, old.Y), out FortTerrainTile? tile)
            ? old with { Cluster = tile.Cluster, Owner = tile.Owner } : old).ToArray();
    }

    /// <summary>
    /// 오브젝트가 지금 그리는 프레임의 그림 상자(셰이프 헤더의 폭·높이·기준점)를 화면 좌표로 구한다.
    /// 원본은 이 상자를 기준으로 선택 괄호와 체력 막대를 그린다. 비행 중이거나 그림이 없으면 null.
    /// </summary>
    private Rectangle? EntityFrameBox(GameEntity entity, Vector2 center)
    {
        if (entity.Flight != null || entity.Kind == ObjectKind.Flyer) return null;
        (TypeInfo type, StructureFrames frames, Vector2 shift) = ObjectSprite(entity.Type, entity, entity.Source);
        ShapeBlock block = _shapes.Blocks[type.LoadIndex];
        if ((uint)frames.Body >= (uint)block.Frames.Count || block.Frames[frames.Body].IsSpecial) return null;
        ShapeFrame frame = block.Frames[frames.Body];
        Vector2 anchor = Screen(IsMobile(entity) ? MobileWorldPixels(entity)
            : WorldPixels(entity.Footprint.AnchorX, entity.Footprint.AnchorY), center) + shift * _zoom;
        // 헤더 값은 (높이, 폭)·(기준점 y, 기준점 x) 순서다.
        return new Rectangle((int)MathF.Round(anchor.X - frame.Origin1 * _zoom), (int)MathF.Round(anchor.Y - frame.Origin0 * _zoom),
            (int)MathF.Round(frame.Bounds1 * _zoom), (int)MathF.Round(frame.Bounds0 * _zoom));
    }

    /// <summary>그림 상자 위에 원본 모양의 체력 막대(검은 바탕 + 초록·노랑·빨강)를 그린다.</summary>
    private void DrawHealthBar(SpriteBatch batch, Rectangle box, GameEntity entity)
    {
        int hitPoints = (int)entity.HitPoints, maximum = (int)entity.MaxHitPoints;
        PixelRect back = SelectionMarks.BarBackground(box.X, box.Y, box.Width);
        PixelRect fill = SelectionMarks.BarFill(box.X, box.Y, box.Width, hitPoints, maximum);
        (byte r, byte g, byte b) = SelectionMarks.HealthColor(hitPoints, maximum);
        batch.Draw(_pixel, new Rectangle(back.X, back.Y, back.Width, back.Height), Color.Black);
        batch.Draw(_pixel, new Rectangle(fill.X, fill.Y, fill.Width, fill.Height), new Color(r, g, b));
    }

    /// <summary>그림 상자의 아래 두 모서리에 소유자 색의 2픽셀 괄호를 그린다 (원본 0x498841 의 선 여덟 개).</summary>
    private void DrawSelectionBrackets(SpriteBatch batch, Rectangle box, int owner)
    {
        Rgb rgb = _palette[SelectionMarks.BracketPaletteIndex(_playerColors.GetValueOrDefault(owner))];
        var color = new Color(rgb.R, rgb.G, rgb.B);
        (int horizontal, int vertical) = SelectionMarks.BracketArms(box.Width, box.Height);
        // 바깥 선과 한 픽셀 안쪽 선을 차례로 그린다
        for (int inset = 0; inset < 2; inset++)
        {
            int left = box.X + inset, right = box.Right - inset, bottom = box.Bottom - inset;
            batch.Draw(_pixel, new Rectangle(left, bottom - vertical, 1, vertical + 1), color);
            batch.Draw(_pixel, new Rectangle(left, bottom, horizontal + 1, 1), color);
            batch.Draw(_pixel, new Rectangle(right, bottom - vertical, 1, vertical + 1), color);
            batch.Draw(_pixel, new Rectangle(right - horizontal, bottom, horizontal + 1, 1), color);
        }
    }

    /// <summary>손상·선택 체력, 기절 보호막 및 비행 중 탄을 그린다. 탄 그림은 임시 표현이다.</summary>
    private void DrawCombat(SpriteBatch batch, SpriteFontBase font, Vector2 center)
    {
        int selected = _session.Player(TestPlayer).SelectedEntityId;
        // 엔티티 번호순으로 체력 표시를 겹쳐 그린다.
        foreach (GameEntity entity in _session.Entities.Where(e => e.MaxHitPoints > 0 && e.IsComplete && !e.IsRegenerating))
        {
            if (entity.HitPoints >= entity.MaxHitPoints && entity.Id != selected && !entity.IsStunned) continue;
            Vector2 anchor = EntityCenterScreen(entity, center);
            if (EntityFrameBox(entity, center) is { } box)
            {
                // 원본 방식: 그림 상자 위의 체력 막대와, 선택한 오브젝트의 아래 두 모서리 괄호
                DrawHealthBar(batch, box, entity);
                if (entity.Id == selected) DrawSelectionBrackets(batch, box, entity.Owner);
            }
            else
            {
                // 그림 상자를 구할 수 없는 오브젝트(비행 중인 공격체 등)는 중심 위의 임시 막대로 표시한다.
                double ratio = entity.HitPoints / entity.MaxHitPoints;
                Color color = ratio > 0.5 ? Color.LimeGreen : ratio > 0.25 ? Color.Gold : Color.OrangeRed;
                int width = Math.Max(8, (int)(HealthBarWidth * _zoom));
                var bar = new Rectangle((int)anchor.X - width / 2, (int)(anchor.Y - 28 * _zoom), width, 4);
                batch.Draw(_pixel, bar, Color.Black);
                batch.Draw(_pixel, new Rectangle(bar.X, bar.Y, (int)(width * ratio), bar.Height), color);
            }
            if (entity.IsStunned)
            {
                // 원본 보호막 애니메이션 연결 전의 황금색 고리 표시다.
                const int segments = 16; // 보호막 타원을 그릴 선분 수.
                for (int index = 0; index < segments; index++)
                {
                    float first = MathHelper.TwoPi * index / segments;
                    float next = MathHelper.TwoPi * (index + 1) / segments;
                    Vector2 a = anchor + new Vector2(MathF.Cos(first) * 12, MathF.Sin(first) * 7) * _zoom;
                    Vector2 b = anchor + new Vector2(MathF.Cos(next) * 12, MathF.Sin(next) * 7) * _zoom;
                    Line(batch, a, b, Color.Gold);
                }
            }
            // 원본 플레이 화면은 선택한 오브젝트 옆에 이름·체력 글자를 쓰지 않는다 (맵 시험 화면에서만 표시).
            if (entity.Id == selected && !_playUi)
                batch.DrawString(font, $"{entity.DisplayName} {entity.HitPoints:0}/{entity.MaxHitPoints:0}",
                    anchor + new Vector2(0, 10), Color.White);
            if (entity.IsSuspended)
                batch.DrawString(font, "허공에서 기절", anchor + new Vector2(-38, -43), Color.LightSkyBlue);
        }
        // 길 복구를 기다리는 작업의 이유를 이동체 위치에 표시한다.
        foreach (GameEntity entity in _session.Entities.Where(e => _session.IsMoveBlocked(e.Id)))
        {
            Vector2 anchor = EntityCenterScreen(entity, center);
            batch.DrawString(font, "길 막힘·대기", anchor + new Vector2(-36, -43), Color.Gold);
        }
        // 세션 틱 사이의 탄 경로를 선형 보간한다. 정지 중에는 탄도 같은 위치에 남는다.
        foreach (CombatShot shot in _session.Shots)
        {
            if (shot.IsBeam) continue;
            float progress = (float)Math.Clamp((_session.Tick - shot.FiredTick) / (double)(shot.ImpactTick - shot.FiredTick), 0, 1);
            Vector2 from = CellCenterScreen(shot.StartX, shot.StartY, center);
            Vector2 to = CellCenterScreen(shot.EndX, shot.EndY, center);
            Vector2 point = Vector2.Lerp(from, to, progress);
            if (DrawCannonProjectile(batch, shot, point)) continue;
            Vector2 tail = Vector2.Lerp(from, to, Math.Max(0, progress - 0.08f));
            Line(batch, tail, point, shot.Owner == TestPlayer ? Color.Gold : Color.OrangeRed);
            batch.Draw(_pixel, new Rectangle((int)point.X - 2, (int)point.Y - 2, 4, 4), Color.White);
        }
        // 영상에서 확인한 흰색·청록색 번개를 짧은 꺾은 선으로 표현한다. 화면 난수는 세션 난수를 소비하지 않는다.
        foreach (CombatShot beam in _session.Lightning)
        {
            Vector2 from = CellCenterScreen(beam.StartX, beam.StartY, center) - new Vector2(0, 26 * _zoom);
            Vector2 to = CellCenterScreen(beam.EndX, beam.EndY, center) - new Vector2(0, 12 * _zoom);
            Vector2 previous = from;
            const int segments = 12; // 번개의 꺾임 수. 원본 절차적 번개 알고리즘은 미분석이다.
            for (int index = 1; index <= segments; index++)
            {
                Vector2 point = Vector2.Lerp(from, to, index / (float)segments);
                if (index < segments) point.Y += (index % 2 == 0 ? 2 : -2) * _zoom;
                Line(batch, previous, point, Color.LightCyan);
                previous = point;
            }
        }
    }
}
