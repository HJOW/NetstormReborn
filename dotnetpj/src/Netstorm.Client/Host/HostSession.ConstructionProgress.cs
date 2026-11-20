using Netstorm.Core.Original;

// 표시와 같은 Host·보상HP·현재전역으로 건설 진행과 완료의 실제 공간 효과를 조립한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession.cpp ConnectBuilding의 실제 진행/밀어내기 결선이다.</summary>
public sealed partial class HostSession
{
    /// <summary>미션보다 오래 빌린 현재 장치와 같은 진행 본체다.</summary>
    private RawConstructionProcess? _constructionProgress;
    /// <summary>실제 탐색기/관계/점유 지도로 완료 발자국의 유닛을 옮긴다.</summary>
    private RawConstructionEffects? _constructionDisplacement;
    /// <summary>현재 시각/역할/진단이 연결된 실제 진행 상태다.</summary>
    private ConstructionProcessState? _constructionProgressState;
    /// <summary>같은 실제30번 진행 본체를 건설 요청/업그레이드가 호출한다.</summary>
    public RawConstructionProcess WorldConstructionProgress => _constructionProgress ?? throw new InvalidOperationException("호스트 건설 진행 미연결");
    /// <summary>독립 사본 대신 실제 세션이 사용하는 상태를 진단 소비자에 빌린다.</summary>
    public ConstructionProcessState WorldConstructionProgressState => _constructionProgressState ?? throw new InvalidOperationException("호스트 건설 진행 상태 미연결");

    /// <summary>외부 화면/음향만 입력받고 같은 실제 표시·Host·HP·현재 역할·공간·밀어내기를 연결한다.</summary>
    public RawConstructionProcess BuildConstructionProgressWorldDevices(ConstructionProcessHooks devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        if (_constructionProgress is not null) throw new InvalidOperationException("호스트 건설 진행 중복 연결");
        _ = BindLivePlayerWorld(); ConstructionVisualSystem visual = WorldConstruction;
        ConstructionProcessHooks actual = devices.Copy();
        Action<int, uint> destroy = BuildSessionDestroy("건설 효과", PlayerProcesses);
        var lookup = new RawContainedLookup(Pool, destroy);
        var effects = new RawConstructionEffects(Pool, _terrainHash!, _terrainPopTypes!, _terrainSpots!, new()
        {
            CurrentAlliances = () => Alliances,
            SharedIslandType = () => BridgeState.IslandType, SharedNoIslandType = () => BridgeState.NoIslandType,
            SharedStalagType = () => BridgeState.StalagType, SharedServer = () => Gameplay.Authority.Server,
        }, new()
        {
            PathType = type => PriestWorld.MovementProcessType(type),
            ClearPath = (sid, type) => { _ = lookup.Find(sid, type, 0, true); },
            Unpop = UnpopWorldObject, Pop = PopWorldObject, Destroy = destroy,
            Sound = (x, y) => actual.PlayAt!(x, y, "islandFall.wav", 0),
            Notify = sid => PriestWorld.FallWalker(sid),
            // 이 private 본체는Displace만 소비한다. DropIsland의 실제낙하/전송은 별도요청 연결에서 잇는다.
            Fall = _ => throw new InvalidOperationException("건설 섬 정리의 실제 낙하 표시 미연결(R6)"),
            Broadcast = (_, _, _, _) => throw new InvalidOperationException("건설 섬 정리의 실제 원격 전송 미연결(R6)"),
        });
        actual.Reserve = static () => true; // 실제 관리 프로세스 생성/부착의 확보 실패는 예외로 전달한다.
        actual.UpdateDisplay = sid => _terrainDisplay!.Update(Pool.Slot(sid));
        actual.RemoveContained = (sid, type) => { _ = lookup.Find(sid, type, 0, true); };
        actual.Unpop = UnpopWorldObject; actual.Repop = RepopWorldObject;
        actual.FormRobust = form => RawFormLinked.Check(Pool, form, sid => RawFormLinked.ReadProcessFormParent(Pool, sid));
        actual.Assertion = (expression, file, line) => throw new InvalidOperationException($"건설 원본 진단: {expression} {file}:{line}");
        actual = visual.MakeProcessHooks(Pool, RawConstructionProcess.MakeHooks(Pool, Reward, effects, actual));
        var state = new ConstructionProcessState
        {
            CurrentTime = () => ProcessTime.Value, CurrentProcessType = () => GameAux.ConstructionProcessTypeBox.Value,
            CurrentLocalPlayer = () => MissionPlayer.BattleOwner, SharedBoss = () => Gameplay.Authority.Boss,
            SharedPushGenus = () => GameAux.ConstructionDisplaceGenusBox.Value,
            Checking = new(() => AiChecking.Read()), QuickBuild = new(() => GameAux.QuickBuildBox.Value),
            QuickVortex = new(() => GameAux.QuickVortexBox.Value),
        };
        var progress = new RawConstructionProcess(Pool, _terrainPopTypes!, state, actual, PlayerProcesses);
        _constructionDisplacement = effects; _constructionProgressState = state; _constructionProgress = progress; return progress;
    }

    /// <summary>현재 완료 발자국의 실제 유닛/경로/공간 이동을 다른 건설 소비자도 공유한다.</summary>
    public bool DisplaceConstructionUnits(float x, float y, uint type, uint owner) =>
        (_constructionDisplacement ?? throw new InvalidOperationException("호스트 건설 밀어내기 미연결")).Displace(x, y, type, owner);

    /// <summary>같은 진행 본체의 프로세스만 실제Form 삭제로 반납하고 다른Kernel 소유자를 보존한다.</summary>
    private void ReleaseConstructionProgressWorld()
    {
        if (_constructionProgress is null) return;
        bool found;
        do
        {
            found = false;
            // 각삭제가현재배열을바꾸므로다시첫PID부터확인한다. 세션과본체의정체성을함께검사한다.
            for (uint pid = 1; pid <= Kernel.Capacity(); pid++)
            {
                if (Kernel.Get(pid) is not ConstructionProcess process || !PlayerProcesses.Owns(process) || !process.BelongsTo(_constructionProgress)) continue;
                PlayerProcesses.Kill(process, 0x10); found = true; break;
            }
        } while (found);
        _constructionProgress = null; _constructionProgressState = null; _constructionDisplacement = null;
    }
}
