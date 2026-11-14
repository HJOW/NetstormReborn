// 같은 실제 월드에 현재 Pop 부모·표시·직렬화·방송 장치를 연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawSurfaceWorld/Storage의 현재 Pop 전송 수명과 독립 typed 월드 방송 관문이다.</summary>
public sealed partial class RawSurfaceWorld
{
    /// <summary>현재 월드가 빌리는 실제 writer다. 버퍼와 누적은 장기 GameAux가 소유한다.</summary>
    private RawSquidTransmission? _squidTransmission;

    /// <summary>현재 TYPE와 진단을 연결하고 PATCH에서만 실제35BYTE/압축 writer를 설치한다.</summary>
    private void BindCurrentPop()
    {
        if (_edition != OriginalEdition.Patch1078) return;
        RawPop pop = _pop ?? throw new InvalidOperationException("현재 Pop 부모 미연결");
        pop.BindDiagnostics(new SquidPopDiagnostics
        {
            Checking = _storage.Checking(), Types = _types, Metamap = () => _storage.OwnerMode.Challenge,
            Trace = _storage.TraceGraph, Assertion = _storage.AssertDisplay,
        });
        RawSquidFactory factory = _factory ?? throw new InvalidOperationException("현재 전송 HP 공장 미연결");
        factory.BindDiagnostics(new SquidFactoryDiagnostics
        {
            Types = _types, Checking = _storage.Checking(), Assertion = _storage.AssertDisplay,
            Detail = _storage.RecordAssertionDetail,
        });
        factory.BindWeakenedMana(_storage.GameAux().PriestQuarterHpBox);
        factory.BindHitPointTypes(_storage.GameAux().PriestTypeBox, _storage.GameAux().DaisTypeBox);
        var writer = new RawSquidTransmission(_storage.Pool, _storage.GameAux().SquidTransmission, new RawSquidTransmissionHooks
        {
            Type = number => new SquidPopTypeReference(() => _types[number]),
            DefaultType = () => _storage.Links.BridgeType,
            MaxHp = number => factory.ReadSelectedHitPoints(_types[number], _types),
            Trace = _storage.TraceGraph, Assertion = _storage.AssertDisplay,
            Broadcast = packet =>
            {
                // cpp RawSurfaceWorld의 독립 typed 월드는 writer 이후 방송에서 자연 반환한다.
                if (!_runtimeBound) return;
                BattlePacketSend send = _runtime?.SquidPacket ?? throw new InvalidOperationException("현재 월드 실제 Squid 방송 미연결");
                send(packet.ToArray());
            },
            Connected = target =>
            {
                Func<int, bool> connected = _runtimeBound ? _runtime?.SquidConnected ?? throw new InvalidOperationException("현재 월드 Player 연결 미연결") : throw new InvalidOperationException("현재 월드 runtime 미연결");
                return connected(target);
            },
            Send = (target, packet) =>
            {
                Action<int, byte[]> send = _runtimeBound ? _runtime?.SquidSend ?? throw new InvalidOperationException("현재 월드 Squid 대상 송신 미연결") : throw new InvalidOperationException("현재 월드 runtime 미연결");
                send(target, packet.ToArray());
            },
            ProcessPacket = (form, packet, argument) => _storage.Processes().Host().WriteFormPacket(form, argument, packet, _storage.GameAux().SquidTransmission),
        });
        _squidTransmission = writer;
        pop.BindTransmission(writer.Pop);
    }
}
