using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 메뉴 제목 등에 쓰는 원본 한 줄 텍스트 위젯 영역이다.

/// <summary>원본+a0의 실제 소유 체인head. 기본 생성/수집/수명과 같은 객체다.</summary>
public class TextGumpNode : GumpCreateNode
{
    /// <summary>실제StyleText 소유 주소. Clear/Measure에 ref로 전달한다.</summary>
    public uint Head;
}

/// <summary>원본 종류BYTE 전역. 위치 정렬 플래그와 구별한다.</summary>
public sealed class TextGumpState
{
    /// <summary>기본 command에 signed 확장하는 두 배포판 초기값45.</summary>
    public sbyte Alignment { get; set; } = 45;
}

/// <summary>파생/기본이 같은 안정된 객체를 반환하는 경계다.</summary>
public sealed class TextGumpHooks
{
    /// <summary>실제 한 줄 파생 노드.</summary>
    public Func<uint, TextGumpNode?>? Node { get; set; }
    /// <summary>같은 기본 생성/수집 노드.</summary>
    public Func<uint, GumpCreateNode?>? BaseNode { get; set; }
}

/// <summary>원본 한 줄 TextGump의 생성/소유/맞춤/내부 클립과 scalar 소멸이다.
/// 근거: cpppj/src/client/TextGump.h/.cpp, 004bb170·004bb240·004bb250·004bb310·004bb380·004bc910.
/// 같은 C++ 파일의 다중 줄 ScrollingTextGump는 StyleTextBuffer 조립 단위에서 추가한다.</summary>
public sealed class TextGump
{
    /// <summary>현재 등록/판본 전역.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 종류BYTE.</summary>
    private readonly TextGumpState _state;
    /// <summary>같은 기본 생성/등록.</summary>
    private readonly GumpCreate _factory;
    /// <summary>같은 기본 scalar 소멸자.</summary>
    private readonly GumpLifetime _lifetime;
    /// <summary>같은 위치/경계/캐시.</summary>
    private readonly GumpCollection _collection;
    /// <summary>현재 논리/VFX 클립.</summary>
    private readonly GumpDrawState _clip;
    /// <summary>같은 실제 소유 조각/폰트/픽셀 처리기.</summary>
    private readonly StyleText _style;
    /// <summary>같은 기본/파생 저장소 경계.</summary>
    private readonly TextGumpHooks _hooks;

