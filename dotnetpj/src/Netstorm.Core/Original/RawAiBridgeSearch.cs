// 원본 AI 다리의 실제 모양·점유·끝점 평가와 호출당 8개 후보 검색을 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 다리 검색 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.

// 0040f880/0040f910이 읽는 배치 후보 16BYTE의 호스트 표현이다.
public struct AiBridgePlacement
{
    /// <summary>첫 칸의 float XY이며 별도 반올림하지 않는다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    public RawPathPoint Point;

    /// <summary>현재 8개 재고에서 읽은 전체 DWORD 패턴 번호다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    public uint Pattern;

    /// <summary>원본 0/2/4/6 회전 값이다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    public uint Direction;

    /// <summary>배치 후보를 묶는다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="point">첫 칸의 좌표다.</param>
    /// <param name="pattern">패턴 번호다.</param>
    /// <param name="direction">회전 값이다.</param>
    public AiBridgePlacement(RawPathPoint point, uint pattern, uint direction)
    {
        Point = point;
        Pattern = pattern;
        Direction = direction;
    }
}

// 0040f400/0040f880/0040f910/004144b0의 실제 계산을 같은 본체·프레임·spot·난수에 적용한다.
// 근거: cpppj o/RawAiBridgeSearch.h/.cpp의 RawAiBridgeSearch다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.
public sealed class RawAiBridgeSearch
{
    /// <summary>8×8 BYTE grid+10개 12BYTE 끝점+DWORD count다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    public const int ShapeBytes = 0xbc;

    /// <summary>0052f83c/0052f85c의 원본 이웃 좌표다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    private static readonly int[] Dx = [0, 1, 1, 1, 0, -1, -1, -1];

    /// <summary>0052f83c/0052f85c의 원본 이웃 좌표다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    private static readonly int[] Dy = [-1, -1, 0, 1, 1, 1, 0, -1];

    /// <summary>0052f8bc의 반대 방향이며 -1 조회는 바로 앞의 float -1 비트도 실제로 읽는다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    private static readonly int[] Opposite = [-1082130432, 4, 5, 6, 7, 0, 1, 2, 3];

    /// <summary>같은 실제 Brain의 유일한 바이트 저장소다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>현재 TYPE 순서의 실제 코드 표다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>현재 65536BYTE 점유 지도다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<byte> _spots;

    /// <summary>현재 bridge 전역이다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    private readonly Func<uint>? _bridge;

    /// <summary>00532710과 같은 게임 전역 난수 상태다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    private readonly GameRandom _random;

    /// <summary>현재 bridge 전역/프레임과 공용 난수를 빌린다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 실제 Brain의 바이트 저장소다.</param>
    /// <param name="frames">현재 TYPE 순서의 코드 표다.</param>
    /// <param name="spots">현재 65536BYTE 점유 지도다.</param>
    /// <param name="bridge">현재 bridge 전역이다.</param>
    /// <param name="random">같은 게임 전역 난수 상태다.</param>
    public RawAiBridgeSearch(RawAiBrainMemory memory, IReadOnlyList<RiftTypeFrames> frames, IReadOnlyList<byte> spots, Func<uint>? bridge, GameRandom random)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(random);
        if (spots.Count != 65536)
        {
            throw new ArgumentException("AI 다리 검색 실제 spot65536 누락");
        }

