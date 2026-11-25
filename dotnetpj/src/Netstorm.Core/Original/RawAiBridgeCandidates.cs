// 실제 AI 다리 탐색의 나선 24칸과 연결점 후보 6개를 같은 현재 월드에서 준비한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 다리 후보 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.

// 원본 getter가 읽는 현재 TYPE 전역이며 번호를 별도 사본으로 고정하지 않는다.
public sealed class AiBridgeCandidateTypes
{
    /// <summary>005411a0의 전체 DWORD로 다리 여부를 비교한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    public Func<uint>? Bridge { get; set; }

    /// <summary>005411c0의 낮은 BYTE는 graph getter의 예외 타입이다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    public Func<uint>? Fencemark { get; set; }

    /// <summary>00541288의 낮은 BYTE는 graph getter의 두 번째 예외 타입이다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    public Func<uint>? WindArcher { get; set; }
}

// 0040fe90의 현재 상태/실제 hash0/spot/프레임 코드만 읽고 후보 상태를 갱신한다.
// 인접 원본 전략 하위만 같은 현재 graph/코드/지도 getter를 공유한다. 별도 월드 사본을 만들지 않는다.
// 근거: cpppj o/RawAiBridgeCandidates.h/.cpp의 RawAiBridgeCandidates다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.
public sealed class RawAiBridgeCandidates
{
    /// <summary>원본 호출 예산이다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private const uint Budget = 24;

    /// <summary>원본 후보 용량이다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private const int Capacity = 6;

    /// <summary>Brain+5f0+58의 실제 배열 시작이다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private const int Candidates = 0x648;

    /// <summary>원본 TerrainBuilder와 같은 여덟 방향 이웃 칸 차이다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private static readonly (int Dx, int Dy)[] NeighborCells =
    [
        (0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1),
    ];

    /// <summary>같은 실제 Brain/목록 소유 메모리다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>실제 50BYTE surface 슬롯을 읽는다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 graph getter의 surface flag 표다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>현재 raw프레임의 방향 코드를 읽는 실제 표다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>같은 hash0의 65536 WORD다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<ushort> _surfaces;

    /// <summary>같은 현재 점유 65536 BYTE다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<byte> _spots;

    /// <summary>원본 전역의 현재 참조다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private readonly AiBridgeCandidateTypes _globals;

