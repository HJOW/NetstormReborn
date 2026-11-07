// Client 수명의 현재 CoreData 포인터 슬롯·활성 값·lastKnown 10바이트와 현재 Player뷰를 소유한다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Player;

// Client CoreData 전역을 담는 네임스페이스다. 2026-11-07 F2d Player 이식에서 추가됐다.

// Player 원문 기록 자체는 외부 실제 소유자에서 빌리고 현재·캐시 저장소만 장기 소유한다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class ClientCoreDataGlobals
{
    /// <summary>같은 실제 9기록 물리 전체다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly byte[] _records = new byte[144];

    /// <summary>같은 고정 캐시 10바이트다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly byte[] _cached = new byte[10];

    /// <summary>실제 가변 메모리 슬롯이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private Memory<byte> _current;

    /// <summary>현재 실제 Player 물리뷰 슬롯이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private Memory<byte> _players;

    /// <summary>실제 한 DWORD다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private uint _active;

    /// <summary>새 Client에서만 현재 NULL·active 0·실제 9기록·별도 lastKnown을 한 번 구성한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public ClientCoreDataGlobals()
    {
        RawPlayerCoreRecords.InitializePlayerCoreRecords(new PlayerCoreRecordsView { Bytes = _records, Origin = 0u });
        RawPlayerCoreRecords.InitializePlayerCore(_cached);
    }

    /// <summary>실제 가변 메모리 슬롯이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Memory<byte> Current
    {
        get => _current;
        set => _current = value;
    }

    /// <summary>실제 한 DWORD다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint Active
    {
        get => _active;
        set => _active = value;
    }

    /// <summary>같은 고정 캐시 10바이트를 빌린다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>캐시 배열이다.</returns>
    public byte[] Cached() => _cached;

    /// <summary>같은 실제 9기록 물리 전체를 빌린다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>장부 배열이다.</returns>
    public byte[] Records() => _records;

    /// <summary>원문 값을 같은 실제 장부에 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <param name="subscriber">쓸 구독자 번호다.</param>
    /// <param name="core">복제된 Core 10바이트다.</param>
    public void StoreMetadata(uint owner, uint subscriber, byte[] core) => RawPlayerCoreRecords.WritePlayerCoreMetadata(new PlayerCoreRecordsView { Bytes = _records, Origin = 0u }, owner, subscriber, core);

    /// <summary>현재 실제 Player 물리뷰 슬롯이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Memory<byte> Players
    {
        get => _players;
        set => _players = value;
    }

    /// <summary>실제 두 owner·known·모드를 같은 Raw helper에 빌린다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="player">현재 월드 상태다.</param>
    /// <param name="metamap">같은 현재 DWORD다.</param>
    /// <returns>현재 전역이다.</returns>
    public PlayerCoreDataGlobals Borrow(PlayerWorldState player, uint metamap)
    {
        ArgumentNullException.ThrowIfNull(player);
        return new PlayerCoreDataGlobals
        {
            TemplateOwner = player.TemplateOwner,
            BattleOwner = player.BattleOwner,
            PlayerKnown = player.PlayerKnown,
            Active = _active,
            Metamap = metamap,
            Current = _current,
            Cached = _cached,
            Players = _players,
        };
    }

    /// <summary>실제 진단·현재 재조회를 직접 실행한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="player">현재 월드 상태다.</param>
    /// <param name="metamap">같은 현재 DWORD다.</param>
    /// <param name="hooks">진단 훅이다.</param>
    /// <returns>현재 CoreData 메모리다.</returns>
    public Memory<byte> Read(PlayerWorldState player, uint metamap, PlayerCoreDataHooks hooks) => RawPlayerCoreData.ReadPlayerCoreData(Borrow(player, metamap), hooks);

    /// <summary>호출자가 원문 위치에서 명시 요청할 때만 실제 선택·정리 본문을 실행한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="player">현재 월드 상태다.</param>
    /// <param name="metamap">같은 현재 DWORD다.</param>
    /// <param name="owner">고를 소유자다.</param>
    /// <param name="templateOwner">쓸 Template 소유자다.</param>
    public void Select(PlayerWorldState player, uint metamap, uint owner, uint templateOwner)
    {
        ArgumentNullException.ThrowIfNull(player);
        PlayerCoreDataGlobals borrowed = Borrow(player, metamap);
        RawPlayerCoreData.SelectPlayerCoreData(borrowed, owner, templateOwner);
        player.TemplateOwner = borrowed.TemplateOwner;
        player.BattleOwner = borrowed.BattleOwner;
        player.PlayerKnown = borrowed.PlayerKnown;
        _active = borrowed.Active;
        _current = borrowed.Current;
    }

    /// <summary>lastKnown이 아닌 같은 실제 9기록을 빌리며 base·owner는 원문 위치에서 재조회한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="edition">원본 판본이다.</param>
    /// <param name="player">현재 월드 상태다.</param>
    /// <param name="metamap">같은 현재 DWORD다.</param>
    /// <param name="state">복원 관문이다.</param>
    /// <param name="owner">고를 소유자다.</param>
    /// <param name="hooks">진단·Chat 훅이다.</param>
    public void Restore(OriginalEdition edition, PlayerWorldState player, uint metamap, PlayerCoreRestoreState state, uint owner, PlayerCoreRestoreHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(player);
        PlayerCoreDataGlobals borrowed = Borrow(player, metamap);
        RawPlayerCoreData.RestorePlayerCoreData(edition, borrowed, new PlayerCoreRecordsView { Bytes = _records, Origin = 0u }, state, owner, hooks);
        player.TemplateOwner = borrowed.TemplateOwner;
        player.BattleOwner = borrowed.BattleOwner;
        player.PlayerKnown = borrowed.PlayerKnown;
        _active = borrowed.Active;
        _current = borrowed.Current;
    }
}
