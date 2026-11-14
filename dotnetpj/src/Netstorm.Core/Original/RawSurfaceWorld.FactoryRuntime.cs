// 실제 World가 현재 Session/기본 종단을 선택하는 짧은 Factory 장치를 소유한다.
namespace Netstorm.Core.Original;

/// <summary>월드의 실제 공간/Form 장치를 공장의 약한 대여에 연결한다.</summary>
public sealed partial class RawSurfaceWorld
{
    /// <summary>이 World가 소유하고 공장은 약하게 빌리는 짧은 실제 대여다.</summary>
    private SquidFactoryRuntimeBindings? _factoryRuntime;

    /// <summary>이미 만든 같은 풀/공장의 실제 장치만 연결하며 생성·분리·전송을 미리 실행하지 않는다.</summary>
    private void BindFactoryRuntime()
    {
        _factoryRuntime = new()
        {
            Unpop = _unpop,
            FormDetach = sid => CurrentContainedUnpop().Detach(sid),
            ContentType = () => CurrentContainedUnpop().ContentFormType(),
            FormTransmit = (sid, flags) => (_runtime?.FormTransmit ?? throw new InvalidOperationException("현재 공통 Form Pop 전송 미연결"))(sid, flags),
        };
        _factory!.BindRuntime(_factoryRuntime);
    }

    /// <summary>그때 살아 있는 실제 Session 종단을 선택하고 반납 뒤 같은 World 기본 종단으로 돌아간다.</summary>
    private RawSquidContainedUnpop CurrentContainedUnpop() => _sessionContainedUnpop is not null && _sessionContainedUnpop.TryGetTarget(out RawSquidContainedUnpop? current) ? current : _containedUnpop;

}
