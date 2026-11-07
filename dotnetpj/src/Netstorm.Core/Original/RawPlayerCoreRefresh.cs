// 현재 Core의 기술 bit와 자기 소유 다이스의 count 하한을 갱신한다.
namespace Netstorm.Core.Original;

// Player Core 갱신 모듈을 담는 네임스페이스다. 2026-11-07 F2d Player 이식에서 추가됐다.

// 현재 raw Pool 뷰와 원문 32비트 주소 역할을 묶는다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreRefreshPool
{
    /// <summary>callback 변경 후 현재 물리를 다시 빌리는 원본 50바이트 슬롯 표다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public byte[] Bytes { get; set; } = [];

    /// <summary>host 포인터가 아닌 원문 주소 산술·유효 관문의 식별값이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint Address { get; set; }

    /// <summary>현재 DWORD이며 iterator cached 상한과 구별한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint Capacity { get; set; }
}

// 가상 level 호출에 넘길 같은 현재 객체 물리와 주소 역할이다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreRefreshForm
{
    /// <summary>현재 Pool의 같은 객체 슬롯이며 복사 버퍼가 아니다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Memory<byte> Bytes { get; set; }

    /// <summary>같은 가상 호출 대상을 식별하는 원문 32비트 주소다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint Address { get; set; }
}

// Begin마다 따로 살아 있는 순회 커서다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreRefreshCursor
{
    /// <summary>실제 현재 후보 번호이며 0이 순회 종료다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint>? Current { get; set; }

    /// <summary>이미 저장한 다음 링크 진행이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action? Next { get; set; }
}

// 현재 전역·기존 실제 자식은 호출 위치에서만 요구한다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreRefreshHooks
{
    /// <summary>진단·현재 재조회를 포함한 실제 Core 뷰다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<Memory<byte>>? Core { get; set; }

    /// <summary>현재 known·owner·Player 기록 조회다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint>? TechnologyParent { get; set; }

    /// <summary>실제 독립 Finder와 기본 참 필터다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint, PlayerCoreRefreshCursor>? Begin { get; set; }

    /// <summary>현재 Pool·base·capacity를 직접 조회하는 순수 제공자다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<PlayerCoreRefreshPool>? Pool { get; set; }

    /// <summary>현재 타입+1ec의 signed DWORD다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint, int>? TechnologyBit { get; set; }

    /// <summary>현재 다이스 타입 DWORD다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint>? PlayerType { get; set; }

    /// <summary>현재 DWORD를 순수 조회한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint>? Owner { get; set; }

    /// <summary>고른 같은 객체의 가상 +80이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<PlayerCoreRefreshForm, int>? Level { get; set; }

    /// <summary>기술 bit 범위 진단이며 복귀 후 같은 Core 물리에 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint>? Assertion { get; set; }
}

