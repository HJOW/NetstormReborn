// ClientBoard의 일반 자산/그림자 수집을 실제 SID/해시/타입/물리 SHP 헤더로 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

/// <summary>일반 자산/효과 슬롯의 실제 수집기다. 같은 수집기 안에서는 원본 단계/y/x/next 순서를 유지한다.</summary>
/// <remarks>cpppj client/ObjectCollection.cpp의 00433660 ↔ CD 004887a0 대응이다. 화면 투영과 실제 SHP 그리기는 후속이며 이 계층은 raw 월드 키와 SID 토큰만 제출한다. 2026-11-26 1.065에서 R5-2c용으로 추가했다.</remarks>
public sealed class RawObjectCollection
{
    /// <summary>타입 플래그1의 abstract 비트다.</summary>
    private const uint Abstract = 0x400000;
    /// <summary>타입 플래그1의 그림자 비트다.</summary>
    private const uint Shadow = 0x40000;
    /// <summary>타입 플래그2의 walker 비트다.</summary>
    private const uint Walker = 0x10000;
    /// <summary>월드 경계 여유다.</summary>
    private const double Padding = 1.0;
    /// <summary>walker 표시 y 보정이다.</summary>
    private const double WalkerBias = 0.5;
    /// <summary>그림자 표시 y 보정이다.</summary>
    private const float ShadowBias = 0.1f;

    /// <summary>좌표별 객체 후보를 찾는 공간 해시다. 풀보다 오래 살아야 한다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>객체 슬롯의 생성·조회·반납에 쓰는 SID 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>프레임의 크기·그림 데이터를 조회하는 셰이프 자료다.</summary>
    private readonly IReadOnlyList<SquidDisplayShape> _shapes;
    /// <summary>객체 토큰 기본이다.</summary>
    private readonly uint _tokenBase;

    /// <summary>표 길이/토큰 범위를 검증하고 기존 raw 풀과 타입/프레임 저장소를 참조한다.</summary>
    /// <param name="hash">공간 해시다.</param>
    /// <param name="pool">SID 풀이다.</param>
    /// <param name="types">타입 표다.</param>
    /// <param name="shapes">셰이프 자료다.</param>
    /// <param name="tokenBase">객체 토큰 기본이다.</param>
    public RawObjectCollection(RawSquidHash hash, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<SquidDisplayShape> shapes, uint tokenBase = 0)
    {
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(shapes);
        if (types.Count == 0 || types.Count != shapes.Count)
        {
            throw new ArgumentException("객체 수집 타입/셰이프 표가 비었거나 길이가 다릅니다");
        }
        if ((ulong)tokenBase + (ulong)(uint)pool.Capacity * (ulong)(uint)pool.SlotStride > 0x100000000UL)
        {
            throw new ArgumentException("객체 수집 토큰 영역이 32비트를 넘습니다", nameof(tokenBase));
        }
        _hash = hash;
        _pool = pool;
        _types = types;
        _shapes = shapes;
        _tokenBase = tokenBase;
    }

