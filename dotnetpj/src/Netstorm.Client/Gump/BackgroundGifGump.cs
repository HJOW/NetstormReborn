using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 원본 GIF 배경 창의 같은 기본/파생·버퍼/입력/출력 수명 영역이다.

/// <summary>원본0xb0BYTE의+a0..ac.기본 노드는 같은 객체를 사용한다.</summary>
public sealed class BackgroundGifGumpNode : GumpCreateNode
{
    /// <summary>종료 때 원시 큐에 전달할 사건.</summary>
    public uint Event { get; set; }
    /// <summary>원시 큐의 첫 인자.</summary>
    public uint First { get; set; }
    /// <summary>원시 큐의 둘째 인자.</summary>
    public uint Second { get; set; }
    /// <summary>독립 소유한 원시GIF 버퍼 토큰.</summary>
    public uint Bitmap { get; set; }
}
/// <summary>호출 시점에 읽는 원본54041e의 signed종류BYTE.</summary>
public sealed class BackgroundGifGumpState
{
    /// <summary>기본 command로 signed확장하는 종류.</summary>
    public sbyte Kind { get; set; } = 3;
}
/// <summary>원본4a1fe0의 실제8인자에 대응하는 원시 복사 요청.</summary>
/// <param name="Position">현재 위치.</param><param name="Bitmap">현재 소유 버퍼.</param><param name="Stride">현재 baseWidth.</param><param name="Source">0,0,폭,높이의 원시영역.</param>
public readonly record struct BackgroundGifCopy(ScreenPoint Position, uint Bitmap, int Stride, ScreenRect Source);
/// <summary>원문4dae60의 소유 버퍼와 실제두DWORD 출력이다.</summary>
public delegate uint BackgroundGifLoad(string path, ref int width, ref int height);
/// <summary>같은 실제기본/자산/현재클립/입력 장치만 공급한다.</summary>
public sealed class BackgroundGifGumpHooks
{
    /// <summary>현재 같은 실제파생.</summary>
    public Func<uint, BackgroundGifGumpNode?>? Node { get; set; }
    /// <summary>같은 기본 주소.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>실제기본 생성(token,0,0,0,1).</summary>
    public Action<uint>? ConstructBase { get; set; }
    /// <summary>실제기본 소멸자.</summary>
    public Action<uint>? DestructBase { get; set; }
    /// <summary>현재 BackgroundGifSpec의 실제 이름 치환.</summary>
    public Func<string, string>? PathSpec { get; set; }
    /// <summary>현재 실제파일 존재 조회.</summary>
    public Func<string, bool>? Exists { get; set; }
    /// <summary>실제GIF 적재/소유 버퍼.</summary>
    public BackgroundGifLoad? Load { get; set; }
    /// <summary>같은 실제소유 메모리 반납.</summary>
    public Action<uint>? ReleaseBitmap { get; set; }
    /// <summary>같은 공통 화면 위치.</summary>
    public Func<uint, ScreenPoint>? Position { get; set; }
    /// <summary>현재클립의 같은 원시픽셀 복사.</summary>
    public Action<BackgroundGifCopy>? Copy { get; set; }
    /// <summary>현재 가상Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>나머지입력의 실제기본 위임.</summary>
    public Func<uint, uint, uint, uint, uint>? BaseHandle { get; set; }
    /// <summary>원문452f90에 정규화 없이 전달하는 원시세DWORD.</summary>
    public Action<uint, uint, uint>? Dispatch { get; set; }
}
/// <summary>cpppj/src/client/BackgroundGifGump.cpp의 원문생성/교체·현재포인터/원시입력/픽셀/소멸 순서.</summary>
public sealed class BackgroundGifGump
{
    /// <summary>원본10.78 가상 표.</summary>
    private const uint Vtable = 0x501b60;
    /// <summary>현재 signed종류 전역.</summary>
    private readonly BackgroundGifGumpState _state;
    /// <summary>같은 현재 파생/기본 저장소와 장치.</summary>
    private readonly BackgroundGifGumpHooks _hooks;
    /// <summary>선택한 효과에서만 장치를 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new ArgumentException("BackgroundGif 실제 장치 미연결");
    /// <summary>같은 실제파생/기본 조회만 미리 요구한다.</summary>
    public BackgroundGifGump(BackgroundGifGumpState state, BackgroundGifGumpHooks hooks) { ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.Base); _state = state; _hooks = hooks; }
    /// <summary>표시사본 없이 현재 같은 객체를 검증한다.</summary>
    private BackgroundGifGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("BackgroundGif 공유 노드 없음"); return n; }
    /// <summary>기본→종류/bitmap0→현재적재→종료입력0이다.적재효과 뒤 현재노드에 세필드를 쓴다.</summary>
    public uint Construct(uint token, string name) { Node(token); Require(_hooks.ConstructBase)(token); var n = Node(token); n.Vtable = Vtable; n.Command = unchecked((uint)_state.Kind); n.Bitmap = 0; Refresh(token, name); Node(token).Event = 0; Node(token).First = 0; Node(token).Second = 0; return token; }
    /// <summary>이전free/현재0→실제조회/적재→base/배율 치수다.적재 반환 뒤 현재노드를 재조회한다.</summary>
    public void Refresh(uint token, string name)
    {
        var n = Node(token); int width = 0, height = 0; if (n.Bitmap != 0) { Require(_hooks.ReleaseBitmap)(n.Bitmap); Node(token).Bitmap = 0; } string path = Require(_hooks.PathSpec)(name); if (Require(_hooks.Exists)(path)) { uint bitmap = Require(_hooks.Load)(path, ref width, ref height); Node(token).Bitmap = bitmap; } Node(token).BaseWidth = width; Node(token).Width = (float)((double)width * Node(token).Scale); Node(token).BaseHeight = height; Node(token).Height = (float)((double)height * Node(token).Scale);
    }
    /// <summary>종료사건→둘째→첫인자→flags600의 원문 쓰기 순서.</summary>
    public void SetEvent(uint token, uint code, uint first, uint second) { var n = Node(token); n.Event = code; n.Second = second; n.First = first; n.Flags |= 0x600; }
    /// <summary>현재x/y로parent0/mode0 가상부착에 맡긴다.</summary>
    public void Reattach(uint token) { var n = Node(token); Require(_hooks.Attach)(token, n.X, n.Y, 0, 0); }
    /// <summary>비해제 좌/우누름·ESC만 원시종료사건을 보낸다.Dispatch가창을반납해도 노드를다시읽지않는다.</summary>
    public uint Handle(uint token, uint code, uint first, uint second) { var n = Node(token); uint key = code & 0x03ffffff; if (n.Event != 0 && (code & 0x80000000) == 0 && key is 0x10000 or 0x20000 or 27) { uint pending = n.Event, a = n.First, b = n.Second; Require(_hooks.Dispatch)(pending, a, b); return 1; } return Require(_hooks.BaseHandle)(token, code, first, second); }
    /// <summary>위치효과 뒤 현재base치수/bitmap을 읽어 원시복사한다.배율치수/투명마스크/팔레트변환은 쓰지않는다.</summary>
    public void Draw(uint token) { if (Node(token).Bitmap == 0) return; var position = Require(_hooks.Position)(token); int width = GumpCollection.Ftol(Node(token).BaseWidth), height = GumpCollection.Ftol(Node(token).BaseHeight); uint bitmap = Node(token).Bitmap; Require(_hooks.Copy)(new(position, bitmap, width, new(0, 0, width, height))); }
    /// <summary>free뒤bitmap0을추가하지않는다.같은기본소멸자로넘긴다.</summary>
    public void Destruct(uint token) { var n = Node(token); n.Vtable = Vtable; if (n.Bitmap != 0) Require(_hooks.ReleaseBitmap)(n.Bitmap); Require(_hooks.DestructBase)(token); }
    /// <summary>실제기본생성/수명/위치만 같은 장치로 연결한다.현재가상부착/기본입력/파일·원시큐는호스트가공급한다.</summary>
    public static void BindBackgroundGifGumpDevices(BackgroundGifGumpHooks hooks, GumpCreate factory, GumpLifetime lifetime, GumpCollection collection) { ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(collection); hooks.ConstructBase = t => factory.Construct(t, 0, 0, 0, 1); hooks.DestructBase = lifetime.DestructBase; hooks.Position = collection.Position; }
    /// <summary>현재GIF창만실제부모사건/직접입력/Draw/scalar로분배하고다른파생은보존한다.</summary>
    public static void BindBackgroundGifGump(GumpEventHooks events, GumpInputHooks input, GumpDrawHooks draw, GumpLifetimeHooks lifetime, Func<uint, BackgroundGifGump?> resolve, Action<uint> release)
    {
        ArgumentNullException.ThrowIfNull(resolve); ArgumentNullException.ThrowIfNull(release);
        // 부모 사건과 직접 입력은 같은 현재 원문Handle에 전달한다.
        uint Route(Func<uint, uint, uint, uint, uint>? previous, uint t, uint c, uint a, uint b) => resolve(t) is { } gif ? gif.Handle(t, c, a, b) : Require(previous)(t, c, a, b);
        var e = events.Handle; var i = input.Handle; events.Handle = (t, c, a, b) => Route(e, t, c, a, b); input.Handle = (t, c, a, b) => Route(i, t, c, a, b); var render = draw.Draw; draw.Draw = t => { if (resolve(t) is { } gif) gif.Draw(t); else Require(render)(t); }; var deleting = lifetime.DeletingDestructor; lifetime.DeletingDestructor = (t, flags) => { if (resolve(t) is { } gif) { gif.Destruct(t); if ((flags & 1) != 0) release(t); } else Require(deleting)(t, flags); };
    }
}
