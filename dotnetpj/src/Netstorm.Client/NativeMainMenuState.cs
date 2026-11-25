using System.Buffers.Binary;
using Netstorm.Core.Original;

using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client;

// 원문004b6dd0의 State3 선택 부모를 같은 Native 블록/현재 장치로 실행한다.
public sealed class NativeMainMenuStateGlobals
{
    /// <summary>같은 다섯144BYTE이며 현재 State3/이전phase를 원문 위치마다 읽는다.</summary>
    public LocalMissionStateStorage State { get; }
    /// <summary>0055a420의 현재 출처다. 앞단에서 0으로 쓰고 현재 값을 보관한다.</summary>
    public UIntBox SourceType { get; }
    /// <summary>005c85ac의 현재 결과다. 음악 선택 뒤 0으로 지운다.</summary>
    public UIntBox Outcome { get; }
    /// <summary>00542530의 현재 첫 메뉴다. 시작 분기 뒤 0으로 지운다.</summary>
    public UIntBox FirstMenu { get; }
    /// <summary>00594fbc의 현재 전투다. prefix 뒤 현재값을 읽는다.</summary>
    public bool Battle { get; set; }
    /// <summary>005c85a4의 현재 편집이다.</summary>
    public bool Editor { get; set; }
    /// <summary>00565e70의 현재 NUL 문자열 물리다. 첫BYTE 쓰기와 전체 이름 복사를 구별한다.</summary>
    private readonly char[] _background;
    /// <summary>0055b490/494의 실제 할당이다. pending NULL은 빈 span으로 표현한다.</summary>
    private readonly byte[] _currentName;
    /// <summary>0055b490/494의 실제 할당이다. pending NULL은 빈 span으로 표현한다.</summary>
    private readonly byte[] _pendingName;
    /// <summary>0054da8c/0053190c의 현재 직접 읽기다. Config 재조회로 바꾸지 않는다.</summary>
    public Func<uint>? UserUpdated { get; set; }
    /// <summary>0054da8c/0053190c의 현재 직접 읽기다. Config 재조회로 바꾸지 않는다.</summary>
    public Func<uint>? TellTips { get; set; }
    /// <summary>원문 대조 장치만 true를 쓴다. 제품은 서비스 종료한 인터넷 검증을 비활성화하므로 기본 false이며 설정/명령줄로 켜지지 않는다.</summary>
    public bool ReferenceValidationNotice { get; set; }

    /// <summary>배경 첫BYTE다.</summary>
    public Span<char> Background => _background;
    /// <summary>현재 Fort 이름 물리다.</summary>
    public Span<byte> CurrentName => _currentName;
    /// <summary>대기 Fort 이름 물리다.</summary>
    public Span<byte> PendingName => _pendingName;

    /// <summary>같은 전역·이름 물리를 빌린다. 이 부모는 별도 복원값을 소유하지 않는다.</summary>
    /// <param name="state">같은 다섯144BYTE다.</param>
    /// <param name="sourceType">0055a420의 현재 출처다.</param>
    /// <param name="outcome">005c85ac의 현재 결과다.</param>
    /// <param name="firstMenu">00542530의 현재 첫 메뉴다.</param>
    /// <param name="background">00565e70의 현재 NUL 문자열 물리다.</param>
    /// <param name="currentName">0055b490의 실제 할당이다.</param>
    /// <param name="pendingName">0055b494의 실제 할당이다. NULL이면 빈 배열이다.</param>
    public NativeMainMenuStateGlobals(LocalMissionStateStorage state, UIntBox sourceType, UIntBox outcome, UIntBox firstMenu, char[] background, byte[] currentName, byte[] pendingName)
    {
        State = state ?? throw new ArgumentNullException(nameof(state));
        SourceType = sourceType ?? throw new ArgumentNullException(nameof(sourceType));
        Outcome = outcome ?? throw new ArgumentNullException(nameof(outcome));
        FirstMenu = firstMenu ?? throw new ArgumentNullException(nameof(firstMenu));
        _background = background ?? throw new ArgumentNullException(nameof(background));
        _currentName = currentName ?? throw new ArgumentNullException(nameof(currentName));
        _pendingName = pendingName ?? throw new ArgumentNullException(nameof(pendingName));
    }
}

