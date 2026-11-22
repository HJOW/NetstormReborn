using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 독립 원본 구분선 입력과 실제 공통 분배 연결을 검증한다.

/// <summary>앞14입력만 실행해 같은 flags/anchor/focus와 공개 효과/최종 가상 부착을 대조한다.</summary>
public sealed class HudDividerInputTests
{
    /// <summary>원문 DWORD 숫자 입력.</summary>
    private static uint N(string text) => uint.Parse(text, CultureInfo.InvariantCulture);
    /// <summary>원문 signed 화면/제한 입력.</summary>
    private static int Signed(string text) => int.Parse(text, CultureInfo.InvariantCulture);
    /// <summary>같은 단정도의 원시 비트.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>같은 원시 단정도를 복원한다.</summary>
    private static float Float(string value) => BitConverter.UInt32BitsToSingle(N(value));
    /// <summary>실제 관찰한 인자만 문화권 없이 기록한다.</summary>
    private static string Packed(params uint[] values) => string.Join(':', values.Select(value => value.ToString(CultureInfo.InvariantCulture)));
    /// <summary>제품 공개 경계가 아닌 원본 내부 진입 표식만 제외한다. 상태와 최종 부착은 별도로 전부 대조한다.</summary>
    private static string Observable(string text) => text == "-" ? "" : string.Join(';', text.Split(';').Where(item => item != "local" && !item.StartsWith("localout:", StringComparison.Ordinal) && !item.StartsWith("anchor:", StringComparison.Ordinal) && !item.StartsWith("capture:", StringComparison.Ordinal) && !item.StartsWith("move:", StringComparison.Ordinal)));
    /// <summary>머리말/주석을 제외한 기존 원본 자료다.</summary>
    private static IEnumerable<string[]> Rows() => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "hud-divider-input-x86.tsv")).Where(line => line.Length != 0 && !line.StartsWith('#') && !line.StartsWith("name\t", StringComparison.Ordinal)).Select(line => line.Split('\t'));

    /// <summary>같은 실제4노드/좌표/입력/Move와 명시 외부 장치를 사용한다. Route 전체는 별도 검사다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 기본 주소의 실제 파생 구분선.</summary>
        public readonly HudDividerNode Divider = new();
        /// <summary>일반 부모 체인의 실제 객체.</summary>
        public readonly GumpCreateNode Parent = new();
        /// <summary>두 번째 부모 체인의 실제 객체.</summary>
        public readonly GumpCreateNode Grandparent = new();
        /// <summary>기존 다른 캡처에 해당하는 실제 객체.</summary>
        public readonly GumpCreateNode Other = new();
        /// <summary>Collection/Tree/Input/Frame의 같은 등록 상태.</summary>
        public readonly GumpCreateState Create = new();
        /// <summary>같은 실제 focus와 캡처/수명 표.</summary>
        public readonly GumpLifetimeState Lifetime = new();
        /// <summary>현재 메뉴 인스턴스의 실제 상태.</summary>
        public readonly MenuGumpState Menus = new();
        /// <summary>폴링 장치의 원시 입력.</summary>
        public uint PollCode;
        /// <summary>기본 Help 장치의 원시 반환값.</summary>
        public uint BaseResult;
        /// <summary>실제 공개 호출 순서.</summary>
        public readonly List<string> Trace = [];
        /// <summary>같은 실제 위치 변환.</summary>
        public readonly GumpCollection Collection;
        /// <summary>같은 실제 사건 처리 장치.</summary>
        public readonly GumpEventHooks EventHooks;
        /// <summary>같은 캡처 직접 분배 장치.</summary>
        public readonly GumpInputHooks InputHooks;
        /// <summary>같은 실제 입력 처리기.</summary>
        public readonly GumpInput Input;
        /// <summary>최종 가상 부착 관측을 사용하는 실제 공통 Move.</summary>
        public readonly GumpTree Tree;
        /// <summary>같은 실제 입력 본체.</summary>
        public readonly HudDividerInput Handler;
        /// <summary>현재 네 객체만 조회한다.</summary>
        public GumpCreateNode? Node(uint token) => token switch { 1 => Divider, 2 => Parent, 3 => Grandparent, 4 => Other, _ => null };
        /// <summary>앞14열만 읽어 같은 실제 객체와 장치를 준비한다.</summary>
        public Scene(string[] row)
        {
            Create.Collection.Edition = OriginalEdition.Patch1078; Divider.Axis = N(row[5]); Divider.Flags = N(row[6]); string[] limits = row[7].Split(':'), geometry = row[8].Split(':'), parents = row[9].Split(':');
            Divider.MinX = Signed(limits[0]); Divider.MaxX = Signed(limits[1]); Divider.MinY = Signed(limits[2]); Divider.MaxY = Signed(limits[3]); Divider.X = Float(geometry[0]); Divider.Y = Float(geometry[1]); Divider.Positioning[0] = N(geometry[2]); Divider.Positioning[1] = N(geometry[3]);
            uint depth = N(parents[0]); Divider.Parent = depth != 0 ? 2U : 0; Parent.Parent = depth == 2 ? 3U : 0; Parent.X = Float(parents[1]); Parent.Y = Float(parents[2]); Lifetime.Focus = N(row[10]) == 1 ? 1U : N(row[10]) == 2 ? 4U : 0;
            PollCode = N(row[11]); Menus.Instances = N(row[12]); BaseResult = N(row[13]); Collection = new(Create.Collection, new() { Node = token => Node(token) });
            EventHooks = new() { Node = Node }; var events = new GumpEvents(Create, Lifetime, new(), Collection, EventHooks); InputHooks = new() { Node = Node }; Input = new(Create, Lifetime, new(), Collection, events, InputHooks);
            Tree = new(Create, new(), new() { Node = Node, Redraw = (_, _) => { }, Push = _ => Trace.Add("detach"), Attach = (_, x, y, parent, mode) => Trace.Add("attach:" + Packed(Bits(x), Bits(y), parent, mode)), Kill = _ => { } });
            Handler = new(Collection, Input, Tree, new()
            {
                Divider = token => token == 1 ? Divider : null, Base = Node,
                BaseHandle = (_, code, first, second) => { Trace.Add("base:" + Packed(code, first, second)); return BaseResult; }, Cursor = cursor => Trace.Add("cursor:" + cursor.ToString(CultureInfo.InvariantCulture)),
                MenuInstances = () => { Trace.Add("modal:" + Menus.Instances.ToString(CultureInfo.InvariantCulture)); return Menus.Instances; }, Poll = code => { Trace.Add("poll:" + code.ToString(CultureInfo.InvariantCulture)); return new(PollCode, 0x11223344, 0x55667788); },
            });
        }
        /// <summary>제품이 실제 쓴 flags/anchor/focus만 원본 규약으로 관찰한다.</summary>
        public string State() => Packed(Divider.Flags, Divider.Positioning[0], Divider.Positioning[1], Lifetime.Focus == 1 ? 1U : Lifetime.Focus == 4 ? 2U : 0);
    }

    /// <summary>6699입력×두x87 제어어의13398관측을 전체 재생한다.</summary>
    [Fact]
    public void IndependentNative13398StateAndCallbacks()
    {
        int count = 0;
        // 뒤6출력은 실행에 사용하지 않고 실행 뒤 비교한다.
        foreach (string[] row in Rows())
        {
            Assert.Equal(20, row.Length); Assert.Equal("1", row[14]); Assert.Equal("0", row[18]); Assert.True(N(row[19]) > 0); var scene = new Scene(row); uint code = N(row[2]), first = unchecked((uint)Signed(row[3])), second = unchecked((uint)Signed(row[4]));
            if (row[1] == "handle") Assert.Equal(N(row[15]), scene.Handler.Handle(1, code, first, second));
            else { Assert.Equal("broadcast", row[1]); scene.Handler.Broadcast(1, code, first, second); }
            Assert.True(scene.State() == row[16], row[0] + " 실제 flags/anchor/focus"); Assert.True(string.Join(';', scene.Trace) == Observable(row[17]), row[0] + " 실제 공개 호출/최종 부착"); count++;
        }
        Assert.Equal(13398, count);
    }
    /// <summary>원본 중 실제 visible/notify/비dead 관문에 도달 가능한 방송2186행을 같은 공통 Frame 분배로 재생한다.</summary>
    [Fact]
    public void NativeBroadcastRowsThroughCommonFrameAndBinding()
    {
        int reached = 0;
        // 직접 본체 호출 자료를 전체 GUI 입력 증거로 확대하지 않고 공통 방송 가능 집합만 확인한다.
        foreach (string[] row in Rows().Where(row => row[1] == "broadcast"))
        {
            var scene = new Scene(row); scene.Create.Entries[0] = 4; scene.Create.Entries[1] = 1; scene.Create.Count = 2; var frameHooks = new GumpFrameHooks { Node = scene.Node, Broadcast = (_, _, _, _) => throw new InvalidOperationException("다른 노드에 예기치 않은 방송") };
            HudDividerInput.BindHudDividerInput(scene.EventHooks, scene.InputHooks, frameHooks, token => token == 1 ? scene.Handler : null); var frame = new GumpFrame(scene.Create, scene.Lifetime, frameHooks); frame.Broadcast(N(row[2]), unchecked((uint)Signed(row[3])), unchecked((uint)Signed(row[4])));
            if ((N(row[6]) & 0x101) == 0x101 && (N(row[6]) & 0x10000) == 0) { Assert.Equal(row[16], scene.State()); Assert.Equal(Observable(row[17]), string.Join(';', scene.Trace)); reached++; }
            else Assert.Empty(scene.Trace);
        }
        Assert.Equal(2186, reached);
    }
}
