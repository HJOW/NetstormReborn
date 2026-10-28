namespace Netstorm.Client.Display;

/// <summary>
/// 원본 Image가 소비하는 행 우선 비트맵 입력(폭·높이·픽셀).
/// 근거: cpppj/src/o/RawTerrainBitmap.h 의 TerrainBitmapImage. R4에서 공용 지형 비트맵으로 재배선할 때까지 이 파일이 소유한다. (R2d-1, 2026-10-28)
/// </summary>
/// <param name="Width">비트맵 폭.</param>
/// <param name="Height">비트맵 높이.</param>
/// <param name="Pixels">행 우선 8비트 픽셀. 투명 0과 불투명 0아님을 구별한다.</param>
public sealed class TerrainBitmapInput(int width, int height, byte[] pixels)
{
    /// <summary>비트맵 폭. crop 전후 원본 +8의 값이다.</summary>
    public int Width { get; } = width;

    /// <summary>비트맵 높이. crop 전후 원본 +c의 값이다.</summary>
    public int Height { get; } = height;

    /// <summary>행 우선 8비트 픽셀. 변환 성공 뒤에는 소비되어 비워진다.</summary>
    public byte[] Pixels { get; set; } = pixels;
}

/// <summary>
/// 원본 Image의 bitmap 소비·VFX 변환·깊은 복사·소멸 수명을 복원한다.
/// 근거: cpppj/src/client/RawImage.cpp (원본 0046d790·0046d900·0046d8a0, 004042cd 동적 이미지 경로). (R2d-1, 2026-10-28)
/// 동적 이미지는 자산 TYPE 그림과 별도 수명으로 소유한다.
/// </summary>
public sealed class RawImage
{
    /// <summary>변환된 VFX 1.10 블록 매직.</summary>
    private const uint ShapeMagic = 0x30312e31;

    /// <summary>원본 +0 bitmap의 독립 소유 버퍼. 변환 성공 뒤에는 소비되어 비워진다.</summary>
    private byte[] _bits = [];

    /// <summary>원본 +4 shape의 독립 소유 버퍼. 1.10 단일 프레임 헤더 40바이트와 RLE 전체다.</summary>
    private byte[] _shape = [];

    /// <summary>원본 +8의 crop 전후 폭.</summary>
    private int _width;

    /// <summary>원본 +c의 crop 전후 높이.</summary>
    private int _height;

    /// <summary>원본 +10의 마지막 shape 바이트 수. Destroy가 쓰지 않으므로 소멸 뒤에도 남는다.</summary>
    private uint _shapeSize;

    /// <summary>
    /// 입력을 소비하고 오른쪽/아래 여백만 줄여 실제 VFX shape를 만든다.
    /// 전부 0이면 원문과 같은 최소 1x1 크기를 남긴다. 왼쪽·위쪽 여백은 자르지 않는다.
    /// </summary>
    /// <param name="bitmap">소비할 행 우선 비트맵.</param>
    /// <param name="crop">참이면 오른쪽/아래의 마지막 0아님까지만 남긴다.</param>
    public void Convert(TerrainBitmapInput bitmap, bool crop)
    {
        ArgumentNullException.ThrowIfNull(bitmap);
        if (_bits.Length != 0)
        {
            throw new InvalidOperationException("Image imageBits 원문 진단: 이미 변환된 그림이 있습니다");
        }
        if (bitmap.Width <= 0 || bitmap.Height <= 0 ||
            (long)bitmap.Width * bitmap.Height != bitmap.Pixels.Length)
        {
            throw new ArgumentException("Image bitmap 크기 오류", nameof(bitmap));
        }
        int stride = bitmap.Width;
        _bits = bitmap.Pixels;
        bitmap.Pixels = [];
        _width = bitmap.Width;
        _height = bitmap.Height;
        if (crop)
        {
            int lastX = 0;
            int lastY = 0;
            // 각 행의 마지막 0아님이 crop 경계를 갱신한다
            for (int y = 0; y < _height; y++)
            {
                // 왼쪽과 위쪽 여백은 자르지 않는다
                for (int x = 0; x < _width; x++)
                {
                    if (_bits[(long)y * stride + x] != 0)
                    {
                        lastX = Math.Max(lastX, x);
                        lastY = y;
                    }
                }
            }
            _width = lastX + 1;
            _height = lastY + 1;
        }
        _shape = EncodeImage(_bits, stride, _width, _height);
        _shapeSize = (uint)_shape.Length;
        _bits = [];
    }

