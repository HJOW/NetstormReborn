using Netstorm.Core.Original;

// 기존 팔레트 계산을 실제 ProcessForm/부모 종속 삭제와 같은 Kernel 수명에 결합한다.
namespace Netstorm.Client.Display;

/// <summary>CPP ThunderProcess의 TYPE55 본체다. 화면은 빌리고 실제 프로세스 소유권은 Host/Kernel에 넘긴다.</summary>
public sealed class ThunderProcess : SquidProcess
{
    /// <summary>10.78 번개 프로세스의 내장 TYPE 번호이며 부착 flags는 원문0x10이다.</summary>
    public const uint ProcessType = 55;
    /// <summary>같은 부분 팔레트 두 배열·64단계·기간/현재 벽시각의 효과 상태다.</summary>
    private readonly PaletteFlashState _state = new();
    /// <summary>원문 대조를 마친 기존 팔레트 초기화/진행/복구 몸체다.</summary>
    private readonly PaletteFlash _flash;
    /// <summary>같은 Client의 현재 PID 등록만 해제하며 Form을 다시 삭제하지 않는 외부 통지다.</summary>
    private readonly Action<uint>? _destroyed;

    /// <summary>생성 당시 모드/요청 밝기만 보관한다. 부착 전에는 시각/팔레트/현재 모드를 읽지 않는다.</summary>
    public ThunderProcess(bool fullScreen, PaletteFlashHooks hooks, Action<uint>? destroyed, int brightness = 0)
    {
        _flash = new PaletteFlash(_state, hooks); _destroyed = destroyed;
        _state.Duration = PaletteFlashRange.ThunderDuration(fullScreen);
        _state.Offset = PaletteFlashRange.ThunderOffset(brightness);
    }

    /// <summary>공통 Attach가 실제 Form·부모·PID를 연결한 뒤 원문 가상+14에서 초기화한다.</summary>
    public override void OnAttached() => _flash.Init();

    /// <summary>실제 Kernel 프레임에서 벽시각 효과가 끝나면 같은 Host의 Form 삭제를 선택한다.</summary>
    public override void RunFrame()
    {
        if (!_flash.Frame())
            (Host() ?? throw new InvalidOperationException("번개의 실제 프로세스 Host 미연결")).Kill(this, 0);
        // Kill 뒤에는 이 객체의 필드/소유권을 다시 읽지 않는다.
    }

    /// <summary>Screen이 살아 있는 동안 취소/복구한 뒤 같은 PID를 통지한다. 자연 종료는 이미 복구해 중복 적용하지 않는다.</summary>
    public override void Dispose()
    {
        _flash.Cancel();
        _destroyed?.Invoke(Pid());
        base.Dispose();
    }
}
