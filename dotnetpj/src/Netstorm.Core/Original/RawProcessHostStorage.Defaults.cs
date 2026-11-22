// 기본 장치는 함수 값만 먼저 보관하고 실제 외부 Kernel 결합 뒤 Host에 인계한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/client/RawProcessHostStorage.cpp의 최초 장기 기본 장치 설치다.</summary>
public sealed partial class RawProcessHostStorage
{
    /// <summary>현재 같은 장기 삭제 부모를 소비 위치에서 읽는다.</summary>
    private Func<RawSquidDestroy>? _defaultDestroy;
    /// <summary>World 없는 실제 VT의 장기 정기 본체다.</summary>
    private RegularHandler? _defaultRegular;
    /// <summary>같은 장기 가상 Form/자산 분배를 소비 위치에서 읽는다.</summary>
    private Func<SquidDestroyHooks>? _defaultAssets;
    /// <summary>같은 장기 Pool 공장을 나중에 반환하는 장치다.</summary>
    private Func<RawSquidFactory>? _defaultFormFactory;
    /// <summary>실제 공유 PostPop 깊이를 사용 시점마다 읽는다.</summary>
    private Func<uint>? _defaultPostPopDepth;
    /// <summary>최초 직렬화 장치의 함수 슬롯 사본이다.</summary>
    private ProcessFormTransmission? _defaultTransmission;
    /// <summary>실제 원문 직렬화 진단 장치다.</summary>
    private Action<string, string, uint>? _defaultFormAssertion;

    /// <summary>Host/독립 Kernel/삭제기를 만들지 않고 수명 함수 값만 최초 보관한다.</summary>
    public void BindLifecycleDefaults(Func<RawSquidDestroy> destroy, RegularHandler regular, Func<SquidDestroyHooks> assets)
    {
        if (_host is not null || _defaultDestroy is not null || destroy is null || regular is null || assets is null)
            throw new ArgumentException("장기 Host 기본 수명 장치의 최초 결합 계약");
        _defaultDestroy = destroy; _defaultRegular = regular; _defaultAssets = assets;
    }
    /// <summary>Host/공장을 만들거나 깊이를 조회하지 않고 실제 공장 장치만 최초 보관한다.</summary>
    public void BindFormFactoryDefaults(Func<RawSquidFactory> factory, Func<uint> depth)
    {
        if (_host is not null || _defaultFormFactory is not null || factory is null || depth is null)
            throw new ArgumentException("장기 Host 기본 공장의 최초 장치 계약");
        _defaultFormFactory = factory; _defaultPostPopDepth = depth;
    }
    /// <summary>같은 버퍼/진단의 장치를 값 복사하며 Host/Kernel/직렬화를 조기 실행하지 않는다.</summary>
    public void BindFormTransmissionDefaults(ProcessFormTransmission transmit, Action<string, string, uint> assertion)
    {
        if (_host is not null || _defaultTransmission?.WriteData is not null || transmit is null || transmit.WriteData is null
            || transmit.WriteFullData is null || transmit.Packet is null || transmit.Transmit is null || assertion is null)
            throw new ArgumentException("장기 Host 기본 직렬화의 최초 장치 계약");
        _defaultTransmission = transmit.Clone(); _defaultFormAssertion = assertion;
    }
    /// <summary>실제 Host 생성 뒤 직렬화→공장→수명의 원문 순서로 한 번 인계한다.</summary>
    private void InstallDefaults(SquidProcessHost host)
    {
        if (_defaultTransmission?.WriteData is not null) host.BindDefaultFormTransmission(_defaultTransmission, _defaultFormAssertion!);
        if (_defaultFormFactory is not null) host.BindDefaultFormFactory(_defaultFormFactory, _defaultPostPopDepth!);
        if (_defaultDestroy is not null) host.BindDefaultLifecycle(_defaultDestroy, _defaultRegular!, _defaultAssets!);
    }
}
