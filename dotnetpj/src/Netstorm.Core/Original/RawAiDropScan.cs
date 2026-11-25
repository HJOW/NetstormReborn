// 실제 표면 지도와 다리 J/K 주변 후보에서 AI 배치 스캔/30개 선택 목록을 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 배치 스캔 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.

// 원본 스캔 전역의 현재 저장소를 빌린다.
public sealed class AiDropScanGlobals
{
    /// <summary>005411a0의 현재 다리 TYPE다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    public Func<uint>? BridgeType { get; set; }

    /// <summary>00544d14..20의 현재 경계이며 CRT 004fc530 초기값은 10,10,246,246이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    public Func<AiBuildSearchBounds>? Bounds { get; set; }
}

// 00412b90의 실제 후보 승인을 같은 본체 3e8 점수 포인터로 받는다.
public delegate bool AiDropScanConsider(uint token, uint type, RawPathPoint at, int direction, ref RawPathPoint best, ref Sid target);

// 이미 복원된 실제 전체 발자국/후보 승리와 선택된 원본 진단만 연결한다.
public sealed class AiDropScanHooks
{
    /// <summary>0040f130의 같은 현재 TYPE/전체 spot 발자국이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    public Func<uint, int, int, bool>? Free { get; set; }

    /// <summary>00412b90의 실제 후보 승영이며 점수 포인터는 같은 본체 3e8이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    public AiDropScanConsider? Consider { get; set; }

    /// <summary>0040f390의 현재 원본 진단이다. 반환하면 현재 count로 계속한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    public Action<string, uint>? Assertion { get; set; }
}

// 같은 본체/월드 지도/원본 공용 난수와 실제 후보 승인만 사용한다.
// 근거: cpppj o/RawAiDropScan.h/.cpp의 RawAiDropScan다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.
// cpp의 RiftTypeRecord는 DN의 OriginalTypeRecord로 받는다(flags2만 쓴다).
public sealed class RawAiDropScan
{
    /// <summary>원본 최대 후보다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private const int Choices = 30;

    /// <summary>일반 호출 예산이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private const int NormalBudget = 1000;

    /// <summary>공급 호출 예산이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private const int SupplyBudget = 500;

    /// <summary>원본 지도 행 stride다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private const int Width = 256;

    /// <summary>같은 본체/진행/후보/점수 저장소다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>현재 표면 TYPE/소유자/프레임을 읽는 실제 풀이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>각 스캔 호출 처음의 TYPE flags2를 읽는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>현재 물리 frame의 signed 첫 문자를 읽는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>원본 현재 65536 BYTE spot이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private readonly byte[] _spots;

    /// <summary>원본 5c84bc의 현재 0단계 65536 WORD 표면이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private readonly ushort[] _surfaces;

    /// <summary>원본 현재 방향별 9×9 DWORD 동맹이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private readonly uint[] _alliances;

    /// <summary>SP/전투/시설과 같은 원본 공용 난수다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private readonly GameRandom _random;

    /// <summary>현재 다리 TYPE와 전체 스캔 경계의 참조다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private readonly AiDropScanGlobals _globals;

    /// <summary>선택된 실제 하위/진단이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private readonly AiDropScanHooks _hooks;

