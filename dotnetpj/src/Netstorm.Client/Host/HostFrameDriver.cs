using Netstorm.Client.Automation;
using Netstorm.Client.Display;
using Netstorm.Client.Input;

namespace Netstorm.Client.Host;

/// <summary>
/// 창 없이도 도는 호스트 프레임 골격. 논리 화면·입력 큐를 소유하고 UI 스크립트 실행기를 프레임 번호에 맞춰 부른다.
/// MonoGame 쪽(<c>Netstorm.Client.Host</c> 프로젝트)은 이 골격에 창·그리기·실제 입력을 붙이기만 한다.
/// 근거: cpppj/src/app/main.cpp 의 --run 프레임 고리. (R1-2a, 2026-10-28)
/// </summary>
public sealed class HostFrameDriver
{
    /// <summary>논리 화면. 호스트가 <see cref="Screen.Output"/>을 붙인 뒤 그린다.</summary>
    public Screen Screen { get; }

    /// <summary>스크립트·실제 입력이 함께 넣는 입력 큐.</summary>
    public InputQueue Input { get; }

    /// <summary>끝난 프레임 수. 스크립트 단계의 프레임 번호와 같은 기준이다(1부터).</summary>
    public ulong Frame { get; private set; }

    /// <summary>관측 단계를 처리한 마지막 프레임. 따라잡기 Update 뒤에도 한 프레임씩 AfterFrame을 부른다.</summary>
    private ulong _ended;

    /// <summary>UI 스크립트 실행기. 스크립트가 없으면 null이다.</summary>
    private readonly UiScriptRunner? _runner;

    /// <summary>
    /// 화면·큐·실행기를 묶는다. 화면 크기는 호출자가 정한다.
    /// </summary>
    /// <param name="screen">논리 화면.</param>
    /// <param name="input">입력 큐.</param>
    /// <param name="runner">UI 스크립트 실행기(없으면 null).</param>
    public HostFrameDriver(Screen screen, InputQueue input, UiScriptRunner? runner = null)
    {
        ArgumentNullException.ThrowIfNull(screen);
        ArgumentNullException.ThrowIfNull(input);
        Screen = screen;
        Input = input;
        _runner = runner;
    }

    /// <summary>프레임 번호를 올리고 스크립트의 이번 프레임 입력 단계를 큐에 넣는다. 실제 입력 펌프보다 먼저 부른다.</summary>
    public void BeginFrame()
    {
        Frame++;
        _runner?.BeforeInput(Frame);
    }

    /// <summary>
    /// 그리기가 끝난 뒤 밀린 프레임의 캡처·관측 단계를 한 프레임씩 처리하고 위치를 전진한다.
    /// 고정 시간 따라잡기로 Update가 여러 번 돈 뒤 Draw가 한 번만 돌아도 중간 프레임을 건너뛰지 않는다.
    /// </summary>
    public void EndFrame()
    {
        while (_ended < Frame)
        {
            _ended++;
            _runner?.AfterFrame(_ended);
        }
    }

    /// <summary>
    /// 프레임 제한에 닿았는지. 제한이 0이면 창을 닫을 때까지 돌므로 항상 거짓이다.
    /// </summary>
    /// <param name="frames">--frames 값.</param>
    public bool ShouldExit(ulong frames) => frames != 0 && Frame >= frames;

    /// <summary>
    /// 스크립트가 있으면 남은 단계 없이 끝났는지 확인한다. 프레임 제한·오류로 남은 단계를 조용히 지나치지 않기 위한 마무리 검사다.
    /// </summary>
    public void Finish() => _runner?.RequireFinished();

    /// <summary>건너뛴 관측 단계("프레임:동작" 형식). 스크립트가 없으면 비어 있다.</summary>
    public List<string> SkippedObservations => _runner?.SkippedObservations ?? [];
}
