using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 Native 자식 정상 복귀를 모아 Core0 뒤 현재 Runtime을 반납한다.
namespace Netstorm.Client;

/// <summary>구성 자체로 정리하지 않으며 같은 Client·Surface·세션 세대의 실제 증빙만 소비한다.</summary>
public sealed class ClientStateRuntimeRetirement
{
    /// <summary>장기 Gameplay/Player/Kernel/물리의 실제 소유자다.</summary>
    private readonly ClientSurfaceGlobals _surface;
    /// <summary>같은 표면의 현재 대여 세대이며 새로운 게임 값을 발급하지 않는다.</summary>
    private readonly ulong _generation;
    /// <summary>정리 도중 다른 세션으로 재선택하지 않는 비소유 현재 정체성이다.</summary>
    private WeakReference<HostSession>? _session;
    /// <summary>실제 단일 Kernel Clear 정상 복귀에서만 생기는 같은 Host의 증빙이다.</summary>
    private RawProcessHostStorage.ClearPass? _processes;

    /// <summary>현재 실제 Client/세션 정체성만 검증하며 Player·Pool·시계를 초기화하지 않는다.</summary>
    public ClientStateRuntimeRetirement(ClientSurfaceGlobals surface)
    {
        ArgumentNullException.ThrowIfNull(surface);
        _surface = surface;
        _generation = surface.Storage.Generation();
        if (surface.Gameplay.Borrowed()) _session = new(surface.Gameplay.RequireSession());
        _ = Current();
    }

    /// <summary>각 선택 위치에서 같은 세대·현재 Player 고정뷰·Kernel을 다시 검증한다.</summary>
    private HostSession? Current()
    {
        ClientGameplayGlobals game = _surface.Gameplay;
        RawSurfaceStorage storage = _surface.Storage;
        if (storage.Generation() != _generation) throw new InvalidOperationException("State 정리 도중 표면 대여 세대 변경");
        if (game.Borrowed())
        {
            HostSession current = game.RequireSession();
            current.EnsureStateRuntimeIdle();
            if (_session is null || !_session.TryGetTarget(out HostSession? expected) || !ReferenceEquals(current, expected)
                || !storage.Borrowed() || !ReferenceEquals(current.Pool, storage.Pool) || !ReferenceEquals(current.Kernel, _surface.Kernel)
                || !ReferenceEquals(current.PlayerRecordsOwner, game.PlayerRecords)
                || !game.PlayerRecords.Records.Equals((Memory<byte>)current.PlayerRecords))
                throw new InvalidOperationException("State 정리 도중 현재 Session/표면/Player 정체성 변경");
            return current;
        }
        if (_session is not null || storage.Borrowed()) throw new InvalidOperationException("State 자식 정상 반납 전에 현재 Runtime 대여가 사라짐");
        return null;
    }

    /// <summary>원문 owner1..8 위치에서 실제 AI reset과 같은 참여 모듈의 증빙 소비를 수행한다.</summary>
    public void ResetAi(uint owner)
    {
        if (owner < 1 || owner > 8) throw new ArgumentOutOfRangeException(nameof(owner), "State AI 정리의 owner1..8 범위");
        if (Current() is { } current)
        {
            var pass = current.ResetAiForState(owner);
            Current()!.ReturnAiForState(pass);
        }
        else if (BinaryPrimitives.ReadUInt32LittleEndian(_surface.Gameplay.PlayerRecords.Records.Span.Slice((int)owner * 180 + 0xb0, 4)) != 0)
            throw new InvalidOperationException("현재 비영 brain의 실제 Runtime 소멸 장치 누락");
    }

    /// <summary>실제0048fcd0과 동일 Deck 반납 증빙을 사용하며 첫 메뉴도 같은 장기 메모리를 반납한다.</summary>
    public void ClearPlayerLinks()
    {
        if (Current() is { } current)
        {
            var pass = current.ClearPlayerLinksForState();
            Current()!.ReturnPlayerLinksForState(pass);
        }
        else RawPlayerLinksCleanup.Run(new()
        {
            Players = () => _surface.Gameplay.PlayerRecords.Records,
            Release = token => _surface.Gameplay.ProductionDeckStorage.Release(token),
        });
    }

    /// <summary>반복 Drain 대신 같은 전역 Kernel의 단일 Clear를 원문 위치에서 실행한다.</summary>
    public void ClearKernel()
    {
        if (_processes is not null) throw new InvalidOperationException("같은 State Runtime의 단일 Kernel Clear 증빙 중복 생성");
        if (Current() is { } current) _processes = current.ClearProcessesForState();
        else _surface.Kernel.Clear();
    }

    /// <summary>known0/Core0·모든 실제 자식 증빙 뒤 동기 owner 반납을 요구하고 빈 callback을 거부한다.</summary>
    public void Retire(Action? releaseOwner)
    {
        HostSession? current = Current();
        ClientGameplayGlobals game = _surface.Gameplay;
        if (game.Player.PlayerKnown != 0 || game.CoreData.Active != 0 || !game.CoreData.Current.IsEmpty)
            throw new InvalidOperationException("State Runtime 반납 전 실제 known0/Core0 선택 필요");
        if (current is null) return;
        if (!current.WorldProductionDeck.PlayerLinksReturnedForState)
            throw new InvalidOperationException("State Runtime 반납 전 실제 Player 링크 증빙 필요");
        // 각각의 원문 owner 증빙을 확인하며 누락을 빈 brain이나 개수로 대신하지 않는다.
        for (uint owner = 1; owner < 9; owner++)
            if (!current.BrainReturnedForState(owner)) throw new InvalidOperationException("State Runtime 반납 전 실제 여덟 AI 증빙 필요");
        if (releaseOwner is null) throw new InvalidOperationException("State Runtime의 실제 소유자 반납 장치 누락");
        if (!current.ProcessBindingsReturned())
        {
            current.ReturnProcessBindingsForState(_processes ?? throw new InvalidOperationException("State Runtime 반납 전 실제 단일 Kernel Clear 증빙 필요"));
            _processes = null;
        }
        releaseOwner();
        if (game.Borrowed() || _surface.Storage.Borrowed() || !current.PlayerRecordViewsReleased)
            throw new InvalidOperationException("State 소유자 장치가 실제 Session/표면/Player 대여를 반납하지 않음");
        _session = null;
    }
}
