// 현재 World의 typed 전송/외부 Host만 장기 writer에 짧게 대여한다.
namespace Netstorm.Core.Original;

/// <summary>같은 Storage writer를 재사용하고 현재 Runtime owner의 호출 수명을 연결한다.</summary>
public sealed partial class RawSurfaceWorld
{
    /// <summary>이 World가 소유하고 Storage가 약하게 빌리는 실제 현재 writer 대여다.</summary>
    private RawSurfacePopBindings? _popRuntime;

    /// <summary>실제 현재 typed 장치를 공개한 뒤 같은 장기 writer를 빌린다. 결합 자체는 송신하지 않는다.</summary>
    private void BindStorageTransmission()
    {
        var runtime = new RawSurfacePopBindings
        {
            Transmit = (sid, flags) => (_squidTransmission ?? throw new InvalidOperationException("현재 World의 실제 Squid writer 미연결")).Pop(sid, flags),
            Broadcast = packet =>
            {
                if (!_runtimeBound) return;
                (_runtime?.SquidPacket ?? throw new InvalidOperationException("현재 World의 Squid 방송 장치 미연결"))(packet.ToArray());
            },
            Connected = target => (_runtimeBound ? _runtime?.SquidConnected ?? throw new InvalidOperationException("현재 대상별 Player 연결 관문 미연결") : throw new InvalidOperationException("현재 World runtime 미연결"))(target),
            Send = (target, packet) => (_runtimeBound ? _runtime?.SquidSend ?? throw new InvalidOperationException("현재 대상별 Squid 송신 장치 미연결") : throw new InvalidOperationException("현재 World runtime 미연결"))(target, packet.ToArray()),
            ProcessPacket = (form, packet, argument) => _storage.Processes().Host().WriteFormPacket(form, argument, packet, _storage.GameAux().SquidTransmission),
            Pin = _runtimePin,
        };
        _storage.BindPopRuntime(runtime); _popRuntime = runtime; _squidTransmission = _storage.TransmissionExecutor();
        _pop!.BindTransmission(_storage.TransmitPop);
    }
    /// <summary>명시 종료/생성 실패에서 자기 현재 대여만 반납한다.</summary>
    private void ReleaseStorageTransmission()
    {
        if (_popRuntime is null) return;
        _storage.ReleasePopRuntime(_popRuntime); _popRuntime = null;
    }
}
