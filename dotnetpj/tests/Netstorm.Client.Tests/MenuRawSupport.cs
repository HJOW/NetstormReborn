using System.Buffers.Binary;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 원본 메뉴 관찰의 실제 필드/전체 미작성 바이트를 보존하는 입출력 코덱이다.

/// <summary>기대 연산을 계산하지 않고 입력/현재 상태만 같은 원본 offset으로 읽고 쓴다.</summary>
internal sealed class MenuRawSupport
{
    /// <summary>원본 관찰의 메뉴/장식/제목 토큰 순서.</summary>
    public static readonly uint[] Tokens = [0x18000010, 0x18001010, 0x18002010];
    /// <summary>MenuGump 기본 관찰이 기록한55DWORD offset.</summary>
    private static readonly int[] Fields = [0, 4, 56, 60, 64, 68, 72, 184, 368, 372, 376, .. Enumerable.Range(0, 40).Select(i => 380 + i * 4), 544, 548, 2952, 2956];
    /// <summary>같은 안정된 실제 메뉴/기본 노드.</summary>
    public Dictionary<uint, MenuGumpNode> Nodes { get; } = [];
    /// <summary>관찰에 포함된 미작성 바이트의 입력/명시 패치 사본.</summary>
    private readonly Dictionary<uint, byte[]> _templates = [];
    /// <summary>원본 little endian DWORD.</summary>
    private static uint Get(byte[] raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset));
    /// <summary>현재 DWORD를 원본 주소에 쓴다.</summary>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset), value);
    /// <summary>원본 float32 비트.</summary>
    private static float Float(byte[] raw, int offset) => BitConverter.UInt32BitsToSingle(Get(raw, offset));
    /// <summary>안정된 같은 객체에 현재 입력 필드만 복사한다.파생/항목 객체 주소를 교체하지 않는다.</summary>
    private static void Decode(byte[] raw, MenuGumpNode n)
    {
        var basic = GumpRawSupport.Decode(raw);
        // 기존 기본 노드 코덱의 속성만 복사해 메뉴 고정 배열 주소를 보존한다.
        foreach (var property in typeof(GumpCreateNode).GetProperties()) if (property.CanWrite) property.SetValue(n, property.GetValue(basic)); basic.Positioning.CopyTo(n.Positioning, 0); basic.TextContext.CopyTo(n.TextContext, 0); if (raw.Length < 2960) return;
        n.Expanding = Get(raw, 184); n.Selection = unchecked((int)Get(raw, 204)); n.ItemWidth = Float(raw, 368); n.TargetWidth = Float(raw, 372); n.TargetHeight = Float(raw, 376); n.SelectionCallback = Get(raw, 540); n.MenuFlags = Get(raw, 544); n.ItemCount = Get(raw, 548); n.Fancy = Get(raw, 2952); n.Title = Get(raw, 2956);
        // 원본 여백/높이/offset과 모든 항목 바이트를 입력 순서로만 읽는다.
        for (int i = 0; i < 4; i++) n.Margins[i] = Float(raw, 188 + i * 4);
        // 기대 폭/높이를 여기서 계산하지 않는다.
        for (int i = 0; i < 40; i++) { n.Heights[i] = Float(raw, 208 + i * 4); n.Offsets[i] = Float(raw, 380 + i * 4); int at = 552 + i * 60; var item = n.Items[i]; raw.AsSpan(at, 40).CopyTo(item.Label); item.ChildId = Get(raw, at + 40); item.Command = Get(raw, at + 44); item.Parameter = Get(raw, at + 48); item.Styled = Get(raw, at + 52); item.Flags = Get(raw, at + 56); }
    }
    /// <summary>전체 입력 또는55DWORD 압축 입력을 원본 주소로 펼친다.</summary>
    public MenuRawSupport(string text, bool packed)
    {
        string[] parts = text.Split(';'); Assert.Equal(3, parts.Length);
        // 토큰 순서/실제 바이트만 등록한다.
        for (int i = 0; i < 3; i++) { byte[] raw = X86Fixture.Bytes(parts[i]); if (packed) { Assert.Equal(Fields.Length * 4, raw.Length); byte[] full = new byte[2960]; for (int j = 0; j < Fields.Length; j++) Put(full, Fields[j], Get(raw, j * 4)); raw = full; } _templates.Add(Tokens[i], raw); var n = new MenuGumpNode(); Decode(raw, n); Nodes.Add(Tokens[i], n); }
    }
    /// <summary>실제 현재 필드와 입력의 미작성 바이트를 같은 원본 배치에 기록한다.</summary>
    public byte[] Raw(uint token)
    {
        byte[] raw = (byte[])_templates[token].Clone(); var n = Nodes[token]; X86Fixture.Bytes(GumpRawSupport.Encode(n)).CopyTo(raw, 0); if (raw.Length < 2960) return raw;
        Put(raw, 184, n.Expanding); Put(raw, 204, unchecked((uint)n.Selection)); Put(raw, 368, BitConverter.SingleToUInt32Bits(n.ItemWidth)); Put(raw, 372, BitConverter.SingleToUInt32Bits(n.TargetWidth)); Put(raw, 376, BitConverter.SingleToUInt32Bits(n.TargetHeight)); Put(raw, 540, n.SelectionCallback); Put(raw, 544, n.MenuFlags); Put(raw, 548, n.ItemCount); Put(raw, 2952, n.Fancy); Put(raw, 2956, n.Title);
        // 현재 실제 배열/항목 필드만 직렬화한다.
        for (int i = 0; i < 4; i++) Put(raw, 188 + i * 4, BitConverter.SingleToUInt32Bits(n.Margins[i]));
        // 모든40칸을 보존해 미사용 칸의 임의 초기화도 검출한다.
        for (int i = 0; i < 40; i++) { Put(raw, 208 + i * 4, BitConverter.SingleToUInt32Bits(n.Heights[i])); Put(raw, 380 + i * 4, BitConverter.SingleToUInt32Bits(n.Offsets[i])); MenuGumpLayout.MenuGumpItemBytes(n.Items[i]).CopyTo(raw, 552 + i * 60); } return raw;
    }
    /// <summary>관찰 형식으로 현재 전체/선택 필드를 기록한다.</summary>
    public string Snapshot(bool packed)
    {
        var result = new List<string>();
        // 원본 토큰 순서로만 기록한다.
        foreach (uint token in Tokens) { byte[] raw = Raw(token); if (packed) { byte[] select = new byte[Fields.Length * 4]; for (int i = 0; i < Fields.Length; i++) Put(select, i * 4, Get(raw, Fields[i])); raw = select; } result.Add(Convert.ToHexStringLower(raw)); } return string.Join(';', result);
    }
    /// <summary>콜백 입력의 명시 DWORD 패치만 현재 안정된 객체와 미작성 바이트에 반영한다.</summary>
    public void Patch(uint token, int offset, uint value) { byte[] raw = Raw(token); Put(raw, offset, value); Decode(raw, Nodes[token]); _templates[token] = raw; }
    /// <summary>zlib과 같은 표준 CRC32로 현재 전체 바이트를 관측한다.기대값으로 상태를 만들지 않는다.</summary>
    public uint Crc()
    {
        uint crc = uint.MaxValue;
        // 원본이 CRC에 합친 세 실제 저장소의 순서를 유지한다.
        foreach (uint token in Tokens)
        {
            // 각 실제 BYTE를 표준 다항식으로만 누적한다.
            foreach (byte value in Raw(token)) { crc ^= value; for (int bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ ((crc & 1) != 0 ? 0xedb88320U : 0); }
        }
        return ~crc;
    }
}
