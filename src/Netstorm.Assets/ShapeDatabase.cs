using System.Buffers.Binary;

namespace Netstorm.Assets;

/// <summary>
/// 셰이프 프레임 헤더. 픽셀 영역은 기준점(0,0) 대비 [XMin..XMax] × [YMin..YMax] (양 끝 포함).
/// </summary>
/// <param name="Offset">파일 내 절대 위치</param>
/// <param name="Bounds0">bounds 첫 값 (원본 그림 크기 추정)</param>
/// <param name="Bounds1">bounds 둘째 값</param>
/// <param name="Origin0">origin 첫 값 (원본 그림 안 기준점 추정)</param>
/// <param name="Origin1">origin 둘째 값</param>
/// <param name="XMin">기준점 대비 왼쪽 끝</param>
/// <param name="YMin">기준점 대비 위쪽 끝</param>
/// <param name="XMax">기준점 대비 오른쪽 끝 (포함)</param>
/// <param name="YMax">기준점 대비 아래쪽 끝 (포함)</param>
public sealed record ShapeFrame(
    int Offset, ushort Bounds0, ushort Bounds1, ushort Origin0, ushort Origin1,
    int XMin, int YMin, int XMax, int YMax)
{
    /// <summary>이미지가 아닌 특수 레코드 판별 기준 (xmin 이 이 값 이상이면 좌표가 아니다)</summary>
    private const int SpecialRecordThreshold = 0x7FFF0000;

    /// <summary>픽셀 영역 폭</summary>
    public int Width => XMax - XMin + 1;

    /// <summary>픽셀 영역 높이</summary>
    public int Height => YMax - YMin + 1;

    /// <summary>이미지가 아닌 특수 레코드 여부 (원본에 90개 존재, 용도 미확정)</summary>
    public bool IsSpecial => XMin >= SpecialRecordThreshold || XMax < XMin || YMax < YMin;
}

/// <summary>셰이프 블록 하나 (= .type 하나의 모든 프레임)</summary>
/// <param name="Index">블록 번호 (= 타입 로딩 순서)</param>
/// <param name="Offset">블록 헤더("1.10") 파일 내 위치</param>
/// <param name="Frames">프레임 목록</param>
public sealed record ShapeBlock(int Index, int Offset, IReadOnlyList<ShapeFrame> Frames)
{
    /// <summary>이 블록이 배정되는 타입 이름 (TypeLoadOrder 기준)</summary>
    public string TypeName => TypeLoadOrder.Names[Index];
}

/// <summary>디코딩된 8비트 인덱스 이미지. 투명 픽셀은 Opaque[i] == false</summary>
/// <param name="Width">폭</param>
/// <param name="Height">높이</param>
/// <param name="Indices">팔레트 인덱스 (행 우선)</param>
/// <param name="Opaque">불투명 여부 (행 우선)</param>
public sealed record IndexedImage(int Width, int Height, byte[] Indices, bool[] Opaque);

/// <summary>
/// d/_shapes.shp (Miles VFX 셰이프 데이터베이스) 읽기. 포맷: docs/formats/shp.md
/// </summary>
public sealed class ShapeDatabase
{
    /// <summary>블록 매직 "1.10" (u32 리틀 엔디언 0x30312E31)</summary>
    private const uint BlockMagic = 0x30312E31;

    /// <summary>블록 헤더 고정부 크기 (매직 u32 + 프레임 수 u32)</summary>
    private const int BlockHeaderSize = 8;

    /// <summary>프레임 테이블 항목 크기 (오프셋 u32 + 컬러맵 오프셋 u32)</summary>
    private const int FrameEntrySize = 8;

    /// <summary>프레임 헤더 크기 (u16×4 + i32×4)</summary>
    private const int FrameHeaderSize = 24;

    /// <summary>원본 파일 이름</summary>
    public const string FileName = "_shapes.shp";

    private readonly byte[] _raw;

    /// <summary>블록 목록 (파일 앞부분에 연속으로 놓인 순서)</summary>
    public IReadOnlyList<ShapeBlock> Blocks { get; }

