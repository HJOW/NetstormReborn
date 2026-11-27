// 같은 물리·현재 장부/진단을 공통 종속 해제와 생성/공간 부모에 한 번 연결한다.
namespace Netstorm.Core.Original;

/// <summary>World가 없어도 유지되는 실제 공장·건물 공간·Unpop과 짧은 종속 종단을 소유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 실제 TYPE/풀과 현재 HP/깊이 owner를 사용하는 공장이다.</summary>
    private RawSquidFactory? _factoryExecutor;
    /// <summary>같은 Hash0의 현재 옆면 BYTE만 변경하는 건물 공간 실행기다.</summary>
    private RawBuildingSpace? _buildingExecutor;
    /// <summary>같은 Hash/spot/표시를 사용하는 공간 해제 실행기다.</summary>
    private RawUnpop? _unpopExecutor;
    /// <summary>현재 Session/World가 소유하는 실제 종속 해제 종단이다.</summary>
    private WeakReference<RawSquidContainedUnpop>? _currentContainedUnpop;
    /// <summary>독립 공통 부모/가상표를 사용하는 장기 기본 종단이다.</summary>
    private RawSquidContainedUnpop? _containedUnpopExecutor;
    /// <summary>현재 선택된 UI 통지를 실제 입력 큐에 전달하는 장기 Client 장치다.</summary>
    private Action<uint, uint, uint>? _contentCommand;
    /// <summary>독립 typed 저장소의 현재 공통 내용물 TYPE DWORD다.</summary>
    public UIntBox ContainedType { get; } = new() { Value = 6 };
    /// <summary>현재 Gump Form TYPE DWORD다. UI 탐색 시점에 읽는다.</summary>
    public UIntBox ContainedGumpType { get; } = new() { Value = 3 };
    /// <summary>현재 UpdateContents UI 명령 DWORD다.</summary>
    public UIntBox ContainedUiEvent { get; } = new() { Value = 0x02000039 };

    /// <summary>실제 명령 함수만 설치하며 슬롯/입력/World를 만들지 않는다.</summary>
    public void BindContentCommand(Action<uint, uint, uint> command) => _contentCommand = command ?? throw new ArgumentNullException(nameof(command));
    /// <summary>같은 풀의 실제 종단을 약하게 빌리고 null이면 장기 기본 종단으로 돌아간다.</summary>
    public void BindContainedUnpop(RawSquidContainedUnpop? binding)
    {
        if (binding is not null && !ReferenceEquals(binding.Pool(), Pool)) throw new ArgumentException("종속 Unpop의 같은 Pool 필요", nameof(binding));
        _currentContainedUnpop = binding is null ? null : new(binding);
    }
    /// <summary>자기 종단이 현재인 경우만 반납한다. 다른 소유자의 결합은 지우지 않는다.</summary>
    internal void ReleaseContainedUnpop(RawSquidContainedUnpop binding)
    {
        if (_currentContainedUnpop is not null && _currentContainedUnpop.TryGetTarget(out var current) && ReferenceEquals(current, binding)) _currentContainedUnpop = null;
    }
    /// <summary>현재 살아 있는 실제 종단을 선택하고 없을 때만 같은 물리의 기본 종단을 구성한다.</summary>
    private RawSquidContainedUnpop CurrentContainedUnpopExecutor() => _currentContainedUnpop is not null && _currentContainedUnpop.TryGetTarget(out var current) ? current : ContainedUnpopExecutor();

    /// <summary>실제 가상표의 공통 부모·이전 링크를 사용하며 미연결 파생 통지는 도달 위치에서 실패한다.</summary>
    public RawSquidContainedUnpop ContainedUnpopExecutor()
    {
        if (_containedUnpopExecutor is null)
        {
            var state = new SquidContainedUnpopState
            {
                ContentFormType = _contentDevices.ContentType ?? ContainedType, GumpFormType = _gumpType ?? ContainedGumpType, UiEvent = ContainedUiEvent,
                CheckingDead = new(() => Checking().Read()), InMetamapMode = () => OwnerMode.Challenge, Boss = () => Authority().Boss,
            };
            var hooks = new SquidContainedUnpopHooks
            {
                Parent = sid => RawContainedLink.CommonContainedParent(Pool, sid), Previous = sid => RawContainedLink.CommonContainedPrevious(Pool, sid),
                SetPrevious = (sid, previous) => RawContainedLink.CommonContainedSetPrevious(Pool, sid, previous),
                ParentNotice = (parent, form, reason) =>
                {
                    // Native는 현재 World의 파생 소비자를 빌리고 독립 저장소는 기존 빈 몸체를 사용한다.
                    if (UsesNativeTransmissionDevices()) NoticeContent(parent, form, reason);
                    else new RawContentParentNotice(Pool).Notice(parent, form, reason);
                },
                Command = (code, x, y) => (_contentCommand ?? throw new InvalidOperationException("공통 종속의 실제 입력 장치 미연결"))(code, x, y),
                Assertion = AssertDisplay,
            };
            _containedUnpopExecutor = new(Pool, state, hooks);
        }
        return _containedUnpopExecutor;
    }
    /// <summary>현재 다리 TYPE와 원문 진단을 같은 건물 공간 실행기에 연결한다.</summary>
    public RawBuildingSpace BuildingExecutor()
    {
        if (_buildingExecutor is null)
        {
            var building = new RawBuildingSpace(Pool, Hash, Types, BuildingSpaceState);
            building.BindCurrentBridgeType(() => Links.BridgeType);
            building.BindNotifyDiagnostics(new(() => Checking().Read()), AssertDisplay); _buildingExecutor = building;
        }
        return _buildingExecutor;
    }
    /// <summary>현재 종속 종단을 소비 시점에 선택하며 World를 장기 소유하지 않는다.</summary>
    public RawUnpop UnpopExecutor()
    {
        if (_unpopExecutor is null)
        {
            var unpop = new RawUnpop(Pool, Hash, Spots, DisplayExecutor());
            unpop.BindDiagnostics(new() { Checking = new(() => Checking().Read()), Types = Types, Assertion = AssertDisplay });
            unpop.BindContainedUnpop(sid => CurrentContainedUnpopExecutor().Unpop(sid)); unpop.SetBuildingSide(BuildingExecutor()); _unpopExecutor = unpop;
        }
        return _unpopExecutor;
    }
    /// <summary>첫 실제 생성자만 같은 풀/TYPE로 구성하고 공간·종속 해제는 선택될 때 조회한다.</summary>
    public RawSquidFactory FactoryExecutor()
    {
        if (_factoryExecutor is null)
        {
            var factory = new RawSquidFactory(Pool, Types);
            factory.BindDiagnostics(new() { Types = Types, Checking = new(() => Checking().Read()), Assertion = AssertDisplay, Detail = RecordAssertionDetail });
            factory.BindUnpop(() => UnpopExecutor()); factory.BindFormDetach(sid => CurrentContainedUnpopExecutor().Detach(sid));
            factory.BindFormPopDepth(PostPopState.DepthBox); factory.BindWeakenedMana(GameAux().PriestQuarterHpBox);
            factory.BindHitPointTypes(GameAux().PriestTypeBox, GameAux().DaisTypeBox); factory.BindFakeSurfaceType(GameAux().FakeSurfaceTypeBox);
            if (UsesNativeTransmissionDevices()) factory.BindFormAttach(() => CurrentContainedUnpopExecutor().ContentFormType(), TransmitPop);
            _factoryExecutor = factory;
        }
        return _factoryExecutor;
    }
}
