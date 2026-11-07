// PATCH 004910a0의 이전 Player와 공장 해제 뒤 새 9기록 전체 초기화를 복원한다.
namespace Netstorm.Core.Original;

// Player 초기화 모듈을 담는 네임스페이스다. 2026-11-07 F2d Player 이식에서 추가됐다.

// Player 초기화 상수를 담는 정적 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public static class PlayerInitializationValues
{
    /// <summary>실제 Player 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public const uint PlayerCount = 9u;

    /// <summary>실제 Player 기록의 바이트 간격이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public const uint PlayerStride = 180u;

    /// <summary>malloc Player 전체 바이트 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public const uint PlayerBytes = 1620u;

    /// <summary>각 공장 블록의 바이트 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public const uint FactoryBytes = 120u;
}

// 실제 두 카운터와 현재 32비트 Player 주소 슬롯을 빌린다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerInitializationGlobals
{
    /// <summary>첫 현재 base 적재 뒤 0을 쓰는 로그인 요새 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint FortCount { get; set; }

    /// <summary>그다음 0을 쓰는 마지막 로그인 Player다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint LastLoginPlayer { get; set; }

    /// <summary>현재 주소를 각 원문 적재 위치에서 순수 조회한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint>? PlayerBase { get; set; }

    /// <summary>동일 현재 슬롯에 NULL·실제 malloc 반환 주소를 직접 기록한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint>? WritePlayerBase { get; set; }
}

// 큰 계산을 기본 성공으로 채우지 않고 현재 실제 할당·메모리·동맹 몸체에 연결한다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerInitializationHooks
{
    /// <summary>현재 주소·요청 폭의 같은 물리뷰다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint, uint, Memory<byte>>? Memory { get; set; }

    /// <summary>현재 각 Player+0의 토큰을 아홉 번 전달한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint>? FreeFactory { get; set; }

    /// <summary>그때 다시 읽은 현재 Player base를 반납한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint>? FreePlayers { get; set; }

    /// <summary>실제 주소 또는 NULL·외부 예외다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint, uint>? MallocPlayers { get; set; }

    /// <summary>실제 불투명 32 할당 주소다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint, uint>? NewFactory { get; set; }

    /// <summary>기존 실제 방향별 동맹·조건부 Chat 몸체다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint, uint>? Alliance { get; set; }
}

// Player 전체 초기화를 복원한 정적 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public static class RawPlayerInitialization
{
    /// <summary>
    /// 이전 공장 9개→이전 Player→NULL→malloc·405 DWORD 0→각 행·신규·동맹 순서를 실행한다.
    /// </summary>
    /// <param name="globals">현재 전역이다.</param>
    /// <param name="hooks">외부 장치 묶음이다.</param>
    /// <exception cref="InvalidOperationException">Player 전체 초기화의 선택 장치 미연결이다.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Player 초기화의 실제 DWORD 물리 부족이다.</exception>
    public static void InitializePlayerRecords(PlayerInitializationGlobals globals, PlayerInitializationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        uint previous = Need(globals.PlayerBase)();
        globals.FortCount = 0u;
        globals.LastLoginPlayer = 0u;
        if (previous != 0u)
        {
            // 해제 복귀 뒤 다음 행은 현재 전역 base를 다시 읽는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (uint owner = 0u; owner < PlayerInitializationValues.PlayerCount; ++owner)
            {
                uint token = Read(hooks, unchecked(Need(globals.PlayerBase)() + owner * PlayerInitializationValues.PlayerStride));
                Need(hooks.FreeFactory)(token);
            }

            Need(hooks.FreePlayers)(Need(globals.PlayerBase)());
            Need(globals.WritePlayerBase)(0u);
        }

        uint allocated = Need(hooks.MallocPlayers)(PlayerInitializationValues.PlayerBytes);
        Need(globals.WritePlayerBase)(allocated);
        // REP의 405 DWORD는 잡은 반환 주소를 순서대로 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint word = 0u; word < PlayerInitializationValues.PlayerBytes / 4u; ++word)
        {
            Write(hooks, unchecked(allocated + word * 4u), 0u);
        }

        // 각 행의 원문 전역 적재 위치와 순서를 보존하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint owner = 0u; owner < PlayerInitializationValues.PlayerCount; ++owner)
        {
            uint offset = unchecked(owner * PlayerInitializationValues.PlayerStride);
            Write(hooks, unchecked(Need(globals.PlayerBase)() + offset + 0x64u), 0u);
            Write(hooks, unchecked(Need(globals.PlayerBase)() + offset + 0x58u), 0u);
            uint record = unchecked(Need(globals.PlayerBase)() + offset);
            uint factory = Need(hooks.NewFactory)(PlayerInitializationValues.FactoryBytes);
            Write(hooks, record, factory);
            Write(hooks, unchecked(record + 4u), 30u);
            Write(hooks, unchecked(record + 8u), 0u);
            Write(hooks, unchecked(record + 0x2cu), 0u);
            Write(hooks, unchecked(record + 0x78u), 127u);
            uint mask = 1u << (int)owner;
            Write(hooks, unchecked(Need(globals.PlayerBase)() + offset + 0x54u), mask);
            uint currentMask = Read(hooks, unchecked(Need(globals.PlayerBase)() + offset + 0x54u));
            Need(hooks.Alliance)(owner, currentMask);
        }
    }

    /// <summary>도달한 실제 장치만 요구한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">Player 전체 초기화의 선택 장치 미연결이다.</exception>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("Player 전체 초기화의 선택 장치 미연결");
        }

        return callback;
    }

    /// <summary>요청한 4바이트 전체를 확인한 뒤 원문 DWORD를 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="hooks">외부 장치 묶음이다.</param>
    /// <param name="address">읽을 주소다.</param>
    /// <returns>읽은 값이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Player 초기화의 실제 DWORD 적재 물리 부족이다.</exception>
    private static uint Read(PlayerInitializationHooks hooks, uint address)
    {
        Memory<byte> view = Need(hooks.Memory)(address, 4u);
        if (view.Length != 4)
        {
            throw new ArgumentOutOfRangeException(nameof(address), "Player 초기화의 실제 DWORD 적재 물리 부족");
        }

        Span<byte> bytes = view.Span;
        uint value = 0u;
        // 원문 little endian 네 바이트만 읽는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint index = 0u; index < 4u; ++index)
        {
            value |= (uint)bytes[(int)index] << (int)(index * 8u);
        }

        return value;
    }

    /// <summary>단일 DWORD 쓰기의 호스트 물리를 먼저 확인한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="hooks">외부 장치 묶음이다.</param>
    /// <param name="address">쓸 주소다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Player 초기화의 실제 DWORD 저장 물리 부족이다.</exception>
    private static void Write(PlayerInitializationHooks hooks, uint address, uint value)
    {
        Memory<byte> view = Need(hooks.Memory)(address, 4u);
        if (view.Length != 4)
        {
            throw new ArgumentOutOfRangeException(nameof(address), "Player 초기화의 실제 DWORD 저장 물리 부족");
        }

        Span<byte> bytes = view.Span;
        // 현재 물리 네 바이트만 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint index = 0u; index < 4u; ++index)
        {
            bytes[(int)index] = (byte)(value >> (int)(index * 8u));
        }
    }
}
