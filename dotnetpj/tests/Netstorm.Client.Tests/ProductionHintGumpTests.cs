using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Tests;
using static Netstorm.Client.Tests.InventoryRawSupport;

namespace Netstorm.Client.Tests; // 원본 안내의 관측 필드와 모든 실제 효과 순서를 대조한다.

/// <summary>독립126입력의 생성/두 wrapper/만료/출력/scalar 검사다.</summary>
public sealed class ProductionHintGumpTests
{
    /// <summary>원본 안내의 안정 토큰이다.</summary>
    private const uint Token = 0x12401000;
    /// <summary>원본 소유 문자열 토큰이다.</summary>
    private const uint Owned = 0x12403000;
    /// <summary>원본 기준 노드 토큰이다.</summary>
    private const uint Reference = 0x12405000;
    /// <summary>명시 외부 응답과 원본 canary 입력만 공급한다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 등록표/루트다.</summary>
        public readonly GumpCreateState Create = new();
        /// <summary>현재 실제 종류/팔레트/serial이다.</summary>
        public readonly ProductionHintState State = new() { LastSerial = 0x76543210 };
        /// <summary>원문과 같은 안정된 파생 주소다.</summary>
        public readonly ProductionHintNode Node = Base(new ProductionHintNode(), Canary(160));
        /// <summary>현재 명시 시각이다.</summary>
        public double Now = 3.25;
        /// <summary>현재 명시 화면점이다.</summary>
        public ScreenPoint Point = new(17, -9);
        /// <summary>현재 명시 경계다.</summary>
        public ScreenRect Box = new(10, 20, 40, 56);
        /// <summary>원문 소유 문자열 바이트다.</summary>
        private string _text = "";
        /// <summary>원문 모든 효과 순서다.</summary>
        public string Trace = "";
        /// <summary>실제 검사 본체다.</summary>
        public readonly ProductionHintGump Handler;
        /// <summary>기대 출력을 계산하지 않는 외부 원문 장치를 연결한다.</summary>
        public Scene()
        {
            Node.Head = Node.Reserved = Node.Owned = Node.Reference = 0xa5a5a5a5; Node.Until = BitConverter.UInt64BitsToDouble(0xa5a5a5a5a5a5a5a5); Create.Collection.Root = Reference;
            Handler = new(Create, State, new()
            {
                Node = token => { Assert.Equal(Token, token); return Node; }, Now = () => { Event("Now"); return Now; }, Allocate = size => { Assert.Equal(184, size); Event("New," + size); return Token; },
                ConstructText = _ => { Event("Text"); Node.Vtable = 0x5144d4; Node.Flags = 0x400001; Node.Serial = 41; Node.Head = 0; },
                Duplicate = value => { _text = value; Event("Dup," + Hex(value)); return Owned; }, String = token => { Assert.Equal(Owned, token); return _text; },
                Free = token => { Assert.Equal(Owned, token); Event("Free,1"); }, Kill = _ => Event("Kill"), Translate = value => { Event("Translate," + Hex(value)); return value; },
                Style = (_, text) => { Event("Style," + Hex(text)); Node.Head = 0x12408000; }, Fit = (_, x, y) => { Event("Fit," + x + ',' + y); Node.BaseWidth = Node.Width = 51.25f; Node.BaseHeight = Node.Height = 13.5f; },
                Idle = (_, on) => { Event("Idle," + (on ? 1 : 0)); Node.Flags |= 0x80; }, Bounds = token => { Event("Bounds," + (token == Reference ? 1 : 0)); return Box; }, Position = _ => { Event("Position"); return Point; },
                Draw = (head, point) => Event("Draw," + (head != 0 ? 1 : 0) + ',' + unchecked((uint)point.X).ToString(CultureInfo.InvariantCulture) + ',' + unchecked((uint)point.Y).ToString(CultureInfo.InvariantCulture) + ",0"),
                Attach = (_, x, y, parent, mode) => { Event("Attach," + B(x) + ',' + B(y) + ',' + parent + ',' + mode); Node.X = x; Node.Y = y; },
                DeleteText = (token, flags) => { Event("TextDelete"); Node.Vtable = 0x50ac70; if ((flags & 1) != 0) Event("Free,2"); return token; },
            });
        }
        /// <summary>실제 호출 순서만 누적한다.</summary>
        private void Event(string text) { if (Trace.Length != 0) Trace += ';'; Trace += text; }
        /// <summary>원본이 관측한116바이트만 동일 offset/폭으로 기록한다.</summary>
        public string Snapshot()
        {
            byte[] raw = Convert.FromHexString(GumpRawSupport.Encode(Node)); var result = new List<byte>();
            // 원본 snapshot의 실제 선택 DWORD 순서를 보존한다.
            foreach (int at in new[] { 0, 4, 0x20, 0x2c, 0x30, 0x3c, 0x40, 0x44, 0x48, 0x4c, 0x50 }) result.AddRange(raw.AsSpan(at, 4).ToArray());
            result.AddRange(raw.AsSpan(0x5c, 16).ToArray()); result.AddRange(raw.AsSpan(0x80).ToArray()); result.AddRange(BitConverter.GetBytes(Node.Head != 0 ? 1u : 0u)); result.AddRange(BitConverter.GetBytes(Node.Reserved)); result.AddRange(BitConverter.GetBytes(Node.Until)); result.AddRange(BitConverter.GetBytes(Node.Owned != 0 ? 1u : 0u)); result.AddRange(BitConverter.GetBytes(Node.Reference)); Assert.Equal(116, result.Count); return Convert.ToHexStringLower(result.ToArray());
        }
    }
    /// <summary>두 x87 정밀도에서 같았던126개 입력의 전체 관측과 반환/serial을 비교한다.</summary>
    [Fact]
    public void Native126ObservedMemoryStringsOrderLifetimeAndWrappers()
    {
        int count = 0;
        // 행의 입력 열만 장치에 공급하고 나머지 열은 비교에만 쓴다.
        foreach (var row in Rows("production-hint"))
        {
            Assert.Equal(5, row.Length); var values = row[1].Split(','); var scene = new Scene();
            // 원본 DWORD/64비트 double을 명시 입력 비트로만 복원한다.
            uint N(int index) => U(values[index]);
            // 입력은 십진 uint64이며 NaN payload도 그대로 보존한다.
            double D(int index) => BitConverter.UInt64BitsToDouble(ulong.Parse(values[index], CultureInfo.InvariantCulture));
            switch (row[0])
            {
                case "C": scene.Now = D(0); scene.State.White = N(2); scene.State.Black = N(3); scene.State.Kind = unchecked((sbyte)(byte)N(4)); string[] texts = ["", "~0~w~ENot enough Storm Power to build this", "~0~w~ENone in inventory"]; scene.Handler.Construct(Token, texts[N(1)]); break;
                case "I": scene.Node.Until = D(0); scene.Now = D(1); scene.Handler.Idle(Token); break;
                case "D": scene.Node.Head = N(0); scene.Point = new(unchecked((int)N(1)), unchecked((int)N(2))); scene.Handler.Draw(Token); break;
                case "X": scene.Node.Owned = N(0) != 0 ? Owned : 0; Assert.Equal(Token, scene.Handler.DeleteScalar(Token, N(1))); break;
                default: Assert.Equal("W", row[0]); scene.Now = D(3); scene.Box = N(4) != 0 ? new(-17, 20, int.MaxValue, int.MinValue) : new(0, 0, 36, 36); scene.Handler.Show(N(0) != 0, unchecked((int)N(1)), unchecked((int)N(2)), Reference); break;
            }
            Assert.Equal(row[2], scene.Snapshot()); Assert.Equal(U(row[3]), scene.State.LastSerial); Assert.Equal(row[4], scene.Trace); count++;
        }
        Assert.Equal(126, count);
    }
}
