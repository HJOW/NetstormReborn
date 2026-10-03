using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>선택한 공격 건물과 들고 있는 공격 유닛의 원본 사거리 반짝임·생성·선택 해제 연출.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>반짝임 그림 변경 속도. 그림 타이머는 아직 측정하지 못해 기존 배치 표시의 24Hz 추정을 유지한다.</summary>
    private const double RangeFramesPerSecond = 24;

    /// <summary>동시에 남아 있는 새 표시와 선택 해제 후 줄어드는 표시.</summary>
    private readonly List<RangeVisual> _rangeVisuals = [];

    /// <summary>마지막 사거리 표시 갱신의 게임 시각. 일시정지 중에는 반짝임 위치와 그림도 멈춘다.</summary>
    private double _rangeClock;

    /// <summary>표시 하나의 수명·중심·규격. 선택을 바꾼 뒤 원형 표시가 줄어드는 동안 유지한다.</summary>
    private sealed class RangeVisual(string key, RangeDisplay display, Footprint footprint)
    {
        public string Key { get; } = key;
        public RangeDisplay Display { get; } = display;
        public Footprint Footprint { get; set; } = footprint;
        public double Age { get; set; }
        public double Radius { get; set; }
        public bool Closing { get; set; }
    }

    /// <summary>현재 배치 미리보기 또는 선택된 완공 공격 건물의 사거리 대상. 생산 창 위·다리 커서에서는 숨긴다.</summary>
    private (string Key, TypeInfo Type, RangeDisplay Display, Footprint Footprint)? RangeTarget()
    {
        if (_bridgeMode) return null;
        if (_placementMode)
        {
            if (_lastCheck is not { } check || _placementCell is not (int x, int y)
                || IsPlayUiPoint(_previousMouse.X, _previousMouse.Y)) return null;
            TypeInfo type = _candidates[_candidateIndex];
            if (RangeIndicator.ForType(type, _cannonRotation) is not { } display) return null;
            return ($"cursor:{type.LoadIndex}:{display.Direction}", type, display,
                check.Site?.Footprint ?? Footprint.ForType(type.Definition, x, y));
        }
        GameEntity? entity = _session.Entity(_session.Player(TestPlayer).SelectedEntityId);
        if (entity == null || !entity.IsComplete || entity.IsRegenerating || entity.Flight != null) return null;
        if (RangeIndicator.ForType(entity.Type, entity.CannonDirection) is not { } selectedDisplay) return null;
        return ($"entity:{entity.Id}:{selectedDisplay.Direction}", entity.Type, selectedDisplay, entity.Footprint);
    }

    /// <summary>자동 검사에만 제공하는 표시 대상·모양·방위. 제품 화면에는 진단 문자열을 쓰지 않는다.</summary>
    private string RangeDetail => RangeTarget() is { } target
        ? $"{target.Type.Name.ToLowerInvariant()}:{target.Display.Shape.ToString().ToLowerInvariant()}:{target.Display.Direction}" : "none";

    /// <summary>변경된 대상의 표시를 열고 이전 원형 표시를 줄인다. 직선은 원본처럼 선택 해제 즉시 없앤다.</summary>
    private void DrawRangeIndicators(SpriteBatch batch, Vector2 center)
    {
        double seconds = Math.Max(0, _walkClock - _rangeClock);
        _rangeClock = _walkClock;
        var target = RangeTarget();
        RangeVisual? active = _rangeVisuals.FirstOrDefault(visual => !visual.Closing);
        if (active != null && active.Key != target?.Key)
        {
            active.Closing = true;
            if (active.Display.Shape == RangeShape.Ray) _rangeVisuals.Remove(active);
            active = null;
        }
        if (target is { } desired)
        {
            if (active == null)
            {
                active = new RangeVisual(desired.Key, desired.Display, desired.Footprint);
                _rangeVisuals.Add(active);
            }
            active.Footprint = desired.Footprint;
        }
        if (_knowledgeTypes.Find("range") is not { } sparkle) return;
        // 활성·사라지는 표시를 함께 갱신해 선택 변경 때의 원형 수축을 보존한다.
        foreach (RangeVisual visual in _rangeVisuals)
        {
            visual.Age += seconds;
            visual.Radius = visual.Closing
                ? Math.Max(0, visual.Radius - seconds * RangeIndicator.ShrinkSpeed)
                : Math.Min(visual.Display.Distance, visual.Radius + seconds * RangeIndicator.GrowSpeed);
            IReadOnlyList<int> frames = sparkle.Definition.Frames.Sequence(visual.Display.FrameSide, TypeFrameTable.DefaultVariant);
            if (frames.Count == 0) continue;
            var origin = new Vector2((float)((visual.Footprint.CenterX - 0.5) * FortMap.CellPixelWidth),
                (float)((visual.Footprint.CenterY - 0.5) * FortMap.CellPixelHeight));
            // 같은 칸 단위 위치를 16:11 지면 비율로 바꿔 원형 사거리가 화면에서는 타원이 되게 한다.
            foreach (RangePoint point in RangeIndicator.Points(visual.Display, visual.Age, visual.Radius, _walkClock))
            {
                var world = origin + new Vector2((float)(point.X * FortMap.CellPixelWidth), (float)(point.Y * FortMap.CellPixelHeight));
                int frame = frames[(int)(_walkClock * RangeFramesPerSecond + point.Index * 3) % frames.Count];
                DrawSprite(batch, sparkle.LoadIndex, frame, Screen(world, center));
            }
        }
        _rangeVisuals.RemoveAll(visual => visual.Closing && visual.Radius <= 0);
    }
}
