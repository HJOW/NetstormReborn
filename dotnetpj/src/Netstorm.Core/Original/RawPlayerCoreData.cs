// Player의 현재 CoreData 조회와 선택·정리·복원을 실제 같은 물리에 복원한다.
using System.Runtime.InteropServices;

namespace Netstorm.Core.Original;

// Player CoreData 모듈을 담는 네임스페이스다. 2026-11-07 F2d Player 이식에서 추가됐다.

// 원문 현재 전역을 직접 빌리는 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreDataGlobals
{
    /// <summary>선택 인자를 가장 먼저 받는 별도 Template 소유자다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint TemplateOwner { get; set; }

    /// <summary>그 다음 인자를 받는 전투 소유자다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint BattleOwner { get; set; }

    /// <summary>비영 소유자 선택 때 1이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint PlayerKnown { get; set; }

    /// <summary>소유자 0 정리 때 0이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint Active { get; set; }

    /// <summary>같은 현재 DWORD로 얻기 진단·선택 분기를 판정한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint Metamap { get; set; }

    /// <summary>현재 포인터 역할의 가변 뷰다. 실제 Player·캐시·외부 물리를 빌린다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Memory<byte> Current { get; set; }

    /// <summary>같은 고정 10바이트다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public byte[] Cached { get; set; } = new byte[10];

    /// <summary>현재 Player 9 물리뷰 슬롯이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Memory<byte> Players { get; set; }
}

// 실제 원문 진단만 외부에 전달하는 훅 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreDataHooks
{
    /// <summary>원문 진단 행 번호를 전달한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint>? Assertion { get; set; }
}

// 복원의 선행 missionPost와 후행 savedGame·현재 checking이다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreRestoreState
{
    /// <summary>선행 missionPost 관문이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public RawCheckingFlag MissionPost { get; set; } = new();

    /// <summary>후행 savedGame 관문이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public RawCheckingFlag SavedGame { get; set; } = new();

    /// <summary>현재 checking 관문이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
}

// 선택한 원문 진단과 저장 게임의 Chat 재초기화 훅 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreRestoreHooks
{
    /// <summary>진단의 표현식·파일·행과 정상 복귀를 보존한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>실제 새 manager·명령·Config 구성이다. CD는 호출하지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action? InitializeChat { get; set; }
}

