namespace Netstorm.Client.Input;

/// <summary>
/// 사건 코드의 비트(창 프로시저 FUN_00436550·FUN_00436020이 만든다). 키와 마우스 버튼 사건이 하나의 32비트 코드에 들어간다.
/// 글자가 아닌 키는 가상 키 코드를 <see cref="VirtualKeyShift"/>만큼 왼쪽으로 밀어 넣고, 글자는 아래 16비트에 넣는다.
/// 근거: cpppj/src/client/InputEvent.h 의 InputCode. (2026-10-28 R1에서 이식)
/// </summary>
public static class InputCode
{
    /// <summary>뗌(키 올림, 버튼 올림).</summary>
    public const uint Release = 0x80000000;

    /// <summary>폴링(00452e00): 지난 확인 뒤에 눌린 적 있음.</summary>
    public const uint PressedSince = 0x40000000;

    /// <summary>Shift 눌림.</summary>
    public const uint Shift = 0x20000000;

    /// <summary>Control 눌림.</summary>
    public const uint Control = 0x10000000;

    /// <summary>Alt 눌림.</summary>
    public const uint Alt = 0x08000000;

    /// <summary>폴링: 커서가 창 밖이거나 창이 비활성.</summary>
    public const uint Outside = 0x04000000;

    /// <summary>프로그램이 넣은 명령 사건(00452fe0).</summary>
    public const uint Command = 0x02000000;

    /// <summary>왼쪽 버튼.</summary>
    public const uint LeftButton = 0x00010000;

    /// <summary>오른쪽 버튼.</summary>
    public const uint RightButton = 0x00020000;

    /// <summary>가운데 버튼.</summary>
    public const uint MiddleButton = 0x00040000;

    /// <summary>가상 키 코드를 코드의 위쪽으로 미는 비트 수.</summary>
    public const int VirtualKeyShift = 16;
}

/// <summary>입력 사건 하나: 코드와 화면 좌표(창 테두리를 뺀 커서 위치). 값이 모두 0이면 "사건 없음"이다.</summary>
/// <param name="Code"><see cref="InputCode"/> 비트의 조합.</param>
/// <param name="X">X 좌표.</param>
/// <param name="Y">Y 좌표.</param>
public readonly record struct InputEvent(uint Code, int X, int Y);

/// <summary>
/// 원본 전역 입력 사건 큐(DAT_0055a510의 40칸, 읽기 위치 DAT_0055a508, 쓰기 위치 DAT_0055a50c). 창 프로시저가 키·마우스를 여기에 넣고
/// 입력 처리(UserInput)가 꺼낸다. 한 칸은 가득 참을 구분하는 데 쓰여 39개까지 담기며, 가득 차면 방금 넣은 사건을 버린다.
/// 근거: 원본 FUN_00452f90(넣기)·FUN_00452fe0(명령 넣기)·FUN_00452f00(꺼내기), cpppj/src/client/InputEvent.cpp.
/// 현재 키·버튼 상태를 읽는 폴링(FUN_00452e00)은 창이 필요하므로 호스트의 몫이다. (2026-10-28 R1에서 이식, 9-1 B 처리)
/// </summary>
public sealed class InputQueue
{
    /// <summary>큐의 칸 수(원본 0x28). 한 칸은 가득 참을 구분하는 데 쓰여 39개까지 담긴다.</summary>
    public const int Capacity = 40;

    /// <summary>꺼낼 때 바꾸는 글자: 0x7f(DEL)는 Ctrl+Backspace(8)로 바뀐다.</summary>
    private const uint DeleteCharacter = 0x7f;

    /// <summary>사건 코드에서 글자 부분을 가려내는 마스크.</summary>
    private const uint CharacterMask = 0x03ffffff;

    /// <summary>명령 사건에 남기는 코드 비트.</summary>
    private const uint CommandCodeMask = 0x0100ffff;

    /// <summary>발생한 입력 사건을 담는 고리 버퍼.</summary>
    private readonly InputEvent[] _events = new InputEvent[Capacity];

    /// <summary>다음에 읽을 칸의 인덱스.</summary>
    private int _read;

    /// <summary>다음에 쓸 칸의 인덱스.</summary>
    private int _write;

    /// <summary>
    /// 사건을 끝에 넣는다(원본 FUN_00452f90). 쓰기 위치가 읽기 위치와 만나면 한 칸 되돌려 방금 넣은 사건을 버린다.
    /// </summary>
    /// <param name="code"><see cref="InputCode"/> 코드.</param>
    /// <param name="x">X 좌표.</param>
    /// <param name="y">Y 좌표.</param>
    public void Push(uint code, int x, int y)
    {
        _events[_write] = new InputEvent(code, x, y);
        _write = (_write + 1) % Capacity;
        if (_write == _read) _write = (_write + Capacity - 1) % Capacity;
    }

    /// <summary>명령 사건을 넣는다(원본 FUN_00452fe0). 코드는 0x0100ffff만 남기고 명령 표시를 붙인다.</summary>
    /// <param name="code">명령 코드.</param>
    /// <param name="x">X 좌표.</param>
    /// <param name="y">Y 좌표.</param>
    public void PushCommand(uint code, int x, int y) => Push((code & CommandCodeMask) | InputCode.Command, x, y);

    /// <summary>
    /// 사건 하나를 꺼낸다(원본 FUN_00452f00). 비었으면 0으로 채운 사건이다. 글자 0x7f(DEL)는 Ctrl+Backspace(8)가 된다.
    /// skipReleases가 참이면 뗌 사건은 꺼내서 버리고 0으로 채운 사건을 돌려준다. 원본은 꺼낸 뒤에 뗌 여부를 보므로 걸러진 뗌 사건도 큐에서는 빠진다.
    /// </summary>
    /// <param name="skipReleases">뗌 사건을 거를지.</param>
    public InputEvent Pop(bool skipReleases)
    {
        if (_read != _write)
        {
            InputEvent e = _events[_read];
            _read = (_read + 1) % Capacity;
            uint code = e.Code;
            if ((code & CharacterMask) == DeleteCharacter) code = (code & 0xff000008u) | 0x10000008u;
            if (!skipReleases || (code & InputCode.Release) == 0) return e with { Code = code };
        }
        return default;
    }

    /// <summary>꺼낼 사건이 없는가. 읽기 위치와 쓰기 위치가 같으면 비었다.</summary>
    public bool Empty => _read == _write;
}
