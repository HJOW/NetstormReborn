namespace Netstorm.Assets;

/// <summary>
/// 배경 GIF(타이틀·구름)를 **색 번호 그대로** 해독한다. 결과의 <see cref="IndexedImage.Indices"/> 는 GIF 의 8비트 색 번호이고,
/// 색은 호출하는 쪽이 게임 팔레트(<see cref="Palette"/>)로 칠한다. GIF 안의 RGB 색 표는 읽지 않고 건너뛴다.
/// 사용법: <c>IndexedImage image = GifImage.Decode(bytes);</c> 뒤에 팔레트를 적용해 텍스처를 만든다.
/// 첫 이미지 하나만 읽으며(게임 배경에는 애니메이션 GIF 가 없다) 인터레이스·투명 번호·LZW 코드 폭 증가를 처리한다.
/// 손상된 파일은 <see cref="InvalidDataException"/> 으로 거부한다.
/// </summary>
/// <remarks>
/// 원본 10.78 의 배경 읽기(00419ec0 → 004dae60)는 GIF 의 색 번호를 화면에 그대로 복사하고 GIF 의 RGB 표로 색을 다시 맞추지 않는다
/// (docs/exe/cpp-menu-reconstruction.md. 기준 구현은 cpppj/src/client/GifImage.cpp).
/// 2026-10-10 추가: d/titleMenu.gif 는 쓰는 색 번호 181개 중 177개의 내장 RGB 가 게임 팔레트(GIFCLOUD.COL)와 달라
/// (채널당 최대 12) 내장 표로 칠하면 원본 화면과 색이 달라지기 때문이다 (LEFT_JOBS.dotnetpj.md 5-5).
/// </remarks>
public static class GifImage
{
    /// <summary>손상된 파일이 요구하는 무제한 할당을 막는 픽셀 수 한도 (원본 자료는 최대 640×480 이다).</summary>
    private const int MaxPixels = 16 * 1024 * 1024;

    /// <summary>LZW 사전의 최대 항목 수 (코드 폭 12비트).</summary>
    private const int DictionarySize = 4096;

    /// <summary>"이전 코드 없음"을 뜻하는 값 (사전 범위 밖).</summary>
    private const int NoCode = DictionarySize;

    /// <summary>인터레이스 네 패스의 시작 행.</summary>
    private static readonly int[] InterlaceStarts = [0, 4, 2, 1];

    /// <summary>인터레이스 네 패스의 행 간격.</summary>
    private static readonly int[] InterlaceSteps = [8, 8, 4, 2];

