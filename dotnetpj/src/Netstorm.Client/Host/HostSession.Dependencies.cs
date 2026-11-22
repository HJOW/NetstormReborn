using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 실제 Form 공장·현재역할·가상통지·공통삭제로 감시 쌍 수명을 유지한다.
namespace Netstorm.Client.Host;

/// <summary>RawSurfaceDepend.cpp의 감시 본체를 실제 미션의 같은 풀/Host에 연결한다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 감시 쌍의 비소유 부모/양방향Form 연결을 처리한다.</summary>
    private RawDependFormHost? _worldDependencies;
    /// <summary>업그레이드 그림과 후속 이동/표적이 공유할 같은 감시 본체다.</summary>
    public RawDependFormHost WorldDependencies => _worldDependencies ?? throw new InvalidOperationException("호스트 감시 쌍 미연결");

    /// <summary>복제 공장을 만들지 않고 현재 실제 공장·Depend TYPE·Form 해제를 빌린다.</summary>
    public RawDependFormHost BuildDependencyWorldDevices()
    {
        if (_worldDependencies is not null) throw new InvalidOperationException("호스트 감시 쌍 중복 연결");
        _ = BindLivePlayerWorld(); SquidProcessHost processes = PlayerProcesses;
        var dependencies = new RawDependFormHost(Pool, processes.Destroy(), NotifyWorldDependency,
            BuildSessionDestroyHooks("감시 쌍", processes), (sid, flags) => _terrainPopTransmissions.Add((sid, flags)));
        dependencies.BindCreation(_fortFactory!, GameAux.DependFormTypeBox,
            (expression, file, line) => throw new InvalidOperationException($"감시 생성 원본 진단: {expression} {file}:{line}"));
        dependencies.BindUnpop(_fortFactory!.UnpopForm);
        dependencies.BindVirtualDestroy(BuildSessionDestroy("감시 가상 삭제", processes), BeforeWorldDependencyFind);
        _worldDependencies = dependencies; return dependencies;
    }

    /// <summary>감시 조회 직전 같은 현재 checking/모드/서버와 부모 dead 비트를 읽는다.</summary>
    private void BeforeWorldDependencyFind(int parent)
    {
        if (AiChecking.Read() != 0 && !OwnerMode.Challenge && (Pool.Slot(parent)[11] & 2) != 0 && !Gameplay.Authority.Boss)
            throw new InvalidOperationException("감시 조회 원본 진단: inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy \\Ns\\O\\Squidfinder.cpp:0x158");
    }

    /// <summary>현재VT의실제+34를선택한다. 원본 빈ret8은 인자/소유자/서버를읽지않는다.</summary>
    public void NotifyWorldDependency(int watcher, int watched, int form)
    {
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(watcher));
        if (RawSurfaceStorage.TryGetAssetDependencyMethod(table, out uint method))
        {
            if (method == 0x0041c100) return;
            if (method == 0x0048f0a0)
            {
                // 실제 파편 더미110의 통지는 같은 월드의 전체 가상10을 실행한다.
                if (table == 0x50eb98 && _worldShrapnel is not null) DestroyWorldObject(watcher, 0);
                else BuildSessionDestroy("감시 자산", PlayerProcesses)(watcher, 0);
                return;
            }
            throw new InvalidOperationException($"감시 자산의 실제 홈 통지 미연결: 0x{method:X8}");
        }
        if (table is 0x512110 or 0x512188 or 0x512208) return;
        if (table == 0x512098) { PlayerProcesses.NotifyDependency(watcher, watched, form); return; }
        throw new InvalidOperationException($"현재 Depend 수신자의 실제 가상34 미연결: 0x{table:X8}");
    }

    /// <summary>미션의실제감시Form들을쌍으로해제하고빌려준통지/업그레이드장치를반납한다.</summary>
    private void ReleaseDependencyWorld()
    {
        if (_worldDependencies is not { } dependencies) return;
        // 일반 종료는 현재 Form을 직접 해제한다. Native 반납 뒤에는 뒤따르는 Pool 초기화가 슬롯을 반납한다.
        // 상대Form이먼저삭제되어도매차례현재물리슬롯의free비트와실제VT를확인한다.
        for (int sid = RawSidPool.FirstOrdinary; !ProcessBindingsReturned() && sid < Pool.Capacity; sid++)
        {
            if ((Pool.Slot(sid)[11] & 1) == 0 && dependencies.IsDependForm(sid)) dependencies.Destroy(sid, 0x10);
        }
        ReleaseFactoryUpgradeCommandWorld();
        _worldFactoryUpgrade = null; _worldDependencies = null;
    }
}
