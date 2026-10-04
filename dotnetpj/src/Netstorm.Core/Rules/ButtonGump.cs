namespace Netstorm.Core.Rules;

/// <summary>
/// 화면의 돌 버튼 하나의 판정 영역. 왼쪽·위쪽 끝은 포함하고 오른쪽·아래쪽 끝은 포함하지 않는다
/// (원본 <c>FUN_004248e0</c>: 지역 좌표 0 ≤ x &lt; 폭, 0 ≤ y &lt; 높이).
/// </summary>
/// <param name="Id">버튼을 구분하는 번호 (화면을 다시 만들어도 같은 버튼이면 같은 값)</param>
/// <param name="X">왼쪽 끝</param>
/// <param name="Y">위쪽 끝</param>
/// <param name="Width">판정 폭</param>
/// <param name="Height">판정 높이</param>
/// <param name="Enabled">눌릴 수 있는지 (비활성 버튼은 눌러도 반응·소리가 없다)</param>
public readonly record struct GumpButton(int Id, int X, int Y, int Width, int Height, bool Enabled = true)
{
    /// <summary>화면 좌표가 판정 영역 안인지.</summary>
    public bool Contains(int x, int y) => x >= X && x < X + Width && y >= Y && y < Y + Height;
}

/// <summary>한 프레임의 입력 처리 결과. 값이 있는 항목만 이번 프레임에 일어난 일이다.</summary>
/// <param name="Pressed">이번 프레임에 눌린 버튼 번호 (클릭음을 내는 시점)</param>
/// <param name="Activated">이번 프레임에 뗐고 실행해야 하는 버튼 번호</param>
public readonly record struct GumpResult(int? Pressed, int? Activated);

/// <summary>
/// 원본 돌 버튼(<c>Buttongump.cpp</c>, 이벤트 처리 <c>FUN_004249a0</c>·이동 처리 <c>FUN_00424dd0</c>)의 누름·떼기 규칙.
/// 2026-10-03 원본 자동 분석(메인 메뉴·팁 창·대화상자 버튼)으로 확인했다 — docs/videos/menu-buttons-20261003.md.
/// <list type="number">
/// <item><description>활성 버튼 위에서 왼쪽 버튼을 **누르는 순간** 버튼이 눌린 모양이 되고 클릭음(<c>button.wav</c>)이 한 번 난다.
/// 비활성 버튼, 버튼 밖, 오른쪽·가운데 버튼은 아무 반응도 소리도 없다.</description></item>
/// <item><description>누르고 있는 동안 커서가 그 버튼 안에 있는 때만 눌린 모양이다. 나가면 평소 모양으로 돌아오고 다시 들어오면
/// 눌린 모양이 된다 (소리는 다시 나지 않는다). 오래 눌러도 소리·실행은 반복되지 않는다.</description></item>
/// <item><description>버튼을 **뗄 때** 커서가 눌렀던 그 버튼 안에 있을 때만 실행한다. 밖에서 떼거나, 다른 버튼 위에서 떼거나,
/// 버튼 밖에서 눌러 안쪽에서 떼면 실행하지 않는다. 실행 때는 소리가 나지 않는다.</description></item>
/// </list>
/// 마우스 호버 표시와 키보드 조작은 없다.
/// </summary>
public sealed class ButtonGump
{
    /// <summary>지금 눌러 붙잡고 있는 버튼 번호 (없으면 null).</summary>
    private int? _held;

    /// <summary>붙잡은 버튼 위에 커서가 있는지.</summary>
    private bool _inside;

    /// <summary>이전 프레임에 왼쪽 버튼이 눌려 있었는지 (누름·뗌 시점을 찾는 데 쓴다).</summary>
    private bool _wasDown;

    /// <summary>지금 눌러 붙잡고 있는 버튼 번호 (없으면 null).</summary>
    public int? Held => _held;

    /// <summary>번호의 버튼이 눌린 모양이어야 하는지: 붙잡고 있고 커서가 그 안에 있을 때.</summary>
    public bool IsPressed(int id) => _held == id && _inside;

    /// <summary>
    /// 한 프레임의 마우스 상태로 누름·떼기를 처리한다. 버튼 목록은 매 프레임 다시 만들어 넘겨도 된다
    /// (같은 버튼은 같은 <see cref="GumpButton.Id"/> 를 가져야 한다).
    /// </summary>
    /// <param name="buttons">지금 화면의 버튼 (겹치면 뒤쪽이 위에 있는 것)</param>
    /// <param name="x">커서 x</param>
    /// <param name="y">커서 y</param>
    /// <param name="leftDown">왼쪽 버튼이 눌려 있는지</param>
    /// <param name="canPress">이번 프레임의 누름을 받을 수 있는지 (펼침 메뉴가 바깥 누름을 가져간 프레임은 false)</param>
    public GumpResult Update(IReadOnlyList<GumpButton> buttons, int x, int y, bool leftDown, bool canPress = true)
    {
        int? pressed = null;
        int? activated = null;
        bool pressEdge = leftDown && !_wasDown;
        bool releaseEdge = !leftDown && _wasDown;
        _wasDown = leftDown;
        if (pressEdge && canPress)
        {
            // 위에 보이는(목록 뒤쪽) 버튼부터 찾아 커서가 있는 버튼 하나만 붙잡는다. 비활성 버튼이면 아무것도 붙잡지 않는다.
            for (int i = buttons.Count - 1; i >= 0; i--)
            {
                if (!buttons[i].Contains(x, y)) continue;
                if (buttons[i].Enabled)
                {
                    _held = buttons[i].Id;
                    _inside = true;
                    pressed = _held;
                }
                break;
            }
        }
        else if (_held is int held)
        {
            GumpButton? target = Find(buttons, held);
            // 붙잡은 버튼이 화면에서 사라졌으면(페이지 전환 등) 취소한다
            if (target == null) { Cancel(); return new GumpResult(pressed, null); }
            _inside = target.Value.Contains(x, y);
            // 뗀 순간 안쪽이면 실행하고, 어느 쪽이든 붙잡기는 끝난다
            if (releaseEdge)
            {
                if (_inside) activated = held;
                Cancel();
            }
            else if (!leftDown) Cancel();
        }
        return new GumpResult(pressed, activated);
    }

    /// <summary>붙잡고 있던 버튼을 놓는다 (실행하지 않는다). 창이 닫히거나 화면이 바뀔 때 쓴다.</summary>
    public void Cancel()
    {
        _held = null;
        _inside = false;
    }

    /// <summary>번호가 같은 버튼을 찾는다 (없으면 null).</summary>
    private static GumpButton? Find(IReadOnlyList<GumpButton> buttons, int id)
    {
        // 목록을 처음부터 훑어 같은 번호를 찾는다
        foreach (GumpButton button in buttons)
        {
            if (button.Id == id) return button;
        }
        return null;
    }
}
