// Native 장기 공통 삭제의 실제 장치·공유 주소·본체를 같은 저장소 수명에 연결한다.
// cpppj/src/client/RawSurfaceDeletion.cpp 의 BindDeletionDevices/BindDeletionRuntime/RestoreDeletionGlobals/CommonDeletion 장기 본문을 이식한다.
// 0.575에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Common 우선)
namespace Netstorm.Core.Original;

/// <summary>현재 Client의 순수 UI 기억/전역 소리를 공통 삭제의 장기 장치로 빌린다.</summary>
public sealed class RawSurfaceDeletionDevices
{
    /// <summary>같은 선택 owner의 실제 rememberedPoint/mouseType을 쓰는 장치다. 0.575에서 추가됐다.</summary>
    public Action<uint, uint, uint>? RememberPlacement { get; set; }

    /// <summary>실제 같은 SoundPlayer의 unitLost.wav를 재생하는 장치다. 0.575에서 추가됐다.</summary>
    public Action<int>? UnitLost { get; set; }

    /// <summary>원문 함수 묶음처럼 값만 복사하며 장치를 실행하지 않는다. 0.575에서 추가됐다.</summary>
    internal RawSurfaceDeletionDevices Copy() => new()
    {
        RememberPlacement = RememberPlacement,
        UnitLost = UnitLost,
    };
}

/// <summary>Session 파생 종속 삭제만 현재 World 수명으로 빌리고 반납 뒤 저장하지 않는다.</summary>
public sealed class RawSurfaceDeletionBindings
{
    /// <summary>현재 Session의 기존 실제 파생 전달이다. 0.575에서 추가됐다.</summary>
    public Action<SquidDestroyEvent>? Forward { get; set; }

    /// <summary>이미 공개된 실제 전체 Runtime을 선택 호출 동안만 보유한다. 0.575에서 추가됐다.</summary>
    public Func<object?>? Pin { get; set; }
}