    /// <summary>
    /// 먼저 현재 버퍼를 파괴한 뒤 source를 깊게 복사한다. 자기 복사도 이 순서다(원본 0046d900).
    /// </summary>
    /// <param name="source">복사할 원본 그림.</param>
    public void CopyFrom(RawImage source)
    {
        ArgumentNullException.ThrowIfNull(source);
        Destroy();
        _width = source._width;
        _height = source._height;
        if (source._bits.Length != 0)
        {
            _bits = (byte[])source._bits.Clone();
        }
        if (source._shape.Length != 0)
        {
            _shapeSize = source._shapeSize;
            if (_shapeSize != 0)
            {
                _shape = (byte[])source._shape.Clone();
            }
        }
    }

    /// <summary>
    /// 두 버퍼/폭/높이를 지우며 shapeSize는 원문대로 보존한다. 중복 호출이 가능하다(원본 0046d8a0).
    /// </summary>
    public void Destroy()
    {
        _bits = [];
        _shape = [];
        _height = 0;
        _width = 0;
    }

    /// <summary>변환 뒤 crop 여부가 확정한 실제 폭이다.</summary>
    public int Width() => _width;

    /// <summary>변환 뒤 crop 여부가 확정한 실제 높이이다.</summary>
    public int Height() => _height;

    /// <summary>마지막 shape 바이트 수이며 소멸 후에도 남는다.</summary>
    public uint ShapeSize() => _shapeSize;

    /// <summary>원본 1.10 블록/프레임/RLE 전체를 빌린다.</summary>
    public ReadOnlySpan<byte> Shape() => _shape;

    /// <summary>변환 성공 후에는 소비되어 빈 비트맵 버퍼다.</summary>
    public ReadOnlySpan<byte> Bits() => _bits;