    /// <summary>
    /// GIF 파일 내용을 논리 화면 크기의 색 번호 이미지로 해독한다. 이미지 사각형 밖은 배경 번호로 채우고,
    /// 투명 번호가 지정된 파일에서는 그 번호인 픽셀(과 사각형 밖)이 불투명하지 않다.
    /// </summary>
    /// <param name="bytes">GIF 파일 전체 내용 (GIF87a 또는 GIF89a)</param>
    /// <returns>논리 화면 폭·높이의 색 번호와 불투명 여부</returns>
    public static IndexedImage Decode(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length < 13 || !(bytes[..6].SequenceEqual("GIF87a"u8) || bytes[..6].SequenceEqual("GIF89a"u8)))
        {
            throw new InvalidDataException("GIF 서명이 아닙니다.");
        }
        var reader = new Reader(bytes) { Position = 6 };
        int width = reader.Word();
        int height = reader.Word();
        int packed = reader.Byte();
        int background = reader.Byte();
        reader.Byte();
        if (width == 0 || height == 0 || (long)width * height > MaxPixels)
        {
            throw new InvalidDataException("GIF 크기가 범위를 벗어났습니다.");
        }
        if ((packed & 0x80) != 0)
        {
            reader.Skip(3 << ((packed & 7) + 1));
        }
        int transparent = -1;
        // 첫 이미지 설명자가 나올 때까지 확장 블록을 넘긴다. 그래픽 제어 확장의 투명 번호만 기억한다.
        while (true)
        {
            int marker = reader.Byte();
            if (marker == 0x21)
            {
                int kind = reader.Byte();
                byte[] data = reader.Blocks();
                if (kind == 0xF9 && data.Length == 4 && (data[0] & 1) != 0)
                {
                    transparent = data[3];
                }
                continue;
            }
            if (marker != 0x2C)
            {
                throw new InvalidDataException("GIF 이미지가 없습니다.");
            }
            int left = reader.Word();
            int top = reader.Word();
            int imageWidth = reader.Word();
            int imageHeight = reader.Word();
            int flags = reader.Byte();
            if (imageWidth == 0 || imageHeight == 0 || left + imageWidth > width || top + imageHeight > height)
            {
                throw new InvalidDataException("GIF 이미지 사각형이 화면을 벗어났습니다.");
            }
            if ((flags & 0x80) != 0)
            {
                reader.Skip(3 << ((flags & 7) + 1));
            }
            int minimum = reader.Byte();
            byte[] compressed = reader.Blocks();
            byte[] decoded = Lzw(compressed, minimum, imageWidth * imageHeight);
            var indices = new byte[width * height];
            var opaque = new bool[width * height];
            Array.Fill(indices, (byte)background);
            Array.Fill(opaque, transparent < 0);
            int source = 0;
            if ((flags & 0x40) != 0)
            {
                // 인터레이스 네 패스를 순서대로 실제 행에 놓는다.
                for (int pass = 0; pass < InterlaceStarts.Length; pass++)
                {
                    // 이번 패스에 속한 행.
                    for (int y = InterlaceStarts[pass]; y < imageHeight; y += InterlaceSteps[pass])
                    {
                        source = CopyRow(decoded, source, indices, opaque, (top + y) * width + left, imageWidth, transparent);
                    }
                }
            }
            else
            {
                // 일반 GIF 는 위에서 아래로 이어진다.
                for (int y = 0; y < imageHeight; y++)
                {
                    source = CopyRow(decoded, source, indices, opaque, (top + y) * width + left, imageWidth, transparent);
                }
            }
            return new IndexedImage(width, height, indices, opaque);
        }
    }

    /// <summary>해독된 한 행을 논리 화면에 옮기고 다음 행의 원본 위치를 돌려준다.</summary>
    /// <param name="decoded">LZW 로 푼 색 번호 (이미지 사각형, 저장 순서)</param>
    /// <param name="source">이 행의 원본 시작 위치</param>
    /// <param name="indices">논리 화면의 색 번호</param>
    /// <param name="opaque">논리 화면의 불투명 여부</param>
    /// <param name="target">이 행의 논리 화면 시작 위치</param>
    /// <param name="count">행의 픽셀 수</param>
    /// <param name="transparent">투명 색 번호 (없으면 -1)</param>
    private static int CopyRow(byte[] decoded, int source, byte[] indices, bool[] opaque, int target, int count, int transparent)
    {
        // 이미지 사각형 안의 픽셀만 채운다.
        for (int x = 0; x < count; x++)
        {
            byte color = decoded[source++];
            indices[target + x] = color;
            opaque[target + x] = color != transparent;
        }
        return source;
    }

    /// <summary>
    /// 낮은 비트부터 읽는 GIF LZW 를 푼다. 사전 재설정(clear)·코드 폭 증가·자기 참조 코드를 처리하고,
    /// 종료 코드에서 픽셀 수가 정확히 맞아야 한다.
    /// </summary>
    /// <param name="bytes">서브 블록을 이어 붙인 압축 자료</param>
    /// <param name="minimum">최소 코드 크기 (2~8)</param>
    /// <param name="pixels">나와야 하는 픽셀 수</param>
    private static byte[] Lzw(byte[] bytes, int minimum, int pixels)
    {
        if (minimum < 2 || minimum > 8)
        {
            throw new InvalidDataException("GIF LZW 코드 크기가 잘못되었습니다.");
        }
        int clear = 1 << minimum;
        int end = clear + 1;
        var prefix = new int[DictionarySize];
        var suffix = new byte[DictionarySize];
        var stack = new byte[DictionarySize];
        int next = end + 1;
        int codeWidth = minimum + 1;
        int previous = NoCode;
        int first = 0;
        long bit = 0;
        long bitCount = (long)bytes.Length * 8;
        var output = new byte[pixels];
        int written = 0;
        // 종료 코드까지 코드를 하나씩 읽는다. 예상 픽셀 수를 넘는 출력은 거부한다.
        while (bit + codeWidth <= bitCount)
        {
            int code = 0;
            // 가변 길이 코드를 낮은 비트부터 모은다.
            for (int i = 0; i < codeWidth; i++, bit++)
            {
                code |= ((bytes[bit >> 3] >> (int)(bit & 7)) & 1) << i;
            }
            if (code == clear)
            {
                next = end + 1;
                codeWidth = minimum + 1;
                previous = NoCode;
                continue;
            }
            if (code == end)
            {
                if (written != pixels)
                {
                    throw new InvalidDataException("GIF 픽셀 수가 맞지 않습니다.");
                }
                return output;
            }
            int input = code;
            int count = 0;
            if (code == next && previous != NoCode)
            {
                stack[count++] = (byte)first;
                code = previous;
            }
            else if (code >= next)
            {
                throw new InvalidDataException("GIF LZW 코드가 잘못되었습니다.");
            }
            // 접두 사전을 거꾸로 따라가며 순환·범위 밖 참조를 막는다.
            while (code >= clear)
            {
                if (code <= end || code >= next || count >= stack.Length - 1)
                {
                    throw new InvalidDataException("GIF LZW 사전이 손상되었습니다.");
                }
                stack[count++] = suffix[code];
                code = prefix[code];
            }
            first = code;
            stack[count++] = (byte)code;
            if (count > pixels - written)
            {
                throw new InvalidDataException("GIF 출력이 이미지 크기를 넘습니다.");
            }
            // 거꾸로 쌓인 바이트를 실제 픽셀 순서로 적는다.
            while (count != 0)
            {
                output[written++] = stack[--count];
            }
            if (previous != NoCode && next < DictionarySize)
            {
                prefix[next] = previous;
                suffix[next] = (byte)first;
                next++;
                if (next == 1 << codeWidth && codeWidth < 12)
                {
                    codeWidth++;
                }
            }
            previous = input;
        }
        throw new InvalidDataException("GIF LZW 종료 코드가 없습니다.");
    }

    /// <summary>파일 경계를 확인하며 GIF 의 바이트·16비트 정수·서브 블록을 읽는 커서.</summary>
    private ref struct Reader
    {
        /// <summary>읽는 파일 내용.</summary>
        private readonly ReadOnlySpan<byte> _bytes;

        /// <summary>다음에 읽을 위치.</summary>
        public int Position;

        /// <summary>파일 내용의 처음부터 읽는 커서를 만든다.</summary>
        /// <param name="bytes">GIF 파일 전체 내용</param>
        public Reader(ReadOnlySpan<byte> bytes)
        {
            _bytes = bytes;
            Position = 0;
        }

        /// <summary>다음 바이트를 읽는다. 파일 끝을 넘으면 거부한다.</summary>
        public int Byte()
        {
            if (Position >= _bytes.Length)
            {
                throw new InvalidDataException("GIF 가 잘렸습니다.");
            }
            return _bytes[Position++];
        }

        /// <summary>리틀 엔디언 16비트 정수를 읽는다.</summary>
        public int Word()
        {
            int low = Byte();
            return low | Byte() << 8;
        }

        /// <summary>쓰지 않는 바이트(색 표 등)를 경계 확인과 함께 건너뛴다.</summary>
        /// <param name="count">건너뛸 바이트 수</param>
        public void Skip(int count)
        {
            if (count > _bytes.Length - Position)
            {
                throw new InvalidDataException("GIF 색 표가 잘렸습니다.");
            }
            Position += count;
        }

        /// <summary>길이 바이트가 0 인 끝 표시까지 서브 블록을 이어 붙여 돌려준다.</summary>
        public byte[] Blocks()
        {
            using var result = new MemoryStream();
            // 이미지 자료와 확장 자료가 함께 쓰는 "길이 + 내용" 반복.
            for (int size = Byte(); size != 0; size = Byte())
            {
                if (size > _bytes.Length - Position)
                {
                    throw new InvalidDataException("GIF 서브 블록이 잘렸습니다.");
                }
                result.Write(_bytes.Slice(Position, size));
                Position += size;
            }
            return result.ToArray();
        }
    }
}
