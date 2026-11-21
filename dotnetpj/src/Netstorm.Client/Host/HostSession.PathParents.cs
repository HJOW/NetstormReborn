using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 사제/공간/내용물/Host를 실제 정지·도착 부모에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>경로 부모의 실제 본체와 현재 Host 경로 조회를 한 세션 수명으로 보관한다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 실제 사제의 현재 정지 본체다.</summary>
    private RawPriestHalt? _pathPriestHalt;
    /// <summary>같은 현재 내용물/Graph를 읽는 공통 도착 본체다.</summary>
    private RawCarrierArrival? _pathCarrierArrival;
    /// <summary>같은 실제 섬/신전 getter를 읽는 사제 도착 본체다.</summary>
    private RawPriestArrival? _pathPriestArrival;
    /// <summary>현재 종속 Form을 찾으며 경로 포인터를 캐시하지 않는 실제 조회기다.</summary>
    private RawContainedLookup? _pathParentLookup;
    /// <summary>실제 정지 본체이며 미설치/반납 뒤에는 실패한다.</summary>
    public RawPriestHalt PathPriestHalt => FixedPacketNeed(_pathPriestHalt);
    /// <summary>실제 Carrier 도착 본체이며 미설치/반납 뒤에는 실패한다.</summary>
    public RawCarrierArrival PathCarrierArrival => FixedPacketNeed(_pathCarrierArrival);
    /// <summary>실제 사제 도착 본체이며 미설치/반납 뒤에는 실패한다.</summary>
    public RawPriestArrival PathPriestArrival => FixedPacketNeed(_pathPriestArrival);
    /// <summary>실제 공통 공간·현재 모드/권한/TYPE와 사제 HP/Carrier를 정지 부모에 연결한다.</summary>
    public RawPriestHalt BuildPriestHaltWorldDevices(PriestHaltHooks effects)
    {
        if (_pathPriestHalt is not null) throw new InvalidOperationException("사제 Halt 중복 설치");
        _ = PathSearch; RawPriestHalt result = new(Pool, FixedPacketNeed(_terrainHash), FixedPacketNeed(_terrainPopTypes), FixedPacketNeed(_terrainSpots), PriestWorld.CarrierCheck, PriestWorld.PriestState, Reward,
            new() { SharedBattle = () => BattleActive() != 0, SharedBoss = () => Gameplay.Authority.Boss, SharedCarrierType = GameAux.RainWalkerTypeBox, SharedNuggetCarrierType = GameAux.EmptyCarrierTypeBox },
            effects with { Unpop = UnpopWorldObject, Pop = PopWorldObject });
        _pathPriestHalt = result; return result;
    }
    /// <summary>실제 현재 종속 Form/Host를 매번 조회하고 같은 부모/풀/종류만 경로로 돌려준다.</summary>
    public RawPathRuntime? CurrentPathRuntime(int parent)
    {
        if (_playerRecordsReleased || ProcessBindingsReturned() || _playerProcessRuntime is null) throw new ObjectDisposedException(nameof(HostSession));
        if (parent < 5 || parent >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(parent));
        ReadOnlySpan<byte> raw = Pool.Slot(parent); uint movement = PriestWorld.MovementProcessType(raw[10]);
        // Source의복원된실제부모생성자만허용한다.187의전용부모는전체PathBindings와함께후속으로잇는다.
        (uint Table, uint Movement) expected = raw[10] switch
        {
            158 => (0x50f210, 54), 70 => (0x508cc8, 24), 134 => (0x5070c0, 23), 105 => (0x509810, 39),
            133 => (0x50e568, 40), 111 or 141 => (0x5040c0, 42), 146 => (0x504198, 42), 135 => (0x503e50, 41),
            137 => (0x503d80, 38), 156 => (0x503f20, 38), _ => throw new InvalidOperationException("현재 Path의 다른 실제 부모 생성자 미연결"),
        };
        if (Pool.Edition != OriginalEdition.Patch1078 || (raw[11] & 9) != 0 || BinaryPrimitives.ReadUInt32LittleEndian(raw) != expected.Table || movement != expected.Movement) throw new InvalidOperationException("현재 Path의 실제 부모TYPE/가상표/이동종류 오류");
        int form = FixedPacketNeed(_pathParentLookup).Find(parent, movement); if (form == 0) return null;
        if (PlayerProcesses.FromForm(form) is not RawPathProcess process || !ReferenceEquals(Pool, process.Pool) || process.Parent() != parent || process.Form() != form) throw new InvalidOperationException("현재 실제 Host Path/Form/부모 계약 오류");
        return process.Runtime;
    }
    /// <summary>실제 현재 위치의 일반 finder에서 원문 부류/시작 단계의 첫 후보만 선택한다.</summary>
    private int PathGenusAt(float x, float y, uint genus)
    {
        if (!float.IsFinite(x) || !float.IsFinite(y) || (double)x < int.MinValue || (double)x >= 2147483648.0 || (double)y < int.MinValue || (double)y >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(x));
        var finder = new RawSquidFinder(Pool, FixedPacketNeed(_terrainHash), FixedPacketNeed(_terrainPopTypes));
        // 현재 후보의TYPE/부류를 매번 읽고 별도의free/dead 필터는 추가하지 않는다.
        for (int candidate = finder.Begin(new((int)x, (int)y, (int)x, (int)y), (genus & 6) == 0 ? 1u : 0u); candidate != 0; candidate = finder.Next()) if ((SquidPacketType(Pool.Slot(candidate)[10]).Flags2 & genus) != 0) return candidate;
        return 0;
    }
    /// <summary>큰 운반/회피/가상삭제 경계는 유지하고 현재 Path·사제/내용물/프레임을 같은 Host에 연결한다.</summary>
    public RawCarrierArrival BuildCarrierArrivalWorldDevices(RawPathCleanup cleanup, RawPlayerPlacementAnchor anchor, CarrierArrivalState state, CarrierArrivalHooks effects)
    {
        if (_pathCarrierArrival is not null) throw new InvalidOperationException("Carrier 도착 중복 설치");
        _ = PathSearch;
        RawContainedLookup lookup = new(Pool, (_, _) => throw new InvalidOperationException("현재 경로 조회기의 삭제는 실제 가상 분배를 사용해야 합니다"));
        state.SharedBoss = () => Gameplay.Authority.Boss; state.SharedNuggetType = GameAux.NuggetTypeBox; state.SharedGeyserType = GameAux.GeyserTypeBox;
        state.SharedAllowAlliedPickup = () => GameAux.AllowAnyCapture;
        state.Debug.Bind(() => AiChecking.Read());
        var result = new RawCarrierArrival(Pool, FixedPacketNeed(_terrainHash), FixedPacketNeed(_terrainPopTypes), PriestWorld.Carrier, cleanup, PriestWorld.PriestState, anchor, FixedPacketNeed(_fortContained), FixedPacketNeed(_islandNeighbors), state, effects with
        {
            Path = CurrentPathRuntime,
            GenusAt = PathGenusAt,
            CurrentControlled = owner => owner < 9 ? BinaryPrimitives.ReadUInt32LittleEndian(PlayerRecords.AsSpan((int)owner * 180 + 0xb0, 4)) != 0 : throw new ArgumentOutOfRangeException(nameof(owner)),
            CurrentAlliance = (first, second) => first < 9 && second < 9 ? Alliances[first * 9 + second] : throw new ArgumentOutOfRangeException(nameof(first)),
        });
        _pathParentLookup = lookup; _pathCarrierArrival = result; return result;
    }
    /// <summary>같은 실제 지면/섬/내용물/공통 도착을 사제 wrapper에 연결한다. 자기삭제/중립귀환은 실제 장치가 필수다.</summary>
    public RawPriestArrival BuildPriestArrivalWorldDevices(RawCanonPlacementTerrain terrain, RawIslandRuntime islands, PriestArrivalState state, PriestArrivalHooks effects)
    {
        if (_pathPriestArrival is not null) throw new InvalidOperationException("사제 도착 중복 설치");
        state.SharedBoss = () => Gameplay.Authority.Boss; state.SharedLoading = () => _worldLoadingDepth != 0;
        state.SharedSuppressNeutralReturn = () => Gameplay.World.MissionPost;
        var result = new RawPriestArrival(Pool, PriestWorld.PriestState, Reward, terrain, islands, FixedPacketNeed(_islandNeighbors), FixedPacketNeed(_fortContained), PathCarrierArrival, state, effects);
        _pathPriestArrival = result; return result;
    }
    /// <summary>실제 프로세스를 먼저 정리한 뒤 부모/현재조회기의 참조만 반납한다.</summary>
    private void ReleasePathParentDevices() { _pathPriestHalt = null; _pathCarrierArrival = null; _pathPriestArrival = null; _pathParentLookup = null; }
}
