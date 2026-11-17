// Native 장기 HP/보상 장치와 Content 수명 본체를 같은 저장소 수명에 연결한다.
// cpppj/src/client/RawSurfaceReward.cpp 의 BindRewardDevices/RewardState/Reward 와
// cpppj/src/client/RawSurfaceContent.cpp 의 ContentLifetime 장기 본문을 이식한다.
// 0.571에서 추가됐다. (R4-1 잔여 중 Reward·ContentLifetime 결합)
namespace Netstorm.Core.Original;

/// <summary>같은 실제 SP/모드 owner를 처리기보다 오래 보관하는 보상 장치 묶음이다.</summary>
public sealed class RawSurfaceRewardDevices
{
    /// <summary>같은 Client SP 저장소의 실제 owner를 별칭으로 빌린다. 0.571에서 추가됐다.</summary>
    public RawSpStore? Sp { get; set; }

    /// <summary>실제 Client의 현재 editor DWORD 주소를 별칭으로 빌린다. 0.571에서 추가됐다.</summary>
    public UIntBox? Editor { get; set; }

    /// <summary>실제 SP 표시 갱신 장치다. 0.571에서 추가됐다.</summary>
    public Action? RefreshSp { get; set; }

    /// <summary>실제 PATCH 세션의 AI 지갑 가산 처리기다. 0.571에서 추가됐다.</summary>
    public Action<uint, int>? CreditWallet { get; set; }

    /// <summary>원문 함수 묶음처럼 값만 복사하며 장치를 실행하지 않는다. 0.571에서 추가됐다.</summary>
    internal RawSurfaceRewardDevices Copy() => new()
    {
        Sp = Sp,
        Editor = Editor,
        RefreshSp = RefreshSp,
        CreditWallet = CreditWallet,
    };
}

/// <summary>World 없이 같은 장기 Pool/TYPE/장부로 HP·보상을 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 실제 SP/모드 owner를 처리기보다 오래 보관하는 보상 장치다. 0.571에서 추가됐다.</summary>
    private RawSurfaceRewardDevices _rewardDevices = new();

    /// <summary>Session 이후에도 같은 현재 장기 모드/TYPE/비율을 읽는 보상 상태다. 0.571에서 추가됐다.</summary>
    private readonly SquidRewardState _rewardState = new();

    /// <summary>Pool/TYPE/장부보다 먼저 반납하며 지역 Session 콜백을 저장하지 않는 보상 본체다. 0.571에서 추가됐다.</summary>
    private RawSquidReward? _reward;

    /// <summary>같은 Reward/공통 삭제/정렬 본체보다 먼저 반납하는 Content 수명 본체다. 0.571에서 추가됐다.</summary>
    private RawContentLifetime? _contentLifetime;

    /// <summary>실제 장기 보상 장치가 설치됐는지만 읽는다. SP/월드 값을 만들지 않는다. 0.571에서 추가됐다.</summary>
    public bool UsesNativeRewardDevices() => _rewardDevices.Sp is not null;

    /// <summary>첫 Native 사용 전에 실제 장치만 연결하며 SP/난수/World를 초기화하지 않는다. 0.571에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 HP/보상 장치 묶음이다.</param>
    public void BindRewardDevices(RawSurfaceRewardDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        // PATCH·Native 전송 뒤, 첫 보상 본체/대여 전에만 실제 Client 장치를 보관한다.
        if (_edition != OriginalEdition.Patch1078 || !UsesNativeTransmissionDevices()
            || devices.Sp is null || devices.Editor is null || devices.RefreshSp is null || devices.CreditWallet is null
            || _rewardDevices.Sp is not null || _reward is not null || _borrowed)
        {
            throw new ArgumentException("장기 HP/보상의 실제 Native 최초 장치 계약", nameof(devices));
        }

        _rewardDevices = devices.Copy();
        _rewardState.SharedFixedSpA = () => OwnerMode.Fort;
        _rewardState.SharedFixedSpB = _rewardDevices.Editor;
        _rewardState.SharedHalfHealth = () => OwnerMode.Battle;
        _rewardState.SharedPercent = GameAux().RefundPercentBox;
        _rewardState.SharedGeyserPool = GameAux().ResourcePoolBox;
        _rewardState.SharedPriestQuarterHp = GameAux().PriestQuarterHpBox;
        _rewardState.SharedPriestType = GameAux().PriestTypeBox;
        _rewardState.SharedAltarType = GameAux().AltarTypeBox;
        _rewardState.SharedDaisType = GameAux().DaisTypeBox;
        _rewardState.SharedSilentType = GameAux().RainBlockerTypeBox;
        _rewardState.SharedAliasType = GameAux().GrowingRainBlockerTypeBox;
        _rewardState.SharedNuggetType = GameAux().NuggetTypeBox;
        _rewardState.Checking.Bind(() => Checking().Read());
    }

    /// <summary>같은 장기 전역 뷰를 Session과 보상 caller가 공유한다. 0.571에서 추가됐다.</summary>
    public SquidRewardState RewardState()
    {
        if (_rewardDevices.Sp is null)
        {
            throw new InvalidOperationException("장기 HP/보상의 실제 SP owner 미연결");
        }

        return _rewardState;
    }

    /// <summary>같은 Pool/TYPE/장부의 장기 HP·보상을 필요한 순간 한 번 구성한다. 0.571에서 추가됐다.</summary>
    public RawSquidReward Reward()
    {
        if (_reward is null)
        {
            if (Source() is null)
            {
                throw new InvalidOperationException("장기 HP/보상의 실제 최초 로딩 자산 미연결");
            }

            // 실제 Native VirtualCount(FortSaveVirtual Level)는 미연결 잔여로 두고 독립 상위 3비트 경로를 쓴다.
            var reward = new RawSquidReward(
                Pool,
                Types,
                PostPopState,
                RewardState(),
                _rewardDevices.Sp,
                new SquidRewardHooks { RefreshSp = _rewardDevices.RefreshSp });
            // 실제 PATCH 월드의 주소 고정 TYPE 표 별칭(BindCurrentTypes 배열/뷰)과 최초 로딩 정수 비용표(BindLoadedCosts)는 미연결 잔여로 두고 생성 시 사본을 쓴다.
            // Storage.Types가 List라 ArraySegment 물리 뷰를 직접 빌릴 수 없으므로 별도 배열 미러가 필요하다.
            reward.BindHitPointDiagnostics(AssertDisplay);
            reward.BindPatchWallet(_rewardDevices.CreditWallet ?? throw new InvalidOperationException("장기 HP/보상의 실제 지갑 장치 미연결"));
            _reward = reward;
        }

        return _reward;
    }

    /// <summary>같은 Reward/공통 깊이/Factory Unpop의 실제 장기 삭제 본문이다. 0.571에서 추가됐다.</summary>
    public RawContentLifetime ContentLifetime()
    {
        if (_contentLifetime is null)
        {
            // 같은 장기 Reward가 실제 지갑/SP를 처리하고 다른 효과는 현재 Client 장치를 선택한다.
            var effects = new ContentLifetimeHooks
            {
                CancelCursor = _contentDevices.Effects.CancelCursor,
                LocalNotice = _contentDevices.Effects.LocalNotice,
                RemoteNotice = _contentDevices.Effects.RemoteNotice,
                Sound = _contentDevices.Effects.Sound,
            };
            _contentLifetime = new RawContentLifetime(
                Pool,
                Types,
                ContentForms(),
                BaseDestroy(),
                ContentState(),
                effects,
                DefaultProcessAssetHooks(),
                Reward());
        }

        return _contentLifetime;
    }
}