/// <summary>선택된 큰 하위만 필수로 요구한다. 공통 prefix/tail은 기존 실제 부모를 호출한다.</summary>
public sealed class NativeMainMenuStateHooks
{
    /// <summary>intro와 WelcomeGiven의 같은 ConfigInt다.</summary>
    public Func<string, int>? ConfigInt { get; set; }
    /// <summary>004b2bc0의 실제 immediate 예약이며 phase를 직접 쓰지 않는다.</summary>
    public Action<uint>? Request { get; set; }
    /// <summary>승패/소스 배경이 선택한004b5470→DeleteFile 경계다.</summary>
    public Action? DeleteSavedState { get; set; }
    /// <summary>00469db0의 현재 음악 요청이다.</summary>
    public Action<string>? RequestMusic { get; set; }
    /// <summary>현재 TYPE 이름/.gif 실파일 존재를 판정하여 존재할 때만 원래 이름을 반환한다.</summary>
    public Func<uint, string?>? SourceBackground { get; set; }
    /// <summary>원문의 이전 Battle14 통지다.</summary>
    public Action? LeaveBattle { get; set; }
    /// <summary>원문의 전체 공통준비다.</summary>
    public Action? Prefix { get; set; }
    /// <summary>원문의 첫 HUD1이다.</summary>
    public Action? HudPrefix { get; set; }
    /// <summary>현재/pending 첫BYTE0 뒤 제목 갱신이다.</summary>
    public Action? Title { get; set; }
    /// <summary>제목 뒤 Guide 소멸이다.</summary>
    public Action? CloseGuide { get; set; }
    /// <summary>004cf960의 실제 고정 Tell 섹션이며 임시 Host MainMenu를 호출하지 않는다.</summary>
    public Action<string>? Tell { get; set; }
    /// <summary>004b5ba0의 실제 reader/벽시각/질문 부모다.</summary>
    public Func<uint>? Startup { get; set; }
    /// <summary>Tell 뒤 실제 Editor 분기다.</summary>
    public Action? EditorPlayers { get; set; }
    /// <summary>일반 메인 공통 후반이다.</summary>
    public Action? Tail { get; set; }
    /// <summary>제목 뒤 선택된 Fort 설치의 정상 확정/미션 조기 반환을 원래 분배기에 전달한다.</summary>
    public Func<LocalMissionParentResult>? ChangedState { get; set; }
}

/// <summary>호스트 메뉴 표시와 Native State 공통 확정을 분리한다.</summary>
public static class NativeMainMenuState
{
    /// <summary>원문에서 선택한 장치만 도달 위치에서 요구한다.</summary>
    /// <typeparam name="T">연결된 장치의 형태다.</typeparam>
    /// <param name="function">요구할 실제 장치다.</param>
    /// <returns>같은 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("Native State3 선택 장치 미연결");

    /// <summary>원문의 strcpy처럼 NUL까지만 쓰며 뒤의 기존 BYTE를 지우지 않는다.</summary>
    /// <param name="target">쓸 대상이다.</param>
    /// <param name="name">쓸 이름이다.</param>
    private static void Background(Span<char> target, string name)
    {
        if (name.Length >= target.Length) throw new ArgumentOutOfRangeException(nameof(name), "Native State3 배경 이름 물리 부족");
        name.CopyTo(target); target[name.Length] = '\0';
    }

    /// <summary>DWORD 블록의 원문 little endian 값을 읽는다.</summary>
    private static uint Word(ReadOnlyMemory<byte> block, int index) => BinaryPrimitives.ReadUInt32LittleEndian(block.Span.Slice(checked(index * 4), 4));

    /// <summary>DWORD 블록의 원문 little endian 값을 쓴다.</summary>
    private static void Word(Memory<byte> block, int index, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(block.Span.Slice(checked(index * 4), 4), value);

    /// <summary>State1/3 공통 앞단의 signed 이전phase&lt;=6 또는11이면 출처0을 쓰고 현재 출처를 보관한다.</summary>
    /// <param name="phase">이전 phase DWORD다.</param>
    /// <param name="sourceType">0055a420의 현재 출처 상자다. Editor/LAN/Meta 재인계의 같은 앞단이다.</param>
    /// <returns>보관한 현재 출처다.</returns>
    /// <remarks>2026-11-26 1.058에서 SL-5 Source 재인계용으로 추가했다. 근거: cpppj NativeMainMenuState.cpp의 ReadNativeFortEntrySource다. Native signed JLE를 같은 현재 값으로 판정하고 출처의 모든32비트를 보존한다.</remarks>
    public static uint ReadNativeFortEntrySource(uint phase, UIntBox sourceType)
    {
        ArgumentNullException.ThrowIfNull(sourceType);
        if (unchecked((int)phase) <= 6 || phase == 11) sourceType.Value = 0;
        return sourceType.Value;
    }

    /// <summary>현재 battle/이전phase14일 때만 cleanup 전 같은52h 알림 장치를 호출한다.</summary>
    /// <param name="state">같은 다섯144BYTE다.</param>
    /// <param name="battle">현재 전투다.</param>
    /// <param name="leave">이전 Battle14 실제 통지다.</param>
    /// <remarks>2026-11-26 1.058에서 SL-5 Source 재인계용으로 추가했다. 근거: cpppj NativeMainMenuState.cpp의 NotifyNativeFortEntryLeave다.</remarks>
    public static void NotifyNativeFortEntryLeave(LocalMissionStateStorage state, bool battle, Action? leave)
    {
        ArgumentNullException.ThrowIfNull(state);
        if (battle && Word(state.Phase, 0) == 14) (leave ?? throw new InvalidOperationException("Fort 진입의 이전 Battle14 실제 통지 미연결"))();
    }

    /// <summary>State1/2의 Editor 진입은 공통 출처/첫BYTE/통지만 실행하고 결과/음악/전체 배경은 그대로 둔다.</summary>
    /// <param name="state">같은 다섯144BYTE다.</param>
    /// <param name="sourceType">0055a420의 현재 출처 상자다.</param>
    /// <param name="background">00565e70의 현재 NUL 문자열 물리다.</param>
    /// <param name="battle">현재 전투다.</param>
    /// <param name="leave">이전 Battle14 실제 통지다.</param>
    /// <remarks>2026-11-26 1.058에서 SL-5 Source 재인계용으로 추가했다. 근거: cpppj NativeMainMenuState.cpp의 PrepareNativeEditorEntry다.</remarks>
    public static void PrepareNativeEditorEntry(LocalMissionStateStorage state, UIntBox sourceType, Span<char> background, bool battle, Action? leave)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(sourceType);
        if (Word(state.Current, 0) != 1 && Word(state.Current, 0) != 2) throw new InvalidOperationException("Native Editor 앞단의 실제 current1/2 계약");
        _ = ReadNativeFortEntrySource(Word(state.Phase, 0), sourceType);
        if (background.IsEmpty) throw new ArgumentOutOfRangeException(nameof(background), "Native Editor 배경 첫BYTE 물리 부족");
        background[0] = '\0';
        NotifyNativeFortEntryLeave(state, battle, leave);
    }

