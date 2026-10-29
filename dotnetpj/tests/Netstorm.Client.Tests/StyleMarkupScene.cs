using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 조각/버퍼/글꼴과 독립 타입 선택 입력을 연결하는 영역이다.

/// <summary>기대 결과를 계산하지 않는 명령 입력 장면.독립 원본 실행 fixture는 아니다.</summary>
internal sealed class StyleMarkupScene
{
    /// <summary>같은 실제 글꼴/버퍼/표시 장면.</summary>
    public StyleBufferScene Scene { get; }
    /// <summary>같은 공통 토큰의 파생 그림 표.</summary>
    public MarkupState Pictures { get; } = new() { TypeCount = 100 };
    /// <summary>원본 그림 처리기.</summary>
    public Markup Picture { get; }
    /// <summary>현재 명령 허용/맞춤.</summary>
    public StyleTextMarkupState State { get; } = new();
    /// <summary>독립 입력의 현재 타입 선택 필드.</summary>
    public StyleTextMarkupType Fields { get; } = new() { FrameCount = 8, DefaultFrame = 1, GumpFrame = 2, LetterKinds = 2 };
    /// <summary>현재 원본 효과/선택 경계.</summary>
    public StyleTextMarkupHooks Hooks { get; } = new();
    /// <summary>같은 한 줄/다중 줄 대괄호 처리기.</summary>
    public StyleTextMarkup Markup { get; }
    /// <summary>실제 다중 줄 파서.</summary>
    public ScrollingTextGumpMarkup Parser { get; }
    /// <summary>소리 이름 관측.</summary>
    public List<string> Sounds { get; } = [];
    /// <summary>다중 줄 그림 추가 요청 관측.</summary>
    public List<(uint Type, uint Frame, int Width, int Height)> Requests { get; } = [];
    /// <summary>한 줄 소유 head의 실제 주소.</summary>
    public uint Head;
    /// <summary>한 줄 현재 태그의 실제 주소.</summary>
    public uint Metadata;
    /// <summary>같은 판본/상태를 빌려 실제 체인과 파서를 만든다.</summary>
    public StyleMarkupScene(OriginalEdition edition = OriginalEdition.Patch1078)
    {
        Scene = new(edition); Fields.First[0] = 3; Fields.First[1] = 6; Picture = new(Scene.Fragments, Pictures, new() { Type = _ => new() { Token = 2, Shape = 2, FrameCount = 100 }, Frame = (_, _, _) => new(30, 20, 0, 12, 0, 0) }, edition); Netstorm.Client.Gump.Markup.BindMarkup(Scene.StyleHooks, Picture); StyleTextBuffer.BindStyleTextBufferMarkup(Scene.BufferHooks, Picture);
        Hooks.FindType = name => name == "Unit" ? 2 : -1; Hooks.Type = _ => Fields; Hooks.Black = () => 9; Hooks.Sound = name => Sounds.Add(name); Hooks.TypeCount = () => 100; Hooks.FrameHeight = (_, _) => 12;
        Markup = new(Scene.Fragments, Picture, State, Hooks, edition); Markup.Hooks(Scene.StyleHooks); Parser = new(Scene.Scroll, Scene.Style, Picture, Markup, new() { Node = t => t == 2 ? Scene.Node : null, Base = t => t == 2 ? Scene.Node : null });
    }
    /// <summary>원본 이름 BYTE를 보존한 입력으로 한 줄 명령을 실행한다.</summary>
    public void Apply(string command) => Markup.Apply(Encoding.Latin1.GetBytes(command), Scene.Context, ref Head, ref Metadata);
    /// <summary>버퍼를 건드리지 않고 원본 다중 줄 그림 선택 결과만 관측한다.</summary>
    public void ApplyScrolling(string command, bool restricted = false) => Markup.ApplyScrolling(Encoding.Latin1.GetBytes(command), Scene.Context, ref Metadata, () => restricted, (t, f, w, h) => Requests.Add((t, f, w, h)));
}
