using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Input;

// 숫자 키의 저장 카메라10칸은 Tab의 최근 배치5칸과 독립인 장기 전역이다.
public sealed class SavedCameraKeyHooks
{
    /// <summary>실제 현재 정지 판정이다.</summary>
    public Func<bool>? Paused { get; set; }
    /// <summary>정지 뒤 소비하는 현재 공개 언어 번호다.</summary>
    public Func<int>? Language { get; set; }
    /// <summary>실제 선택음이다.</summary>
    public Action? Sound { get; set; }
    /// <summary>선택음 뒤 실제 Renderer 지도 중심을 읽는다.</summary>
    public Func<ConstructionNoticePoint>? Capture { get; set; }
    /// <summary>저장된 두 단정도 좌표를 실제 즉시 카메라에 전달한다.</summary>
    public Action<float, float>? Center { get; set; }
}

/// <summary>숫자 호출·Shift 실제 기호 저장이며 다른 입력·뗌은 소비하지 않는다.</summary>
public static class RawSavedCameraKey
{
    /// <summary>표준·영어1·독일어3의 열 기호를 같은 인덱스로 대응한다.</summary>
    private const string Digits = "1234567890";
    /// <summary>나머지 공개 번호가 쓰는 원문 기본 표다.</summary>
    private const string DefaultSymbols = "!@#$%^&*()";
    /// <summary>공개 번호 1의 영어 기호 표다.</summary>
    private const string EnglishOneSymbols = "!\"\u00a3$%^&*()";
    /// <summary>공개 번호 3의 독일어 기호 표다.</summary>
    private const string GermanSymbols = "!\"\u00a7$%&/()=";

    /// <summary>나머지 공개 번호는 원문 기본 표를 사용한다.</summary>
    /// <param name="language">현재 공개 언어 번호다.</param>
    /// <returns>Shift 실제 기호 표다.</returns>
    private static string Symbols(int language) => language == 3 ? GermanSymbols : language == 1 ? EnglishOneSymbols : DefaultSymbols;

    /// <summary>원문의 signed ADD 하위DWORD 감김이다.</summary>
    /// <param name="first">첫 피연산자다.</param>
    /// <param name="second">더할 값이다.</param>
    /// <returns>감긴 합이다.</returns>
    private static int Add(int first, int second) => unchecked((int)(unchecked((uint)first) + unchecked((uint)second)));

    /// <summary>언어를 미리 읽지 않고 세 표에 속할 수 있는 문자만 좁은 키 몸체로 받는다.</summary>
    /// <param name="character">사건 코드의 아래쪽 값이다.</param>
    /// <returns>숫자·기호 후보이면 참이다.</returns>
    private static bool Candidate(uint character)
    {
        if (character > 255) return false; char value = (char)character;
        return Digits.Contains(value) || DefaultSymbols.Contains(value) || EnglishOneSymbols.Contains(value) || GermanSymbols.Contains(value);
    }

    /// <summary>최초 열 좌표를 모두 같은 원문 무효 Point로 쓴다.</summary>
    /// <param name="saved">숫자 키의 별도80BYTE 장기 소유자다.</param>
    public static void InitializeSavedCameraPositions(SavedCameraPositions saved)
    {
        ArgumentNullException.ThrowIfNull(saved);
        for (int index = 0; index < saved.Points.Length; index++) saved.Points[index] = new(-10000f, -10000f);
    }

    /// <summary>현재 Renderer·지도LTRB와 signed DWORD 감김을 보존한다.</summary>
    /// <param name="view">현재 Renderer 앞64BYTE다.</param>
    /// <param name="viewport">왼쪽·위·오른쪽·아래 지도 영역이다.</param>
    /// <returns>단정도 지도 중심이다.</returns>
    public static ConstructionNoticePoint CaptureSavedCameraCenter(RawCameraViewState view, int[] viewport)
    {
        ArgumentNullException.ThrowIfNull(view); ArgumentNullException.ThrowIfNull(viewport);
        if (viewport.Length != 4) throw new ArgumentOutOfRangeException(nameof(viewport), "저장카메라지도영역LTRB오류");
        int x = Add(Add(viewport[0], viewport[2]) / 2, view.Pixels[0]), y = Add(Add(viewport[1], viewport[3]) / 2, view.Pixels[1]);
        return new(unchecked((float)((double)x / BitConverter.UInt32BitsToSingle(view.CellFloatBits[0]))), unchecked((float)((double)y / BitConverter.UInt32BitsToSingle(view.CellFloatBits[1]))));
    }

    /// <summary>두 축 모두 0 이상 256 미만이면 참이다. Tab의 strict 판정·현재 세계 크기와 합치지 않는다.</summary>
    /// <param name="point">검사할 저장 좌표다.</param>
    /// <returns>불러올 수 있으면 참이다.</returns>
    public static bool IsSavedCameraPositionValid(ConstructionNoticePoint point) => 0f <= point.X && 0f <= point.Y && point.X < 256f && point.Y < 256f;

    /// <summary>숫자 호출·Shift 실제 기호 저장을 실행한다. 저장·불러오기·무시가 모두 참이다.</summary>
    /// <param name="input">소비할 입력 사건이다.</param>
    /// <param name="saved">숫자 키의 별도80BYTE 장기 소유자다.</param>
    /// <param name="hooks">정지·언어·소리·중심·즉시 카메라 장치다.</param>
    /// <returns>항상 참이며 거짓은 뗌·후보 아님뿐이다.</returns>
    /// <remarks>2026-11-26 1.049에서 DI-4c 저장 카메라용으로 추가했다. 근거: cpppj SavedCameraKey.cpp의 DispatchSavedCameraKey다. Ctrl/Alt/PressedSince는 제거하지 않는다.</remarks>
    public static bool DispatchSavedCameraKey(InputEvent input, SavedCameraPositions saved, SavedCameraKeyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(saved); ArgumentNullException.ThrowIfNull(hooks);
        if ((input.Code & InputCode.Release) != 0 || !Candidate(input.Code & 0x03ffffffu)) return false;
        if (hooks.Paused is null) throw new InvalidOperationException("숫자저장카메라실제정지판정미연결"); if (hooks.Paused()) return true;
        if (hooks.Language is null) throw new InvalidOperationException("숫자저장카메라현재공개언어미연결"); string symbols = Symbols(hooks.Language()); char character = (char)(input.Code & 255u);
        if (!Digits.Contains(character) && !symbols.Contains(character)) return true;
        bool shift = (input.Code & InputCode.Shift) != 0; if ((input.Code & 0x03ffffffu) == '*' && !shift) return true;
        uint plain = input.Code & ~InputCode.Shift; uint index;
        if (shift) { if (plain > 255) return true; int found = symbols.IndexOf((char)plain); if (found < 0) return true; index = unchecked((uint)found); }
        else index = plain == '0' ? 9u : plain - (uint)'1';
        if (index >= (uint)saved.Points.Length) return true;
        if (shift)
        {
            if (hooks.Sound is null) throw new InvalidOperationException("숫자저장카메라실제선택음미연결"); hooks.Sound();
            if (hooks.Capture is null) throw new InvalidOperationException("숫자저장카메라현재Renderer중심미연결"); saved.Points[index] = hooks.Capture();
        }
        else if (IsSavedCameraPositionValid(saved.Points[index]))
        {
            if (hooks.Center is null) throw new InvalidOperationException("숫자저장카메라실제즉시중심미연결"); hooks.Center(saved.Points[index].X, saved.Points[index].Y);
        }
        return true;
    }
}
