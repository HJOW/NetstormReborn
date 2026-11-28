using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

// 같은 Game의 기술 DWORD·화면·캐시 시각을 실제 Surface Process Host에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>CPP ClientMain의 StartThunderFlash/ClearThunderFlashes에 해당하는 장기 소유자다. Kernel이 실제 프로세스를 소유한다.</summary>
public sealed class HostThunderFlashes
{
    /// <summary>현재 Pool/TYPE/권한/Process Host를 빌리는 실제 Game Surface다.</summary>
    public ClientSurfaceGlobals Surface { get; }
    /// <summary>복구할 저장 팔레트와 현재 화면 비트를 소유한다.</summary>
    public Screen Screen { get; }
    /// <summary>같은 Process Host가 사용하는 실제 Kernel이다.</summary>
    public Kernel Kernel { get; }
    /// <summary>동적 시계가 아닌 같은 Frame.Wall 캐시를 빌린다.</summary>
    public ClientClockGlobals Clock { get; }
    /// <summary>정상 부착 복귀 뒤의 실제 PID만 순서대로 기록한다. 소유권과 프레임 실행을 대신하지 않는다.</summary>
    private readonly List<uint> _registered = [];
    /// <summary>실제 소멸 callback이 아직 반납하지 않은 정상 부착 수다.</summary>
    public int RegisteredCount => _registered.Count;

    /// <summary>동일 장기 장치만 확인한다. 기술/시각/팔레트·Host 생성·부착은 실행하지 않는다.</summary>
    public HostThunderFlashes(ClientSurfaceGlobals surface, Screen screen, Kernel kernel, ClientClockGlobals clock)
    {
        ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(screen);
        ArgumentNullException.ThrowIfNull(kernel); ArgumentNullException.ThrowIfNull(clock);
        if (!ReferenceEquals(surface.Kernel, kernel) || !ReferenceEquals(surface.Clock, clock)
            || !ReferenceEquals(clock.LocalState.Gameplay, surface.Gameplay)
            || !ReferenceEquals(surface.Storage.Authority(), surface.Gameplay.Authority)
            || !ReferenceEquals(surface.Storage.GameAux(), surface.Gameplay.GameAux)
            || surface.Storage.Pool.Edition != OriginalEdition.Patch1078
            || !surface.Processes.HasDevices() || !surface.Processes.UsesKernel(kernel))
            throw new ArgumentException("번개의 실제 같은 Surface/Kernel/Frame 소유자 필요", nameof(surface));
        Surface = surface; Screen = screen; Kernel = kernel; Clock = clock;
    }

    /// <summary>현재 미션의 실제 기술 부모가 있을 때만 TYPE55/flags10으로 부착한다. 미준비 Player는 기록도 읽지 않는다.</summary>
    public void Request(int brightness = 0)
    {
        if (Surface.Gameplay.Player.PlayerKnown == 0) return;
        RawSurfaceStorage storage = Surface.Storage;
        if (storage.OwnerMode.Challenge)
            throw new InvalidOperationException("번개의 메타맵00479650 실제 기술 기록 조회 미복원");
        Memory<byte> players = Surface.Gameplay.CoreData.Players;
        long offset = (long)Surface.Gameplay.Player.BattleOwner * 180 + 0x9c;
        if (offset > players.Length || players.Length - offset < 4)
            throw new ArgumentOutOfRangeException(nameof(players), "번개의 현재 Player 기술 DWORD 물리 범위");
        uint parent = BinaryPrimitives.ReadUInt32LittleEndian(players.Span.Slice(checked((int)offset), 4));
        if (parent == 0) return;
        if (parent > ushort.MaxValue || parent >= storage.Pool.Capacity)
            throw new ArgumentOutOfRangeException(nameof(parent), "번개 기술 부모의 전체 DWORD SID 범위");
        var process = new ThunderProcess((Screen.Flags & ScreenMode.FullScreen) != 0, new()
        {
            WallSeconds = () => Clock.Frame.Value.Wall,
            FullScreen = () => (Screen.Flags & ScreenMode.FullScreen) != 0,
            ReadPalette = (start, count, destination) => Screen.Palette.Slice(checked((int)start), checked((int)count)).CopyTo(destination),
            ApplyPalette = (start, count, colors) => Screen.SetPalette(start, count, colors, true),
            RestorePalette = () => Screen.SetPalette(0, 256, null, true),
        }, id => _registered.RemoveAll(current => current == id), brightness);
        SquidProcessHost host = Surface.Processes.Host();
        SquidProcess? attached = host.Attach(process, ThunderProcess.ProcessType, checked((int)parent), 0x10);
        // OnAttached 이전/도중의 부분 실패는 Host/Kernel의 소유 계약을 따른다. 임의 rollback이나 수동 Dispose를 하지 않는다.
        if (attached is null) return;
        uint pid = Kernel.Find(attached);
        if (!ReferenceEquals(attached, process) || pid == 0 || !ReferenceEquals(host.FromForm(attached.Form()), attached))
            throw new InvalidOperationException("번개의 정상 부착과 실제 Kernel/Form 정체성 불일치");
        _registered.Add(pid);
    }

    /// <summary>소멸 callback이 바꾼 현재 뒤 PID를 다시 읽어 실제 Form을 즉시 삭제하고 화면 변경 전 복구한다.</summary>
    public void Clear()
    {
        // 목록 사본을 돌지 않는다. 각 Kill의 실제 Dispose가 같은 목록에서 해당 PID를 지운다.
        while (_registered.Count != 0)
        {
            uint pid = _registered[^1];
            if (Kernel.Get(pid) is not ThunderProcess process || Kernel.Find(process) != pid)
                throw new InvalidOperationException("번개 등록 목록과 실제 Kernel 객체 불일치");
            SquidProcessHost host = Surface.Processes.Host(); int form = process.Form();
            if (form <= 0 || form >= Surface.Storage.Pool.Capacity || !host.IsProcessForm(form)
                || !ReferenceEquals(host.FromForm(form), process))
                throw new InvalidOperationException("번개의 실제 ProcessForm/Host 정체성 불일치");
            host.Kill(process, 0x10);
            if (_registered.Contains(pid)) throw new InvalidOperationException("번개의 실제 소멸 뒤 PID 반납 미완료");
        }
    }
}
