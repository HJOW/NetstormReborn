using System.Buffers.Binary;

namespace Netstorm.Assets;

/// <summary>
/// TrueType Collection(.ttc) 에서 글꼴 하나(face)를 단독 TrueType(.ttf) 바이트로 분리한다.
/// 한국어 글꼴 D2Coding 이 TTC 로 제공되는데, 일부 글꼴 렌더러는 TTC 의 face 선택을 지원하지 않기 때문이다.
/// </summary>
public static class TrueTypeCollection
{
    /// <summary>TTC 파일 매직 "ttcf"</summary>
    private const uint TtcTag = 0x74746366;

    /// <summary>TTC 헤더에서 face 개수(u32) 위치</summary>
    private const int OffsetNumFonts = 8;

    /// <summary>TTC 헤더에서 face 별 오프셋 테이블 시작 위치</summary>
    private const int OffsetFontTable = 12;

    /// <summary>sfnt 오프셋 테이블 고정부 크기 (sfntVersion u32 + numTables u16 + searchRange·entrySelector·rangeShift u16×3)</summary>
    private const int SfntHeaderSize = 12;

    /// <summary>테이블 레코드 크기 (tag, checksum, offset, length 각 u32)</summary>
    private const int TableRecordSize = 16;

    /// <summary>데이터가 TTC 인지 검사한다</summary>
    /// <param name="data">글꼴 파일 내용</param>
    public static bool IsCollection(ReadOnlySpan<byte> data) =>
        data.Length >= 4 && BinaryPrimitives.ReadUInt32BigEndian(data) == TtcTag;

    /// <summary>TTC 안의 face 개수</summary>
    /// <param name="data">TTC 파일 내용</param>
    public static int FaceCount(ReadOnlySpan<byte> data) =>
        (int)BinaryPrimitives.ReadUInt32BigEndian(data[OffsetNumFonts..]);

    /// <summary>
    /// face 하나를 단독 .ttf 바이트로 만든다. 테이블 데이터는 그대로 복사하고 오프셋만 다시 계산한다.
    /// </summary>
    /// <param name="data">TTC 파일 내용</param>
    /// <param name="faceIndex">face 번호 (D2Coding: 0 일반, 1 Bold, 2 ligature, 3 ligature Bold)</param>
    public static byte[] ExtractFace(ReadOnlySpan<byte> data, int faceIndex)
    {
        if (!IsCollection(data))
        {
            throw new InvalidDataException("TTC 파일이 아닙니다.");
        }
        int faces = FaceCount(data);
        if (faceIndex < 0 || faceIndex >= faces)
        {
            throw new ArgumentOutOfRangeException(nameof(faceIndex), $"face 는 0~{faces - 1} 범위여야 합니다.");
        }
        int sfnt = (int)BinaryPrimitives.ReadUInt32BigEndian(data[(OffsetFontTable + faceIndex * 4)..]);
        int numTables = BinaryPrimitives.ReadUInt16BigEndian(data[(sfnt + 4)..]);
        int headerLength = SfntHeaderSize + numTables * TableRecordSize;

        // 새 파일 크기 계산: 헤더 + 각 테이블(4바이트 정렬)
        int total = headerLength;
        // 테이블 레코드마다 길이를 4바이트 경계로 올려 더한다
        for (int i = 0; i < numTables; i++)
        {
            int length = (int)BinaryPrimitives.ReadUInt32BigEndian(data[(sfnt + SfntHeaderSize + i * TableRecordSize + 12)..]);
            total += Align4(length);
        }

        var output = new byte[total];
        data.Slice(sfnt, headerLength).CopyTo(output);
        int writePos = headerLength;
        // 테이블을 차례로 복사하고 레코드의 오프셋을 새 위치로 고친다
        for (int i = 0; i < numTables; i++)
        {
            int record = SfntHeaderSize + i * TableRecordSize;
            int offset = (int)BinaryPrimitives.ReadUInt32BigEndian(data[(sfnt + record + 8)..]);
            int length = (int)BinaryPrimitives.ReadUInt32BigEndian(data[(sfnt + record + 12)..]);
            data.Slice(offset, length).CopyTo(output.AsSpan(writePos));
            BinaryPrimitives.WriteUInt32BigEndian(output.AsSpan(record + 8), (uint)writePos);
            writePos += Align4(length);
        }
        return output;
    }

    /// <summary>4바이트 경계로 올림</summary>
    /// <param name="value">원래 값</param>
    private static int Align4(int value) => (value + 3) & ~3;
}