// 현재 Core 갱신을 복원한 정적 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public static class RawPlayerCoreRefresh
{
    /// <summary>PATCH 10.78 raw 객체 슬롯 폭이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const uint Stride = 50u;

    /// <summary>슬롯 안 타입 바이트 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const uint TypeOffset = 10u;

    /// <summary>슬롯 안 상태 바이트 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const uint FlagsOffset = 11u;

    /// <summary>슬롯 안 종류 WORD 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const uint KindOffset = 18u;

    /// <summary>슬롯 안 목록 플래그 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const uint ListFlagsOffset = 20u;

    /// <summary>슬롯 안 소유자 바이트 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const uint OwnerOffset = 34u;

    /// <summary>
    /// 기술 순회를 먼저 완료하고 같은 현재 소유자의 다이스 count로 Core+5 하한을 갱신한다.
    /// </summary>
    /// <param name="hooks">외부 장치 묶음이다.</param>
    public static void RefreshPlayerCore(PlayerCoreRefreshHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Write(Need(hooks.Core)().Span, 6u, 4u, 0u);
        uint parent = Need(hooks.TechnologyParent)();
        PlayerCoreRefreshCursor cursor = Need(hooks.Begin)(parent);
        for (;;)
        {
            uint candidate = Need(cursor.Current)();
            if (candidate == 0u)
            {
                break;
            }

            PlayerCoreRefreshPool candidatePool = Need(hooks.Pool)();
            uint offset = unchecked(candidate * Stride);
            uint flags = Read(candidatePool.Bytes, unchecked(offset + ListFlagsOffset), 1u);
            uint kind = Read(candidatePool.Bytes, unchecked(offset + KindOffset), 2u);
            if ((flags & 4u) != 0u)
            {
                int bit = Need(hooks.TechnologyBit)(kind);
                if (bit != -1)
                {
                    SetTechnology(Need(hooks.Core)().Span, bit, hooks);
                }
            }

            Need(cursor.Next)();
        }

        PlayerCoreRefreshPool initial = Need(hooks.Pool)();
        int cachedEnd = Signed(initial.Capacity);
        int index = 1;
        int maximum = 1;
        Advance(ref index, cachedEnd, Need(hooks.Pool)());
        PlayerCoreRefreshPool pool = Need(hooks.Pool)();
        for (;;)
        {
            uint offset = unchecked((uint)index * Stride);
            uint address = unchecked(pool.Address + offset);
            if (address < unchecked(pool.Address + Stride) || address >= unchecked(pool.Address + Need(hooks.Pool)().Capacity * Stride))
            {
                break;
            }

            if (Read(pool.Bytes, unchecked(offset + TypeOffset), 1u) == Need(hooks.PlayerType)() && Read(pool.Bytes, unchecked(offset + OwnerOffset), 1u) == Need(hooks.Owner)())
            {
                int first = Need(hooks.Level)(Form(pool, offset));
                pool = Need(hooks.Pool)();
                if (first > maximum && (Read(pool.Bytes, unchecked(offset + FlagsOffset), 1u) & 2u) == 0u)
                {
                    maximum = Need(hooks.Level)(Form(pool, offset));
                    pool = Need(hooks.Pool)();
                }
            }

            Advance(ref index, cachedEnd, pool);
        }

        int lower = Signed(unchecked((uint)maximum - 1u));
        int firstByte = (int)Read(Need(hooks.Core)().Span, 5u, 1u);
        if (lower > firstByte)
        {
            Write(Need(hooks.Core)().Span, 5u, 1u, unchecked((uint)lower));
        }
        else
        {
            uint selected = Read(Need(hooks.Core)().Span, 5u, 1u);
            Write(Need(hooks.Core)().Span, 5u, 1u, selected);
        }
    }

    /// <summary>선택해서 도달한 자식만 그 위치에서 필수로 요구한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">현재 Core 갱신의 실제 하위 장치 미연결이다.</exception>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("현재 Core 갱신의 실제 하위 장치 미연결");
        }

        return callback;
    }

    /// <summary>원문 DWORD 비트를 signed 연산·비교에 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 값이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>실제 단일 적재 폭만 검사한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다.</param>
    /// <returns>조합한 값이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">현재 Core 갱신의 실제 적재 물리 범위다.</exception>
    private static uint Read(ReadOnlySpan<byte> bytes, uint offset, uint width)
    {
        if (offset > (uint)bytes.Length || width > (uint)bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "현재 Core 갱신의 실제 적재 물리 범위");
        }

        uint value = 0u;
        // 지정 폭 전체를 원문 레지스터에 읽는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint index = 0u; index < width; ++index)
        {
            value |= (uint)bytes[(int)(offset + index)] << (int)(index * 8u);
        }

        return value;
    }

    /// <summary>같은 쓰기 전체 폭의 호스트 물리를 확인한 뒤 그 바이트만 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="width">쓰는 폭이다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">현재 Core 갱신의 실제 저장 물리 범위다.</exception>
    private static void Write(Span<byte> bytes, uint offset, uint width, uint value)
    {
        if (offset > (uint)bytes.Length || width > (uint)bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "현재 Core 갱신의 실제 저장 물리 범위");
        }

        // 이웃 Core 필드는 그대로 남기는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint index = 0u; index < width; ++index)
        {
            bytes[(int)(offset + index)] = (byte)(value >> (int)(index * 8u));
        }
    }

    /// <summary>부모가 항상 값 1로 호출하는 실제 범위 진단·마스크 shift·같은 this 쓰기다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="core">쓸 Core 뷰다.</param>
    /// <param name="bit">고를 기술 bit다.</param>
    /// <param name="hooks">외부 장치 묶음이다.</param>
    private static void SetTechnology(Span<byte> core, int bit, PlayerCoreRefreshHooks hooks)
    {
        if (bit < 0 || bit >= 32)
        {
            Need(hooks.Assertion)(0x15au);
        }

        uint old = Read(core, 6u, 4u);
        Write(core, 6u, 4u, old | (1u << (bit & 31)));
    }

    /// <summary>실제 객체 주소와 접근 가능한 그 슬롯의 같은 물리뷰를 넘긴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="pool">읽을 Pool이다.</param>
    /// <param name="offset">자를 위치다.</param>
    /// <returns>같은 물리뷰와 주소다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">현재 Core 갱신 가상 객체 물리 범위다.</exception>
    private static PlayerCoreRefreshForm Form(PlayerCoreRefreshPool pool, uint offset)
    {
        if (offset > (uint)pool.Bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "현재 Core 갱신 가상 객체 물리 범위");
        }

        int length = Math.Min((int)Stride, pool.Bytes.Length - (int)offset);
        return new PlayerCoreRefreshForm { Bytes = pool.Bytes.AsMemory((int)offset, length), Address = unchecked(pool.Address + offset) };
    }

    /// <summary>최초와 후속 루프는 1을 증가한 뒤 cached 상한 안에서 free bit 0만 건너뛴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="index">올릴 번호다.</param>
    /// <param name="cachedEnd">cached 상한이다.</param>
    /// <param name="pool">읽을 Pool이다.</param>
    private static void Advance(ref int index, int cachedEnd, PlayerCoreRefreshPool pool)
    {
        index = Signed(unchecked((uint)index + 1u));
        while (index < cachedEnd)
        {
            if ((Read(pool.Bytes, unchecked((uint)index * Stride + FlagsOffset), 1u) & 1u) == 0u)
            {
                return;
            }

            index = Signed(unchecked((uint)index + 1u));
        }
    }
}
