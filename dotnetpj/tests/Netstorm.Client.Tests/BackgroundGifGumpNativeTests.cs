using System.Globalization;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 원본GIF창308행의현재필드/효과/명시중단을독립대조한다.

/// <summary>앞13열은독립입력,뒤6열은기대관측으로만사용한다.</summary>
public sealed class BackgroundGifGumpNativeTests
{
    /// <summary>원문signed입력의하위DWORD.</summary>
    private static uint U(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>원시DWORD목록만읽는다.</summary>
    private static uint[] Numbers(string value) => value.Split(':').Select(U).ToArray();
    /// <summary>현재float32의비트.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>현재실제14필드만관측한다.</summary>
    private static uint[] Fields(BackgroundGifGumpNode n) => [n.Vtable, n.Flags, n.Command, Bits(n.Scale), Bits(n.BaseWidth), Bits(n.BaseHeight), Bits(n.Width), Bits(n.Height), Bits(n.X), Bits(n.Y), n.Event, n.First, n.Second, n.Bitmap];
    /// <summary>독립입력/장치돌연변이만같은현재필드에설치한다.</summary>
    private static void Set(BackgroundGifGumpNode n, uint[] a) { n.Vtable = a[0]; n.Flags = a[1]; n.Command = a[2]; n.Scale = BitConverter.UInt32BitsToSingle(a[3]); n.BaseWidth = BitConverter.UInt32BitsToSingle(a[4]); n.BaseHeight = BitConverter.UInt32BitsToSingle(a[5]); n.Width = BitConverter.UInt32BitsToSingle(a[6]); n.Height = BitConverter.UInt32BitsToSingle(a[7]); n.X = BitConverter.UInt32BitsToSingle(a[8]); n.Y = BitConverter.UInt32BitsToSingle(a[9]); n.Event = a[10]; n.First = a[11]; n.Second = a[12]; n.Bitmap = a[13]; }
    /// <summary>장치가명시한중단을정상RET/제품오류와구별한다.</summary>
    private sealed class ExplicitStop : Exception;
    /// <summary>모든원문몸체/현재상태재조회/dispatch후접근없음을원본과비교한다.</summary>
    [Fact]
    public void All308OriginalGifBodiesAndCurrentEffectsMatch()
    {
        int count = 0, stops = 0; string[] names = ["vtable", "flags", "kind", "scale", "bw", "bh", "width", "height", "x", "y", "event", "first", "second", "bitmap"];
        // 각 관측을 독립 입력으로 실행하고 기대 필드를 장치에 쓰지 않는다.
        foreach (string[] f in X86Fixture.Read("background-gif-gump"))
        {
            if (f[0] == "name") continue; Assert.Equal(19, f.Length); var n = new BackgroundGifGumpNode(); Set(n, Numbers(f[3])); var state = new BackgroundGifGumpState { Kind = unchecked((sbyte)U(f[4])) }; var trace = new List<string>(); bool poisoned = false;
            // 실제 장치 인자 기록 뒤에만 명시 입력의 변경/중단을 적용한다.
            void Event(string name, params uint[] args) { trace.Add(name + (args.Length == 0 ? "" : ':' + string.Join(':', args))); if (f[11] != "-") { string[] mutation = f[11].Split('.'); if (mutation[0] == name) { if (mutation[1] == "kind") state.Kind = unchecked((sbyte)U(mutation[2])); else if (mutation[1] == "poison") { Set(n, Enumerable.Repeat(0x5c5c5c5cU, 14).ToArray()); poisoned = true; } else { uint[] fields = Fields(n); int index = Array.IndexOf(names, mutation[1]); Assert.True(index >= 0); fields[index] = U(mutation[2]); Set(n, fields); } } } if (f[12] == name) throw new ExplicitStop(); }
            var h = new BackgroundGifGumpHooks
            {
                Node = token => { if (poisoned) throw new InvalidOperationException("dispatch 뒤 원문이 읽지 않는 노드를 재조회함"); return token == 1 ? n : null; }, Base = token => token == 1 ? n : null,
                ConstructBase = _ => { n.Vtable = 0x501b60; n.Flags = 1; n.BaseWidth = n.BaseHeight = n.Width = n.Height = 0; Event("base"); }, DestructBase = _ => Event("destructBase"), PathSpec = name => { Assert.Equal("title", name); Event("spec"); return "spec"; }, Exists = path => { Assert.Equal("spec", path); Event("exists"); return f[5] == "1"; },
                Load = (string path, ref int width, ref int height) => { Assert.Equal("spec", path); uint[] size = Numbers(f[7]); width = unchecked((int)size[0]); height = unchecked((int)size[1]); Event("load"); return U(f[6]); }, ReleaseBitmap = bitmap => Event("free", bitmap), Position = _ => { uint[] point = Numbers(f[9]); Event("position"); return new(unchecked((int)point[0]), unchecked((int)point[1])); },
                Copy = r => Event("copy", unchecked((uint)r.Position.X), unchecked((uint)r.Position.Y), r.Bitmap, unchecked((uint)r.Stride), unchecked((uint)r.Source.Left), unchecked((uint)r.Source.Top), unchecked((uint)r.Source.Right), unchecked((uint)r.Source.Bottom)), Attach = (_, x, y, parent, mode) => Event("attach", Bits(x), Bits(y), parent, mode), Dispatch = (code, first, second) => Event("dispatch", code, first, second), BaseHandle = (_, code, first, second) => { Event("baseHandle", code, first, second); return U(f[10]); }
            }; var body = new BackgroundGifGump(state, h); bool completed = true; uint result = 0;
            try { uint[] a = Numbers(f[8]); switch (f[1]) { case "construct": result = body.Construct(1, "title"); break; case "refresh": body.Refresh(1, "title"); break; case "setEvent": body.SetEvent(1, a[0], a[1], a[2]); break; case "handle": result = body.Handle(1, a[0], a[1], a[2]); break; case "draw": body.Draw(1); break; case "destroy": body.Destruct(1); break; case "reattach": body.Reattach(1); break; default: throw new ArgumentException("원본GIF창진입이름"); } } catch (ExplicitStop) { completed = false; stops++; }
            Assert.Equal(f[13] == "1", completed); Assert.Equal(f[14], string.Join(':', Fields(n))); Assert.Equal(f[15], string.Join(';', trace)); Assert.Equal(U(f[16]), completed ? result : 0); uint stack = f[1] switch { "construct" or "refresh" => 8, "setEvent" or "handle" => 16, _ => 4 }; if (completed) Assert.Equal(stack, U(f[17])); else _ = U(f[17]); Assert.True(U(f[18]) > 0); count++;
        }
        Assert.Equal(308, count); Assert.Equal(16, stops);
    }
}
