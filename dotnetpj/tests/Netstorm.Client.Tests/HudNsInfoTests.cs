using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Gump;
using Netstorm.Tests;

namespace Netstorm.Client.Tests; // NSInfo의 현재 배경/전역/일반 부착 관측을 대조한다.

/// <summary>독립 원본310행의 모든8노드·24전역·호출순서와 중단8행을 비교한다.</summary>
public sealed class HudNsInfoTests
{
    /// <summary>명시 callback이 멈춘 실제 경계다.</summary>
    private sealed class Stop : Exception { }
    /// <summary>원문 DWORD와 signed 표현을 같은 하위32비트로 읽는다.</summary>
    private static uint N(string text) => unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
    /// <summary>현재 원문 float 비트다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>명시 원문 비트를 float로 복원한다.</summary>
    private static float Float(string value) => BitConverter.UInt32BitsToSingle(N(value));
    /// <summary>정규화한 검사 토큰과 실제 원문 주소를 구별한다.</summary>
    private static uint Address(uint token) => token == 0 ? 0 : 0x1000100 + (token - 1) * 0x200;
    /// <summary>원문 주소를 같은 고정8노드의1기반 토큰으로 변환한다.</summary>
    private static uint Token(uint address)
    {
        if (address == 0) return 0; if (address < 0x1000100 || (address - 0x1000100) % 0x200 != 0 || (address - 0x1000100) / 0x200 >= 8) throw new InvalidOperationException("NSInfo 검사 원문 주소"); return (address - 0x1000100) / 0x200 + 1;
    }
    /// <summary>기대 기하를 계산하지 않고 현재 객체의 기본 필드만 입력/관측한다.</summary>
    private sealed class Scene
    {
        /// <summary>원본이 사용하는 같은8개의 안정된 실제 기본 주소다.</summary>
        private readonly GumpCreateNode[] _nodes = Enumerable.Range(0, 8).Select(i => i == 0 ? new HudNsInfoNode() : new GumpCreateNode()).ToArray();
        /// <summary>읽지 않는 확장/guard를 보존하는 전체 원시 입력이다.</summary>
        private readonly byte[][] _raw = new byte[8][];
        /// <summary>앞22장치값과 실제 mode/known2개를 보존한다.</summary>
        private readonly uint[] _globals;
        /// <summary>명시 현재 변경 입력이다.</summary>
        private readonly string[][] _effects;
        /// <summary>경계별 실제1기반 호출 수다.</summary>
        private readonly Dictionary<string, int> _calls = [];
        /// <summary>원본 인자 순서의 외부 효과다.</summary>
        public string Trace = "";
        /// <summary>같은 실제 파생/기본/모드 owner를 빌리는 제품 본체다.</summary>
        public readonly HudNsInfo Body;
        /// <summary>앞6열의 입력만 실제 저장소에 설치한다.</summary>
        public Scene(string[] row)
        {
            var data = Convert.FromHexString(row[2]); Assert.Equal(8 * 0x11c, data.Length);
            // 각 원문 주소의 미작성 영역도 함께 보관한다.
            for (int i = 0; i < 8; i++) Decode(i, data.AsSpan(i * 0x11c, 0x11c).ToArray());
            _globals = row[3].Split(':').Select(N).ToArray(); Assert.Equal(24, _globals.Length); _effects = row[5] == "-" ? [] : row[5].Split(';').Select(value => value.Split('/')).ToArray();
            Body = new(new() { Metamap = () => _globals[22], PlayerKnown = () => _globals[23] }, new()
            {
                Node = token => token == 1 ? (HudNsInfoNode)_nodes[0] : null, Base = Base,
                BaseAttach = (token, x, y, parent, mode) => Attach("baseAttach", token, x, y, parent, mode),
                Push = token => { Base(token).Flags |= 8; Event("push", token); }, Attach = (token, x, y, parent, mode) => Attach("attach", token, x, y, parent, mode),
                MetamapLayout = token => Event("metamapLayout", token),
            });
        }
        /// <summary>같은 현재 기본 주소만 반환한다.</summary>
        private GumpCreateNode Base(uint token) => token is >= 1 and <= 8 ? _nodes[token - 1] : throw new InvalidOperationException("NSInfo 검사 토큰");
        /// <summary>같은 기본/파생 주소를 교체하지 않고 원문 필드만 복사한다.</summary>
        private void Decode(int index, byte[] raw)
        {
            _raw[index] = raw; var decoded = GumpRawSupport.Decode(raw); decoded.Parent = Token(decoded.Parent); var node = _nodes[index];
            // 기본 필드만 복사하고 실제 파생과 고정 배열 주소는 유지한다.
            foreach (var property in typeof(GumpCreateNode).GetProperties()) if (property.CanWrite) property.SetValue(node, property.GetValue(decoded));
            decoded.Positioning.CopyTo(node.Positioning, 0); decoded.TextContext.CopyTo(node.TextContext, 0); if (index == 0) ((HudNsInfoNode)node).Background = Token(BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0xa0)));
        }
        /// <summary>모든 현재 기본 필드와 원문 포인터를 원래 guard 위에 기록한다.</summary>
        private byte[] Encode(int index)
        {
            var result = (byte[])_raw[index].Clone(); var raw = Convert.FromHexString(GumpRawSupport.Encode(_nodes[index])); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(20), Address(_nodes[index].Parent)); raw.CopyTo(result, 0);
            if (index == 0) BinaryPrimitives.WriteUInt32LittleEndian(result.AsSpan(0xa0), Address(((HudNsInfoNode)_nodes[0]).Background)); return result;
        }
        /// <summary>입력에 명시된 한 현재 필드/전역 변경만 수행한다.</summary>
        private void Effect(string[] effect)
        {
            uint value = N(effect[3]);
            if (effect[1].StartsWith("node", StringComparison.Ordinal))
            {
                int index = int.Parse(effect[1].AsSpan(4), CultureInfo.InvariantCulture); var bytes = Encode(index); int at = effect[2] switch { "flags" => 4, "baseWidth" => 0x3c, "baseHeight" => 0x40, "scale" => 0x38, "background" => 0xa0, _ => throw new InvalidOperationException("NSInfo 검사 변경 필드") }; BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(at), value); Decode(index, bytes);
            }
            else if (effect[1] == "global") _globals[effect[2] == "metamap" ? 22 : effect[2] == "playerKnown" ? 23 : throw new InvalidOperationException("NSInfo 검사 변경 전역")] = value;
            else if (effect[1] != "control") throw new InvalidOperationException("NSInfo 검사 변경 대상");
        }
        /// <summary>실제 하위 효과 뒤에 명시 변경과 중단을 입력 순서로 적용한다.</summary>
        private void Event(string name, params uint[] values)
        {
            if (Trace.Length != 0) Trace += ';'; Trace += name + string.Concat(values.Select(value => ":" + value.ToString(CultureInfo.InvariantCulture))); string trigger = name + (_calls[name] = _calls.GetValueOrDefault(name) + 1).ToString(CultureInfo.InvariantCulture); bool stop = false;
            // 같은 경계의 여러 현재 변경을 빠뜨리지 않는다.
            foreach (var effect in _effects) if (effect[0] == trigger) { Effect(effect); if (effect[1] == "control" && effect[2] == "stop" && effect[3] == "1") stop = true; }
            if (stop) throw new Stop();
        }
        /// <summary>명시 기본 부착 장치의 필드 쓰기와 현재 인자만 실행한다.</summary>
        private void Attach(string name, uint token, float x, float y, uint parent, uint mode)
        {
            var node = Base(token); node.Flags &= ~8U; node.X = x; node.Y = y; if (parent != 0) node.Parent = parent; Event(name, token, Bits(x), Bits(y), parent, mode);
        }
        /// <summary>원문 고정8노드의 전체 guard/상태다.</summary>
        public string Nodes() => string.Concat(Enumerable.Range(0, 8).Select(index => Convert.ToHexStringLower(Encode(index))));
        /// <summary>읽지 않은 전역도 보존했는지 전체24DWORD를 기록한다.</summary>
        public string Globals() => string.Join(':', _globals.Select(value => value.ToString(CultureInfo.InvariantCulture)));
    }
    /// <summary>310행을 모두 대조하며 메타맵 후속 본문 전체의 완료 검사로 확대하지 않는다.</summary>
    [Fact]
    public void Native310NormalAttachmentFullMemoryCurrentGlobalsAndEightStops()
    {
        int count = 0, stops = 0;
        // 앞6입력과 뒤8관측을 구별해 전체 원본을 재생한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "hud-nsinfo-attach-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith("name\t", StringComparison.Ordinal) || line.StartsWith('#')) continue; var row = line.Split('\t'); Assert.Equal(14, row.Length); var scene = new Scene(row); var args = row[4].Split(':'); Assert.Equal(4, args.Length); bool completed = true;
            try { scene.Body.Attach(1, Float(args[0]), Float(args[1]), N(args[2]), N(args[3])); } catch (Stop) { completed = false; stops++; }
            Assert.Equal(row[6] == "1", completed); Assert.Equal(row[7], scene.Nodes()); Assert.Equal(row[8], scene.Globals()); Assert.Equal(row[9], scene.Trace.Length == 0 ? "-" : scene.Trace); if (completed) Assert.Equal("20", row[11]); Assert.True(N(row[12]) > 0); Assert.Equal("0", row[13]); count++;
        }
        Assert.Equal(310, count); Assert.Equal(8, stops);
    }
    /// <summary>선택된 실제 메타 후속은 기본 부분 쓰기를 마친 뒤 누락을 거부한다.</summary>
    [Fact]
    public void SelectedMetamapMustHaveRealContinuationAfterBackgroundAttachment()
    {
        var ns = new HudNsInfoNode { Background = 2, BaseWidth = 80, BaseHeight = 75 }; var background = new GumpCreateNode { Scale = 1 }; int attached = 0;
        var body = new HudNsInfo(new() { Metamap = () => 1, PlayerKnown = () => 1 }, new() { Node = _ => ns, Base = _ => background, BaseAttach = (_, _, _, _, _) => { }, Push = _ => { }, Attach = (_, _, _, _, _) => attached++ });
        Assert.Throws<InvalidOperationException>(() => body.Attach(1, 0, 0, 0, 0)); Assert.Equal(1, attached); Assert.Equal(80, background.BaseWidth); Assert.Equal(75, background.BaseHeight); Assert.Equal(80, background.Width); Assert.Equal(75, background.Height);
    }
}
