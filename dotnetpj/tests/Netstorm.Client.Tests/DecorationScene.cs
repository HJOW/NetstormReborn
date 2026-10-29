using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 배경/모서리 RLE와 같은 기본 트리·픽셀·수명 조립 입력이다.

/// <summary>고정 기대 연산 없이 입력 자산과 실제 처리기를 연결한다.독립 원본 실행 fixture는 아니다.</summary>
internal sealed class DecorationScene
{
    /// <summary>같은 실제 기본/트리/수명/표면.</summary>
    public ScrollWidgetScene Scene { get; }
    /// <summary>실제 장식 토큰50.</summary>
    public GumpDecorationNode Node { get; } = new();
    /// <summary>현재 종류/자산 전역.</summary>
    public GumpDecorationState State { get; } = new() { BackgroundType = 82, CornerType = 82 };
    /// <summary>현재 실제 모서리/영역 경계.</summary>
    public GumpDecorationHooks Hooks { get; }
    /// <summary>같은 실제 배경 생성기.</summary>
    public GumpBackgroundCreate Background { get; }
    /// <summary>같은 실제 배경 출력기.</summary>
    public GumpBackgroundDraw BackgroundDraw { get; }
    /// <summary>배경 호출 직후 위치 변경 같은 명시 외부 입력 경계.</summary>
    public GumpBackgroundDrawHooks BackgroundHooks { get; }
    /// <summary>실제 모서리 자산.</summary>
    public GumpDecorationAssets Assets { get; }
    /// <summary>실제 현재 클립/원색 모서리 출력.</summary>
    public GumpDecorationPixelTarget Pixels { get; }
    /// <summary>실제 장식 처리기.</summary>
    public GumpDecoration Decoration { get; }
    /// <summary>끝 제외 변경 영역 관측.</summary>
    public List<ScreenRect> Invalidated { get; } = [];
    /// <summary>실제 모서리 호출 순서/인자.</summary>
    public List<(uint Shape, uint Frame, ScreenPoint Position)> Corners { get; } = [];
    /// <summary>독립 입력 메타의 little endian 쓰기.기대 픽셀을 생성하지 않는다.</summary>
    private static void Put(List<byte> raw, int offset, uint value, int size = 4)
    {
        // 메타 필드의 실제 바이트 순서만 적용한다.
        for (int i = 0; i < size; i++) raw[offset + i] = unchecked((byte)(value >> (i * 8)));
    }
    /// <summary>한 픽셀씩 다른 네 모서리와 출력하지 않는 특수 프레임.상대 RECT도 서로 다르다.</summary>
    public static ShapeDatabase CornerShapes()
    {
        var raw = new List<byte>(new byte[48]); Put(raw, 0, 0x30312e31); Put(raw, 4, 5);
        // 네 RLE와 한 특수 레코드를 실제 SHP 테이블/추가 헤더에 넣는다.
        for (int i = 0; i < 5; i++) { int at = raw.Count; raw.AddRange(new byte[60]); Put(raw, 8 + i * 8, (uint)(at + 36)); Put(raw, at + 24, 1, 2); Put(raw, at + 26, 1, 2); uint x = i == 4 ? 0x7fff0000U : i is 0 or 3 ? uint.MaxValue : 0; uint y = i < 2 ? uint.MaxValue : 0; Put(raw, at + 44, x); Put(raw, at + 48, y); Put(raw, at + 52, x); Put(raw, at + 56, y); if (i < 4) raw.AddRange([2, (byte)(41 + i), 0]); } return new(raw.ToArray());
    }
    /// <summary>기존 공통 장면에 실제 장식 생성/출력/프레임/수명 분배를 추가한다.</summary>
    public DecorationScene(OriginalEdition edition = OriginalEdition.Patch1078, ScrollWidgetScene? shared = null)
    {
        Scene = shared ?? new(edition); Scene.Nodes.Add(50, Node); var backgrounds = GumpBackgroundTestAssets.Assets(); var create = new GumpBackgroundCreateHooks { Node = t => Scene.Nodes.GetValueOrDefault(t) as GumpBackgroundNode }; BackgroundHooks = new() { Node = create.Node }; GumpBackgroundAssets.BindGumpBackgroundAssets(create, BackgroundHooks, backgrounds); Background = new(Scene.Create, Scene.Factory, () => 0, create); var target = new GumpPixelTarget(Scene.Pixels, 64, 128, Scene.Clip); new GumpBackgroundPixelTarget(target, backgrounds, GumpBackgroundTestAssets.Maps(), new byte[256]).Hooks(Scene.Clip, BackgroundHooks); BackgroundDraw = new(Scene.Collection, Scene.Clip, new(), BackgroundHooks);
        Assets = new(CornerShapes(), [new(82, 0, 0), new(83, 0, 1)]); Hooks = new() { Node = t => Scene.Nodes.GetValueOrDefault(t) as GumpDecorationNode, Base = t => Scene.Nodes.GetValueOrDefault(t), Invalidate = r => Invalidated.Add(r) }; GumpDecorationAssets.BindGumpDecorationAssets(Hooks, Assets); Pixels = new(target, Assets); Pixels.Hooks(Hooks); var shape = Hooks.Shape; Hooks.Shape = (s, f, p) => { Corners.Add((s, f, p)); shape!(s, f, p); }; Decoration = new(Scene.Create, State, Background, BackgroundDraw, Scene.Collection, Scene.Lifetime, Hooks); GumpDecoration.BindGumpDecoration(Scene.DrawHooks, Scene.FrameHooks, Scene.LifetimeHooks, t => Scene.Nodes.GetValueOrDefault(t) is GumpDecorationNode ? Decoration : null);
    }
    /// <summary>같은 실제 기본 생성/등록 후 원본 기본 트리에 부착한다.</summary>
    public void Construct(float width = 10.75f, float height = 7.75f) { Decoration.Construct(50, width, height); Scene.Tree.Attach(50, 5, 6, 1, 1); }
}