        _memory = memory;
        _frames = frames;
        _spots = spots;
        _bridge = bridge;
        _random = random;
    }

    /// <summary>비정렬 DWORD의 주변 바이트를 읽지 않는다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트 범위다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int at)
    {
        if (at < 0 || at > raw.Length || raw.Length - at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 다리 검색 DWORD 읽기 범위");
        }

        // little endian 네 BYTE를 조합하는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(at, 4));
    }

    /// <summary>지정 네 BYTE만 갱신한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Write(Span<byte> raw, int at, uint value)
    {
        if (at < 0 || at > raw.Length || raw.Length - at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 다리 검색 DWORD 쓰기 범위");
        }

        // 원본 순서대로 낮은 바이트부터 쓰는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(at, 4), value);
    }

    /// <summary>원문 DWORD의 signed 비교를 보존한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원본 __ftol signed64 절삭의 낮은 DWORD이며 indefinite 하위는 0이다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭한 낮은 DWORD다.</returns>
    private static uint Truncate(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((uint)(long)value);
    }

    /// <summary>INT_MIN도 원본 xor/sub의 32비트 감김으로 처리한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="value">절댓값을 구할 DWORD다.</param>
    /// <returns>음수가 아닌 signed 값이다.</returns>
    private static int Absolute(uint value) => Signed(Signed(value) < 0 ? unchecked(0u - value) : value);

    /// <summary>전체 방향 번호를 검사한 뒤 원본 이웃 표를 조회한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="value">검사할 방향 번호다.</param>
    /// <returns>이웃 표 번호다.</returns>
    private static int Direction(uint value)
    {
        if (value >= 8)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 다리 검색 방향 표 범위");
        }

        return (int)value;
    }

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <returns>현재 bridge 전역이다.</returns>
    private uint Bridge() => _bridge is null
        ? throw new InvalidOperationException("AI 다리 검색 실제 전역 미연결")
        : _bridge();

    /// <summary>매 호출마다 현재 소유 블록을 다시 조회한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 다리 검색 실제0xb08 본체 누락");
        }

        return raw;
    }

    /// <summary>원본 생성자의 bridge 경로이며 패턴 번호에 임의 마스크를 추가하지 않는다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="placement">부를 배치 후보다.</param>
    /// <returns>같은 모양의 디코더다.</returns>
    private CanonDecoder Decoder(AiBridgePlacement placement)
    {
        uint bridge = Bridge();
        ReadOnlySpan<CanonPattern> patterns = CanonDecoder.BridgePatterns();
        if (bridge >= (uint)_frames.Count || placement.Pattern >= (uint)patterns.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(placement), "AI 다리 검색 현재 TYPE/패턴 범위");
        }

        return new CanonDecoder(_frames[(int)bridge], patterns[(int)placement.Pattern], Signed(placement.Direction), placement.Point.X, placement.Point.Y);
    }

    /// <summary>원본 0040f400은 caller가 이미 비운 64BYTE grid를 다시 비우지 않는다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="pattern">부를 패턴 번호다.</param>
    /// <param name="direction">부를 회전 값이다.</param>
    /// <param name="raw">쓸 0xbc 모양 버퍼다.</param>
    public void Shape(uint pattern, uint direction, Span<byte> raw)
    {
        if (raw.Length != ShapeBytes)
        {
            throw new ArgumentException("AI 다리 모양 실제0xbc 크기", nameof(raw));
        }

        CanonDecoder decoder = Decoder(new AiBridgePlacement(new RawPathPoint(1, 1), pattern, direction));
        var cells = new List<(int X, int Y)>();
        // 실제 decoder가 내는 칸 순서를 끝점 검사에도 유지하는 반복문이다.
        for (; decoder.Valid(); decoder.Advance())
        {
            int x = Signed(Truncate(decoder.X()));
            int y = Signed(Truncate(decoder.Y()));
            if (x < 1 || x > 6 || y < 1 || y > 6)
            {
                throw new ArgumentOutOfRangeException(nameof(raw), "AI 다리 모양1..6 좌표");
            }

            raw[x * 8 + y] = (byte)decoder.Side();
            cells.Add((x, y));
        }

        Write(raw, 0xb8, 0);
        // 같은 칸의 북→남 또는 동→서 순서로만 열린 끝점을 기록하는 반복문이다.
        foreach ((int cx, int cy) in cells)
        {
            byte side = raw[cx * 8 + cy];
            if (side != (byte)'J' && side != (byte)'K')
            {
                continue;
            }

            // J는 수직/K는 수평 두 끝만 검사한다.
            foreach (int edge in side == (byte)'J' ? new int[] { 0, 4 } : new int[] { 2, 6 })
            {
                if (raw[(cx + Dx[edge]) * 8 + cy + Dy[edge]] != 0)
                {
                    continue;
                }

                uint count = Read(raw, 0xb8);
                int offset = 0x40 + (int)count * 12;
                Write(raw, 0xb8, count + 1);
                Write(raw, offset, (uint)cx);
                Write(raw, offset + 4, (uint)cy);
                Write(raw, offset + 8, (uint)edge);
            }
        }
    }

    /// <summary>원본 0040f880은 y 절삭→x 절삭→flat spot 조회 순서다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="placement">검사할 배치 후보다.</param>
    /// <returns>모두 비어 있으면 참이다.</returns>
    public bool Clear(AiBridgePlacement placement)
    {
        CanonDecoder decoder = Decoder(placement);
        // 첫 점유 칸에서 즉시 거짓이며 빈 decoder는 참인 반복문이다.
        for (; decoder.Valid(); decoder.Advance())
        {
            uint y = Truncate(decoder.Y());
            uint x = Truncate(decoder.X());
            uint index = unchecked(y * 256u + x);
            if (index >= (uint)_spots.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(placement), "AI 다리 검색 spot 물리 범위");
            }

            if (_spots[(int)index] != 0)
            {
                return false;
            }
        }

        return true;
    }

    /// <summary>원본 0040f910은 X 절대점은 x87에 유지하고 Y 절대점만 float에 먼저 저장한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="placement">평가할 배치 후보다.</param>
    /// <param name="targetX">목표 가로 칸이다.</param>
    /// <param name="targetY">목표 세로 칸이다.</param>
    /// <param name="shape">끝점 모양 버퍼다.</param>
    /// <param name="excluded">뺄 끝점 번호다.</param>
    /// <param name="endpointScratch">최저점 출력 임시 좌표다.</param>
    /// <returns>strict 최저 점수다.</returns>
    public int Score(AiBridgePlacement placement, int targetX, int targetY, ReadOnlySpan<byte> shape, uint excluded, ref RawPathPoint endpointScratch)
    {
        if (shape.Length != ShapeBytes)
        {
            throw new ArgumentException("AI 다리 끝점 실제0xbc 크기", nameof(shape));
        }

        int best = 65536;
        // 원본 순회와 strict 최소 비교이므로 끝점 자체 동률은 바꾸지 않는 반복문이다.
        for (uint index = 0; Signed(index) < Signed(Read(shape, 0xb8)); index++)
        {
            if (index == excluded)
            {
                continue;
            }

            int offset = 0x40 + (int)index * 12;
            double x = (double)Signed(unchecked(Read(shape, offset) - 1u)) + placement.Point.X;
            float y = (float)((double)Signed(unchecked(Read(shape, offset + 4) - 1u)) + placement.Point.Y);
            uint dx = Truncate((double)targetX - x);
            uint dy = Truncate((double)targetY - y);
            int absX = Absolute(dx);
            int absY = Absolute(dy);
            int score = Signed(unchecked((uint)absX + (uint)absY));
            int direction = absX == absY ? -1 : (absX > absY || absX == 2) ? (Signed(dx) <= 0 ? 6 : 2) : (Signed(dy) <= 0 ? 0 : 4);
            int edge = Signed(Read(shape, offset + 8));
            if (edge != direction)
            {
                if (edge == Opposite[direction + 1] || absX == 2 || absY == 2)
                {
                    score = 65536;
                }
                else
                {
                    score = Signed(unchecked((uint)score + 4u));
                    if (score <= 6)
                    {
                        score = 6;
                    }
                }
            }

            if (score < best)
            {
                best = score;
                endpointScratch = new RawPathPoint((float)x, y);
            }
        }

        return best;
    }

    /// <summary>본체의 +d4 회전→+d8 역순 재고→+dc 정순 연결점 상태를 그대로 재개한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="endpointScratch">끝점 출력 임시 좌표다.</param>
    /// <returns>네 회전이 끝나면 참이며 예산 중단은 거짓이다.</returns>
    public bool Search(uint token, ref RawPathPoint endpointScratch)
    {
        byte[] raw = Body(token);
        int budget = 8;
        if (Signed(Read(raw, 0x6c8)) >= Signed(Read(raw, 0x628)))
        {
            Write(raw, 0x6c8, unchecked(Read(raw, 0x628) - 1u));
        }

        // 네 회전이 끝날 때만 참이며 예산 중단은 현재 진행 필드를 유지하는 반복문이다.
        while (Signed(Read(raw, 0x6c4)) <= 7)
        {
            // 재고 마지막 조각부터 첫 조각으로 내려가는 반복문이다.
            while (Signed(Read(raw, 0x6c8)) >= 0)
            {
                // 같은 회전/조각에 각 연결점을 정순으로 대입하는 반복문이다.
                while (Signed(Read(raw, 0x6cc)) < Signed(Read(raw, 0x690)))
                {
                    if (budget < 1)
                    {
                        return false;
                    }

                    int candidate = unchecked(0x648 + (int)Read(raw, 0x6cc) * 12);
                    int edge = Direction(Read(raw, candidate + 8));
                    int desired = Opposite[edge + 1];
                    byte[] shape = new byte[ShapeBytes];
                    uint pattern = Read(raw, unchecked(0x608 + (int)Read(raw, 0x6c8) * 4));
                    uint rotation = Read(raw, 0x6c4);
                    budget--;
                    Shape(pattern, rotation, shape);
                    // 8개 예산은 끝점 수와 별개이며 같은 모양의 모든 일치 끝점을 검사하는 반복문이다.
                    for (uint endpoint = 0; Signed(endpoint) < Signed(Read(shape, 0xb8)); endpoint++)
                    {
                        int offset = 0x40 + (int)endpoint * 12;
                        if (Signed(Read(shape, offset + 8)) != desired)
                        {
                            continue;
                        }

                        var placement = new AiBridgePlacement(
                            new RawPathPoint(
                                Signed(unchecked(Read(raw, candidate) + 1u + unchecked((uint)Dx[edge]) - Read(shape, offset))),
                                Signed(unchecked(Read(raw, candidate + 4) + 1u + unchecked((uint)Dy[edge]) - Read(shape, offset + 4)))),
                            pattern,
                            rotation);
                        if (!Clear(placement))
                        {
                            continue;
                        }

                        int score = Score(placement, Signed(Read(raw, 0x640)), Signed(Read(raw, 0x644)), shape, endpoint, ref endpointScratch);
                        int best = Signed(Read(raw, 0x6a8));
                        if (score > best || (score == best && _random.Next(100) >= 50u))
                        {
                            continue;
                        }

                        Write(raw, 0x6ac, BitConverter.SingleToUInt32Bits(placement.Point.X));
                        Write(raw, 0x6b0, BitConverter.SingleToUInt32Bits(placement.Point.Y));
                        Write(raw, 0x6b4, pattern);
                        Write(raw, 0x6b8, rotation);
                        Write(raw, 0x6a8, unchecked((uint)score));
                        Write(raw, 0x6bc, BitConverter.SingleToUInt32Bits(endpointScratch.X));
                        Write(raw, 0x6c0, BitConverter.SingleToUInt32Bits(endpointScratch.Y));
                    }

                    Write(raw, 0x6cc, Read(raw, 0x6cc) + 1u);
                }

                Write(raw, 0x6cc, 0);
                Write(raw, 0x6c8, Read(raw, 0x6c8) - 1u);
            }

            Write(raw, 0x6c8, unchecked(Read(raw, 0x628) - 1u));
            Write(raw, 0x6c4, Read(raw, 0x6c4) + 2u);
        }

        return true;
    }
}
