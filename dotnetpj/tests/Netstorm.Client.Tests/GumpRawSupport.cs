using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // Gump 원본 바이트를 검사 입력/관측으로 변환하는 영역이다.

/// <summary>원본 Gump 기본 40DWORD의 직렬화만 공유한다. 기대 생성/링크/좌표는 계산하지 않는다.
/// 근거: cpppj/tests/GumpRawSupport.h.</summary>
internal static class GumpRawSupport
{
    /// <summary>packed DWORD를 little endian으로 읽는다.</summary>
    private static uint Get(byte[] raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset));

    /// <summary>packed DWORD를 little endian으로 쓴다.</summary>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset), value);

    /// <summary>float32 비트를 읽으며 계산/폭 보정을 하지 않는다.</summary>
    private static float Float(byte[] raw, int offset) => BitConverter.UInt32BitsToSingle(Get(raw, offset));

    /// <summary>128/160바이트의 기본/생성 노드를 원본 offset으로만 읽는다.</summary>
    public static GumpCreateNode Decode(byte[] raw)
    {
        var n = new GumpCreateNode
        {
            Vtable = Get(raw, 0), Flags = Get(raw, 4), AlignFlags = Get(raw, 8), Next = Get(raw, 12), Previous = Get(raw, 16),
            Parent = Get(raw, 20), Child = Get(raw, 24), Id = Get(raw, 28), Command = Get(raw, 32), Object = Get(raw, 36),
            Form = Get(raw, 40), Serial = Get(raw, 44), Depth = Get(raw, 48), SecondaryDepth = Get(raw, 52),
            Scale = Float(raw, 56), BaseWidth = Float(raw, 60), BaseHeight = Float(raw, 64), Width = Float(raw, 68), Height = Float(raw, 72),
            X = Float(raw, 76), Y = Float(raw, 80), Cached = new(unchecked((int)Get(raw, 108)), unchecked((int)Get(raw, 112))),
            Untouched = Get(raw, 116), TextPointer = Get(raw, 120), AlternateTextPointer = Get(raw, 124),
        };
        // 위치/속도 필드는 원본 DWORD 비트로 보존한다.
        for (int i = 0; i < n.Positioning.Length; i++) n.Positioning[i] = Get(raw, 84 + i * 4);
        // 생성 입력에만 있는 텍스트 문맥을 그대로 읽는다.
        if (raw.Length >= 160)
        {
            // 문맥의 8DWORD는 기본 생성 전 임의 값도 보존한다.
            for (int i = 0; i < n.TextContext.Length; i++) n.TextContext[i] = Get(raw, 128 + i * 4);
        }
        return n;
    }

    /// <summary>현재 모든 생성 필드를 원본 offset의160바이트로 직렬화한다.</summary>
    public static string Encode(GumpCreateNode n)
    {
        byte[] raw = new byte[160];
        uint[] words = [n.Vtable, n.Flags, n.AlignFlags, n.Next, n.Previous, n.Parent, n.Child, n.Id, n.Command, n.Object, n.Form, n.Serial,
            n.Depth, n.SecondaryDepth, BitConverter.SingleToUInt32Bits(n.Scale), BitConverter.SingleToUInt32Bits(n.BaseWidth), BitConverter.SingleToUInt32Bits(n.BaseHeight),
            BitConverter.SingleToUInt32Bits(n.Width), BitConverter.SingleToUInt32Bits(n.Height), BitConverter.SingleToUInt32Bits(n.X), BitConverter.SingleToUInt32Bits(n.Y)];
        // 실제 현재 필드를 원래 주소 순서로만 쓴다.
        for (int i = 0; i < words.Length; i++) Put(raw, i * 4, words[i]);
        // 위치/속도 입력의 실제 비트를 쓴다.
        for (int i = 0; i < n.Positioning.Length; i++) Put(raw, 84 + i * 4, n.Positioning[i]);
        Put(raw, 108, unchecked((uint)n.Cached.X)); Put(raw, 112, unchecked((uint)n.Cached.Y)); Put(raw, 116, n.Untouched);
        Put(raw, 120, n.TextPointer); Put(raw, 124, n.AlternateTextPointer);
        // 현재 텍스트 문맥을 그대로 쓴다.
        for (int i = 0; i < n.TextContext.Length; i++) Put(raw, 128 + i * 4, n.TextContext[i]);
        return Convert.ToHexStringLower(raw);
    }

    /// <summary>토큰=원본바이트 입력을 주소 순서의 안정된 저장소로 읽는다.</summary>
    public static SortedDictionary<uint, GumpCreateNode> Nodes(string text)
    {
        var nodes = new SortedDictionary<uint, GumpCreateNode>();
        // 완성 입력의 모든 노드를 원본 바이트로만 채운다.
        foreach (string item in text.Split(';'))
        {
            string[] parts = item.Split('='); nodes.Add(X86Fixture.UInt(parts[0]), Decode(X86Fixture.Bytes(parts[1])));
        }
        return nodes;
    }

    /// <summary>현재 노드를 주소 순서로 직렬화한다. 기대 상태를 계산하지 않는다.</summary>
    public static string EncodeNodes(SortedDictionary<uint, GumpCreateNode> nodes) => string.Join(';', nodes.Select(pair => $"{pair.Key}={Encode(pair.Value)}"));

    /// <summary>명시 응답의 DWORD 하나를 원본 offset에 쓴다. C++ 구조체 대입처럼 같은 노드 인스턴스에 필드만 복사한다.</summary>
    public static void ApplyDword(GumpCreateNode node, int offset, uint value)
    {
        byte[] raw = X86Fixture.Bytes(Encode(node)); Put(raw, offset, value); GumpCreateNode changed = Decode(raw);
        // 현재 필드의 값만 복사하고 처리기가 빌린 노드 참조는 교체하지 않는다.
        foreach (System.Reflection.PropertyInfo property in typeof(GumpCreateNode).GetProperties())
        {
            if (property.CanWrite) property.SetValue(node, property.GetValue(changed));
        }
        changed.Positioning.CopyTo(node.Positioning, 0); changed.TextContext.CopyTo(node.TextContext, 0);
    }

    /// <summary>원본 공통 draw entry의16바이트를 현재 값으로 쓴다. guard/미사용 영역도 같은 방법으로 대조한다.</summary>
    public static string EncodeEntries(ReadOnlySpan<RenderCollectionEntry> entries)
    {
        byte[] raw = new byte[entries.Length * 16];
        // 구조체의 CLR 배치 대신 명시한 원본 offset을 쓴다.
        for (int i = 0; i < entries.Length; i++)
        {
            RenderCollectionEntry e = entries[i]; int offset = i * 16;
            Put(raw, offset, BitConverter.SingleToUInt32Bits(e.X)); Put(raw, offset + 4, BitConverter.SingleToUInt32Bits(e.Y)); Put(raw, offset + 8, e.Token);
            BinaryPrimitives.WriteInt16LittleEndian(raw.AsSpan(offset + 12), e.Depth); raw[offset + 14] = e.Kind; raw[offset + 15] = e.Tail;
        }
        return Convert.ToHexStringLower(raw);
    }

    /// <summary>표시 입력/관측의 signed 정수 좌표열.</summary>
    public static string Coordinates(ScreenPoint point) => $"{point.X},{point.Y}";

    /// <summary>표시 경계 관측의 원본 left/top/right/bottom 순서.</summary>
    public static string Coordinates(ScreenRect rect) => $"{rect.Left},{rect.Top},{rect.Right},{rect.Bottom}";
}