    /// <summary>raw 슬롯의 unaligned little endian 필드를 읽는다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 바이트 폭이다. CD 프레임 옆 extra를 섞지 않는다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // 요청한 바이트 폭만 읽는다
        for (int i = 0; i < width; i++) value |= (uint)raw[offset + i] << (8 * i);
        return value;
    }

    /// <summary>패치 frameCheck(false)/물리 범위와 CD의 실제 SHP 프레임 참조를 검사한다.</summary>
    /// <param name="type">타입 번호다.</param>
    /// <param name="frame">프레임 번호다.</param>
    /// <returns>물리 표시 프레임이다.</returns>
    private SquidDisplayFrame Frame(uint type, int frame)
    {
        OriginalTypeRecord record = _types[(int)type];
        SquidDisplayShape shape = _shapes[(int)type];
        if (!shape.Loaded || frame < 0 || frame >= shape.Frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(frame), "객체 수집 물리 프레임이 범위를 벗어났습니다");
        }
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            long count = (long)shape.FrameCount * (((record.Flags1 & (Abstract | Shadow)) != 0) ? 2 : 1);
            if (frame >= count || record.MaxHitPoints == unchecked((int)0xDDDDDDDD) || record.MaxHitPoints == unchecked((int)0xCDCDCDCD))
            {
                throw new ArgumentOutOfRangeException(nameof(frame), "객체 수집 frameCheck에 걸렸습니다");
            }
        }
        return shape.Frames[frame];
    }

    /// <summary>+0.5/CRT 하위 DWORD→단계 크기 나눗셈→상단 제한으로 확장 전 칸을 얻는다.</summary>
    /// <param name="coordinate">월드 좌표다.</param>
    /// <param name="level">해시 단계다.</param>
    /// <returns>확장 전 칸이다.</returns>
    public static int Cell(float coordinate, int level)
    {
        double value = (double)coordinate + 0.5;
        long integer = 0;
        if (double.IsFinite(value) && value >= -9.223372036854776E18 && value < 9.223372036854776E18)
        {
            integer = (long)value;
        }
        int scale = RawSquidHash.WorldCells / RawSquidHash.Side(level);
        // 원문 정수 절삭과 signed idiv를 사용한 뒤 현재 단계의 상단 칸으로 제한한다
        int cell = unchecked((int)(uint)integer) / scale;
        return Math.Min(cell, RawSquidHash.Side(level) - 1);
    }

    /// <summary>float 저장 위치와 판본별 NaN 조건을 보존해 월드 프레임의 포함 교차를 판정한다.</summary>
    /// <param name="view">수집에 쓰는 월드 사각형이다.</param>
    /// <param name="x">객체 월드 가로다.</param>
    /// <param name="y">객체 월드 세로다.</param>
    /// <param name="frame">물리 표시 프레임이다.</param>
    /// <returns>보이면 참이다.</returns>
    private bool Visible(RenderWorldRect view, float x, float y, SquidDisplayFrame frame)
    {
        // 패치의 x 차이는 double에 남고 y 차이는 float로 저장된다
        double dx = (double)x - frame.CellHotspotX;
        double dy = (float)((double)y - frame.CellHotspotY);
        double left = (double)view.Left - frame.CellWidth - Padding;
        double top = (double)view.Top - frame.CellHeight - Padding;
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            return left <= dx && dx <= view.Right && top <= dy && dy <= view.Bottom;
        }
        // CD는 차이 둘과 top 경계를 float로 저장한다
        double cdX = (float)dx, cdTop = (float)top;
        return !(left > cdX) && !(cdX > view.Right) && dy >= cdTop && !(dy > view.Bottom);
    }

    /// <summary>1~3단계에서 각 SID의 본체/그림자를 별도로 판정한다. 정상 반환 때 해시는 2단계가 된다.</summary>
    /// <param name="view">수집에 쓰는 월드 사각형이다.</param>
    /// <param name="output">쓸 작업 배열이다.</param>
    /// <param name="limit">전체 count&lt;limit의 엄격한 assert다. 누적 표시 용량이 아니다.</param>
    /// <returns>쓴 항목 수다.</returns>
    public int Collect(RenderWorldRect view, RenderCollectionEntry[] output, int limit)
    {
        ArgumentNullException.ThrowIfNull(output);
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        int count = 0;
        // 화면/해시 단계 범위만 방문한다. 다른 칸의 반복 SID를 전역 중복 제거하지 않는다.
        for (int level = 1; level < 4; level++)
        {
            _hash.Select(level);
            int side = RawSquidHash.Side(level);
            // 원문 DWORD 한 칸 확장과 각 끝의 독립 클램프를 signed overflow 없이 재현한다
            static int Expand(int cell, uint step, int side) => Math.Clamp(unchecked((int)((uint)cell + step)), 0, side - 1);
            int left = Expand(Cell(view.Left, level), 0xffffffffU, side);
            int right = Expand(Cell(view.Right, level), 1, side);
            int top = Expand(Cell(view.Top, level), 0xffffffffU, side);
            int bottom = Expand(Cell(view.Bottom, level), 1, side);
            // y 바깥, x 안쪽 순서이며 빈 칸은 체인 조회를 생략한다
            for (int y = top; y <= bottom; y++)
            {
                // 한 칸의 next 체인을 끝까지 방문한 뒤 다음 x 칸으로 이동한다
                for (int x = left; x <= right; x++)
                {
                    int sid = _hash.At(x, y);
                    var chain = new HashSet<int>();
                    // 같은 체인 안의 순환만 거부한다. 풀 범위/type/frame은 기록 전에 검사한다.
                    while (sid != 0)
                    {
                        if (!chain.Add(sid))
                        {
                            throw new InvalidOperationException("객체 수집 SID 체인에 순환이 있습니다");
                        }
                        ReadOnlySpan<byte> raw = _pool.Slot(sid);
                        uint type = raw[10];
                        if (type == 0 || type >= (uint)_types.Count)
                        {
                            throw new ArgumentOutOfRangeException(nameof(sid), "객체 수집 타입이 0이거나 표를 넘었습니다");
                        }
                        OriginalTypeRecord record = _types[(int)type];
                        int frame = patch ? unchecked((int)Read(raw, 36)) : raw[34];
                        float positionX = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]);
                        float positionY = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]);
                        // 수집되는 레이어만 기록하고 원문 strict limit은 기록 후 검사한다
                        void Append(bool shadow)
                        {
                            if (count >= output.Length)
                            {
                                throw new InvalidOperationException("객체 수집 작업 배열이 찼습니다");
                            }
                            ReadOnlySpan<byte> current = _pool.Slot(sid);
                            float ey = BinaryPrimitives.ReadSingleLittleEndian(current[18..]);
                            if (shadow)
                            {
                                ey = (float)((double)ey - ShadowBias);
                            }
                            else if ((record.Flags2 & Walker) != 0)
                            {
                                ey = (float)((double)ey + WalkerBias);
                            }
                            output[count] = new RenderCollectionEntry(
                                BinaryPrimitives.ReadSingleLittleEndian(current[14..]),
                                ey,
                                unchecked(_tokenBase + (uint)sid * (uint)_pool.SlotStride),
                                unchecked((short)(sbyte)current[patch ? 35 : 33]),
                                0,
                                (byte)(shadow || ((record.Flags1 & Abstract) != 0 && frame >= _shapes[(int)type].FrameCount) ? 4 : 0));
                            count++;
                            if (count >= limit)
                            {
                                throw new InvalidOperationException("객체 수집 개수가 limit에 닿았습니다");
                            }
                        }
                        if (Visible(view, positionX, positionY, Frame(type, frame)))
                        {
                            Append(false);
                        }
                        if ((record.Flags1 & Shadow) != 0)
                        {
                            int shadowFrame = unchecked((int)((uint)frame + (uint)_shapes[(int)type].FrameCount));
                            if (Visible(view, positionX, positionY, Frame(type, shadowFrame)))
                            {
                                Append(true);
                            }
                        }
                        sid = unchecked((ushort)Read(_pool.Slot(sid), 4, 2));
                    }
                }
            }
        }
        _hash.Select(2);
        return count;
    }

    /// <summary>원본 tokenBase/stride와 슬롯 경계를 검사해 같은 풀의 번호만 반환한다.</summary>
    /// <param name="token">객체 토큰이다.</param>
    /// <returns>실제 SID 번호다.</returns>
    public int Object(uint token)
    {
        if (token < _tokenBase)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "객체 수집 토큰이 범위를 벗어났습니다");
        }
        uint offset = token - _tokenBase;
        uint stride = (uint)_pool.SlotStride;
        if (offset % stride != 0 || offset == 0 || offset / stride >= (uint)_pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "객체 수집 토큰의 SID가 범위를 벗어났습니다");
        }
        return (int)(offset / stride);
    }
}
