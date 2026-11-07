// Player 전체 초기화의 선택 장치 요구와 앞선 부분 효과를 검증한다.
using System.Globalization;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawPlayerInitialization의 장치 계약을 검증하는 테스트 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerInitializationTests
{
    /// <summary>명시 Player 물리 식별자다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private static readonly uint[] Bases = [0x12200000u, 0x12201000u, 0x12202000u];

    /// <summary>실제 Player 기록 바이트 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int PlayerBytes = 1620;

    /// <summary>주소→물리 저장소다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly Dictionary<uint, byte[]> _blocks = new();

    /// <summary>호출 순서 기록이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly uint[] _calls = new uint[5];

    /// <summary>현재 base 슬롯이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private uint _base;

    /// <summary>현재 base 슬롯에 쓰는 갈고리다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly PlayerInitializationGlobals _globals = new();

    /// <summary>세 블록을 미리 확보한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="old">이전 base 번호다. 3이면 0이다.</param>
    private void Prepare(uint old)
    {
        _blocks.Clear();
        // 세 주소를 각각 독립 저장소에 두는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (uint address in Bases)
        {
            _blocks[address] = new byte[PlayerBytes];
        }

        _base = old == 3u ? 0u : Bases[(int)old];
        _globals.FortCount = 0x12345678u;
        _globals.LastLoginPlayer = 0x89abcdefu;
        _globals.PlayerBase = () => _base;
        _globals.WritePlayerBase = address => _base = address;
        Array.Clear(_calls);
    }

    /// <summary>현재 실제 할당에서 요청한 영역의 복사본을 빌린다. 이 검사의 쓰기는 관찰하지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="address">읽을 주소다.</param>
    /// <param name="size">읽을 폭이다.</param>
    /// <returns>물리 복사본이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">미등록 실제 물리다.</exception>
    private byte[] Memory(uint address, uint size)
    {
        if (address < PlayerBytes)
        {
            throw new ArgumentOutOfRangeException(nameof(address), "초기화 입력의 NULL 물리");
        }

        // 세 블록 안에서 포함하는 저장소를 찾는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach ((uint bottom, byte[] block) in _blocks)
        {
            if (address >= bottom && (ulong)address + size <= (ulong)bottom + (uint)block.Length)
            {
                byte[] view = new byte[size];
                Array.Copy(block, (int)(address - bottom), view, 0, (int)size);
                return view;
            }
        }

        throw new ArgumentOutOfRangeException(nameof(address), "Player 초기화의 미등록 실제 물리");
    }

    /// <summary>실제 외부 장치를 묶는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>훅 묶음이다.</returns>
    private PlayerInitializationHooks Hooks()
    {
        return new PlayerInitializationHooks
        {
            Memory = (address, size) => Memory(address, size),
            FreeFactory = token => ++_calls[0],
            FreePlayers = token => ++_calls[1],
            MallocPlayers = size => { ++_calls[2]; return Bases[2]; },
            NewFactory = size => { ++_calls[3]; return 0x12310000u + (_calls[3] - 1u) * 0x100u; },
            Alliance = (owner, mask) => ++_calls[4],
        };
    }

    /// <summary>선택하지 않은 해제는 요구하지 않으며 선택 누락·짧은 DWORD는 앞선 실제 효과를 남긴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void RequiredDevicesKeepEarlierPartialEffects()
    {
        Prepare(3u);
        PlayerInitializationHooks hooks = Hooks();
        hooks.FreeFactory = null;
        hooks.FreePlayers = null;
        hooks.MallocPlayers = null;
        bool rejected = false;
        try
        {
            RawPlayerInitialization.InitializePlayerRecords(_globals, hooks);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }

        Assert.True(rejected && _globals.FortCount == 0u && _globals.LastLoginPlayer == 0u && _base == 0u && _calls[0] == 0u && _calls[1] == 0u && _calls[3] == 0u);
        Prepare(3u);
        byte[] before = (byte[])_blocks[Bases[2]].Clone();
        PlayerInitializationHooks shortHooks = Hooks();
        shortHooks.Memory = (address, size) =>
        {
            byte[] view = Memory(address, size);
            byte[] shortView = new byte[3];
            Array.Copy(view, 0, shortView, 0, 3);
            return shortView;
        };
        rejected = false;
        try
        {
            RawPlayerInitialization.InitializePlayerRecords(_globals, shortHooks);
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }

        Assert.True(rejected && _base == Bases[2] && _globals.FortCount == 0u && _globals.LastLoginPlayer == 0u && _calls[2] == 1u && _calls[3] == 0u && _blocks[Bases[2]].AsSpan().SequenceEqual(before));
    }
}
