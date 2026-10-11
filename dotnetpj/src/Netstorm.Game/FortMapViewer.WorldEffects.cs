using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Display;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>탄·착탄·번개를 현재 위치의 원본 타입 깊이로 공통 월드 목록에 넣는다.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>비행 중 탄의 실제 타입. 절차적 태양 탄과 번개는 타입이 없어 zoMISSILES 깊이를 사용한다.</summary>
    private TypeInfo? ProjectileType(CombatShot shot)
    {
        string? name = shot.AttackerType?.ToLowerInvariant() switch
        {
            "raincannon" => "rainCannonMissile", "thundercannon" => "thunderCannonMissile",
            "sunarcher" => "sunDisc", "windarcher" => "bolt", _ => null,
        };
        return name == null ? null : _knowledgeTypes.Find(name);
    }

    /// <summary>시뮬레이션 보간 위치의 탄·착탄·번개를 등록한다. 체력·사정거리 표시는 별도 화면 오버레이다.</summary>
    private void AddCombatEffects(List<WorldDisplayItem> items, Vector2 center)
    {
        // 날아가는 탄은 저장·신규 객체 및 비행체와 함께 깊이 정렬한다.
        foreach (CombatShot shot in _session.Shots.Where(s => !s.IsBeam))
        {
            float progress = (float)Math.Clamp((CombatRenderTick - shot.FiredTick) / (shot.ImpactTick - shot.FiredTick), 0, 1);
            Vector2 from = CellCenterScreen(shot.StartX, shot.StartY, center);
            Vector2 to = CellCenterScreen(shot.EndX, shot.EndY, center);
            Vector2 point = Vector2.Lerp(from, to, progress);
            var order = new DrawOrder((float)(shot.StartX + (shot.EndX - shot.StartX) * progress),
                (float)(shot.StartY + (shot.EndY - shot.StartY) * progress),
                (short)(ProjectileType(shot)?.ZOrder ?? TypeCatalog.ParseZOrder("zoMISSILES")));
            // 타입이 없는 임시 탄도 확인된 탄 깊이를 사용한다.
            if (shot.AttackerType?.Equals("sunCannon", StringComparison.OrdinalIgnoreCase) == true)
                order = order with { Depth = (short)TypeCatalog.ParseZOrder("zoMISSILES") };
            items.Add(new WorldDisplayItem(order, paint =>
            {
                if (DrawProjectile(paint, shot, point, from, to)) return;
                Vector2 tail = Vector2.Lerp(from, to, Math.Max(0, progress - 0.08f));
                Line(paint, tail, point, shot.Owner == TestPlayer ? Color.Gold : Color.OrangeRed);
                paint.Draw(_pixel, new Rectangle((int)point.X - 2, (int)point.Y - 2, 4, 4), Color.White);
            }));
        }
        if (_knowledgeTypes.Find("anim") is { } animation)
        {
            // 착탄·폭발은 원본 anim의 zoFLARES 깊이와 실제 발생 위치를 사용한다.
            foreach (CombatImpact impact in _session.Impacts)
            {
                int frame = ProjectileAnimation.ImpactFrame(animation.Definition, impact, CombatRenderTick, _session.TicksPerSecond);
                if (frame < 0) continue;
                Vector2 world = new((float)impact.X * FortMap.CellPixelWidth, (float)impact.Y * FortMap.CellPixelHeight);
                items.Add(new WorldDisplayItem(WorldOrder(animation, world), paint =>
                    DrawSprite(paint, animation.LoadIndex, frame, CellCenterScreen(impact.X, impact.Y, center) - new Vector2(0, 20 * _zoom))));
            }
        }
        // 임시 절차적 번개는 세션 난수를 소비하지 않고 기존 모양을 탄 깊이에서 그린다.
        foreach (CombatShot beam in _session.Lightning)
        {
            var order = new DrawOrder((float)beam.EndX, (float)beam.EndY, (short)TypeCatalog.ParseZOrder("zoMISSILES"));
            items.Add(new WorldDisplayItem(order, paint =>
            {
                Vector2 from = CellCenterScreen(beam.StartX, beam.StartY, center) - new Vector2(0, 26 * _zoom);
                Vector2 to = CellCenterScreen(beam.EndX, beam.EndY, center) - new Vector2(0, 12 * _zoom);
                Vector2 previous = from;
                const int segments = 12; // 기존 근사 번개의 꺾임 수. 원본 절차적 알고리즘은 미복원이다.
                // 고정된 꺾임으로 번개를 잇는다.
                for (int index = 1; index <= segments; index++)
                {
                    Vector2 point = Vector2.Lerp(from, to, index / (float)segments);
                    if (index < segments) point.Y += (index % 2 == 0 ? 2 : -2) * _zoom;
                    Line(paint, previous, point, Color.LightCyan);
                    previous = point;
                }
            }));
        }
    }
}
