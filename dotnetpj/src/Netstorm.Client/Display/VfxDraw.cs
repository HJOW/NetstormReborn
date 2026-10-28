namespace Netstorm.Client.Display;

/// <summary>
/// 셰이프 그림의 사각형 경계. 원본의 양 끝을 포함하는 좌표다.
/// 근거: cpppj/src/client/VFXDraw.h 의 ShapeRect. (R2b, 2026-10-28)
/// </summary>
/// <param name="Left">왼쪽(포함).</param>
/// <param name="Top">위쪽(포함).</param>
/// <param name="Right">오른쪽(포함).</param>
/// <param name="Bottom">아래쪽(포함).</param>
public readonly record struct ShapeRect(int Left, int Top, int Right, int Bottom);

/// <summary>
/// 셰이프 프레임의 위치·크기와 픽셀 데이터 정보.
/// 근거: cpppj/src/client/VFXDraw.h 의 ShapeFrame. (R2b, 2026-10-28)
/// </summary>
public sealed class ShapeFrame
{
    /// <summary>원본 헤더 순서: 그림 상자 높이·폭.</summary>
    public ushort[] Bounds { get; } = new ushort[2];

    /// <summary>원본 헤더 순서: 그림 상자 안의 기준점 y·x.</summary>
    public ushort[] Origin { get; } = new ushort[2];

    /// <summary>기준점에 상대적인 실제 압축 픽셀 영역.</summary>
    public ShapeRect Rect { get; set; }

    /// <summary>파일 안의 프레임 시작.</summary>
    public int Offset { get; set; }

    /// <summary>다음 레코드 경계(공유 프레임의 압축 스트림 끝).</summary>
    public int End { get; set; }

    /// <summary>블록 상대 주소. 기본 VFX 그리기는 이 값을 사용하지 않는다.</summary>
    public uint ColorMapOffset { get; set; }

    /// <summary>좌표 대신 메타데이터가 들어 있는 레코드인지 검사한다. 바이트는 그대로 보존한다.</summary>
    public bool IsSpecial() => Rect.Left >= 0x7fff0000 || Rect.Right < Rect.Left || Rect.Bottom < Rect.Top;
}

/// <summary>
/// 같은 셰이프에 속한 프레임과 부가 정보의 묶음.
/// 근거: cpppj/src/client/VFXDraw.h 의 ShapeBlock. (R2b, 2026-10-28)
/// </summary>
public sealed class ShapeBlock
{
    /// <summary>프레임 주소의 기준이 되는 "1.10" 헤더 위치.</summary>
    public int Offset { get; set; }

    /// <summary>타입별 프레임 자료.</summary>
    public List<ShapeFrame> Frames { get; } = [];
}

/// <summary>
/// 객체 발자국·출력 크기·핫스폿을 계산할 프레임 치수.
/// 근거: cpppj/src/client/VFXDraw.h 의 SquidFrameMetrics. (R2b, 2026-10-28)
/// </summary>
/// <param name="CellWidth">SHP의 VFX 앞 36바이트: Pop의 hash level 입력.</param>
/// <param name="CellHeight">SHP의 VFX 앞 36바이트: Pop의 hash level 입력.</param>
/// <param name="Width">VFX 앞 12바이트: Renderer 경계 입력.</param>
/// <param name="Height">VFX 앞 12바이트: Renderer 경계 입력.</param>
/// <param name="HotspotX">VFX 앞 12바이트: Renderer 경계 입력.</param>
/// <param name="HotspotY">VFX 앞 12바이트: Renderer 경계 입력.</param>
/// <param name="CellHotspotX">VFX 앞 28/24바이트: 월드 표시 수집의 float 기준점.</param>
/// <param name="CellHotspotY">VFX 앞 28/24바이트: 월드 표시 수집의 float 기준점.</param>
/// <param name="WalkStepX">추가 헤더의 보행 간격이며 수집용 기준점과 별도다.</param>
/// <param name="WalkStepY">추가 헤더의 보행 간격이며 수집용 기준점과 별도다.</param>
public readonly record struct SquidFrameMetrics(float CellWidth, float CellHeight, short Width, short Height,
    short HotspotX, short HotspotY, float CellHotspotX, float CellHotspotY, float WalkStepX, float WalkStepY);

/// <summary>
/// 팔레트 인덱스 픽셀로 구성한 그림. 0과 255도 유효한 불투명 색이다.
/// 근거: cpppj/src/client/VFXDraw.h 의 IndexedImage. (R2b, 2026-10-28)
/// </summary>
public sealed class IndexedImage
{
    /// <summary>가로 크기.</summary>
    public int Width { get; set; }

    /// <summary>세로 크기.</summary>
    public int Height { get; set; }

    /// <summary>팔레트 번호 픽셀.</summary>
    public byte[] Indices { get; set; } = [];

