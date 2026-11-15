// 같은 현재 장부/Graph/선택 장치에 PATCH 공통 부모를 연결한다.
namespace Netstorm.Core.Original;

/// <summary>장기 Client 기본 장치다. 현재 Session 전용 장치가 없을 때 같은 현재 owner를 선택한다.</summary>
public sealed class SquidPostPopDevices
{
    /// <summary>현재 Player/brain 관문 뒤의 실제 AI 생성 통지다.</summary>
    public Action<uint, int, bool>? AiNotice { get; set; }
    /// <summary>AI 효과 뒤의 같은 현재 전체 로컬 owner다.</summary>
    public Func<uint>? LocalOwner { get; set; }
    /// <summary>장부 뒤의 현재 mouseType/기억점/selected 재검사다.</summary>
    public Action? RecheckSelection { get; set; }
}

/// <summary>cpppj/src/o/SquidPostPopPatch.cpp와 공통 부모에 필요한 현재 장치/TYPE/최초 비용 연결이다.</summary>
public sealed partial class RawPostPop
{
    /// <summary>실제 PATCH의 같은 현재 TYPE 대여다.</summary>
    private IReadOnlyList<OriginalTypeRecord>? _currentTypes;
    /// <summary>현재 TYPE 또는 독립 생성자 snapshot이다.</summary>
    private IReadOnlyList<OriginalTypeRecord> CurrentTypes => _currentTypes ?? _types;
    /// <summary>실제 Client의 최초 비용 로딩 표다. 현재 TYPE 비용 쓰기와 구분한다.</summary>
    private IReadOnlyList<OriginalTypeRecord>? _loadedCosts;
    /// <summary>같은 현재 checking과 선택 단언이다.</summary>
    private ManaRegistrationDiagnostics _manaDiagnostics = new();
    /// <summary>같은 장기 기본 Client 장치의 함수 사본이다.</summary>
    private SquidPostPopDevices _defaultDevices = new();
    /// <summary>현재 Session 전용 AI 생성 통지다.</summary>
    private Action<uint, int, bool>? _aiNotice;
    /// <summary>현재 Session 전용 전체 로컬 owner getter다.</summary>
    private Func<uint>? _localOwner;
    /// <summary>현재 Session 전용 선택 재검사다.</summary>
    private Action? _selectionRecheck;
    /// <summary>선택된 현재 전체 Runtime을 동기 호출 동안 보유하는 장치다.</summary>
    private Func<object?>? _runtimePin;

    /// <summary>실제 PATCH의 현재188개 TYPE만 빌리고 빈 입력은 독립 snapshot으로 복귀한다.</summary>
    public void BindCurrentTypes(IReadOnlyList<OriginalTypeRecord>? types)
    {
        ValidateBorrowedTypes(types, "postPop 현재TYPE"); _currentTypes = types is { Count: > 0 } ? types : null;
    }
    /// <summary>실제 최초 로딩 비용188개 표를 빌리고 빈 입력은 생성자 snapshot의 비용으로 복귀한다.</summary>
    public void BindLoadedCosts(IReadOnlyList<OriginalTypeRecord>? types)
    {
        ValidateBorrowedTypes(types, "postPop 로딩 비용표"); _loadedCosts = types is { Count: > 0 } ? types : null;
    }
    /// <summary>대여 중 값을 읽거나 초기화하지 않고 판본/크기만 확인한다.</summary>
    private void ValidateBorrowedTypes(IReadOnlyList<OriginalTypeRecord>? types, string name)
    {
        if (types is { Count: > 0 } && (_pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)) throw new ArgumentException(name + " 대여의 판본/길이 오류", nameof(types));
    }
    /// <summary>마나 등록과 공통80b의 같은 현재 진단 뷰/함수 사본만 연결한다.</summary>
    public void BindManaDiagnostics(ManaRegistrationDiagnostics diagnostics)
    {
        ArgumentNullException.ThrowIfNull(diagnostics); _manaDiagnostics = new() { Checking = new(diagnostics.Checking), Assertion = diagnostics.Assertion };
    }
    /// <summary>장기 기본 장치는 첫 효과 전에 완전하게 연결하고 실제 게임 조회는 실행하지 않는다.</summary>
    public void BindDefaultPatchDevices(SquidPostPopDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        if (_pool.Edition != OriginalEdition.Patch1078 || devices.AiNotice is null || devices.LocalOwner is null || devices.RecheckSelection is null) throw new ArgumentException("기본 공통 후처리의 실제 Client 장치 누락", nameof(devices));
        _defaultDevices = new() { AiNotice = devices.AiNotice, LocalOwner = devices.LocalOwner, RecheckSelection = devices.RecheckSelection };
    }
    /// <summary>현재 Session의 선택 재검사를 대여하고 null은 이 짧은 장치만 반납한다.</summary>
    public void BindPatchSelectionRecheck(Action? recheck) { RequirePatch(); _selectionRecheck = recheck; }
    /// <summary>현재 Session의 전체 로컬 owner를 대여하고 null은 기본 Client 장치로 복귀한다.</summary>
    public void BindPatchLocalOwner(Func<uint>? owner) { RequirePatch(); _localOwner = owner; }
    /// <summary>현재 Session의 실제 AI 생성 통지만 대여한다.</summary>
    public void BindPatchAiNotice(Action<uint, int, bool>? notice) { RequirePatch(); _aiNotice = notice; }
    /// <summary>현재 실제 외곽 owner를 선택 호출 끝까지 보유할 장치만 연결한다.</summary>
    public void BindRuntimePin(Func<object?>? pin) => _runtimePin = pin;
    /// <summary>현재 PATCH 장치의 다른 판본 결합을 효과 없이 거부한다.</summary>
    private void RequirePatch() { if (_pool.Edition != OriginalEdition.Patch1078) throw new InvalidOperationException("현재 공통 후처리 장치의 판본 오류"); }