    /// <summary>같은 PATCH188/65536 지도와 원본 본체를 빌린다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 실제 Brain/목록 소유 메모리다.</param>
    /// <param name="pool">실제 50BYTE surface 슬롯 풀이다.</param>
    /// <param name="types">현재 graph getter의 surface flag 표다.</param>
    /// <param name="frames">현재 raw프레임의 방향 코드 표다.</param>
    /// <param name="surfaces">같은 hash0의 65536 WORD다.</param>
    /// <param name="spots">같은 현재 점유 65536 BYTE다.</param>
    /// <param name="globals">원본 전역의 현재 참조다.</param>
    public RawAiBridgeCandidates(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, IReadOnlyList<ushort> surfaces, IReadOnlyList<byte> spots, AiBridgeCandidateTypes globals)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(surfaces);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(globals);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || frames.Count != 188 || surfaces.Count != 65536 || spots.Count != 65536)
        {
            throw new ArgumentException("AI 다리 후보의 실제 PATCH188/지도 저장소 누락");
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _frames = frames;
        _surfaces = surfaces;
        _spots = spots;
        _globals = globals;
    }

    /// <summary>원본 DWORD의 signed 해석을 구현 의존 변환 없이 적용한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>실제 __ftol의 signed64 절삭 하위 DWORD이며 indefinite 하위는 0이다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 float다.</param>
    /// <returns>절삭한 낮은 DWORD다.</returns>
    private static uint Truncate(float value)
    {
        if (!float.IsFinite(value) || (double)value < -9223372036854775808.0 || (double)value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((uint)(long)value);
    }

    /// <summary>원본 xor/sub의 INT_MIN 감김도 signed 최소거리 비교에 그대로 전달한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="value">절댓값을 구할 DWORD다.</param>
    /// <returns>음수가 아닌 DWORD다.</returns>
    private static uint Absolute(uint value) => Signed(value) < 0 ? unchecked(0u - value) : value;

    /// <summary>raw 슬롯/본체의 비정렬 DWORD를 원본 little endian으로 읽는다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, int at)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 다리 후보 DWORD 읽기 범위");
        }

        // 지정한 네 바이트만 합치는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at, 4));
    }

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static uint GlobalUint(Func<uint>? function) => function is null
        ? throw new InvalidOperationException("AI 다리 후보 실제 전역 미연결")
        : function();

    /// <summary>크기 없는 가짜 본체나 반납된 핸들을 처리하지 않는다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    internal byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 다리 후보의 실제0xb08 본체 누락");
        }

        return raw;
    }

    /// <summary>원본이 각 위치에서 다시 읽는 현재 필드를 캐시하지 않는다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>현재 비정렬 DWORD다.</returns>
    internal uint Field(uint token, int offset) => Read(Body(token), offset);

    /// <summary>음수 후보 번호도 원본 DWORD 주소 감김 뒤 실제 본체 안이면 그 바이트를 쓴다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    internal void Field(uint token, int offset, uint value)
    {
        byte[] raw = Body(token);
        if (offset < 0 || (uint)offset > (uint)raw.Length || (uint)raw.Length - (uint)offset < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "AI 다리 후보 DWORD 쓰기 범위");
        }

        // 주변 필드는 그대로 보존하는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset, 4), value);
    }

    /// <summary>x255의 오른쪽처럼 같은 flat 지도 안의 인접 주소는 원본대로 다음 행을 읽는다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="x">고를 가로 칸이다.</param>
    /// <param name="y">고를 세로 칸이다.</param>
    /// <returns>해당 칸 spot BYTE다.</returns>
    internal byte Spot(int x, int y)
    {
        long index = (long)y * 256 + x;
        if (index < 0 || index >= _spots.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "AI 다리 후보 spot 물리 범위");
        }

        return _spots[(int)index];
    }

    /// <summary>free/dead/void나 소유자 필터를 getter에 추가하지 않는다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="sid">조회할 surface SID다.</param>
    /// <returns>현재 raw graph BYTE다.</returns>
    internal uint Graph(Sid sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid.Value);
        byte type = raw[RawSidPool.TypeOffset];
        if (type >= _types.Count || ((_types[type].Flags1 & 0x800u) == 0 && type != (byte)GlobalUint(_globals.Fencemark) && type != (byte)GlobalUint(_globals.WindArcher)))
        {
            throw new ArgumentException("AI 다리 후보 graph getter 타입 오류");
        }

        return raw[30];
    }

    /// <summary>현재 코드의 A~P 연결표는 지형 생성과 같은 원본 0052f910을 공유한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="sid">조회할 surface SID다.</param>
    /// <returns>현재 연결 비트다.</returns>
    private uint Connections(Sid sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid.Value);
        byte type = raw[RawSidPool.TypeOffset];
        uint frame = BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(36, 4));
        if (type < 70 || type >= _frames.Count || frame >= (uint)_frames[type].Codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "AI 다리 후보의 현재 프레임 범위");
        }

        return (uint)TerrainBuilder.Connection((char)_frames[type].Codes[(int)frame].Side);
    }

    /// <summary>0040faa0의 북/동/남/서 첫 열린 끝점만 확인하며 지면의 owner/graph는 caller가 검사한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="surface">검사할 surface SID다.</param>
    /// <param name="x">현재 가로 칸이다.</param>
    /// <param name="y">현재 세로 칸이다.</param>
    /// <returns>빈 이웃이 있으면 같은 SID며 없으면 0이다.</returns>
    internal Sid Open(Sid surface, int x, int y)
    {
        // 다리 TYPE는 BYTE로 축소하지 않는다.
        bool bridge = (uint)_pool.Slot(surface.Value)[RawSidPool.TypeOffset] == GlobalUint(_globals.Bridge);
        uint connections = Connections(surface);
        // 현재 코드의 각 cardinal 비트와 빈 이웃을 같은 순서로 검사하는 반복문이다.
        for (uint direction = 0; direction < 8; direction += 2)
        {
            (int dx, int dy) = NeighborCells[(int)direction];
            if (((connections & (1u << (int)(direction / 2))) != 0) == bridge && Spot(x + dx, y + dy) == 0)
            {
                return surface;
            }
        }

        return new Sid(0);
    }

    /// <summary>현재 맵 경계를 clamp하지 않으며 같은 거리에서는 먼저 발견한 X/Y를 유지한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="target">거리를 잴 목표점이다.</param>
    /// <param name="bounds">탐색할 사각형이다.</param>
    /// <returns>현재 그래프의 열린 시작점이며 없으면 {-10000,-10000}이다.</returns>
    public RawPathPoint Start(uint token, RawPathPoint target, AiBuildSearchBounds bounds)
    {
        _ = Body(token);
        uint targetX = Truncate(target.X);
        uint targetY = Truncate(target.Y);
        int best = 65536;
        RawPathPoint result = new(-10000, -10000);
        // 원본은 미정 결과를 두 float에 직접 쓴다. 원본의 fild 비교는 float 저장 전 정수값이며 X가 바깥 순회다.
        for (uint x = Truncate(bounds.Left); (double)Signed(x) < bounds.Right; x++)
        {
            // 각 X마다 현재 위 경계를 다시 절삭하고 Y를 올리는 반복문이다.
            for (uint y = Truncate(bounds.Top); (double)Signed(y) < bounds.Bottom; y++)
            {
                // 원본은 각 칸에서 현재 본체 두 필드를 다시 읽는다.
                uint graph = Field(token, 0x298);
                uint owner = Field(token, 0x260);
                if (Signed(x) < 0 || Signed(y) < 0 || x >= 256 || y >= 256)
                {
                    continue;
                }

                ushort surface = _surfaces[(int)y * 256 + (int)x];
                if (surface == 0 || Graph(new Sid(surface)) != graph || _pool.Slot(surface)[34] != owner || (Spot(Signed(x) - 1, Signed(y)) != 0 && Spot(Signed(x) + 1, Signed(y)) != 0 && Spot(Signed(x), Signed(y) - 1) != 0 && Spot(Signed(x), Signed(y) + 1) != 0))
                {
                    continue;
                }

                int score = Signed(unchecked(Absolute(unchecked(targetX - x)) + Absolute(unchecked(targetY - y))));
                // 거리부터 비교하고 엄격히 작은 후보만 유효 연결 검사를 실행한다.
                if (score >= best || Open(new Sid(surface), Signed(x), Signed(y)).Value == 0)
                {
                    continue;
                }

                result = new RawPathPoint(Signed(x), Signed(y));
                best = score;
            }
        }

        return result;
    }

    /// <summary>helper 효과가 없는 실제 읽기 순서와 후보 기록 뒤 나선 진행을 보존한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>나선이 끝나면 참이며 예산이 떨어지면 거짓이다.</returns>
    public bool Prepare(uint token)
    {
        // 원본은 진입 시 그래프/소유자 두 DWORD만 스택에 보관한다.
        uint graph = Field(token, 0x5f8);
        uint owner = Field(token, 0x5fc);
        uint budget = Budget;
        // 한 나선 변이 끝날 때마다 바깥 종료 조건을 다시 검사하는 반복문이다.
        for (; ; )
        {
            int x = Signed(Field(token, 0x6a0));
            int y = Signed(Field(token, 0x6a4));
            int stage = Signed(Field(token, 0x698));
            if (x <= 10 || y <= 10 || x >= 246 || y >= 246 || stage >= 24 || (stage >= 8 && Signed(Field(token, 0x690)) >= 3))
            {
                return true;
            }

            // 현재 변의 나머지는 24칸 예산이 소진될 때까지 이어가는 반복문이다.
            while (Field(token, 0x69c) != 0)
            {
                if (budget == 0)
                {
                    return false;
                }

                budget--;
                int px = Signed(Field(token, 0x6a0));
                int py = Signed(Field(token, 0x6a4));
                ushort number = 0;
                // 원본 map 조회는 두 축 0..255를 먼저 검사한다.
                if (px >= 0 && py >= 0 && px < 256 && py < 256)
                {
                    number = _surfaces[py * 256 + px];
                }

                if (number != 0)
                {
                    var sid = new Sid(number);
                    if (Graph(sid) == graph && _pool.Slot(number)[34] == owner && (Spot(px - 1, py) == 0 || Spot(px + 1, py) == 0 || Spot(px, py - 1) == 0 || Spot(px, py + 1) == 0))
                    {
                        // 다리 TYPE는 BYTE로 축소하지 않는다.
                        bool bridge = (uint)_pool.Slot(number)[RawSidPool.TypeOffset] == GlobalUint(_globals.Bridge);
                        uint connections = Connections(sid);
                        // 원본은 북/동/남/서만 이 순서로 최대 6개 저장하는 반복문이다.
                        for (uint direction = 0; direction < 8; direction += 2)
                        {
                            uint count = Field(token, 0x690);
                            if (Signed(count) >= Capacity)
                            {
                                break;
                            }

                            (int dx, int dy) = NeighborCells[(int)direction];
                            if (((connections & (1u << (int)(direction / 2))) != 0) == bridge && Spot(px + dx, py + dy) == 0)
                            {
                                Field(token, 0x690, count + 1u);
                                // 원본 DWORD 곱셈/주소 감김과 count 증가 선행을 유지한다.
                                int at = unchecked(Candidates + (int)(count * 12u));
                                Field(token, at, Field(token, 0x6a0));
                                Field(token, at + 4, Field(token, 0x6a4));
                                Field(token, at + 8, direction);
                            }
                        }
                    }
                }

                // 현재 변의 방향은 후보 방향과 다른 필드다.
                uint spiral = Field(token, 0x694);
                if (spiral >= NeighborCells.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(token), "AI 다리 후보 나선 방향 범위");
                }

                (int sx, int sy) = NeighborCells[(int)spiral];
                Field(token, 0x6a0, unchecked(Field(token, 0x6a0) + (uint)sx));
                Field(token, 0x6a4, unchecked(Field(token, 0x6a4) + (uint)sy));
                Field(token, 0x69c, Field(token, 0x69c) - 1u);
            }

            // 나선 회전은 signed 나머지, 변 길이는 signed 산술 시프트다.
            uint next = unchecked((uint)(Signed(unchecked(Field(token, 0x694) + 2u)) % 8));
            uint stageBits = Field(token, 0x698) + 1u;
            Field(token, 0x694, next);
            Field(token, 0x698, stageBits);
            Field(token, 0x69c, unchecked((uint)(Signed(stageBits) >> 1)));
        }
    }

    /// <summary>공유 세계의 주소를 노출해 연결 계층이 같은 풀인지 확인한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
