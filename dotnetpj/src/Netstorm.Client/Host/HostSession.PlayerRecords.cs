using System.Runtime.InteropServices;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 실제장기Player/factory와단일세션고정뷰의초기화·인계·반납을연결한다.
namespace Netstorm.Client.Host;

/// <summary>Player소비자가현재배열/동맹/카운터를복사하지않고장기소유자에서빌린다.</summary>
public sealed partial class HostSession
{
    /// <summary>이세션이명시적으로빌린실제장기소유자다. 독립조합은null이다.</summary>
    private ClientPlayerRecords? _playerRecordsOwner;
    /// <summary>모든실제Player소비자보다먼저획득하고나중에반납하는단일대여다.</summary>
    private ClientPlayerRecords.PlayerRecordsUse? _playerRecordsUse;
    /// <summary>현재고정뷰의실제전체배열이다. 명시초기화가끝난뒤만할당한다.</summary>
    private byte[] _playerRecords = [];
    /// <summary>같은실제방향별표또는독립조합소유표다.</summary>
    private uint[] _alliances = [];
    /// <summary>반납한세션이이전고정뷰를다시소비하지못하게한다.</summary>
    private bool _playerRecordsReleased;
    /// <summary>독립조합의이전카운터주소를유지한다.</summary>
    private readonly UIntBox _standaloneFortCount = new();
    /// <summary>독립조합의이전마지막로그인주소를유지한다.</summary>
    private readonly UIntBox _standaloneLastLoginPlayer = new();
    /// <summary>원문초기화/로그인이함께쓰는같은요새카운터다.</summary>
    public UIntBox FortCountBox => _playerRecordsOwner?.FortCountBox ?? _standaloneFortCount;
    /// <summary>원문초기화/로그인이함께쓰는같은마지막Player다.</summary>
    public UIntBox LastLoginPlayerBox => _playerRecordsOwner?.LastLoginPlayerBox ?? _standaloneLastLoginPlayer;
    /// <summary>같은명시소유자정체성을관찰한다. 조회로초기화하지않는다.</summary>
    public ClientPlayerRecords? PlayerRecordsOwner => _playerRecordsOwner;

    /// <summary>원문Core0선행과실제초기화또는준비인계뒤정확한같은배열을빌린다.</summary>
    private void AcquirePlayerRecordViews(ClientPlayerRecords? records, bool adoptInitializedPlayers)
    {
        if (records is null)
        {
            if (adoptInitializedPlayers) throw new ArgumentException("준비Player인계의실제소유자누락", nameof(records));
            _playerRecords = new byte[PlayerRecordBytes]; _alliances = new uint[AllianceDwords]; return;
        }
        if (records.Stride != PlayerRecordStride || !ReferenceEquals(records.BoundCoreData, Gameplay.CoreData))
            throw new ArgumentException("미션Player/Core의실제장기소유자또는판본불일치", nameof(records));
        // 기존대여거부는현재known/Core에손대기전에수행한다.
        ClientPlayerRecords.PlayerRecordsUse use = records.Acquire();
        try
        {
            if (adoptInitializedPlayers) use.AdoptCurrent();
            else
            {
                MissionPlayer.PlayerKnown = 0;
                Gameplay.CoreData.Select(MissionPlayer, OwnerMode.Challenge ? 1u : 0u, 0, 1);
                use.Initialize((owner, mask) =>
                {
                    var initial = new RawPlayerParticipation(OriginalEdition.Patch1078, records.Records, records.Alliances,
                        HiddenPlayers, new(), new());
                    initial.BindCurrentInputs(new Netstorm.Core.Original.PlayerParticipationReads
                    {
                        LocalOwner = () => MissionPlayer.BattleOwner, PlayerKnown = () => MissionPlayer.PlayerKnown,
                        Battle = () => OwnerMode.Battle, Metamap = () => OwnerMode.Challenge, LocalTransport = () => false,
                    });
                    initial.SetAlliance(owner, mask);
                });
            }
            if (!MemoryMarshal.TryGetArray((ReadOnlyMemory<byte>)records.Records, out ArraySegment<byte> physical)
                || physical.Array is null || physical.Offset != 0 || physical.Count != PlayerRecordBytes || physical.Array.Length != PlayerRecordBytes)
                throw new InvalidOperationException("미션Player의정확한실제배열대여미연결");
            _playerRecords = physical.Array; _alliances = records.Alliances; _playerRecordsOwner = records; _playerRecordsUse = use;
        }
        catch { use.Dispose(); throw; }
    }

    /// <summary>정상완료된실제Player의아홉공장헤더를같은후처리목록에연결한다.</summary>
    private void BindPlayerFactoryViews()
    {
        if (_playerRecordsUse is not { } use) return;
        // 모든owner의+0/+4/+8을현재getter로빌리며초기공장내용/개수는복사하지않는다.
        for (uint owner = 0; owner < 9; owner++) use.ReuseOrBindFactory(TerrainBookkeeping.OwnerFactories[owner], owner);
    }

    /// <summary>실제자식정리의정상복귀뒤에만고정뷰대여를반납한다. 실패시대여를유지한다.</summary>
    public void ReleasePlayerRecordViews()
    {
        EnsureRuntimeCanRelease(); if (_playerRecordsReleased) return;
        ReleaseMissionAiBrains();
        ReleaseConstructionWorld();
        ReleasePlayerProcesses();
        _fixedLoginModules = null; _fixedAllianceParticipation = null; _fixedAllianceEffects = null;
        _playerRecordsUse?.Dispose(); _playerRecordsUse = null; _playerRecordsReleased = true;
    }
}