// Player CoreData 조회·선택·복원을 복원한 정적 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public static class RawPlayerCoreData
{
    /// <summary>실제 Player 기록의 바이트 간격이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const uint PlayerStride = 0xb4u;

    /// <summary>기록 안 Core 10바이트의 상대 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const uint CoreRelative = 0x6cu;

    /// <summary>
    /// 메타맵의 실제 포인터 동일성 진단 뒤 현재 포인터를 재조회해 같은 가변뷰를 반환한다.
    /// </summary>
    /// <param name="globals">현재 전역이다.</param>
    /// <param name="hooks">진단 훅이다.</param>
    /// <returns>현재 CoreData 메모리다.</returns>
    /// <exception cref="InvalidOperationException">현재 CoreData 진단 장치 누락이다.</exception>
    public static Memory<byte> ReadPlayerCoreData(PlayerCoreDataGlobals globals, PlayerCoreDataHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (globals.Metamap != 0u && !SameAddress(globals.Current, globals.Cached))
        {
            if (hooks.Assertion is null)
            {
                throw new InvalidOperationException("현재 CoreData 진단 장치 누락");
            }

            hooks.Assertion(0x55u);
        }

        return globals.Current;
    }

    /// <summary>
    /// 두 소유자→0이면 이전 현재 10바이트를 캐시에 옮기고, 비영은 실제 alias를 잡는다.
    /// </summary>
    /// <param name="globals">현재 전역이다.</param>
    /// <param name="owner">고를 소유자다.</param>
    /// <param name="templateOwner">쓸 Template 소유자다.</param>
    /// <exception cref="ArgumentOutOfRangeException">CoreData 선택의 실제 Player 물리 부족이다.</exception>
    public static void SelectPlayerCoreData(PlayerCoreDataGlobals globals, uint owner, uint templateOwner)
    {
        ArgumentNullException.ThrowIfNull(globals);
        globals.TemplateOwner = templateOwner;
        globals.BattleOwner = owner;
        if (owner == 0u)
        {
            Memory<byte> source = globals.Current;
            if (HasAddress(source))
            {
                CopyCorePiece(globals.Cached, source.Span, 0u, 4u);
                CopyCorePiece(globals.Cached, source.Span, 4u, 4u);
                CopyCorePiece(globals.Cached, source.Span, 8u, 2u);
            }

            globals.Current = default;
            globals.Active = 0u;
            return;
        }

        globals.PlayerKnown = 1u;
        if (globals.Metamap != 0u)
        {
            globals.Current = globals.Cached;
            return;
        }

        uint offset = unchecked(owner * PlayerStride + CoreRelative);
        Memory<byte> players = globals.Players;
        if (offset > (uint)players.Length || 10u > (uint)players.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "CoreData 선택의 실제 Player 물리 부족");
        }

        globals.Current = players.Slice((int)offset, 10);
    }

    /// <summary>
    /// 현재 9기록 Core를 Player에 4·4·2로 복원하고 고른 active·current·저장 바이트를 갱신한다.
    /// </summary>
    /// <param name="edition">원본 판본이다.</param>
    /// <param name="globals">현재 전역이다.</param>
    /// <param name="records">장부 뷰다.</param>
    /// <param name="state">복원 관문이다.</param>
    /// <param name="owner">고를 소유자다.</param>
    /// <param name="hooks">진단·Chat 훅이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">CoreData 복원의 실제 물리 범위다.</exception>
    /// <exception cref="InvalidOperationException">CoreData 복원의 진단·Chat 장치 누락이다.</exception>
    public static void RestorePlayerCoreData(OriginalEdition edition, PlayerCoreDataGlobals globals, PlayerCoreRecordsView records, PlayerCoreRestoreState state, uint owner, PlayerCoreRestoreHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(records);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (state.MissionPost)
        {
            return;
        }

        bool patch = edition == OriginalEdition.Patch1078;
        uint stride = patch ? 0xb4u : 0xacu;
        uint coreOffset = patch ? 0x6cu : 0x64u;
        uint savedOffset = coreOffset + 5u;
        uint playerOffset = unchecked(owner * stride);
        uint recordOffset = unchecked(records.Origin + owner * 16u);
        uint initialKnown = patch ? globals.PlayerKnown : 0u;
        Memory<byte> players = globals.Players;
        uint cdOwner = patch ? 0u : globals.BattleOwner;
        byte saved = (byte)RestoreRead(players.Span, unchecked(playerOffset + savedOffset), 1u);
        bool local = patch ? initialKnown == 0u || owner == globals.BattleOwner : globals.PlayerKnown == 0u || owner == cdOwner;
        uint checking = patch ? state.Checking.Read() : 0u;
        if (local)
        {
            globals.Active = 1u;
        }

        if (checking != 0u)
        {
            uint subscriber = RestoreRead(players.Span, unchecked(playerOffset + 0x2cu), 4u);
            uint cachedSubscriber = RestoreRead(records.Bytes, recordOffset, 4u);
            if (subscriber != cachedSubscriber)
            {
                if (hooks.Assertion is null)
                {
                    throw new InvalidOperationException("CoreData 복원의 진단 장치 누락");
                }

                hooks.Assertion("getPlayer(playerId)->subscriberId == cachedCoreData[playerId].subId", "\\Ns\\O\\Player.cpp", local ? 0x224u : 0x21fu);
                players = globals.Players;
            }
        }

        Memory<byte> destination = CoreAt(players, unchecked(playerOffset + coreOffset));
        Memory<byte> source = CoreAt(records.Bytes, unchecked(recordOffset + 4u));
        CopyCorePiece(destination.Span, source.Span, 0u, 4u);
        CopyCorePiece(destination.Span, source.Span, 4u, 4u);
        CopyCorePiece(destination.Span, source.Span, 8u, 2u);
        if (local)
        {
            if (patch)
            {
                RestoreCurrent(globals, globals.Players, unchecked(globals.BattleOwner * stride + coreOffset));
            }
            else
            {
                RestoreCurrent(globals, players, unchecked(cdOwner * stride + coreOffset));
            }
        }

        if (!state.SavedGame)
        {
            return;
        }

        Memory<byte> restored = patch ? globals.Players : players;
        uint offset = unchecked(playerOffset + savedOffset);
        if (offset >= (uint)restored.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "CoreData 저장 바이트 복원의 실제 쓰기 물리 범위");
        }

        restored.Span[(int)offset] = saved;
        if (patch)
        {
            if (hooks.InitializeChat is null)
            {
                throw new InvalidOperationException("CoreData 복원의 Chat 재초기화 장치 누락");
            }

            hooks.InitializeChat();
        }
    }

    /// <summary>두 뷰가 같은 물리 주소를 가리키는지 본다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="first">첫 뷰다.</param>
    /// <param name="second">둘째 뷰다.</param>
    /// <returns>같은 주소면 참이다.</returns>
    public static bool SameAddress(Memory<byte> first, Memory<byte> second)
    {
        if (!MemoryMarshal.TryGetArray(first, out ArraySegment<byte> a) || !MemoryMarshal.TryGetArray(second, out ArraySegment<byte> b))
        {
            return false;
        }

        return ReferenceEquals(a.Array, b.Array) && a.Offset == b.Offset;
    }

    /// <summary>뷰가 실제 물리 주소를 가졌는지 본다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="view">확인할 뷰다.</param>
    /// <returns>주소가 있으면 참이다.</returns>
    public static bool HasAddress(Memory<byte> view)
    {
        if (!MemoryMarshal.TryGetArray(view, out ArraySegment<byte> segment))
        {
            return false;
        }

        return segment.Array is not null && segment.Array.Length != 0;
    }

    /// <summary>한 원문 load의 전체 폭을 확인한 뒤 같은 폭을 캐시에 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="destination">쓸 곳이다.</param>
    /// <param name="source">읽을 곳이다.</param>
    /// <param name="offset">읽고 쓸 위치다.</param>
    /// <param name="width">폭이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">CoreData 원문 load·store 물리 범위다.</exception>
    private static void CopyCorePiece(Span<byte> destination, Span<byte> source, uint offset, uint width)
    {
        if (offset > (uint)source.Length || width > (uint)source.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "현재 CoreData 원문 load 물리 범위");
        }

        Span<byte> word = stackalloc byte[4];
        // 대상 쓰기보다 먼저 전체 원본을 읽는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint index = 0u; index < width; ++index)
        {
            word[(int)index] = source[(int)(offset + index)];
        }

        if (offset > (uint)destination.Length || width > (uint)destination.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "CoreData 캐시 원문 store 물리 범위");
        }

        // 앞선 쓰기의 효과를 다음 원본 읽기에 유지하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint index = 0u; index < width; ++index)
        {
            destination[(int)(offset + index)] = word[(int)index];
        }
    }

    /// <summary>원문 포인터에 해당하는 같은 물리 뒤쪽 메모리를 빌린다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">자를 메모리다.</param>
    /// <param name="offset">시작 위치다.</param>
    /// <returns>시작 위치 뒤의 메모리다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">CoreData 복원의 실제 시작 물리 범위다.</exception>
    private static Memory<byte> CoreAt(Memory<byte> bytes, uint offset)
    {
        if (offset > (uint)bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "CoreData 복원의 실제 시작 물리 범위");
        }

        return bytes.Slice((int)offset);
    }

    /// <summary>원문 포인터에 해당하는 같은 물리 뒤쪽 메모리를 빌린다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">자를 바이트다.</param>
    /// <param name="offset">시작 위치다.</param>
    /// <returns>시작 위치 뒤의 메모리다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">CoreData 복원의 실제 시작 물리 범위다.</exception>
    private static Memory<byte> CoreAt(byte[] bytes, uint offset)
    {
        ArgumentNullException.ThrowIfNull(bytes);
        if (offset > (uint)bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "CoreData 복원의 실제 시작 물리 범위");
        }

        return bytes.AsMemory((int)offset);
    }

    /// <summary>선택된 원문 바이트·DWORD load만 검사하고 현재 값을 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다.</param>
    /// <returns>조합한 값이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">CoreData 복원의 실제 load 물리 범위다.</exception>
    private static uint RestoreRead(Span<byte> bytes, uint offset, uint width)
    {
        if (offset > (uint)bytes.Length || width > (uint)bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "CoreData 복원의 실제 load 물리 범위");
        }

        uint value = 0u;
        // 동일 원문 load의 전체 폭을 읽는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint index = 0u; index < width; ++index)
        {
            value |= (uint)bytes[(int)(offset + index)] << (int)(index * 8u);
        }

        return value;
    }

    /// <summary>원문 현재 포인터 쓰기를 Player 10바이트의 같은 alias로 저장한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="globals">현재 전역이다.</param>
    /// <param name="players">읽을 Player 물리다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <exception cref="ArgumentOutOfRangeException">CoreData 복원 뒤 현재 포인터의 실제 물리 범위다.</exception>
    private static void RestoreCurrent(PlayerCoreDataGlobals globals, Memory<byte> players, uint offset)
    {
        if (offset > (uint)players.Length || 10u > (uint)players.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "CoreData 복원 뒤 현재 포인터의 실제 물리 범위");
        }

        globals.Current = players.Slice((int)offset, 10);
    }
}
