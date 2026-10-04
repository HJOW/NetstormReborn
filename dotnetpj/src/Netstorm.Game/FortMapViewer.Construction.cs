using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>사제 도착 전 공사장과 도착 뒤 바닥부터 드러나는 건물. TEST02 원본 녹화의 화면 순서를 따른다.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>사제가 오기 전 공사장에 남는 바닥·초목 띠의 높이. 정확한 원본 마스크는 미복원인 화면 근사다.</summary>
    private const int SiteGroundPixels = 20;

    /// <summary>워크샵 완공 직전에 창문 조각을 켜는 진행률. 원본 관찰의 순서를 구현한 근사다.</summary>
    private const double ConstructionLightsAt = 0.9;

    /// <summary>건물 그림자를 먼저 그리고 바닥부터 현재 진행률에 해당하는 높이만 드러낸다.</summary>
    private void DrawConstruction(SpriteBatch batch, GameEntity site, Vector2 center, bool waiting)
    {
        TypeInfo type = site.Type;
        int frame = type.Definition.Frames.DefaultFrame;
        Vector2 anchor = Screen(WorldPixels(site.Footprint.AnchorX, site.Footprint.AnchorY), center) + HotFootShift(type) * _zoom;
        DrawShadow(batch, type, frame, anchor, 1);
        if (GetTexture(type.LoadIndex, frame, _playerColors.GetValueOrDefault(site.Owner)) is not { } sprite) return;
        double progress = waiting ? 0 : _session.ConstructionProgress(site);
        int height = waiting ? Math.Min(SiteGroundPixels, sprite.Texture.Height)
            : Math.Clamp((int)Math.Ceiling(sprite.Texture.Height * progress), 1, sprite.Texture.Height);
        var source = new Rectangle(0, sprite.Texture.Height - height, sprite.Texture.Width, height);
        Vector2 top = anchor + (sprite.Offset.ToVector2() + new Vector2(0, source.Y)) * _zoom;
        batch.Draw(sprite.Texture, top, source, waiting ? SiteShadowTint * SiteShadowAlpha : Color.White,
            0f, Vector2.Zero, _zoom, SpriteEffects.None, 0f);
        if (!waiting && site.Kind == ObjectKind.Workshop && progress >= ConstructionLightsAt)
        {
            // Sun 워크샵의 창문 불은 본체와 별개인 원본 A02 클러스터다.
            int lights = type.Definition.Frames.Find('A', TypeFrameTable.DefaultVariant, 2);
            if (lights >= 0) DrawSprite(batch, type.LoadIndex, lights, anchor, _playerColors.GetValueOrDefault(site.Owner));
        }
        if (!_playUi && !waiting) DrawProgressBar(batch, anchor, progress);
    }

    /// <summary>논리상 취소되어 점유하지 않는 현장의 그림자를 잠깐 더 보여 준다.</summary>
    private void DrawCancelledSites(SpriteBatch batch, Vector2 center)
    {
        // 취소 순서대로 화면용 그림자를 그리며 실제 건물 목록에는 넣지 않는다.
        foreach (CancelledSiteVisual cancelled in _cancelledSites)
        {
            if (cancelled.Age < CancelledSiteSeconds) DrawConstruction(batch, cancelled.Site, center, waiting: true);
        }
    }
}