    /// <summary>같은 노드/클립/조각 처리기를 빌린다. 파생/기본 조회만 필수다.</summary>
    public TextGump(GumpCreateState create, TextGumpState state, GumpCreate factory, GumpLifetime lifetime, GumpCollection collection, GumpDrawState clip, StyleText style, TextGumpHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(clip); ArgumentNullException.ThrowIfNull(style); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null || hooks.BaseNode is null) throw new ArgumentException("TextGump 공유 저장소 필요"); _create = create; _state = state; _factory = factory; _lifetime = lifetime; _collection = collection; _clip = clip; _style = style; _hooks = hooks;
    }
    /// <summary>복제한 기반 노드로 연결한 잘못된 파생 저장소는 거부한다.</summary>
    private TextGumpNode Node(uint token)
    {
        TextGumpNode? n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.BaseNode!(token))) throw new InvalidOperationException("TextGump 공유 노드 없음"); return n;
    }
    /// <summary>기본 생성 뒤 판본별 가상 표/head/색/정수 크기·전체32바이트 문맥과 클립 비트를 쓴다. 이전head는 반납하지 않는다.</summary>
    public uint Construct(uint token, int width, int height, uint font, uint color)
    {
        Node(token); _factory.Construct(token, 0, 0, 0, 1); TextGumpNode n = Node(token); bool patch = _create.Collection.Edition == OriginalEdition.Patch1078;
        if (patch) n.Vtable = 0x5144d4; n.Head = 0; if (!patch) n.Vtable = 0x505dc0; n.Command = unchecked((uint)(int)_state.Alignment);
        n.BaseWidth = width; n.Width = (float)((double)width * n.Scale); n.BaseHeight = height; n.Height = (float)((double)height * n.Scale); uint[] context = [0, font, color, 0, 0, 0, 1, 1]; context.CopyTo(n.TextContext, 0); n.Flags |= GumpDraw.DrawClip; return token;
    }
    /// <summary>현재 소유head를 원본 scalar 체인 삭제 순서로 비운다.</summary>
    public void Clear(uint token) => _style.Clear(ref Node(token).Head);
    /// <summary>-1만NUL 탐색이며0길이는 노드 조회도 하지 않는다. 명시 길이의 중간NUL도 복사한다.</summary>
    public void AppendText(uint token, ReadOnlySpan<byte> bytes, uint[] context, int length = -1)
    {
        if (length == -1) { length = bytes.IndexOf((byte)0); if (length < 0) throw new ArgumentException("TextGump NUL 범위 필요"); }
        if (length == 0) return; if (length < 0) throw new ArgumentException("TextGump 명시 길이 음수"); uint fragment = _style.NewText(bytes, context, (uint)length); _style.Append(ref Node(token).Head, fragment);
    }
    /// <summary>두 번 순회한 현재 폭/높이에 정수 여백을 감김으로 더하고 float 좁힘 전 정수로 현재scale을 곱한다.</summary>
    public void Fit(uint token, int padWidth = 0, int padHeight = 0)
    {
        int width = 0, height = 0; _style.Measure(in Node(token).Head, ref width, ref height); TextGumpNode n = Node(token); width = unchecked(width + padWidth); n.BaseWidth = width; n.Width = (float)((double)width * n.Scale); height = unchecked(height + padHeight); n.BaseHeight = height; n.Height = (float)((double)height * n.Scale);
    }
    /// <summary>비어 있는head만 조기 반환한다. 공통 Draw의 보호 비트가 없어도 내부 클립→현재위치→현재head 출력을 수행한다.</summary>
    public void Draw(uint token)
    {
        if (Node(token).Head == 0) return; ScreenRect bounds = _collection.Bounds(token); using var guard = new GumpClipScope(_clip, bounds); ScreenPoint position = _collection.Position(token); _style.Draw(Node(token).Head, position);
    }
    /// <summary>파생 가상 표→현재head Clear 두 번→기본 scalar 순서다. 첫Clear 콜백이 쓴 현재head도 두 번째에 읽는다.</summary>
    public uint DeleteScalar(uint token, uint flags)
    {
        Node(token).Vtable = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0x5144d4U : 0x505dc0U; Clear(token); Clear(token); return _lifetime.DeleteScalar(token, flags);
    }
    /// <summary>원본 가상Draw 분류 중 한 줄 처리기를 연결하고 기존 파생 분배를 보존한다. 다중 줄 조립 때3인자 원본 분류를 확장한다.</summary>
    public static void BindTextGumpDraw(GumpDrawHooks hooks, Func<uint, TextGump?> single)
    {
        ArgumentNullException.ThrowIfNull(single); Action<uint>? previous = hooks.Draw; hooks.Draw = token => { if (single(token) is { } text) text.Draw(token); else if (previous is not null) previous(token); else throw new InvalidOperationException("Gump 가상 Draw 처리기 없음"); };
    }
    /// <summary>한 줄 scalar 소멸만 같은 실제 수명 처리기에 연결하며 기존 다른 파생 소멸자를 보존한다.</summary>
    public static void BindTextGumpLifetime(GumpLifetimeHooks hooks, Func<uint, TextGump?> resolve)
    {
        ArgumentNullException.ThrowIfNull(resolve); Action<uint, uint>? previous = hooks.DeletingDestructor; hooks.DeletingDestructor = (t, f) => { if (resolve(t) is { } text) text.DeleteScalar(t, f); else if (previous is not null) previous(t, f); else throw new InvalidOperationException("Gump scalar 소멸자 처리기 없음"); };
    }
}
