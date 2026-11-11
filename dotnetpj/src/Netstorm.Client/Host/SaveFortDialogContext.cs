namespace Netstorm.Client.Host;

/// <summary>Save as 확인의 시나리오 전용 분기와 파일 실패 안내를 묶는 실제 문맥이다.
/// 근거: cpppj/src/client/SaveFortNameDialog.h 의 SaveFortNameDialogContext 중 S1 3종(editorAccept·editorCancel·reportError)과 다중 줄 여부다.
/// 2026-11-11 0.365에서 cpppj HEAD(68a9545e2)의 RawMenuSaveFort.inc 분기를 반영했다.</summary>
public sealed class SaveFortDialogContext
{
    /// <summary>클론 Scenario 전용 확정이다. 거짓이면 같은 편집창을 유지한다.</summary>
    public Func<string, bool>? EditorAccept { get; init; }

    /// <summary>Scenario 취소 뒤 실제 패널로 돌아가는 일반 명령을 예약한다.</summary>
    public Action? EditorCancel { get; init; }

    /// <summary>Scenario 본문 Enter는 줄바꿈이고 OK 버튼으로만 확정하면 참이다.</summary>
    public bool EditorMultiline { get; init; }

    /// <summary>클론 파일 작업 실패를 같은 창에 안내한다. 없으면 원문 예외 계약이다.</summary>
    public Action<string>? ReportError { get; init; }

    /// <summary>시나리오 확정 분기를 판정한다. 확정 장치가 있으면 본문으로 확정하고 참을 돌려준다.</summary>
    /// <param name="context">토큰에 묶은 문맥이다. 없거나 확정 장치가 없으면 거짓이다.</param>
    /// <param name="text">실제 편집 버퍼의 전체 본문이다.</param>
    /// <param name="result">확정 결과다. 참이면 1, 거짓이면 0이며 같은 편집창을 유지한다.</param>
    /// <returns>시나리오 분기로 처리했으면 참이다.</returns>
    public static bool TryAcceptScenario(SaveFortDialogContext? context, string text, out uint result)
    {
        if (context?.EditorAccept is null)
        {
            result = 0;
            return false;
        }

        result = context.EditorAccept(text) ? 1u : 0u;
        return true;
    }

    /// <summary>시나리오 취소 분기를 판정한다. 취소 장치가 있으면 예약하고 참을 돌려준다.</summary>
    /// <param name="context">토큰에 묶은 문맥이다. 없거나 취소 장치가 없으면 거짓이다.</param>
    /// <param name="result">취소 결과다. 처리했으면 1이다.</param>
    /// <returns>시나리오 분기로 처리했으면 참이다.</returns>
    public static bool TryCancelScenario(SaveFortDialogContext? context, out uint result)
    {
        if (context?.EditorCancel is null)
        {
            result = 0;
            return false;
        }

        context.EditorCancel();
        result = 1u;
        return true;
    }

    /// <summary>일반 확인을 파일 실패 안내로 감싼다. 안내 장치가 없으면 원문처럼 예외를 그대로 던진다.</summary>
    /// <param name="context">토큰에 묶은 문맥이다. 없으면 원문 예외 계약이다.</param>
    /// <param name="normal">일반 이름 확인 본체다.</param>
    /// <returns>일반 확인 결과다. 실패를 안내했으면 0이며 원본/이전 파일을 보존한다.</returns>
    public static uint AcceptWithReport(SaveFortDialogContext? context, Func<uint> normal)
    {
        ArgumentNullException.ThrowIfNull(normal);
        try
        {
            return normal();
        }
        catch (Exception error)
        {
            if (context?.ReportError is null)
            {
                throw;
            }

            context.ReportError(error.Message);
            return 0;
        }
    }
}
