namespace Netstorm.Core.Display;

/// <summary>가장자리 스크롤 판정에 필요한 한 프레임의 입력·상태.</summary>
/// <param name="MouseX">커서 x (화면 픽셀, 창 클라이언트 좌표)</param>
/// <param name="MouseY">커서 y (화면 픽셀)</param>
/// <param name="ScreenWidth">화면(창 클라이언트) 폭, 픽셀</param>
/// <param name="ScreenHeight">화면 높이, 픽셀</param>
/// <param name="Enabled">Options 의 Edge Scroll in Fullscreen 체크 여부</param>
/// <param name="BorderlessScreen">전체화면이거나 테두리 없는 화면 전체 창인지 (원본: 창 테두리 오프셋 = 0)</param>
/// <param name="LeftButtonDown">왼쪽 마우스 버튼이 눌려 있는지 (누르고 있으면 스크롤하지 않는다)</param>
/// <param name="ShiftDown">Shift 가 눌려 있는지 (모서리에서 한 축만 스크롤)</param>
/// <param name="PopupOpen">우클릭 메뉴 등 팝업·모달 창이 열려 있는지</param>
/// <param name="TopEdgeBlocked">화면 맨 위 게임 메뉴 막대가 열려 있어 위쪽 스크롤을 막아야 하는지</param>
public readonly record struct EdgeScrollInput(
    int MouseX, int MouseY, int ScreenWidth, int ScreenHeight,
    bool Enabled, bool BorderlessScreen, bool LeftButtonDown, bool ShiftDown, bool PopupOpen, bool TopEdgeBlocked);

/// <summary>
/// 풀스크린에서 커서를 화면 끝에 대면 카메라를 이동시키는 규칙 (원본 <c>Netstorm.exe</c> <c>004d65de</c>~<c>004d67ad</c> 분석,
/// <see href="../../../docs/exe/edge-scroll.md">docs/exe/edge-scroll.md</see>).
/// 속도는 프레임당 픽셀로 정의돼 있어 클론은 기준 프레임 수(75)로 초당 픽셀로 환산한다.
/// </summary>
public sealed class EdgeScrollController
{
    /// <summary>가장자리 판정 폭. 원본은 커서 좌표가 0~1(맨 바깥 두 줄)이면 왼쪽·위, 끝에서 1칸 안이면 오른쪽·아래로 본다.</summary>
    public const int EdgeMargin = 1;

    /// <summary>가장자리에 처음 닿았을 때의 속도 (프레임당 픽셀). 원본 상수 <c>[00506590] = 2.0</c></summary>
    public const double StartSpeed = 2.0;

    /// <summary>가장자리에 머무는 동안 속도가 늘어나는 비율 (초당, 프레임당 픽셀). 원본 상수 <c>[00501708] = 30.0</c></summary>
    public const double SpeedRampPerSecond = 30.0;

    /// <summary>속도 상한의 기본값 = 보유 <c>setup.cfg</c> 의 <c>edgeScrollSpeed = 35</c></summary>
    public const int DefaultMaxSpeed = 35;

    /// <summary>프레임당 픽셀을 초당 픽셀로 바꾸는 기준 프레임 수 = 원본 화면 루프 상한 <c>maxFPS = 75</c></summary>
    public const int ReferenceFramesPerSecond = 75;

    /// <summary>가장자리에 머문 시간(초). 가장자리를 벗어나면 0 으로 되돌린다.</summary>
    private double _edgeSeconds;

    /// <summary>직전 갱신에서 가장자리 스크롤 중이었는지 (처음 닿은 순간을 알아내기 위함)</summary>
    private bool _onEdge;

    /// <summary>속도 상한(프레임당 픽셀, 원본 edgeScrollSpeed). 0 미만은 0 으로 본다.</summary>
    public int MaxSpeed { get; set; } = DefaultMaxSpeed;