    /// <summary>투명 건너뛰기는 색 번호와 분리하여 0/255로 저장한다.</summary>
    public byte[] Opacity { get; set; } = [];
}

/// <summary>
/// 셰이프 블록과 프레임을 읽어 그림으로 변환하는 저장소.
/// 근거: cpppj/src/client/VFXDraw.cpp 의 ShapeDatabase (원본 00401d92). (R2b, 2026-10-28)
/// </summary>
public sealed class ShapeDatabase
{
    /// <summary>VFX 1.10 셰이프 블록의 리틀 엔디언 매직.</summary>
    private const uint ShapeMagic = 0x30312e31;

    /// <summary>프레임의 bounds·origin·사각형 헤더 크기.</summary>
    private const int FrameHeaderSize = 24;

    /// <summary>Squid 전용 추가 헤더 크기(36바이트).</summary>
    private const int SquidPrefixSize = 36;

    /// <summary>손상된 파일 때문에 거대한 버퍼를 할당하지 않도록 정한 상한.</summary>
    private const long MaxDecodedPixels = 64L * 1024 * 1024;

    /// <summary>원본 바이트.</summary>
    private readonly byte[] _bytes;

    /// <summary>타입 로딩 순서에 대응하는 블록 목록.</summary>
    private readonly List<ShapeBlock> _blocks = [];

    /// <summary>추가 헤더가 블록 테이블을 읽는 일을 막는 경계.</summary>
    private int _headerEnd;

    /// <summary>
    /// 블록 상대 오프셋과 공유 프레임을 해석하고 파일 경계를 검증한다. 손상된 파일은 거부한다.
    /// </summary>
    /// <param name="bytes">셰이프 파일 바이트.</param>
    public ShapeDatabase(byte[] bytes)
    {
        ArgumentNullException.ThrowIfNull(bytes);
        _bytes = bytes;
        var frameOffsets = new List<int>();
        // 이어지는 블록 헤더를 순서대로 읽어 타입 인덱스를 보존한다
        int position = 0;
        while (position <= _bytes.Length && _bytes.Length - position >= 4 && U32(position) == ShapeMagic)
        {
            uint count = U32(position + 4);
            CheckRange(position, 8);
            if (count > (uint)(_bytes.Length - position - 8) / 8)
            {
                throw new InvalidDataException("셰이프 프레임 개수가 파일 범위를 벗어났습니다");
            }
            var block = new ShapeBlock { Offset = position };
            // 테이블에 같은 오프셋이 여러 번 있어도 각각의 프레임 인덱스를 유지한다
            for (long i = 0; i < count; i++)
            {
                uint relative = U32(position + 8 + (int)i * 8);
                if (relative > (uint)(_bytes.Length - position))
                {
                    throw new InvalidDataException("셰이프 프레임 오프셋이 파일 범위를 벗어났습니다");
                }
                int offset = position + (int)relative;
                CheckRange(offset, FrameHeaderSize);
                var frame = new ShapeFrame { Offset = offset, ColorMapOffset = U32(position + 12 + (int)i * 8) };
                if (frame.ColorMapOffset != 0 && frame.ColorMapOffset >= (uint)(_bytes.Length - position))
                {
                    throw new InvalidDataException("셰이프 색 표 오프셋이 파일 범위를 벗어났습니다");
                }
                frame.Bounds[0] = U16(offset);
                frame.Bounds[1] = U16(offset + 2);
                frame.Origin[0] = U16(offset + 4);
                frame.Origin[1] = U16(offset + 6);
                frame.Rect = new ShapeRect(I32(offset + 8), I32(offset + 12), I32(offset + 16), I32(offset + 20));
                block.Frames.Add(frame);
                frameOffsets.Add(offset);
            }
            position += 8 + (int)count * 8;
            _blocks.Add(block);
        }
        if (_blocks.Count == 0)
        {
            throw new InvalidDataException("VFX 1.10 셰이프 머리가 없습니다");
        }
        _headerEnd = position;
        frameOffsets.Sort();
        // 블록 헤더와 겹치는 주소는 거부하고 공유 프레임은 같은 경계를 갖게 한다
        var unique = frameOffsets.Distinct().ToList();
        foreach (ShapeBlock block in _blocks)
        {
            // 다음 고유 레코드 전까지만 압축 스트림을 읽도록 제한한다
            foreach (ShapeFrame frame in block.Frames)
            {
                if (frame.Offset < position)
                {
                    throw new InvalidDataException("셰이프 프레임이 블록 머리와 겹칩니다");
                }
                int next = unique.Count;
                for (int i = 0; i < unique.Count; i++)
                {
                    if (unique[i] > frame.Offset)
                    {
                        next = i;
                        break;
                    }
                }
                frame.End = next == unique.Count ? _bytes.Length : unique[next];
                if (frame.End - frame.Offset < FrameHeaderSize)
                {
                    throw new InvalidDataException("셰이프 프레임이 겹칩니다");
                }
            }
        }
    }

