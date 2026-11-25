using System.Buffers.Binary;
using Netstorm.Core.Original;

// State가 고른 실제 가상 삭제를 같은 현재 Session의 기존 분배기에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>가상 삭제 본체를 복제하지 않고 같은 Pool의 실제 설치된 분배기만 빌린다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 월드의 기존 가상+10 분배기다. 미설치 선택을 일반 삭제로 대체하지 않는다.</summary>
    private RawSquidFormDispatch? _stateDeletion;

    /// <summary>원본 가상 삭제를 수행할 같은 실제 월드 분배기를 설치하며 현재 값/Pool을 바꾸지 않는다.</summary>
    public void BindStateDeletion(RawSquidFormDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        EnsureRuntimeCanRelease();
        if (_playerRecordsReleased || ProcessBindingsReturned()) throw new InvalidOperationException("State 반납 뒤 가상 삭제 분배 설치 금지");
        if (!ReferenceEquals(dispatch.Pool, Pool)) throw new ArgumentException("State 가상 삭제의 실제 Pool 불일치", nameof(dispatch));
        if (_stateDeletion is not null && !ReferenceEquals(_stateDeletion, dispatch)) throw new InvalidOperationException("현재 State 가상 삭제 분배 중복 설치");
        _stateDeletion = dispatch;
    }

    /// <summary>호출자가 같은 전체 Runtime을 pin한 선택 위치에서만 기존 가상 삭제를 실행한다.</summary>
    internal bool DestroyForState(int sid, uint flags)
    {
        if (_playerRecordsReleased || ProcessBindingsReturned()) throw new InvalidOperationException("State 반납 뒤 실제 가상 삭제 금지");
        return (_stateDeletion ?? throw new InvalidOperationException("State의 실제 Native World 가상 삭제 분배 미연결")).Destroy(sid, flags);
    }

    /// <summary>파편/건설도 State와 같은 실제 전체 가상10 분배를 사용한다. 미설치를 일반 Pre/Post로 대체하지 않는다.</summary>
    public void DestroyWorldObject(int sid, uint flags) => _ = DestroyForState(sid, flags);

    /// <summary>제품 Root 미션 흐름에 전체 Native World 자산 가상 삭제 분배를 설치한다.</summary>
    /// <returns>같은 세션 수명으로 공유하는 실제 전체 분배기다. MetaZone 마커 삭제가 이 분배를 사용한다.</returns>
    /// <remarks>2026-11-26 1.036에서 WP-3 전체 Native World 자산 가상 삭제 분배용으로 추가했다. 근거: cpppj src/client/RawSurfaceWorld.cpp 373행의 BindDeletion과 RawCampaignSession.cpp 3026행의 MakeCampaignDeletionEffects 상당 결선이다. 자산 뿌리는 원문 83개 생성 가상표면 전체를 실제 공통 본체로 보내며, 등록되지 않은 가상표는 명시 거부한다. 일반 삭제 Pre/Post로 대신하지 않는다. 감시 쌍·프로세스 Form은 같은 세션 장치를 등록하고 Gump·내용물 Form은 UI·내용물 본체(R3)에서 잇는다.</remarks>
    public RawSquidFormDispatch BuildWorldDeletionRootDevices()
    {
        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("월드 전체 삭제의 실제 프로세스 Runtime 미연결");
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage);
        RawSquidFormDispatch dispatch = new(Pool, processes.Destroy(), storage.DefaultProcessAssetHooks(), AssetDestroyFull);
        dispatch.Bind(processes);
        dispatch.Bind(_worldDependencies ?? BuildDependencyWorldDevices());
        BindWorldDeletion(dispatch);
        return dispatch;
    }

    /// <summary>원문 83개 생성 가상표면 전체 자산을 실제 공통 본체에 연결한다. 목록에 없으면 명시 거부한다.</summary>
    /// <param name="sid">지울 뿌리 번호다.</param>
    /// <param name="flags">삭제 플래그다.</param>
    /// <param name="hooks">분배기의 실제 공통 효과다.</param>
    /// <returns>공통 본체가 삭제를 받아들였는지다.</returns>
    /// <remarks>2026-11-26 1.036에서 WP-3용으로 추가했다. 테스트용 5개 가상표 한정이 아니라 원문 표 전체를 허용한다.</remarks>
    private bool AssetDestroyFull(int sid, uint flags, SquidDestroyHooks hooks)
    {
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid));
        if (!RawSquidFactory.SupportsConstructedVtable(Pool.Edition, table))
            throw new InvalidOperationException($"전체 Native 자산 가상 삭제의 미복원 가상표: 0x{table:X8}");
        PlayerProcesses.Destroy().Destroy(sid, flags, hooks);
        return true;
    }

    /// <summary>같은 Host의 소진/실패 정리도 현재 전체 가상 분배로 보낸다. State 전용 설치와 구별한다.</summary>
    public void BindWorldDeletion(RawSquidFormDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        EnsureRuntimeCanRelease();
        ProcessHostRuntimeBindings runtime = _playerProcessRuntime ?? throw new InvalidOperationException("월드 전체 삭제의 실제 프로세스 Runtime 미연결");
        if (!ReferenceEquals(dispatch.DestroyBody, runtime.Destroy)) throw new ArgumentException("월드 전체 삭제의 실제 Host 삭제 본체/깊이 불일치", nameof(dispatch));
        BindStateDeletion(dispatch);
        runtime.Asset = dispatch.Hooks();
        if (_worldDependencies is { } dependencies) dependencies.BindVirtualDestroy(DestroyWorldObject, BeforeWorldDependencyFind);
    }
}