    /// <summary>실제 Graph/현재 목록/장부/선택에 공통 부모를 연결하고 실패 전의 효과를 보존한다.</summary>
    private void PostPopPatch(int sid, uint flags)
    {
        object? owner = _runtimePin?.Invoke();
        var hooks = new SquidPostPopParentHooks
        {
            Suppressed = () => _state.CurrentSuppressed, GraphsEnabled = () => _state.GraphsEnabled,
            Type = number => number < CurrentTypes.Count ? CurrentTypes[number] : throw new ArgumentOutOfRangeException(nameof(sid), "공통 후처리 현재TYPE 범위 오류"),
            Region = current => { if (_graph is RawGraph graph) graph.InvalidateRegion(current); else (_graph ?? throw new InvalidOperationException("실제 후처리 영역 Graph 미연결")).PostPop(current, true, false); },
            Add = current => { if (_graph is RawGraph graph) graph.Add(current); else (_graph ?? throw new InvalidOperationException("실제 후처리 Add Graph 미연결")).PostPop(current, false, true); },
            Assertion = (expression, file, line) => (_manaDiagnostics.Assertion ?? throw new InvalidOperationException("공통 후처리 실제 진단 미연결"))(expression, file, line),
            ResetGraph = current => { if (_graph is RawGraph graph) graph.SetNumber(current, _state.InvalidGraph); else _pool.RawSlot(current)[30] = _state.InvalidGraph; },
            Cost = current => _state.CurrentTotalCost = TotalCost(_pool.Slot(current)[10]),
            Provider = current => RawManaRegistration.Register(_pool.Edition, _state.Providers, ref _state.ProductionDirtyValue, (uint)current, _manaDiagnostics),
            OwnerFactory = (player, current) => { if (player == 0) return; if (player >= _state.OwnerFactories.Length) throw new ArgumentOutOfRangeException(nameof(player), "공통 후처리 현재 owner 공장 범위 오류"); _state.OwnerFactories[player].Append(current); },
            Factory = current => { _state.Factories.Append(current); _state.ProductionDirty = unchecked(_state.ProductionDirty + 1); },
            AiNotice = (player, current) =>
            {
                Action<uint, int, bool>? notice = _aiNotice ?? _defaultDevices.AiNotice; if (notice is not null) { notice(player, current, true); return; }
                if (player >= _state.AiAttached.Length) throw new ArgumentOutOfRangeException(nameof(player), "공통 후처리 현재 AI owner 범위 오류");
                if (player != 0 && _state.AiAttached[player]) throw new InvalidOperationException("공통 후처리 실제 AI 통지 미연결");
            },
            LocalOwner = () => (_localOwner ?? _defaultDevices.LocalOwner)?.Invoke() ?? _state.LocalOwner,
            LocalCount = number => { _state.LocalCounts[number] = unchecked(_state.LocalCounts[number] + 1); _state.LocalSecondaryCounts[number] = unchecked(_state.LocalSecondaryCounts[number] + 1); },
            GlobalCount = number => _state.GlobalCounts[number] = unchecked(_state.GlobalCounts[number] + 1),
            RecheckSelection = () => { Action? recheck = _selectionRecheck ?? _defaultDevices.RecheckSelection; if (recheck is not null) recheck(); else if (_state.PendingPlacement) throw new InvalidOperationException("공통 후처리 실제 선택 재검사 미연결"); },
        };
        try { RawSquidPostPopParent.Run(_pool, sid, flags, ref _state.ActivationDepth, hooks); }
        finally { GC.KeepAlive(owner); }
    }
}
