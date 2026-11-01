using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Host; // 호스트 화면과 공통 Gump가 같은 fortGump 장식 자산을 사용한다.

/// <summary>실제 fortGump 배경·모서리와 공통 트리로 HTML 등의 장식을 생성하고 그린다.
/// 근거: cpppj/src/client/RawMenuControls.cpp의 decorationState와 공통 배경·모서리 조립.</summary>
public sealed class HostGumpDecoration
{
    /// <summary>현재 메뉴와 공유하는 실제 노드·등록·수명.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>현재 fortGump 배경 자산.</summary>
    private readonly GumpBackgroundAssets _backgroundAssets;
    /// <summary>같은 SHP의 I 모서리 프레임.</summary>
    private readonly GumpDecorationAssets _cornerAssets;
    /// <summary>실제 배경과 모서리 타입.</summary>
    private readonly uint _type;
    /// <summary>장식 변경을 같은 화면 갱신에 알린다.</summary>
    private readonly Action _changed;

    /// <summary>공통 트리와 실제 배경·모서리 자산을 빌린다.</summary>
    public HostGumpDecoration(GumpButtonTree tree, GumpBackgroundAssets backgrounds, GumpDecorationAssets corners,
        uint type, Action changed)
    {
        ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(backgrounds);
        ArgumentNullException.ThrowIfNull(corners); ArgumentNullException.ThrowIfNull(changed);
        _tree = tree; _backgroundAssets = backgrounds; _cornerAssets = corners; _type = type; _changed = changed;
    }

    /// <summary>현재 표면 장치와 동일한 실제 노드·배경 생성기를 사용하는 장식 처리기를 만든다.</summary>
    private GumpDecoration Device(GumpBackgroundDraw background, GumpDecorationHooks hooks)
    {
        hooks.Node = t => _tree.Node(t) as GumpDecorationNode; hooks.Base = _tree.Node;
        hooks.Invalidate = _ => _changed(); GumpDecorationAssets.BindGumpDecorationAssets(hooks, _cornerAssets);
        return new GumpDecoration(_tree.Create, new() { BackgroundType = _type, BackgroundFrame = 0, CornerType = _type },
            _tree.Backgrounds, background, _tree.Collection, _tree.Lifetime, hooks);
    }

    /// <summary>공통 저장소에 이미 할당된 장식 또는 HTML 파생 노드의 원본 기본 부분을 생성한다.</summary>
    /// <param name="token">현재 공통 저장소가 소유한 장식 파생 토큰.</param>
    /// <param name="width">원본 기본 폭.</param><param name="height">원본 기본 높이.</param>
    public uint Construct(uint token, float width, float height)
    {
        var background = new GumpBackgroundDraw(_tree.Collection, new(), new(), new() { Node = t => _tree.Node(t) as GumpBackgroundNode });
        return Device(background, new()).Construct(token, width, height);
    }

    /// <summary>현재 장식의 배경과 네 모서리를 같은 논리 8비트 화면에 출력한다.</summary>
    /// <param name="token">생성·부착된 장식 토큰.</param><param name="screen">파일 팔레트가 적용된 논리 화면.</param>
    public void Draw(uint token, Screen screen)
    {
        ArgumentNullException.ThrowIfNull(screen);
        byte[] pixels = screen.Lock();
        try
        {
            var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, screen.Width, screen.Height));
            var target = new GumpPixelTarget(pixels, screen.Pitch, screen.Height, clip);
            var colors = new GumpGlyphColorState();
            var backgroundState = new GumpBackgroundDrawState(); GumpBackgroundDraw.SetGumpBackgroundColors(backgroundState, screen.Colors());
            var backgroundHooks = new GumpBackgroundDrawHooks { Node = t => _tree.Node(t) as GumpBackgroundNode };
            GumpBackgroundAssets.BindGumpBackgroundAssets(new(), backgroundHooks, _backgroundAssets);
            new GumpBackgroundPixelTarget(target, _backgroundAssets, screen.ShadeMaps(), colors.VfxColors).Hooks(clip, backgroundHooks);
            var background = new GumpBackgroundDraw(_tree.Collection, clip, backgroundState, backgroundHooks);
            var cornerHooks = new GumpDecorationHooks(); new GumpDecorationPixelTarget(target, _cornerAssets).Hooks(cornerHooks);
            Device(background, cornerHooks).Draw(token);
        }
        finally { screen.Unlock(); }
        screen.Update(new(0, 0, screen.Width, screen.Height));
    }
}