/// <summary>World 없이 같은 장기 HP/보상/장부로 공통 삭제를 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>실제 Client 장치만 장기 보관하며 Session을 소유하지 않는다. 0.575에서 추가됐다.</summary>
    private RawSurfaceDeletionDevices _deletionDevices = new();

    /// <summary>현재 파생 종속 효과만 약하게 빌리는 약한 삭제 대여다. 0.575에서 추가됐다.</summary>
    private WeakReference<RawSurfaceDeletionBindings>? _deletionRuntime;

    /// <summary>같은 장기 공통 깊이/장부/Graph/보상을 실제 Native caller가 공유하는 유일한 본체다. 0.575에서 추가됐다.</summary>
    private RawSquidDestroyLifecycle? _commonDeletion;

    /// <summary>장기 공통 PostPop 기본 장치다. 현재 Session 전용 장치가 없을 때 같은 현재 owner를 선택한다. 0.575에서 추가됐다.</summary>
    private SquidPostPopDevices _postPopDevices = new();

    /// <summary>첫 공통 삭제 전에 실제 UI/소리 장치만 연결한다. 0.575에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 공통 삭제 장치 묶음이다.</param>
    public void BindDeletionDevices(RawSurfaceDeletionDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        // Native 전송·보상 SP 뒤, 첫 공통 본체/대여 전에만 실제 Client 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _rewardDevices.Sp is null || _borrowed || _commonDeletion is not null
            || _deletionDevices.UnitLost is not null
            || devices.UnitLost is null || devices.RememberPlacement is null)
        {
            throw new ArgumentException("장기 공통 삭제의 실제 Native 최초 장치 계약", nameof(devices));
        }

        _deletionDevices = devices.Copy();
    }

    /// <summary>현재 World의 실제 파생 종단만 짧게 빌린다. 0.575에서 추가됐다.</summary>
    /// <param name="runtime">현재 World가 소유하는 실제 파생 전달 대여다.</param>
    public void BindDeletionRuntime(RawSurfaceDeletionBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        if (!UsesNativeTransmissionDevices() || runtime.Forward is null)
        {
            throw new ArgumentException("현재 공통 삭제 파생 대여의 중복/실제 종단 누락", nameof(runtime));
        }

        if (_deletionRuntime is not null && _deletionRuntime.TryGetTarget(out _))
        {
            throw new ArgumentException("현재 공통 삭제 파생 대여의 중복/실제 종단 누락", nameof(runtime));
        }

        _deletionRuntime = new(runtime);
    }

    /// <summary>현재 살아 있는 같은 삭제 대여만 선택 호출 동안 강하게 빌린다. 0.575에서 추가됐다.</summary>
    private RawSurfaceDeletionBindings? DeletionRuntime()
    {
        if (_deletionRuntime is not null && _deletionRuntime.TryGetTarget(out RawSurfaceDeletionBindings? current))
        {
            return current;
        }

        return null;
    }

    /// <summary>장기 공통 PostPop 기본 장치를 첫 효과 전에 연결한다. 0.575에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 기본 Client 장치 묶음이다.</param>
    public void BindPostPopDevices(SquidPostPopDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        if (_edition != OriginalEdition.Patch1078
            || devices.AiNotice is null || devices.LocalOwner is null || devices.RecheckSelection is null)
        {
            throw new ArgumentException("장기 공통 후처리의 실제 Client 장치 누락", nameof(devices));
        }

        bool first = _postPopDevices.AiNotice is null;
        if (first && (_borrowed || _postPopExecutor is not null))
        {
            throw new InvalidOperationException("Native 공통 후처리 장치는 첫 World/실행기 전에 필요");
        }

        _postPopDevices = new SquidPostPopDevices
        {
            AiNotice = devices.AiNotice,
            LocalOwner = devices.LocalOwner,
            RecheckSelection = devices.RecheckSelection,
        };
        if (first)
        {
            PostPopState.GraphsEnabled = true;
            PostPopState.SharedSuppressed = OwnerMode.ChallengeStorage;
        }

        if (_postPopExecutor is not null)
        {
            _postPopExecutor.BindDefaultPatchDevices(_postPopDevices);
        }
    }

    /// <summary>일반 fresh 대여가 상태를 교체해도 같은 장기 원문 주소만 다시 연결한다. 0.575에서 추가됐다.</summary>
    public void RestoreDeletionGlobals()
    {
        if (_serializationGlobals is null)
        {
            throw new InvalidOperationException("공통 삭제의 실제 현재 로딩 깊이 owner 미연결");
        }

        DeletionState.SharedLostX = GameAux().LostXBox;
        DeletionState.SharedLostY = GameAux().LostYBox;
        DeletionState.SharedLoadingDepth = LoadingDepthBox;
        DeletionState.SharedFortMode = OwnerMode.FortBox;
        DeletionState.SharedSilentType = GameAux().RainBlockerTypeBox;
        DeletionState.SharedSpecialSurfaceType = Links.NoIslandTypeBox;
        DeletionState.SharedReplacementSurfaceType = GameAux().FakeSurfaceTypeBox;
        DeletionState.SharedLargeSurfaceType = GameAux().FakeSurfaceTypeBox;
    }

    /// <summary>같은 장기 공통 깊이/장부/Graph/보상을 실제 Native caller가 공유한다. 0.575에서 추가됐다.</summary>
    public RawSquidDestroyLifecycle CommonDeletion()
    {
        if (!UsesNativeTransmissionDevices() || _deletionDevices.UnitLost is null
            || _postPopDevices.AiNotice is null || _postPopDevices.LocalOwner is null)
        {
            throw new InvalidOperationException("장기 공통 삭제의 실제 Client 장치 미연결");
        }

        RestoreDeletionGlobals();
        if (_commonDeletion is null)
        {
            var hooks = new SquidDeletionHooks
            {
                SelectedRaw = () => CurrentProcessSelection(),
                LocalOwner = () => (_postPopDevices.LocalOwner ?? throw new InvalidOperationException("장기 공통 후처리의 실제 로컬 owner 미연결"))(),
                RememberPlacement = (x, y, type) => (_deletionDevices.RememberPlacement ?? throw new InvalidOperationException("장기 공통 삭제의 실제 기억점 장치 미연결"))(x, y, type),
                Emit = @event =>
                {
                    if (@event.Effect == SquidDeletionEffect.ClearSelection)
                    {
                        (DefaultProcessAssetHooks().Emit ?? throw new InvalidOperationException("장기 공통 삭제 전달의 자산 효과 경계 미연결"))(new SquidDestroyEvent(SquidDestroyEffect.ClearSelection, @event.Sid, 0, 0));
                        return;
                    }

                    if (@event.Effect == SquidDeletionEffect.UnitLostSound)
                    {
                        (_deletionDevices.UnitLost ?? throw new InvalidOperationException("장기 공통 삭제의 실제 소리 장치 미연결"))(@event.Sid);
                        return;
                    }

                    throw new InvalidOperationException("장기 공통 삭제 Refund는 같은 실제 HP/보상만 사용합니다");
                },
                Forward = @event =>
                {
                    RawSurfaceDeletionBindings? current = DeletionRuntime();
                    if (current is null)
                    {
                        (DefaultProcessAssetHooks().Emit ?? throw new InvalidOperationException("장기 공통 삭제 전달의 자산 효과 경계 미연결"))(@event);
                        return;
                    }

                    Func<object?>? pin = current.Pin;
                    if (pin is null)
                    {
                        throw new InvalidOperationException("공통 삭제 파생 전달의 실제 전체 Runtime pin 미연결");
                    }

                    object? owner = pin();
                    try
                    {
                        Action<SquidDestroyEvent>? forward = current.Forward;
                        if (forward is null)
                        {
                            throw new InvalidOperationException("공통 삭제의 선택된 실제 파생 전달 미연결");
                        }

                        forward(@event);
                    }
                    finally
                    {
                        RawRuntimePin.Release(owner);
                    }
                },
            };
            var common = new RawSquidDestroyLifecycle(
                Pool,
                Types,
                PostPopState,
                DeletionState,
                BaseDestroy(),
                hooks,
                GraphExecutor(),
                Reward());
            Action<uint, int, bool>? notice = _postPopDevices.AiNotice;
            if (notice is null)
            {
                throw new InvalidOperationException("장기 공통 후처리의 실제 AI 통지 미연결");
            }

            common.BindPatchAiNotice(notice);
            _commonDeletion = common;
        }

        return _commonDeletion;
    }

}
