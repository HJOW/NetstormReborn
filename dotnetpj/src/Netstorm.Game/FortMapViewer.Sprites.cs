using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 오브젝트의 지금 그림 고르기(가이저 증기·워크샵 레벨·신전 회오리·풍선 흔들림)와 그림자 그리기.
/// 프레임 규칙은 <see cref="StructureAnimation"/>, 근거는 2026-10-03 TEST01 녹화 대조(docs/videos/test01-visuals-20261003.md)다.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>
    /// 그림자의 불투명도(0~255). 녹화에서 가이저 그림자가 덮은 지면의 밝기가 주변의 0.59~0.70배(평균 약 0.64)였으므로
    /// 검은색을 36% 덮는다. 원본은 팔레트 음영표로 어둡게 하므로 색조까지 같지는 않다.
    /// </summary>
    private const int ShadowAlpha = 92;

    /// <summary>그림자를 그리는 방식. 타입의 typeflags 로 정해진다.</summary>
    private enum ShadowStyle
    {
        /// <summary>그림자 없음 (typeflags 에 shadow·flyershadow 가 없다)</summary>
        None,
        /// <summary>고르게 어두운 그림자 (typeflags shadow: 건물·가이저·걷는 유닛)</summary>
        Solid,
        /// <summary>한 픽셀 건너 한 픽셀만 어두운 체크무늬 그림자 (typeflags flyershadow: 풍선·비행체)</summary>
        Dithered,
    }

    /// <summary>타입 번호 → 그림자 방식. 매 프레임 typeflags 문자열을 훑지 않도록 기억한다.</summary>
    private readonly Dictionary<int, ShadowStyle> _shadowStyles = [];

    /// <summary>(프레임 위치, 체크무늬 여부) → 그림자 텍스처와 기준점 오프셋. 그림이 없는 프레임은 null 로 기억한다.</summary>
    private readonly Dictionary<(int Frame, bool Dithered), (Texture2D Texture, Point Offset)?> _shadowTextures = [];

    /// <summary>타입의 그림자 방식 (typeflags flyershadow → 체크무늬, shadow → 고른 그림자).</summary>
    private ShadowStyle ShadowStyleOf(TypeInfo type)
    {
        if (!_shadowStyles.TryGetValue(type.LoadIndex, out ShadowStyle style))
        {
            style = type.Definition.HasFlag("flyershadow") ? ShadowStyle.Dithered
                : type.Definition.HasFlag("shadow") ? ShadowStyle.Solid : ShadowStyle.None;
            _shadowStyles[type.LoadIndex] = style;
        }
        return style;
    }

    /// <summary>
    /// 그림자 레이어 프레임(셰이프 번호 = 클러스터 수 + 본체 클러스터)을 반투명 검정 텍스처로 만든다.
    /// 원본 그림자 프레임은 한 가지 팔레트 색(150 또는 217)으로 칠한 모양 틀이므로 색은 쓰지 않고 모양만 쓴다.
    /// </summary>
    /// <param name="type">오브젝트 타입</param>
    /// <param name="bodyFrame">본체 클러스터 번호</param>
    /// <param name="dithered">체크무늬 그림자인지</param>
    private (Texture2D Texture, Point Offset)? GetShadowTexture(TypeInfo type, int bodyFrame, bool dithered)
    {
        ShapeBlock block = _shapes.Blocks[type.LoadIndex];
        int index = type.Definition.Clusters.Count + bodyFrame;
        if (bodyFrame < 0 || index >= block.Frames.Count || block.Frames[index].IsSpecial) return null;
        ShapeFrame frame = block.Frames[index];
        var key = (frame.Offset, dithered);
        if (_shadowTextures.TryGetValue(key, out var cached)) return cached;
        IndexedImage image = _shapes.Decode(frame);
        var pixels = new Color[image.Width * image.Height];
        // 미리 곱한 알파를 쓰므로 검정의 RGB 는 0 이고 알파만 준다
        var shade = new Color(0, 0, 0, ShadowAlpha);
        // 틀의 불투명 픽셀만 칠한다 (체크무늬는 기준점 기준 좌표의 합이 짝수인 픽셀만)
        for (int i = 0; i < pixels.Length; i++)
        {
            if (!image.Opaque[i]) continue;
            int x = frame.XMin + i % image.Width, y = frame.YMin + i / image.Width;
            if (dithered && ((x + y) & 1) != 0) continue;
            pixels[i] = shade;
        }
        var texture = new Texture2D(_device, image.Width, image.Height);
        texture.SetData(pixels);
        var sprite = (texture, new Point(frame.XMin, frame.YMin));
        _shadowTextures[key] = sprite;
        return sprite;
    }

    /// <summary>오브젝트 본체를 그리기 직전에 그 클러스터의 그림자를 같은 기준점에 그린다. 그림자가 없는 타입은 아무것도 하지 않는다.</summary>
    /// <param name="type">그릴 타입</param>
    /// <param name="bodyFrame">본체 클러스터 번호</param>
    /// <param name="anchor">화면 기준점</param>
    private void DrawShadow(SpriteBatch batch, TypeInfo type, int bodyFrame, Vector2 anchor, float alpha = 1f)
    {
        ShadowStyle style = ShadowStyleOf(type);
        if (style == ShadowStyle.None) return;
        if (GetShadowTexture(type, bodyFrame, style == ShadowStyle.Dithered) is not { } shadow) return;
        batch.Draw(shadow.Texture, anchor + shadow.Offset.ToVector2() * _zoom, null, Color.White * alpha,
            0f, Vector2.Zero, _zoom, SpriteEffects.None, 0f);
    }

    /// <summary>타입 번호 → hotFootRatio 로 옮길 논리 픽셀. 매 프레임 속성 문자열을 읽지 않도록 기억한다.</summary>
    private readonly Dictionary<int, Vector2> _hotFootShifts = [];

    /// <summary>타입의 그림 기준점 이동량 (<see cref="StructureAnimation.HotFootShift"/>).</summary>
    private Vector2 HotFootShift(TypeInfo type)
    {
        if (!_hotFootShifts.TryGetValue(type.LoadIndex, out Vector2 shift))
        {
            (int x, int y) = StructureAnimation.HotFootShift(type.Definition, FortMap.CellPixelWidth, FortMap.CellPixelHeight);
            _hotFootShifts[type.LoadIndex] = shift = new Vector2(x, y);
        }
        return shift;
    }

    /// <summary>
    /// 오브젝트가 지금 그릴 타입·프레임과 기준점 이동량을 고른다. 전투 상태(캐논·재성장) → 가이저 증기 → 워크샵 레벨 →
    /// 신전 회오리 → 풍선 → 걷는 유닛 → 저장 프레임·기본 프레임 순서로 본다. 이동량은 오브젝트 타입의 hotFootRatio 다.
    /// </summary>
    /// <param name="type">오브젝트 타입</param>
    /// <param name="live">세션의 오브젝트 (회수·파괴 등으로 없으면 null)</param>
    /// <param name="item">맵에 저장된 오브젝트 (게임 중 만든 것이면 null)</param>
    private (TypeInfo Type, StructureFrames Frames, Vector2 Shift) ObjectSprite(TypeInfo type, GameEntity? live, FortMapObject? item)
    {
        Vector2 shift = HotFootShift(type);
        if (live != null && CombatSprite(live) is { } combat) return (combat.Type, new StructureFrames(combat.Frame), shift);
        TypeDefinition definition = type.Definition;
        ObjectKind kind = live?.Kind ?? ObjectKinds.Of(type);
        if (kind == ObjectKind.Geyser)
        {
            // 다 쓴 가이저는 원본 emptyGeyser 그림으로 바꿔 그린다 (도움말 "Empty Storm Geyser")
            if (live is { IsDepletedGeyser: true } && _emptyGeyserType != null)
                return (_emptyGeyserType, new StructureFrames(_emptyGeyserType.Definition.Frames.DefaultFrame), HotFootShift(_emptyGeyserType));
            return (type, new StructureFrames(StructureAnimation.GeyserFrame(definition, _walkClock)), shift);
        }
        if (kind == ObjectKind.Workshop)
            return (type, StructureAnimation.FactoryFrames(definition, WorkshopLevel(live, item), _walkClock), shift);
        // 짓는 중인 신전은 회오리 없이 기본 그림만 보인다
        if (kind == ObjectKind.Temple && live is null or { IsComplete: true })
            return (type, StructureAnimation.VortexFrames(definition, _walkClock), shift);
        if (definition.HasFlag("balloon"))
            return (type, new StructureFrames(StructureAnimation.BalloonFrame(definition, _walkClock) ?? definition.Frames.DefaultFrame), shift);
        if (live != null && IsMobile(live))
        {
            int frame = MobileFrame(live)
                ?? StructureAnimation.SailFrame(definition, live.Heading, _session.IsMoving(live.Id), _walkClock)
                ?? RestFrame(live);
            return (type, new StructureFrames(frame), shift);
        }
        int rest = item != null ? MapSpriteFrames.BodyFrame(item, _terrain.TerritoryTheme(item.Territory)) : definition.Frames.DefaultFrame;
        return (type, new StructureFrames(rest), shift);
    }

    /// <summary>워크샵의 레벨 (1~3). 세션의 생산 창 값이 있으면 그것, 없으면 맵 저장 상태(0·1·2) + 1.</summary>
    private int WorkshopLevel(GameEntity? live, FortMapObject? item)
    {
        int stored = (item?.Object.FactoryState ?? live?.Source?.Object.FactoryState ?? 0) + 1;
        if (live == null) return stored;
        PlayerState? owner = _session.Players.FirstOrDefault(p => p.Number == live.Owner);
        int level = owner?.Deck.WorkshopLevel(live.Id) ?? 0;
        return level > 0 ? level : stored;
    }

    /// <summary>오브젝트 하나를 그림자 → 본체 → 겹침 그림 순서로 그린다. 본체 그림이 없으면 false.</summary>
    /// <param name="alpha">본체·겹침 그림의 불투명도 (건설 중 표시용)</param>
    /// <param name="color">소유자 색 번호. 그림자는 원본처럼 색 변환을 적용하지 않는다.</param>
    private bool DrawObjectSprite(SpriteBatch batch, TypeInfo type, StructureFrames frames, Vector2 anchor, float alpha = 1f, int color = 0,
        Color? tint = null)
    {
        var sprite = GetTexture(type.LoadIndex, frames.Body, color);
        if (!sprite.HasValue) return false;
        DrawShadow(batch, type, frames.Body, anchor, alpha);
        var (texture, offset) = sprite.Value;
        batch.Draw(texture, anchor + offset.ToVector2() * _zoom, null, (tint ?? Color.White) * alpha,
            0f, Vector2.Zero, _zoom, SpriteEffects.None, 0f);
        if (frames.Overlay is int overlay) DrawSprite(batch, type.LoadIndex, overlay, anchor, color, alpha: alpha, tint: tint);
        return true;
    }
}
