// 실제 사제 목록과 보호막 조회·가상 삭제를 World/Session보다 긴 같은 장기 저장소에 연결한다.
// cpppj/src/client/RawSurfacePriestDeletion.cpp 의 PriestForcefield/PriestPre 장기 본문을 이식한다.
// 0.581에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Priest 우선)
namespace Netstorm.Core.Original;

/// <summary>World 없이 같은 장기 Pool/Hash/TYPE로 사제 삭제 준비를 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 현재 TYPE과 실제 장기 주소를 빌리는 사제 보호막 조회기다. 0.581에서 추가됐다.</summary>
    private RawPriestForcefield? _priestForcefield;

    /// <summary>같은 장기 목록/보호막 조회와 실제 Carrier 부모를 공유하는 사제 전처리 본체다. 0.581에서 추가됐다.</summary>
    private RawPriestPreDestroy? _priestPre;

    /// <summary>같은 장기 사제 목록/소유자를 실제 세션과 공유하는 삭제 상태다. 0.581에서 추가됐다.</summary>
    private readonly PriestPostPopState _priestDeletionState = new();

    /// <summary>조회와 Shield 생성이 같은 현재 TYPE과 실제 장기 주소를 사용한다. 0.581에서 추가됐다.</summary>
    public RawPriestForcefield PriestForcefield()
    {
        if (!UsesNativeTransmissionDevices())
        {
            throw new InvalidOperationException("장기 사제 보호막의 Native 판본 계약");
        }

        if (_priestForcefield is null)
        {
            _priestForcefield = new RawPriestForcefield(Pool, Hash, PhysicalTypes(), () => GameAux().PriestForcefieldType);
        }

        return _priestForcefield;
    }

    /// <summary>현재 사제 목록/보호막 조회와 실제 Carrier 부모를 공유한다. 0.581에서 추가됐다.</summary>
    public RawPriestPreDestroy PriestPre()
    {
        if (!UsesNativeTransmissionDevices())
        {
            throw new InvalidOperationException("장기 사제 전처리의 Native 판본 계약");
        }

        if (_priestPre is null)
        {
            _priestDeletionState.SharedPriests = SidLists.Priests;
            _priestDeletionState.SharedOwnerState = PriestOwnerState;
            var hooks = RawCarrierPreDestroy.MakeCarrierPreDestroyHooks(
                Pool,
                CarrierPre(),
                RawPriestForcefield.MakePriestForcefieldHooks(
                    Pool,
                    PriestForcefield(),
                    new PriestPreDestroyHooks
                    {
                        DestroyForcefield = (sid, flags) => DestroyAsset(sid, flags),
                    }));
            var body = new RawPriestPreDestroy(Pool, _priestDeletionState, hooks);
            body.EnableNativeBody();
            _priestPre = body;
        }

        return _priestPre;
    }
}
