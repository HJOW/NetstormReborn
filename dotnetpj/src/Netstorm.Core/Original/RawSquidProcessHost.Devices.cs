// 장기 ProcessHost의 실제 기본 장치와 현재 월드 장치를 원문 소비 위치에서 고른다.
namespace Netstorm.Core.Original;

/// <summary>월드 대여가 없는 동안에도 같은 저장소의 삭제·Regular·공장·직렬화 부모를 연결한다.</summary>
public sealed partial class SquidProcessHost
{
    /// <summary>같은 장기 Pool/Unpop/깊이를 소유하는 실제 공통 삭제 getter다.</summary>
    private Func<RawSquidDestroy>? _defaultDestroy;
    /// <summary>같은 장기 실제 VT 분배기가 처리하는 기본 Regular다.</summary>
    private RegularHandler? _defaultRegular;
    /// <summary>현재 실제 Form 분배와 Client 선택을 읽는 장기 getter다.</summary>
    private Func<SquidDestroyHooks>? _defaultAssets;
    /// <summary>같은 장기 Pool의 실제 공장을 소비 위치에서 반환한다.</summary>
    private Func<RawSquidFactory>? _defaultFormFactory;
    /// <summary>SharedRegular 진단이 읽는 같은 장기 Pop 깊이다.</summary>
    private Func<uint>? _defaultPostPopDepth;

    /// <summary>장기 Host에 실제 기본 삭제/Regular/자산 getter를 한 번 연결한다. 아직 조회나 생성을 하지 않는다.</summary>
    public void BindDefaultLifecycle(Func<RawSquidDestroy> destroy, RegularHandler regular, Func<SquidDestroyHooks> assets)
    {
        if (_currentRuntime is null || _defaultDestroy is not null || destroy is null || regular is null || assets is null)
            throw new ArgumentException("장기 Host의 실제 기본 삭제/Regular 장치 계약 위반");
        _defaultDestroy = destroy; _defaultRegular = regular; _defaultAssets = assets;
    }

    /// <summary>장기 공장/깊이 getter만 한 번 보관한다. 연결 시 Pool이나 깊이를 미리 읽지 않는다.</summary>
    public void BindDefaultFormFactory(Func<RawSquidFactory> factory, Func<uint> depth)
    {
        if (_currentRuntime is null || _defaultFormFactory is not null || factory is null || depth is null)
            throw new ArgumentException("장기 Host의 실제 기본 공장/깊이 계약 위반");
        _defaultFormFactory = factory; _defaultPostPopDepth = depth;
    }

    /// <summary>완전한 기본 직렬화/송신 장치와 진단을 한 번 값 복사한다. 현재 World 대여를 만들지 않는다.</summary>
    public void BindDefaultFormTransmission(ProcessFormTransmission transmit, Action<string, string, uint> assertion)
    {
        if (_currentRuntime is null || _transmit?.WriteData is not null || transmit is null || transmit.WriteData is null ||
            transmit.WriteFullData is null || transmit.Packet is null || transmit.Transmit is null || assertion is null)
            throw new ArgumentException("장기 ProcessForm의 실제 기본 직렬화 장치 계약 위반");
        _transmit = transmit.Clone(); _state.Assertion = assertion;
    }

    /// <summary>현재 World가 있으면 그 공장만 요구하고 없으면 같은 장기 공장을 선택한다.</summary>
    private RawSquidFactory? FormFactory()
    {
        if (_currentRuntime is null) return _directFormFactory;
        ProcessHostRuntimeBindings? current = _currentRuntime();
        RawSquidFactory factory = current is not null
            ? current.Factory ?? throw new InvalidOperationException("현재 World의 같은 Pool 공장 미연결")
            : (_defaultFormFactory ?? throw new InvalidOperationException("World 없는 Host의 실제 장기 공장 미연결"))();
        if (!ReferenceEquals(factory.Pool, _pool)) throw new ArgumentException("Host 현재/기본 공장의 실제 Pool 불일치");
        return factory;
    }

    /// <summary>현재 대여의 진단이 null이면 그대로 유지하며 대여 자체가 없을 때만 장기 진단을 선택한다.</summary>
    private Action<string, string, uint>? AssertionDevice()
    {
        ProcessHostRuntimeBindings? current = _currentRuntime?.Invoke();
        return current is not null ? current.Assertion : _state.Assertion;
    }

    /// <summary>Native Kill의 단언 식/파일/줄을 현재 장치의 함수 사본으로 전달하고 정상 복귀를 허용한다.</summary>
    private void DiagnoseKill(string expression, uint line)
    {
        Action<string, string, uint> assertion = AssertionDevice() ?? throw new InvalidOperationException("Native Process Kill의 실제 진단 미연결");
        assertion(expression, @"\Ns\O\Baseprocess.cpp", line);
    }
}