    /// <summary>파일의 블록 순서가 곧 타입의 그래픽 순서다.</summary>
    public IReadOnlyList<ShapeBlock> Blocks() => _blocks;

    /// <summary>
    /// 런타임 frame table이 가리키는 VFX 레코드 앞의 원본 Squid 전용 헤더다.
    /// 순수 VFX·글꼴 파일에는 추가 헤더가 없으므로 거부한다.
    /// </summary>
    public SquidFrameMetrics SquidMetrics(int block, int frame)
    {
        int offset = _blocks[block].Frames[frame].Offset;
        if (offset < _headerEnd || offset - _headerEnd < SquidPrefixSize)
        {
            throw new InvalidDataException("Squid SHP 추가 헤더가 없습니다");
        }
        return new SquidFrameMetrics(
            U32Float(offset - SquidPrefixSize), U32Float(offset - SquidPrefixSize + 4),
            I16(offset - 12), I16(offset - 10), I16(offset - 8), I16(offset - 6),
            U32Float(offset - SquidPrefixSize + 8), U32Float(offset - SquidPrefixSize + 12),
            U32Float(offset - SquidPrefixSize + 16), U32Float(offset - SquidPrefixSize + 20));
    }

    /// <summary>
    /// VFX 프레임 앞의 원본 36바이트를 복사한다. 해석하지 않은 메타 필드도 그림 조각의 원본 헤더 조회에 공급한다.
    /// </summary>
    public byte[] SquidHeader(int block, int frame)
    {
        int offset = _blocks[block].Frames[frame].Offset;
        if (offset < _headerEnd || offset - _headerEnd < SquidPrefixSize)
        {
            throw new InvalidDataException("Squid SHP 추가 헤더가 없습니다");
        }
        var result = new byte[SquidPrefixSize];
        Array.Copy(_bytes, offset - SquidPrefixSize, result, 0, SquidPrefixSize);
        return result;
    }

    /// <summary>
    /// 원본의 0=행 끝, 1=투명 건너뛰기, 홀수=직접 복사, 짝수=반복 복사를 구현한다.
    /// RLE를 풀되 투명 픽셀과 팔레트 번호를 각각 보존한다.
    /// </summary>
    public IndexedImage Decode(int blockIndex, int frameIndex)
    {
        ShapeFrame frame = _blocks[blockIndex].Frames[frameIndex];
        if (frame.IsSpecial())
        {
            throw new InvalidDataException("특수 셰이프 레코드는 풀어낸 그림이 없습니다");
        }
        long width = (long)frame.Rect.Right - frame.Rect.Left + 1;
        long height = (long)frame.Rect.Bottom - frame.Rect.Top + 1;
        if (width > MaxDecodedPixels || height > MaxDecodedPixels || width * height > MaxDecodedPixels)
        {
            throw new InvalidDataException("셰이프 그림이 풀어내기 상한을 넘었습니다");
        }
        var image = new IndexedImage { Width = (int)width, Height = (int)height };
        image.Indices = new byte[(long)image.Width * image.Height];
        image.Opacity = new byte[image.Indices.Length];
        int position = frame.Offset + FrameHeaderSize;
        // 헤더가 선언한 모든 행을 끝 토큰까지 해석한다
        for (int row = 0; row < image.Height; row++)
        {
            int column = 0;
            // 픽셀 폭을 채웠더라도 행 끝 0은 반드시 소비한다
            while (true)
            {
                if (position >= frame.End)
                {
                    throw new InvalidDataException("셰이프 행 끝 표시가 없습니다");
                }
                byte token = _bytes[position++];
                if (token == 0)
                {
                    break;
                }
                if (position >= frame.End)
                {
                    throw new InvalidDataException("셰이프 연속 구간이 잘렸습니다");
                }
                int count = token == 1 ? _bytes[position++] : token >> 1;
                if (count > image.Width - column)
                {
                    throw new InvalidDataException("셰이프 연속 구간이 행 폭을 넘었습니다");
                }
                if (token != 1)
                {
                    int inputSize = (token & 1) != 0 ? count : 1;
                    if (inputSize > frame.End - position)
                    {
                        throw new InvalidDataException("셰이프 연속 픽셀이 잘렸습니다");
                    }
                    // 팔레트 번호 0을 포함하여 실제로 기록된 픽셀에만 불투명 마스크를 켠다
                    for (int i = 0; i < count; i++)
                    {
                        int output = row * image.Width + column + i;
                        image.Indices[output] = _bytes[position + ((token & 1) != 0 ? i : 0)];
                        image.Opacity[output] = 255;
                    }
                    position += inputSize;
                }
                column += count;
            }
        }
        return image;
    }