    /// <summary>생성 중 스캔/난수/장치를 실행하지 않는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 본체/진행/후보/점수 저장소다.</param>
    /// <param name="pool">현재 표면 TYPE/소유자/프레임을 읽는 실제 풀이다.</param>
    /// <param name="types">각 스캔 호출 처음의 TYPE flags2를 읽는다.</param>
    /// <param name="frames">현재 물리 frame의 signed 첫 문자를 읽는다.</param>
    /// <param name="spots">원본 현재 65536 BYTE spot이다.</param>
    /// <param name="surfaces">원본 5c84bc의 현재 0단계 65536 WORD 표면이다.</param>
    /// <param name="alliances">원본 현재 방향별 9×9 DWORD 동맹이다.</param>
    /// <param name="random">SP/전투/시설과 같은 원본 공용 난수다.</param>
    /// <param name="globals">현재 다리 TYPE와 전체 스캔 경계의 참조다.</param>
    /// <param name="hooks">선택된 실제 하위/진단이다.</param>
    public RawAiDropScan(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, byte[] spots, ushort[] surfaces, uint[] alliances, GameRandom random, AiDropScanGlobals globals, AiDropScanHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(surfaces);
        ArgumentNullException.ThrowIfNull(alliances);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || frames.Count != types.Count || spots.Length != 65536 || surfaces.Length != 65536 || alliances.Length != 81)
        {
            throw new ArgumentException("AI 배치 스캔 실제PATCH월드누락");
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _frames = frames;
        _spots = spots;
        _surfaces = surfaces;
        _alliances = alliances;
        _random = random;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>해제/작은 가짜 본체를 거부한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 배치 스캔 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 DWORD의 실제 물리 범위를 확인해 읽는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int at)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 스캔 DWORD 범위");
        }

        // 지정 네 BYTE만 조합하는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(at, 4));
    }

    /// <summary>원본 DWORD의 signed 해석이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원본 DWORD의 덧셈/뺄셈 감김이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="value">첫 값이다.</param>
    /// <param name="delta">더할 DWORD다.</param>
    /// <returns>감김 signed 합이다.</returns>
    private static int Add(int value, uint delta) => Signed(unchecked((uint)value) + delta);

    /// <summary>ftol의 signed64 절삭 하위 DWORD다. 무효 FISTP의 하위값은 0이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭 하위 DWORD의 signed 해석이다.</returns>
    private static int Ftol(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return Signed(unchecked((uint)(long)Math.Truncate(value)));
    }

    /// <summary>선택된 실제 하위가 없으면 앞선 상태/효과를 보존하며 거부한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 배치 스캔 실제 하위 미연결");

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static uint GlobalUint(Func<uint>? function) => Need(function)();

    /// <summary>각 하위 효과 뒤 현재 본체를 다시 읽는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>지정 필드만 쓰고 다른 상태를 동기화하지 않는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int at, uint value)
    {
        byte[] raw = Body(token);
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 스캔 쓰기 범위");
        }

        // 지정 네 BYTE만 쓰는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at, 4), value);
    }

    /// <summary>행 경계를 clamp하면 원본 세 칸 검사의 주소가 달라지므로 전체 선형 범위만 확인한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="x">읽을 가로 칸이다.</param>
    /// <param name="y">읽을 세로 칸이다.</param>
    /// <returns>실제 spot이다.</returns>
    private byte Spot(int x, int y)
    {
        long at = (long)y * Width + x;
        if (at < 0 || at >= _spots.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "AI 다리 후보 spot 물리 범위");
        }

        return _spots[at];
    }

    /// <summary>정상 후보 배열의 범위를 넘는 손상 count/index는 명시 거부한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="index">고를 후보 번호다.</param>
    /// <returns>좌표 기록 위치다.</returns>
    private static int Point(int index)
    {
        if (index < 0 || index >= Choices)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "AI 배치 후보 좌표 인덱스");
        }

        return 0x408 + index * 8;
    }

    /// <summary>같은 후보의 표적은 별도 30 DWORD 배열이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="index">고를 후보 번호다.</param>
    /// <returns>표적 기록 위치다.</returns>
    private static int Target(int index)
    {
        if (index < 0 || index >= Choices)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "AI 배치 후보 표적 인덱스");
        }

        return 0x4f8 + index * 4;
    }

    /// <summary>각 쓰기 순서와 현재 count 재조회를 스캔 호출자가 정한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="index">쓸 후보 번호다.</param>
    /// <param name="point">쓸 좌표다.</param>
    /// <param name="target">쓸 표적 SID다.</param>
    private void Choice(uint token, int index, RawPathPoint point, Sid target)
    {
        int at = Point(index);
        Field(token, at, BitConverter.SingleToUInt32Bits(point.X));
        Field(token, at + 4, BitConverter.SingleToUInt32Bits(point.Y));
        Field(token, Target(index), target.Value);
    }

    /// <summary>양쪽 모두 가능하면 J의 위쪽/K의 왼쪽이 우선이며 실패 때 난수를 소비하지 않는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="surface">검사할 표면 SID다.</param>
    /// <param name="x">기준 가로를 쓰고 조정 결과를 받는 참조다.</param>
    /// <param name="y">기준 세로를 쓰고 조정 결과를 받는 참조다.</param>
    /// <returns>조정했으면 참이다.</returns>
    public bool AdjustBridge(Sid surface, ref int x, ref int y)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(surface.Value);
        if (raw[10] != GlobalUint(_globals.BridgeType))
        {
            return false;
        }

        if (raw[10] >= _frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(surface), "AI 다리 후보 현재TYPE 범위");
        }

        int side = RawPathAnimation.Side(_frames[raw[10]], Signed(Read(raw, 36)));
        if (side == 'J')
        {
            if (Spot(x, Add(y, 0xffffffffu)) == 0 && Spot(x, Add(y, 0xfffffffeu)) == 0 && Spot(x, Add(y, 0xfffffffeu - 1u)) == 0)
            {
                y = Add(y, 0xffffffffu);
            }
            else
            {
                if (Spot(x, Add(y, 1)) != 0 || Spot(x, Add(y, 2)) != 0 || Spot(x, Add(y, 3)) != 0)
                {
                    return false;
                }

                y = Add(y, 3);
            }

            uint draw = _random.Next(3u);
            x = Add(x, draw);
            return true;
        }

        if (side == 'K')
        {
            if (Spot(Add(x, 0xffffffffu), y) == 0 && Spot(Add(x, 0xfffffffeu), y) == 0 && Spot(Add(x, 0xfffffffeu - 1u), y) == 0)
            {
                x = Add(x, 0xffffffffu);
            }
            else
            {
                if (Spot(Add(x, 1), y) != 0 || Spot(Add(x, 2), y) != 0 || Spot(Add(x, 3), y) != 0)
                {
                    return false;
                }

                x = Add(x, 3);
            }

            uint draw = _random.Next(3u);
            y = Add(y, draw);
            return true;
        }

        return false;
    }

    /// <summary>원본 유일 호출과 같은 3e8의 현재 점수를 그 자리에서 평가·비교한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">스캔할 TYPE 번호다.</param>
    /// <param name="direction">스캔할 방향이다.</param>
    /// <returns>예산이 남으면 참(미완료), 소진하면 거짓이다.</returns>
    public bool Scan(uint token, uint type, int direction)
    {
        if ((ulong)type >= (ulong)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "AI 배치 스캔 현재TYPE 범위");
        }

        uint flags = _types[(int)type].Flags2;
        AiBuildSearchBounds bounds = Need(_globals.Bounds)();
        int left = Ftol(bounds.Left);
        int top = Ftol(bounds.Top);
        int right = Ftol(bounds.Right);
        int bottom = Ftol(bounds.Bottom);
        int budget = (flags & 0x800u) != 0 ? SupplyBudget : NormalBudget;
        // 각 호출의 flags/경계는 snapshot이고 본체 진행/소유자/지도를 각 원문 위치에서 새로 읽는 반복문이다.
        do
        {
            int x = Add(Signed(Field(token, 0xaf8)), 1);
            Field(token, 0xaf8, unchecked((uint)x));
            if (x > right)
            {
                Field(token, 0xafc, Field(token, 0xafc) + 1u);
                Field(token, 0xaf8, unchecked((uint)left));
                if (Signed(Field(token, 0xafc)) > bottom)
                {
                    Field(token, 0xafc, unchecked((uint)top));
                    return false;
                }
            }

            x = Signed(Field(token, 0xaf8));
            int y = Signed(Field(token, 0xafc));
            if (x >= 0 && y >= 0 && x < 256 && y < 256)
            {
                ushort surface = _surfaces[(long)y * Width + x];
                if (surface != 0)
                {
                    bool bridge = AdjustBridge(new Sid(surface), ref x, ref y);
                    budget -= 5;
                    bool eligible = bridge;
                    if (!bridge && (flags & 0x4040800u) != 0 && (Spot(x, y) & 0x3fu) == 2u && Field(token, 0x2c8) != 0)
                    {
                        uint surfaceOwner = _pool.Slot(surface)[34];
                        uint owner = Field(token, 0x260);
                        ulong at = (ulong)surfaceOwner * 9ul + owner;
                        if (at >= (ulong)_alliances.Length)
                        {
                            throw new ArgumentOutOfRangeException(nameof(token), "AI 배치 스캔 현재 동맹 인덱스");
                        }

                        if (_alliances[at] != 0)
                        {
                            eligible = surfaceOwner == owner && Need(_hooks.Free)(type, x, y);
                            budget -= 10;
                        }
                    }

                    if (eligible)
                    {
                        int previous = Signed(Field(token, 0x3e8));
                        var point = new RawPathPoint();
                        var target = new Sid(0);
                        if (Need(_hooks.Consider)(token, type, new RawPathPoint(x, y), direction, ref point, ref target))
                        {
                            if (Signed(Field(token, 0x3e8)) > previous)
                            {
                                Field(token, 0x570, 0);
                                Choice(token, 0, point, target);
                                Field(token, 0x570, Field(token, 0x570) + 1u);
                            }
                            else
                            {
                                int count = Signed(Field(token, 0x570));
                                if (count >= Choices)
                                {
                                    int index = Signed(_random.Next(unchecked((uint)count)));
                                    Choice(token, index, point, target);
                                }
                                else
                                {
                                    Choice(token, count, point, target);
                                    Field(token, 0x570, Field(token, 0x570) + 1u);
                                }
                            }
                        }

                        budget -= 125;
                    }
                }
            }

            --budget;
        }
        while (budget > 0);

        return true;
    }

    /// <summary>진단 뒤 현재 count를 먼저 줄이고 복사하지 않는 꼬리/점수/진행 위치는 보존한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="index">지울 후보 번호다.</param>
    public void Remove(uint token, int index)
    {
        if (Signed(Field(token, 0x570)) < 1)
        {
            Need(_hooks.Assertion)("totalChoices > 0", 0x1ac);
        }

        Field(token, 0x570, Field(token, 0x570) - 1u);
        // 현재 count까지 XY→표적 순서로 안정 압축하고 꼬리 바이트는 그대로 두는 반복문이다.
        while (index < Signed(Field(token, 0x570)))
        {
            int at = Point(index);
            int next = Point(Add(index, 1));
            Field(token, at, Field(token, next));
            Field(token, at + 4, Field(token, next + 4));
            Field(token, Target(index), Field(token, Target(Add(index, 1))));
            index = Add(index, 1);
        }
    }

    /// <summary>동일 실제 풀 연결 확인용이다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