    /// <summary>
    /// 가장자리 스크롤 방향을 판정한다. 각 성분은 −1(왼쪽·위), 0, +1(오른쪽·아래).
    /// 조건을 하나라도 만족하지 못하면 (0, 0) 이다.
    /// </summary>
    /// <param name="input">한 프레임의 입력</param>
    public static (int X, int Y) Direction(in EdgeScrollInput input)
    {
        // 설정이 꺼져 있거나 창 테두리가 있거나 팝업·드래그 중이면 스크롤하지 않는다.
        if (!input.Enabled || !input.BorderlessScreen || input.PopupOpen || input.LeftButtonDown)
        {
            return (0, 0);
        }
        int x = 0;
        int y = 0;
        // 왼쪽·위 가장자리 (위쪽은 메뉴 막대가 열려 있으면 제외)
        if (input.MouseX <= EdgeMargin)
        {
            x = -1;
        }
        if (input.MouseY <= EdgeMargin && !input.TopEdgeBlocked)
        {
            y = -1;
        }
        // 오른쪽·아래 가장자리: 원본은 오른쪽·아래가 위쪽·왼쪽보다 우선한다 (뒤에서 덮어씀)
        if (input.MouseX >= input.ScreenWidth - EdgeMargin)
        {
            x = 1;
        }
        if (input.MouseY >= input.ScreenHeight - EdgeMargin)
        {
            y = 1;
        }
        // Shift 를 누르면 모서리에서 한 축만 남긴다: x 가 더 클 때만 x, 같거나 y 가 크면 y (원본 004d6762).
        if (input.ShiftDown && x != 0 && y != 0)
        {
            (x, y) = Math.Abs(x) > Math.Abs(y) ? (x, 0) : (0, y);
        }
        return (x, y);
    }

    /// <summary>
    /// 가장자리에 머문 시간에서 프레임당 속도(픽셀)를 구한다: <c>min(최대 속도, 정수로 자른(시간 × 30 + 2))</c>.
    /// </summary>
    /// <param name="edgeSeconds">처음 닿은 뒤 지난 시간(초)</param>
    /// <param name="maxSpeed">속도 상한 (edgeScrollSpeed)</param>
    public static int SpeedPerFrame(double edgeSeconds, int maxSpeed)
    {
        // 원본은 double 값을 정수로 자른 뒤 상한과 비교한다.
        double ramp = Math.Truncate(Math.Max(0, edgeSeconds) * SpeedRampPerSecond + StartSpeed);
        return (int)Math.Min(Math.Max(0, maxSpeed), ramp);
    }

    /// <summary>
    /// 이번 갱신에서 카메라를 옮길 화면 픽셀 양을 돌려주고 가속 시간을 이어 간다.
    /// 값은 논리(원본) 픽셀 단위이며 양수 x 는 오른쪽, 양수 y 는 아래쪽이다.
    /// </summary>
    /// <param name="input">한 프레임의 입력</param>
    /// <param name="deltaSeconds">지난 갱신 이후 경과 시간(초)</param>
    public (double X, double Y) Update(in EdgeScrollInput input, double deltaSeconds)
    {
        (int x, int y) = Direction(input);
        if (x == 0 && y == 0)
        {
            // 가장자리를 벗어나면 가속을 처음부터 다시 시작한다 (원본: 시각 표식을 "없음"으로 되돌림).
            _onEdge = false;
            _edgeSeconds = 0;
            return (0, 0);
        }
        if (_onEdge)
        {
            _edgeSeconds += Math.Max(0, deltaSeconds);
        }
        else
        {
            // 처음 닿은 프레임은 경과 0초 = 시작 속도.
            _onEdge = true;
            _edgeSeconds = 0;
        }
        // 프레임당 픽셀을 이번 갱신 시간에 맞는 픽셀로 환산한다.
        double pixels = SpeedPerFrame(_edgeSeconds, MaxSpeed) * ReferenceFramesPerSecond * Math.Max(0, deltaSeconds);
        return (x * pixels, y * pixels);
    }
}