    /// <summary>메모리에 올린 파일 내용으로부터 만든다</summary>
    /// <param name="raw">_shapes.shp 전체 내용</param>
    public ShapeDatabase(byte[] raw)
    {
        _raw = raw;
        var blocks = new List<ShapeBlock>();
        int pos = 0;
        // "1.10" 매직이 이어지는 동안 블록 헤더를 차례로 읽는다
        while (pos + BlockHeaderSize <= raw.Length && ReadUInt32(pos) == BlockMagic)
        {
            int count = ReadInt32(pos + 4);
            var frames = new ShapeFrame[count];
            // 프레임 테이블을 읽는다. 프레임 오프셋은 블록 헤더 시작 기준이다
            for (int i = 0; i < count; i++)
            {
                int rel = ReadInt32(pos + BlockHeaderSize + i * FrameEntrySize);
                frames[i] = ReadFrameHeader(pos + rel);
            }
            blocks.Add(new ShapeBlock(blocks.Count, pos, frames));
            pos += BlockHeaderSize + count * FrameEntrySize;
        }
        Blocks = blocks;
    }

    /// <summary>파일에서 읽는다</summary>
    /// <param name="path">_shapes.shp 경로</param>
    public static ShapeDatabase Load(string path) => new(File.ReadAllBytes(path));

    /// <summary>타입 이름(대소문자 무시)으로 블록을 찾는다</summary>
    /// <param name="typeName">예: "sunCannon"</param>
    public ShapeBlock? FindBlock(string typeName)
    {
        int index = TypeLoadOrder.IndexOf(typeName);
        return index >= 0 && index < Blocks.Count ? Blocks[index] : null;
    }

    /// <summary>
    /// 프레임 픽셀을 디코딩한다. 행마다 토큰을 읽는다:
    /// 0x00 = 행 끝, 0x01 n = 투명 n 픽셀, 홀수 t = (t>>1) 바이트 복사, 짝수 t = 다음 1바이트를 (t>>1) 번 반복
    /// </summary>
    /// <param name="frame">디코딩할 프레임 (특수 레코드는 불가)</param>
    public IndexedImage Decode(ShapeFrame frame)
    {
        if (frame.IsSpecial)
        {
            throw new InvalidOperationException("특수 레코드는 이미지가 아닙니다.");
        }
        int w = frame.Width;
        int h = frame.Height;
        var indices = new byte[w * h];
        var opaque = new bool[w * h];
        int pos = frame.Offset + FrameHeaderSize;
        // 행 수만큼 반복
        for (int y = 0; y < h; y++)
        {
            int x = 0;
            int rowBase = y * w;
            // 행 끝 토큰(0)이 나올 때까지 토큰을 처리
            while (true)
            {
                byte t = _raw[pos++];
                if (t == 0)
                {
                    break;
                }
                if (t == 1)
                {
                    x += _raw[pos++];
                    continue;
                }
                int n = t >> 1;
                bool literal = (t & 1) != 0;
                // n 개 픽셀을 채운다 (리터럴이면 바이트를 차례로, 반복이면 같은 바이트로). 폭을 넘는 픽셀은 버린다
                for (int k = 0; k < n; k++)
                {
                    if (x < w)
                    {
                        indices[rowBase + x] = literal ? _raw[pos + k] : _raw[pos];
                        opaque[rowBase + x] = true;
                    }
                    x++;
                }
                pos += literal ? n : 1;
            }
        }
        return new IndexedImage(w, h, indices, opaque);
    }

    /// <summary>프레임 헤더를 읽는다</summary>
    /// <param name="offset">프레임 헤더 파일 내 위치</param>
    private ShapeFrame ReadFrameHeader(int offset)
    {
        ReadOnlySpan<byte> s = _raw.AsSpan(offset, FrameHeaderSize);
        return new ShapeFrame(
            offset,
            BinaryPrimitives.ReadUInt16LittleEndian(s),
            BinaryPrimitives.ReadUInt16LittleEndian(s[2..]),
            BinaryPrimitives.ReadUInt16LittleEndian(s[4..]),
            BinaryPrimitives.ReadUInt16LittleEndian(s[6..]),
            BinaryPrimitives.ReadInt32LittleEndian(s[8..]),
            BinaryPrimitives.ReadInt32LittleEndian(s[12..]),
            BinaryPrimitives.ReadInt32LittleEndian(s[16..]),
            BinaryPrimitives.ReadInt32LittleEndian(s[20..]));
    }

    /// <summary>리틀 엔디언 int32 읽기</summary>
    /// <param name="offset">파일 내 위치</param>
    private int ReadInt32(int offset) => BinaryPrimitives.ReadInt32LittleEndian(_raw.AsSpan(offset, 4));

    /// <summary>리틀 엔디언 uint32 읽기</summary>
    /// <param name="offset">파일 내 위치</param>
    private uint ReadUInt32(int offset) => BinaryPrimitives.ReadUInt32LittleEndian(_raw.AsSpan(offset, 4));
}