    /// <summary>모든 선택 효과가 정상 복귀한 경우만 공통 commit으로 반환한다. intro0은 실제 immediate39를 예약한다.</summary>
    /// <param name="globals">같은 전역/이름 물리다.</param>
    /// <param name="hooks">선택된 큰 하위 장치다.</param>
    /// <returns>정상 복귀하면 Commit이며 다른 Fort 설치 결과는 ChangedState가 정한다.</returns>
    /// <remarks>2026-11-26 1.058에서 SL-5 큰 메뉴 부모용으로 추가했다. 근거: cpppj NativeMainMenuState.cpp의 InstallNativeMainMenuState(004b6dd0 State3 선택 부모)다. 공통144BYTE 확정과 다음 Poll은 기존 RawLocalMissionState가 수행한다.</remarks>
    public static LocalMissionParentResult InstallNativeMainMenuState(NativeMainMenuStateGlobals globals, NativeMainMenuStateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        if (Word(globals.State.Current, 0) != 3) throw new InvalidOperationException("Native State3 실제 current 계약");
        Word(globals.State.Current, 33, 0); Word(globals.State.Immediate, 33, 0); Word(globals.State.Next, 33, 0);
        if (Need(hooks.ConfigInt)("intro") == 0) { Need(hooks.Request)(39); Word(globals.State.Immediate, 35, 1); return LocalMissionParentResult.Commit; }
        uint source = ReadNativeFortEntrySource(Word(globals.State.Phase, 0), globals.SourceType);
        uint outcome = globals.Outcome.Value;
        if (globals.Background.IsEmpty) throw new ArgumentOutOfRangeException(nameof(globals), "Native State3 배경 첫BYTE 물리 부족");
        globals.Background[0] = '\0';
        if (outcome == 1) { Need(hooks.DeleteSavedState)(); Background(globals.Background, "victory"); Need(hooks.RequestMusic)("fanfare.mus"); }
        else if (outcome == 2) { Need(hooks.DeleteSavedState)(); Background(globals.Background, "defeat"); Need(hooks.RequestMusic)("defeat.mus"); }
        else Background(globals.Background, "titleMenu");
        globals.Outcome.Value = 0;
        if (source != 0)
        {
            Need(hooks.DeleteSavedState)();
            Need(hooks.RequestMusic)("fanfare.mus");
            if (Need(hooks.SourceBackground)(globals.SourceType.Value) is { } name) Background(globals.Background, name);
        }
        NotifyNativeFortEntryLeave(globals.State, globals.Battle, hooks.LeaveBattle);
        Need(hooks.Prefix)(); Need(hooks.HudPrefix)();
        if (!globals.PendingName.IsEmpty) globals.PendingName[0] = 0;
        if (globals.CurrentName.IsEmpty) throw new ArgumentOutOfRangeException(nameof(globals), "Native State3 현재 Fort 첫BYTE 물리 부족");
        globals.CurrentName[0] = 0;
        Need(hooks.Title)();
        if (Word(globals.State.Current, 0) != 3) return Need(hooks.ChangedState)();
        Need(hooks.CloseGuide)();
        if (Need(hooks.ConfigInt)("WelcomeGiven") == 0) Need(hooks.Tell)("Welcome");
        else
        {
            Need(hooks.Tell)("Blank");
            if (Need(hooks.Startup)() == 0 && globals.FirstMenu.Value != 0)
            {
                if (globals.ReferenceValidationNotice && Need(globals.UserUpdated)() == 0) Need(hooks.Tell)("UserNotUpdated");
                if (Need(globals.TellTips)() != 0) Need(hooks.Tell)("Tips");
            }
            globals.FirstMenu.Value = 0;
        }
        if (globals.Editor) Need(hooks.EditorPlayers)();
        Need(hooks.Tail)(); return LocalMissionParentResult.Commit;
    }
}
