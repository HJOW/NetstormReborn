using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 원본 방식의 배치 미리보기(플레이 화면): 들고 있는 유닛 그림, 발자국을 두른 흰 사각형, 왼쪽의 필요 원소 아이콘,
/// 아래쪽의 안내 글·노란 비용 숫자와 Storm Power 보석, 고정 캐논의 발사 방향으로 흘러가는 사거리 반짝임.
/// 근거: 2026-10-03 TEST01 녹화 103~162초 (docs/videos/test01-visuals-20261003.md 6절).
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>원소 아이콘(mana 타입, 16×16) 사이의 세로 간격. 녹화에서 두 아이콘이 17px 간격으로 쌓여 있었다.</summary>
    private const int ElementIconPitch = 17;

    /// <summary>발자국 사각형 아래 끝에서 첫 글줄까지의 간격(논리 픽셀).</summary>
    private const int PreviewTextGap = 2;

    /// <summary>사거리 반짝임 사이의 간격(칸). 녹화에서 세로 68·가로 100 캡처 픽셀 = 5칸이었다.</summary>
    private const double RangeSparkleSpacing = 5;

    /// <summary>사거리 반짝임이 바깥으로 흘러가는 속도(칸/초). 녹화에서 프레임당 세로 7·가로 10 캡처 픽셀 → 약 15칸/초.</summary>
    private const double RangeSparkleSpeed = 15;

    /// <summary>사거리 반짝임 그림(range 타입 A00~A12)이 바뀌는 초당 횟수. 측정하지 못해 증기와 같은 24Hz 로 둔 추정값이다.</summary>
    private const double RangeSparkleFramesPerSecond = 24;

    /// <summary>놓을 수 없는 자리(겹침·섬 밖)의 사각형과 채움 색. 녹화의 붉은 사각형.</summary>
    private static readonly Color PreviewBlockedColor = new(230, 40, 30);

    /// <summary>비용 숫자의 색 (녹화의 노란 글자).</summary>
    private static readonly Color PreviewCostColor = new(255, 216, 40);

    /// <summary>배치 미리보기가 지금 가리키는 발자국 기준 칸 (커서가 지도 밖이면 null).</summary>
    private (int X, int Y)? _placementCell;

    /// <summary>
    /// 원소 글자(w·r·t·s) → mana 타입의 아이콘 순번. 그림은 어두운 것 4장(Wind, Rain, Thunder, Sun) 뒤에 밝은 것 4장이 같은 순서로 있다.
    /// </summary>
    private const string ElementIconOrder = "wrts";

    /// <summary>
    /// 놓을 수 없는 이유에 맞는 원본 안내 글. 녹화·1-1 자동 분석에서 본 문구는 두 가지("Energy not satisfied",
    /// "Only on Controlled Island")이며, 겹친 자리는 글 없이 붉은 사각형만 보였다. 다른 이유의 문구는 확인하지 못해 비워 둔다.
    /// </summary>
    private string PreviewMessage(SessionPlacementCheck check)
    {
        if (check.Allowed || check.Failure != CommandFailure.Placement || check.Site == null) return "";
        return check.Site.Problem switch
        {
            PlacementProblem.NotEnoughEnergy => Ui("에너지가 충족되지 않았습니다", "Energy not satisfied"),
            PlacementProblem.OthersIsland or PlacementProblem.NotConnected or PlacementProblem.NotOnIslandOrBridgeEnd
                or PlacementProblem.BuildingNeedsIsland or PlacementProblem.TempleNeedsEmptyIsland
                => Ui("점령한 섬에만 놓을 수 있습니다", "Only on Controlled Island"),
            _ => "",
        };
    }

    /// <summary>사각형을 붉게 보일지: 자리 자체가 안 되는 경우다. 에너지·Storm Power 부족은 흰 사각형 그대로다.</summary>
    private static bool PreviewBlocked(SessionPlacementCheck check) =>
        !check.Allowed && !(check.Failure == CommandFailure.Placement
            && check.Site?.Problem is PlacementProblem.NotEnoughEnergy or PlacementProblem.NotEnoughStormPower);

    /// <summary>들고 있는 타입의 그림·발자국 사각형·원소 아이콘·안내 글·비용·사거리 반짝임을 커서 자리에 그린다.</summary>
    private void DrawPlayPlacement(SpriteBatch batch, Vector2 center)
    {
        if (_lastCheck is not { } check || _placementCell is not (int cellX, int cellY)) return;
        // 커서가 생산 창 위에 있는 동안에는 지도에 미리보기를 남기지 않는다.
        if (IsPlayUiPoint(_previousMouse.X, _previousMouse.Y)) return;
        TypeInfo type = _candidates[_candidateIndex];
        Footprint footprint = check.Site?.Footprint ?? Footprint.ForType(type.Definition, cellX, cellY);
        bool blocked = PreviewBlocked(check);
        // 발자국이 화면에 보이는 사각형: 왼쪽 위 칸의 왼쪽 위 모서리 ~ 기준 칸의 기준점(오른쪽 아래 모서리)
        Vector2 topLeft = Screen(WorldPixels(footprint.Left - 1, footprint.Top - 1), center);
        Vector2 bottomRight = Screen(WorldPixels(footprint.AnchorX, footprint.AnchorY), center);
        var rect = new Rectangle((int)MathF.Round(topLeft.X), (int)MathF.Round(topLeft.Y),
            (int)MathF.Round(bottomRight.X - topLeft.X), (int)MathF.Round(bottomRight.Y - topLeft.Y));
        DrawRangeSparkles(batch, type, footprint, center);
        // 유닛 그림: 고정 캐논은 고른 방위, 나머지는 놓였을 때와 같은 그림
        (TypeInfo drawn, StructureFrames frames, Vector2 shift) = ObjectSprite(type, null, null);
        int body = CannonAnimation.IsFixed(type)
            ? type.Definition.Frames.Find(CannonAnimation.Side(_cannonRotation), TypeFrameTable.DefaultVariant, 0) : frames.Body;
        DrawSprite(batch, drawn.LoadIndex, body, bottomRight + shift * _zoom, tint: blocked ? Color.Salmon : Color.White);
        if (blocked) batch.Draw(_pixel, rect, PreviewBlockedColor * 0.35f);
        Outline(batch, rect, blocked ? PreviewBlockedColor : Color.White);
        DrawElementIcons(batch, check, type, rect);
        // 사각형 아래: 안내 글(있으면) 다음 줄에 비용
        string message = PreviewMessage(check);
        float lineY = rect.Bottom + PreviewTextGap;
        if (message.Length > 0)
        {
            Vector2 size = _uiSkin.Body.MeasureString(message);
            OriginalUiSkin.Text(batch, _uiSkin.Body, message, new Vector2(rect.Center.X - size.X / 2, lineY));
            lineY += size.Y;
        }
        string cost = StormPower.TypeCost(type.Definition).ToString();
        Vector2 costSize = _uiSkin.Title.MeasureString(cost);
        float costX = rect.Center.X - (costSize.X + _uiSkin.FrameWidth("A03") + 2) / 2;
        OriginalUiSkin.Text(batch, _uiSkin.Title, cost, new Vector2(costX, lineY), PreviewCostColor);
        _uiSkin.DrawFrame(batch, "A03", new Point((int)(costX + costSize.X) + 2, (int)lineY + 2));
    }

    /// <summary>
    /// 필요 원소 아이콘을 사각형 왼쪽에 위에서부터 쌓아 그린다. 공급원이 배정된 원소는 밝은 그림, 아니면 어두운 그림이다.
    /// </summary>
    private void DrawElementIcons(SpriteBatch batch, SessionPlacementCheck check, TypeInfo type, Rectangle rect)
    {
        if (_knowledgeTypes.Find("mana") is not { } mana) return;
        EnergyRequirement requirement = check.Site?.Requirement ?? EnergyRequirement.ForType(type.Definition);
        // 요구 글자마다 아이콘 하나
        for (int i = 0; i < requirement.Letters.Length; i++)
        {
            int element = ElementIconOrder.IndexOf(requirement.Letters[i]);
            if (element < 0) continue;
            bool supplied = check.Site != null && i < check.Site.Energy.Assigned.Count && check.Site.Energy.Assigned[i] != null;
            int frame = element + (supplied ? ElementIconOrder.Length : 0);
            // mana 그림의 기준점은 오른쪽 아래이므로 사각형 왼쪽 변에 오른쪽을 맞춘다.
            DrawSprite(batch, mana.LoadIndex, frame, new Vector2(rect.Left, rect.Top + 16 + i * ElementIconPitch), scale: 1f);
        }
    }

    /// <summary>
    /// 고정 캐논(아이스·썬더)을 들고 있을 때 발사 방향으로 사거리만큼 반짝임을 5칸 간격으로 흘려보낸다.
    /// Crossbow 처럼 부채꼴로 쏘는 유닛의 두 갈래 반짝임은 각도 규칙을 확인하지 못해 그리지 않는다.
    /// </summary>
    private void DrawRangeSparkles(SpriteBatch batch, TypeInfo type, Footprint footprint, Vector2 center)
    {
        if (!CannonAnimation.IsFixed(type) || _knowledgeTypes.Find("range") is not { } sparkle) return;
        double range = type.Definition.GetDouble("range") ?? 0;
        IReadOnlyList<int> frames = sparkle.Definition.Frames.Sequence('A', TypeFrameTable.DefaultVariant);
        if (range <= 0 || frames.Count == 0) return;
        // 방위 0~3 = 북·동·남·서의 칸 방향
        (int dx, int dy) = (_cannonRotation % 4) switch { 0 => (0, -1), 1 => (1, 0), 2 => (0, 1), _ => (-1, 0) };
        // 발자국이 보이는 영역의 중심 (칸 c 는 픽셀 (16(c−1), 16c] 이므로 반 칸 앞당긴다)
        var origin = new Vector2((float)((footprint.CenterX - 0.5) * FortMap.CellPixelWidth), (float)((footprint.CenterY - 0.5) * FortMap.CellPixelHeight));
        double phase = _walkClock * RangeSparkleSpeed % RangeSparkleSpacing;
        // 발사 방향으로 사거리 끝까지 반짝임을 놓는다
        for (int index = 0; phase + index * RangeSparkleSpacing <= range; index++)
        {
            double distance = phase + index * RangeSparkleSpacing;
            var world = origin + new Vector2((float)(dx * distance * FortMap.CellPixelWidth), (float)(dy * distance * FortMap.CellPixelHeight));
            int frame = frames[(int)(_walkClock * RangeSparkleFramesPerSecond + index * 3) % frames.Count];
            DrawSprite(batch, sparkle.LoadIndex, frame, Screen(world, center));
        }
    }
}
