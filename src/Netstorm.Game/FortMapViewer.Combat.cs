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
            DrawSprite(batch, entity.Type.LoadIndex, frame, anchor);
        }
    }

    /// <summary>완공 신전 목록이 바뀔 때 지면 테마·소유자색을 다시 만든다. 섬 마스크는 그대로다.</summary>
    private void RefreshTerritoryAppearance()
    {
        GameEntity[] temples = _session.Entities.Where(e => e.Kind == ObjectKind.Temple && e.IsComplete).ToArray();
        string signature = string.Join(',', temples.Select(e => e.Id));
        if (signature == _terrainTempleSignature) return;
        _terrainTempleSignature = signature;
        _terrain = new FortTerrainPreview(_map, _terrainType.Definition, territory =>
        {
            GameEntity? temple = temples.FirstOrDefault(e => e.Territory == territory);
            return (temple?.Owner ?? 0, temple?.Type.Definition.GetString("theme") ?? "sun");
        });
        _fringes = FortTerrainFringe.Create(_terrain, _terrainType.Definition, _fringeType.Definition);
        var tiles = _terrain.Tiles.ToDictionary(tile => (tile.X, tile.Y));
        // 장식 위치는 다리 시작 금지 칸과 공유하므로 유지하고, 새 지면의 프레임과 소유자색만 바꾼다.
        _edgeFarms = _edgeFarms.Select(old => tiles.TryGetValue((old.X, old.Y), out FortTerrainTile? tile)
            ? old with { Cluster = tile.Cluster, Owner = tile.Owner } : old).ToArray();
    }

    /// <summary>손상·선택 체력, 기절 보호막 및 비행 중 탄을 그린다. 탄 그림과 막대 크기는 임시 표현이다.</summary>
    private void DrawCombat(SpriteBatch batch, SpriteFontBase font, Vector2 center)
    {
        int selected = _session.Player(TestPlayer).SelectedEntityId;
        // 엔티티 번호순으로 체력 표시를 겹쳐 그린다.
        foreach (GameEntity entity in _session.Entities.Where(e => e.MaxHitPoints > 0 && e.IsComplete))
        {
            if (entity.HitPoints >= entity.MaxHitPoints && entity.Id != selected) continue;
            Vector2 anchor = CellCenterScreen(entity.WorldX, entity.WorldY, center);
            double ratio = entity.HitPoints / entity.MaxHitPoints;
            Color color = ratio > 0.5 ? Color.LimeGreen : ratio > 0.25 ? Color.Gold : Color.OrangeRed;
            int width = Math.Max(8, (int)(HealthBarWidth * _zoom));
            var bar = new Rectangle((int)anchor.X - width / 2, (int)(anchor.Y - 28 * _zoom), width, 4);
            batch.Draw(_pixel, bar, Color.Black);
            batch.Draw(_pixel, new Rectangle(bar.X, bar.Y, (int)(width * ratio), bar.Height), color);
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
            if (entity.Id == selected)
                batch.DrawString(font, $"{entity.DisplayName} {entity.HitPoints:0}/{entity.MaxHitPoints:0}",
                    anchor + new Vector2(0, 10), Color.White);
        }
        // 세션 틱 사이의 탄 경로를 선형 보간한다. 정지 중에는 탄도 같은 위치에 남는다.
        foreach (CombatShot shot in _session.Shots)
        {
            if (shot.IsBeam) continue;
            float progress = (float)Math.Clamp((_session.Tick - shot.FiredTick) / (double)(shot.ImpactTick - shot.FiredTick), 0, 1);
            Vector2 from = CellCenterScreen(shot.StartX, shot.StartY, center);
            Vector2 to = CellCenterScreen(shot.EndX, shot.EndY, center);
            Vector2 point = Vector2.Lerp(from, to, progress);
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
