using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>스톰 파워의 파란 꼬리·원본 원소 보석 그림·자원 도착 후 실체화를 표시한다.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>파란 스톰 파워 꼬리가 남는 게임 시간(초). 화면 연출 근사이며 규칙에는 영향이 없다.</summary>
    private const double ProductionTrailSeconds = 0.55;
    /// <summary>꼬리를 찍을 논리 픽셀 간격.</summary>
    private const float ProductionParticleSpacing = 2;
    /// <summary>실체화 첫 순간의 불투명도. 남은 실체화 시간 동안 1까지 증가한다.</summary>
    private const float ProductionInitialOpacity = 0.2f;
    /// <summary>스톰 파워 머리의 논리 픽셀 크기.</summary>
    private const int ProductionHeadSize = 3;
    /// <summary>스톰 파워 이동음의 반복 간격(초). 팬게임의 경로 전환음 대신 쓰는 임시 주기다.</summary>
    private const double ProductionSoundSeconds = 0.25;
    /// <summary>스톰 파워 꼬리의 시작 색. 팬게임의 파랑→흰색 입자를 참고한다.</summary>
    private static readonly Color ProductionTrailBlue = new(35, 90, 255);
    /// <summary>생산 중인 유닛 번호별 꼬리. 시뮬레이션 시간·좌표만 보존한다.</summary>
    private readonly Dictionary<int, List<(double Time, double X, double Y)>> _productionTrails = [];
    /// <summary>다음 스톰 파워 이동음을 재생할 게임 시각.</summary>
    private double _nextProductionSound;

    /// <summary>자동 입력 검사용 생산 단계·기점·도착한 자원 수. 게임 화면에는 표시하지 않는다.</summary>
    private string ProductionDetail => string.Join(',', _session.Entities.Where(e => e.Owner == TestPlayer && e.Production != null)
        .Select(e => $"{e.Type.Name.ToLowerInvariant()}:{(e.Production!.AwaitingDeliveries ? "delivery" : "materializing")}:source={e.Production.SourceId}:origin={_session.Entity(e.Production.SourceId)?.Type.Name.ToLowerInvariant() ?? "gone"}:arrived={e.Production.Deliveries.Count(d => d.HasArrived)}/{e.Production.Deliveries.Count}"))
        + $";productioncount={_session.Entities.Count(e => e.Owner == TestPlayer && e.Production != null)}";

    /// <summary>모든 자원이 도착한 뒤 유닛이 반투명에서 불투명으로 변하는 정도.</summary>
    private float ProductionOpacity(GameEntity entity) => ProductionInitialOpacity + (1 - ProductionInitialOpacity)
        * (float)_session.ConstructionProgress(entity);

    /// <summary>운송의 직전·현재 틱 좌표를 보간해 화면 좌표로 바꾼다. 일시정지 때는 현재 틱에 고정한다.</summary>
    private Vector2 DeliveryScreen(ProductionDelivery delivery, Vector2 center)
    {
        double alpha = SimulationRunning ? _session.InterpolationAlpha : 1;
        double x = delivery.PreviousX + (delivery.X - delivery.PreviousX) * alpha;
        double y = delivery.PreviousY + (delivery.Y - delivery.PreviousY) * alpha;
        return Screen(new Vector2((float)(x * FortMap.CellPixelWidth), (float)(y * FortMap.CellPixelHeight)), center);
    }

    /// <summary>활성 운송만 그린다. 저장 맵 오브젝트와 자원이 도착한 유닛에는 새 효과를 붙이지 않는다.</summary>
    private void DrawProductionDeliveries(SpriteBatch batch, Vector2 center)
    {
        var active = new HashSet<int>();
        TypeInfo? manaBolt = _knowledgeTypes.Find("manabolt");
        bool movingSound = false;
        // 엔티티 번호순으로 독립적인 자원 운송을 그린다.
        foreach (GameEntity entity in _session.Entities.Where(e => e.Production != null))
        {
            // 스톰 파워와 필요 원소 보석은 각각의 현재 위치에서 그린다.
            foreach (ProductionDelivery delivery in entity.Production!.Deliveries.Where(d => !d.HasArrived))
            {
                Vector2 position = DeliveryScreen(delivery, center);
                if (delivery.IsStormPower)
                {
                    active.Add(entity.Id);
                    DrawStormDelivery(batch, entity.Id, delivery, position, center);
                    movingSound |= position.X >= PlaySidebarWidth && position.X < _viewSize.X
                        && position.Y >= HeaderHeight && position.Y < _viewSize.Y;
                }
                else if (manaBolt != null)
                {
                    char side = delivery.Element switch { Element.Wind => 'A', Element.Rain => 'B', Element.Thunder => 'C', _ => 'D' };
                    int frame = manaBolt.Definition.Frames.Find(side, TypeFrameTable.DefaultVariant, 1);
                    if (frame >= 0) DrawSprite(batch, manaBolt.LoadIndex, frame, position);
                }
            }
        }
        // 끝난 생산의 꼬리를 비워 장시간 플레이에도 기록이 쌓이지 않게 한다.
        foreach (int id in _productionTrails.Keys.Where(id => !active.Contains(id)).ToArray()) _productionTrails.Remove(id);
        if (SimulationRunning && movingSound && _session.Seconds >= _nextProductionSound)
        {
            QueueSound("teleportEffect.wav");
            _nextProductionSound = _session.Seconds + ProductionSoundSeconds;
        }
    }

    /// <summary>스톰 파워가 지나온 실제 경로를 짧은 파랑·흰색 입자 꼬리로 표시한다. 시뮬레이션 난수는 쓰지 않는다.</summary>
    private void DrawStormDelivery(SpriteBatch batch, int id, ProductionDelivery delivery, Vector2 position, Vector2 center)
    {
        if (!_productionTrails.TryGetValue(id, out var trail)) _productionTrails[id] = trail = [];
        double now = _session.Seconds;
        if (trail.Count == 0 || trail[^1].Time < now) trail.Add((now, delivery.X, delivery.Y));
        trail.RemoveAll(p => now - p.Time > ProductionTrailSeconds);
        // 기록된 경로의 각 선분을 촘촘히 채워 낮은 화면 FPS에서도 꼬리가 점선으로 끊기지 않게 한다.
        for (int index = 1; index < trail.Count; index++)
        {
            var previous = trail[index - 1];
            var current = trail[index];
            Vector2 from = Screen(new Vector2((float)(previous.X * FortMap.CellPixelWidth), (float)(previous.Y * FortMap.CellPixelHeight)), center);
            Vector2 to = index == trail.Count - 1 ? position
                : Screen(new Vector2((float)(current.X * FortMap.CellPixelWidth), (float)(current.Y * FortMap.CellPixelHeight)), center);
            int count = Math.Max(1, (int)(Vector2.Distance(from, to) / (ProductionParticleSpacing * _zoom)));
            float life = (float)Math.Clamp(1 - (now - current.Time) / ProductionTrailSeconds, 0, 1);
            // 각 선분의 결정적인 작은 흔들림은 같은 프레임에서 여러 번 그려도 같은 모양이다.
            for (int particle = 0; particle < count; particle++)
            {
                Vector2 point = Vector2.Lerp(from, to, particle / (float)count);
                point.Y += ((index + particle) % 3 - 1) * _zoom;
                Color color = Color.Lerp(ProductionTrailBlue, Color.White, life * life) * life;
                int size = Math.Max(1, (int)(ProductionParticleSpacing * _zoom));
                batch.Draw(_pixel, new Rectangle((int)point.X, (int)point.Y, size, size), color);
            }
        }
        int head = Math.Max(1, (int)(ProductionHeadSize * _zoom));
        batch.Draw(_pixel, new Rectangle((int)position.X - head / 2, (int)position.Y - head / 2, head, head), Color.White);
    }
}
