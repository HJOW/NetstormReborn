// Native 장기 자산 생성·소유자·삭제 상위 결선을 같은 저장소 수명에 연결한다.
// cpppj/src/client/RawSurfaceStorage.cpp 의 DestroyAsset/CreateAsset/SetAssetOwner 장기 본문을 이식한다.
// 0.574에서 추가됐다. (R4-1 잔여 중 피해/해방·삭제/배치 상위 결선)
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>World 없이 같은 장기 공장·소유자·삭제로 자산을 배치한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 장기 Native 전송 장치와 실제 삭제 장치를 선택해 자산을 지운다. 0.574에서 추가됐다.</summary>
    /// <param name="sid">지울 객체 번호다.</param>
    /// <param name="flags">삭제 플래그다.</param>
    public bool DestroyAsset(int sid, uint flags)
    {
        // PATCH 삭제 모드에서는 장기 분배 본문을 쓰고 독립 typed 장치 계약은 유지한다.
        // 삭제 파생 대여가 없으면(NoWorld) 장기 분배를 선택한다. 파생 대여 결선은 잔여다.
        if (UsesNativeTransmissionDevices() && _patchDestroyDiagnostics)
        {
            bool result = ProcessFormDispatch().Destroy(sid, flags);
            if (result)
            {
                NoteChanged();
            }

            return result;
        }

        // 현재 장치를 호출이 끝날 때까지 보유하며 Session/World를 캡처하지 않는다.
        var destroy = _resourceDevices.Destroy ?? throw new InvalidOperationException("장기 자산의 현재 가상 삭제 장치 미연결");
        return destroy(sid, flags);
    }

    /// <summary>같은 장기 공장으로 실제 생성 뒤 변경만 알리고 새 월드를 요구하지 않는다. 0.574에서 추가됐다.</summary>
    /// <param name="type">만들 타입 번호다.</param>
    /// <param name="flags">할당 영역 플래그다.</param>
    public int CreateAsset(uint type, uint flags = 0)
    {
        NoteChanged();
        return FactoryExecutor().Create(type, (SidAllocation)flags);
    }

    /// <summary>같은 실제가상 owner 분배를 World 없이 선택하며 장부/현재 모드를 복제하지 않는다. 0.574에서 추가됐다.</summary>
    /// <param name="sid">소유자를 정할 객체 번호다.</param>
    /// <param name="owner">새 소유자 번호다.</param>
    public void SetAssetOwner(int sid, uint owner)
    {
        // 표면 특수부모 뒤 실제 현재 신전 가상74를 분배한다.
        Action<int, uint> set = RawAssetOwner.MakeAssetOwnerDispatch(
            Pool,
            Links,
            PostPopState.ProductionDirtyBox,
            (id, player) =>
            {
                if (UsesNativeTransmissionDevices() && HandlesRawVortexOwner(id))
                {
                    Action<int, uint>? body = _vortexOwner;
                    if (body is null)
                    {
                        throw new InvalidOperationException("장기 신전의 실제 가상 소유자 미결합");
                    }

                    body(id, player);
                    return;
                }

                OwnerExecutor().Set(id, player);
            });
        set(sid, owner);
        NoteChanged();
    }

    /// <summary>같은 장기 신전 가상 소유자 분배 대상인지 현재 VT로만 판정한다. 0.586에서 추가됐다.</summary>
    /// <param name="sid">확인할 객체 번호다.</param>
    private bool HandlesRawVortexOwner(int sid)
    {
        ReadOnlySpan<byte> raw = Pool.Slot(sid);
        uint table = (uint)(raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24));
        // 네 실제 신전 가상 표만 같은 전체 장기 부모를 선택한다.
        foreach (uint candidate in new uint[] { 0x005196f0u, 0x005197a8u, 0x00519860u, 0x00519918u })
        {
            if (table == candidate)
            {
                return true;
            }
        }

        return false;
    }
}
