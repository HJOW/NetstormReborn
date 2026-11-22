using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Client.Tests; // 생산 UI의 원본 바이트 입력/출력만 변환한다.

/// <summary>기대 분기/배치값을 계산하지 않는 실제 offset 직렬화다.</summary>
internal static class InventoryRawSupport
{
    /// <summary>원본 격리 관측은 할당하지 않은 SID1/50도 입력하므로 검사에서만 실제 입력 배열을 쓴다.</summary>
    private static readonly System.Reflection.FieldInfo PoolBytes = typeof(RawSidPool).GetField("_bytes", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance)!;
    /// <summary>기대값을 쓰지 않고 명시 입력 바이트만 준비하는 검사 전용 물리 뷰다.</summary>
    public static Span<byte> Raw(RawSidPool pool, int sid) => ((byte[])PoolBytes.GetValue(pool)!).AsSpan(checked(sid * RawSidPool.Stride), RawSidPool.Stride);
    /// <summary>독립 관측의 풀 전체 불변 검사용 읽기 뷰다. 제품에는 노출하지 않는다.</summary>
    public static ReadOnlySpan<byte> All(RawSidPool pool) => pool.Bytes;
    /// <summary>원본 DWORD 십진 입력이다.</summary>
    public static uint U(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>원본 float32 입력 비트다.</summary>
    public static float F(uint bits) => BitConverter.UInt32BitsToSingle(bits);
    /// <summary>원본 float32 출력 비트다.</summary>
    public static uint B(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>원본 비정렬 DWORD다.</summary>
    public static uint Get(byte[] raw, int at) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at));
    /// <summary>원본 비정렬 DWORD 쓰기다.</summary>
    public static void Put(byte[] raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at), value);
    /// <summary>실제 같은 기본 노드 객체에 원문 입력 필드만 복사한다.</summary>
    public static T Base<T>(T node, byte[] raw) where T : GumpCreateNode
    {
        var source = GumpRawSupport.Decode(raw);
        // 기본/상속의 쓰기 가능한 의미 필드를 동일 주소에만 옮긴다.
        foreach (var property in typeof(GumpCreateNode).GetProperties()) if (property.CanWrite) property.SetValue(node, property.GetValue(source));
        source.Positioning.CopyTo(node.Positioning, 0); source.TextContext.CopyTo(node.TextContext, 0); return node;
    }
    /// <summary>미작성 필드의 입력을 구별하는 전체 원문 canary다.</summary>
    public static byte[] Canary(int bytes) => Enumerable.Repeat((byte)0xa5, bytes).ToArray();
    /// <summary>실제0xc0 항목의 모든 필드를 같은 폭으로 읽는다.</summary>
    public static InventoryItemNode Item(byte[] raw)
    {
        var node = Base(new InventoryItemNode(), raw); node.Content = Get(raw, 160); node.ReservedA4 = Get(raw, 164); node.BlinkUntil = BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(raw.AsSpan(168))); node.Blink = raw[176]; node.Decoration = raw[177]; node.ReservedB2 = BinaryPrimitives.ReadUInt16LittleEndian(raw.AsSpan(178)); node.RequireImageHit = Get(raw, 180); node.Additional = Get(raw, 184); node.IconScale = F(Get(raw, 188)); return node;
    }
    /// <summary>실제0xc4 격자의 모든 확장 입력이다.</summary>
    public static InventoryGridNode Grid(byte[] raw)
    {
        var node = Base(new InventoryGridNode(), raw); node.MinimumColumns = unchecked((int)Get(raw, 160)); node.MinimumRows = unchecked((int)Get(raw, 164)); node.CellWidth = F(Get(raw, 168)); node.CellHeight = F(Get(raw, 172)); node.ContentMask = Get(raw, 176); node.AcceptedCursor = Get(raw, 180); node.RequireImageHit = Get(raw, 184); node.Fixed = Get(raw, 188); node.ItemScale = F(Get(raw, 192)); return node;
    }
    /// <summary>동일 기본 노드와 모든 확장을 전체0xc0바이트로 기록한다.</summary>
    public static string Encode(InventoryItemNode node)
    {
        byte[] raw = Convert.FromHexString(GumpRawSupport.Encode(node)); Array.Resize(ref raw, 192); Put(raw, 160, node.Content); Put(raw, 164, node.ReservedA4); BinaryPrimitives.WriteUInt64LittleEndian(raw.AsSpan(168), BitConverter.DoubleToUInt64Bits(node.BlinkUntil)); raw[176] = node.Blink; raw[177] = node.Decoration; BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(178), node.ReservedB2); Put(raw, 180, node.RequireImageHit); Put(raw, 184, node.Additional); Put(raw, 188, B(node.IconScale)); return Convert.ToHexStringLower(raw);
    }
    /// <summary>동일 기본 노드와 모든 확장을 전체0xc4바이트로 기록한다.</summary>
    public static string Encode(InventoryGridNode node)
    {
        byte[] raw = Convert.FromHexString(GumpRawSupport.Encode(node)); Array.Resize(ref raw, 196); Put(raw, 160, unchecked((uint)node.MinimumColumns)); Put(raw, 164, unchecked((uint)node.MinimumRows)); Put(raw, 168, B(node.CellWidth)); Put(raw, 172, B(node.CellHeight)); Put(raw, 176, node.ContentMask); Put(raw, 180, node.AcceptedCursor); Put(raw, 184, node.RequireImageHit); Put(raw, 188, node.Fixed); Put(raw, 192, B(node.ItemScale)); return Convert.ToHexStringLower(raw);
    }
    /// <summary>외부 문자열의 원문 좁은 바이트다.</summary>
    public static string Hex(string text) => Convert.ToHexStringLower(Encoding.Latin1.GetBytes(text));
    /// <summary>명시 입력 DWORD의 전체 배열이다.</summary>
    public static uint[] Inputs(string text) => text.Split(',').Select(U).ToArray();
    /// <summary>기존 원본 파일을 한 소비자 경로에서 읽는다.</summary>
    public static IEnumerable<string[]> Rows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name + "-x86.tsv")).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
}