    /// <summary>덧셈 오버플로 없이 파일 범위를 확인한다.</summary>
    private void CheckRange(int offset, int size)
    {
        if (offset < 0 || offset > _bytes.Length || size > _bytes.Length - offset)
        {
            throw new InvalidDataException("셰이프 파일이 잘렸습니다");
        }
    }

    /// <summary>정렬과 호스트 엔디언에 의존하지 않고 16비트를 읽는다.</summary>
    private ushort U16(int offset)
    {
        CheckRange(offset, 2);
        return (ushort)(_bytes[offset] | (_bytes[offset + 1] << 8));
    }

    /// <summary>정렬과 호스트 엔디언에 의존하지 않고 32비트를 읽는다.</summary>
    private uint U32(int offset)
    {
        CheckRange(offset, 4);
        uint value = 0;
        // 낮은 바이트부터 4바이트를 모은다
        for (int i = 0; i < 4; i++) value |= (uint)_bytes[offset + i] << (i * 8);
        return value;
    }

    /// <summary>좌표의 32비트 부호를 유지한다.</summary>
    private int I32(int offset) => unchecked((int)U32(offset));

    /// <summary>비트 패턴을 float로 읽는다.</summary>
    private float U32Float(int offset) => BitConverter.UInt32BitsToSingle(U32(offset));

    /// <summary>부호 있는 16비트를 읽는다.</summary>
    private short I16(int offset) => unchecked((short)U16(offset));
}

/// <summary>
/// 원본 pane의 원점과 양 끝 포함 사각형을 유지하며 8비트 화면에 합성한다.
/// 근거: cpppj/src/client/VFXDraw.cpp 의 DrawShape (원본 00401d92). (R2b, 2026-10-28)
/// </summary>
public static class VfxDraw
{
    /// <summary>
    /// 셰이프 프레임을 8비트 화면에 그린다. 불투명 픽셀만 써서 투명 영역의 기존 화면 값을 유지한다.
    /// 반환: 0 성공, -1 잘못된 화면 크기, -2 빈 pane, -3 화면 밖, -4 역전된 프레임 영역.
    /// </summary>
    /// <param name="destination">그리기 화면.</param>
    /// <param name="width">화면 폭.</param>
    /// <param name="height">화면 높이.</param>
    /// <param name="database">셰이프 저장소.</param>
    /// <param name="block">블록 번호.</param>
    /// <param name="frame">프레임 번호.</param>
    /// <param name="x">그리기 기준 x.</param>
    /// <param name="y">그리기 기준 y.</param>
    /// <param name="pane">원본 pane(양 끝 포함).</param>
    public static int DrawShape(Span<byte> destination, int width, int height, ShapeDatabase database,
        int block, int frame, int x, int y, ShapeRect pane)
    {
        ArgumentNullException.ThrowIfNull(database);
        if (width <= 0 || height <= 0)
        {
            return -1;
        }
        if ((long)width * height > destination.Length)
        {
            throw new ArgumentException("셰이프 그리기 화면이 작습니다", nameof(destination));
        }
        int left = Math.Max(pane.Left, 0);
        int top = Math.Max(pane.Top, 0);
        int right = Math.Min(pane.Right, width - 1);
        int bottom = Math.Min(pane.Bottom, height - 1);
        if (right < left || bottom < top)
        {
            return -2;
        }
        if (block < 0 || block >= database.Blocks().Count)
        {
            throw new ArgumentOutOfRangeException(nameof(block), block, "셰이프 블록 범위 오류");
        }
        ShapeFrame record = database.Blocks()[block].Frames[frame];
        if (record.Rect.Right < record.Rect.Left || record.Rect.Bottom < record.Rect.Top)
        {
            return -4;
        }
        long originX = (long)x + pane.Left;
        long originY = (long)y + pane.Top;
        long frameLeft = originX + record.Rect.Left;
        long frameTop = originY + record.Rect.Top;
        long frameRight = originX + record.Rect.Right;
        long frameBottom = originY + record.Rect.Bottom;
        if (frameRight < left || frameBottom < top || frameLeft > right || frameTop > bottom)
        {
            return -3;
        }
        IndexedImage image = database.Decode(block, frame);
        // 화면과 pane에 모두 들어가는 행만 처리한다
        for (long row = Math.Max(frameTop, top); row <= Math.Min(frameBottom, bottom); row++)
        {
            for (long column = Math.Max(frameLeft, left); column <= Math.Min(frameRight, right); column++)
            {
                int input = (int)(row - frameTop) * image.Width + (int)(column - frameLeft);
                if (image.Opacity[input] != 0)
                {
                    destination[(int)row * width + (int)column] = image.Indices[input];
                }
            }
        }
        return 0;
    }
}