    /// <summary>
    /// 원본 16비트 좌표 필드를 낮은 바이트부터 쓴다.
    /// </summary>
    /// <param name="output">쓸 shape 버퍼.</param>
    /// <param name="offset">쓸 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void ImageWord(List<byte> output, int offset, ushort value)
    {
        output[offset] = (byte)value;
        output[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>
    /// 원문 DWORD의 감김도 부호 넘침 없이 같은 비트로 기록한다. 비정렬 헤더도 같은 리틀 엔디언이다.
    /// </summary>
    /// <param name="output">쓸 shape 버퍼.</param>
    /// <param name="offset">쓸 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void ImageDword(List<byte> output, int offset, uint value)
    {
        // 원본 low byte부터 네 바이트를 조합한다
        for (int i = 0; i < 4; i++)
        {
            output[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>
    /// 행 시작 pair와 literal 내부 triple을 구별하고 끝의 투명 run은 버린다(원본 0040463b/004047ab).
    /// </summary>
    /// <param name="output">쌓을 shape 버퍼.</param>
    /// <param name="row">한 행의 source 픽셀.</param>
    private static void ImageRow(List<byte> output, ReadOnlySpan<byte> row)
    {
        int cursor = 0;
        int pending = 0;
        // 한 행의 모든 source 픽셀을 원문 선택 순서로 소비한다
        while (cursor < row.Length)
        {
            if (row[cursor] == 0)
            {
                cursor++;
                pending++;
                continue;
            }
            // 다음 불투명 run이 있을 때만 255 단위 skip 토큰을 쓴다
            while (pending != 0)
            {
                int skip = Math.Min(pending, 255);
                output.Add(1);
                output.Add((byte)skip);
                pending -= skip;
            }
            int end = cursor + 1;
            // 시작 위치의 같은 색을 먼저 확인한다
            while (end < row.Length && row[end] == row[cursor])
            {
                end++;
            }
            // 첫 pair는 길이 2부터 repeat로 선택한다
            if (end - cursor >= 2)
            {
                // repeat는 127픽셀마다 같은 색을 다시 쓴다
                while (cursor < end)
                {
                    int count = Math.Min(end - cursor, 127);
                    output.Add((byte)(count * 2));
                    output.Add(row[cursor]);
                    cursor += count;
                }
                continue;
            }
            int start = cursor++;
            // 단일 색은 literal의 시작이며 중간 pair는 literal에 남긴다
            while (cursor < row.Length && row[cursor] != 0)
            {
                // 세 번 같은 색이 나오는 위치 직전까지 literal을 모은다
                if (cursor + 2 < row.Length && row[cursor] == row[cursor + 1] && row[cursor] == row[cursor + 2])
                {
                    break;
                }
                cursor++;
            }
            int first = start;
            // literal도 127단위로 나누되 payload의 255/0 의미를 바꾸지 않는다
            while (first < cursor)
            {
                int count = Math.Min(cursor - first, 127);
                output.Add((byte)(count * 2 + 1));
                for (int i = 0; i < count; i++)
                {
                    output.Add(row[first + i]);
                }
                first += count;
            }
        }
        output.Add(0);
    }

    /// <summary>
    /// 실제 0아님 사각형만 인코딩하되 상자/기준점은 crop 크기를 보존한다(원본 004dad20→004042cd).
    /// </summary>
    /// <param name="bits">행 우선 비트맵(투명 0·불투명 0아님).</param>
    /// <param name="stride">비트맵 한 행의 바이트 간격.</param>
    /// <param name="width">인코딩할 폭.</param>
    /// <param name="height">인코딩할 높이.</param>
    private static byte[] EncodeImage(byte[] bits, int stride, int width, int height)
    {
        int left = int.MaxValue;
        int top = left;
        int right = -left;
        int bottom = -left;
        // crop 영역의 실제 불투명 경계를 먼저 찾는다
        for (int y = 0; y < height; y++)
        {
            // 투명 색 0과 불투명 0아님을 구별한다
            for (int x = 0; x < width; x++)
            {
                if (bits[(long)y * stride + x] != 0)
                {
                    left = Math.Min(left, x);
                    right = Math.Max(right, x);
                    top = Math.Min(top, y);
                    bottom = Math.Max(bottom, y);
                }
            }
        }
        var result = new List<byte>(new byte[40]);
        ImageDword(result, 0, ShapeMagic);
        ImageDword(result, 4, 1);
        ImageDword(result, 8, 16);
        ImageWord(result, 16, (ushort)(height - 1));
        ImageWord(result, 18, (ushort)(width - 1));
        ImageWord(result, 20, (ushort)(height - 1));
        ImageWord(result, 22, (ushort)(width - 1));
        ImageDword(result, 24, unchecked((uint)left - (uint)(width - 1)));
        ImageDword(result, 28, unchecked((uint)top - (uint)(height - 1)));
        ImageDword(result, 32, unchecked((uint)right - (uint)(width - 1)));
        ImageDword(result, 36, unchecked((uint)bottom - (uint)(height - 1)));
        // 빈 sentinel은 반복을 시작하지 않으며 40바이트 shape를 그대로 유지한다
        for (int y = top; y <= bottom; y++)
        {
            int first = y * stride + left;
            ImageRow(result, new ReadOnlySpan<byte>(bits, first, right - left + 1));
        }
        return result.ToArray();
    }
}
