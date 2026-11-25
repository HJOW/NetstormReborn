using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// 일반 전투 !state.dat 쓰기와 저장 상태 복원을 같은 세션 전역·파일 장치에 연결한다.
public sealed partial class HostSession
{
    /// <summary>쓰기 실행이 복원 owner 출력을 쓰는 같은 상자다. 읽기 전역과 공유한다.</summary>
    public UIntBox SavedStateRestoreOwner { get; } = new();

    /// <summary>쓰기 실행이 복원 구독자를 쓰는 같은 상자다. 읽기 전역과 공유한다.</summary>
    public UIntBox SavedStateRestoreSubscriber { get; } = new();

    /// <summary>쓰기 실행이 이전 이름을 복사하는 같은 64바이트다. 읽기 전역과 공유한다.</summary>
    public byte[] SavedStatePreviousHandle { get; } = new byte[64];

    /// <summary>같은 세션 전역으로 조립한 쓰기 전역이다. 비어 있으면 미조립이다.</summary>
    private SavedStateWriteGlobals? _savedStateWriteGlobals;

    /// <summary>같은 세션 전역으로 조립한 쓰기 장치다. 비어 있으면 미조립이다.</summary>
    private SavedStateWriteHooks? _savedStateWriteHooks;

    /// <summary>같은 세션 전역으로 조립한 읽기 전역이다. 비어 있으면 미조립이다.</summary>
    private SavedStateReadGlobals? _savedStateReadGlobals;

    /// <summary>같은 세션 전역으로 조립한 읽기 장치다. 비어 있으면 미조립이다.</summary>
    private SavedStateReadHooks? _savedStateReadHooks;

    /// <summary>실제 전투 저장 주기를 세는 세션 소유 기한이다. 전투 진입 때 0으로 둔다.</summary>
    private double _savedStateDeadline;

    /// <summary>실제 입력 경로가 저장을 요청하는 같은 세션 쓰기 전역이다.</summary>
    public SavedStateWriteGlobals SavedStateWriteGlobals => _savedStateWriteGlobals ?? throw new InvalidOperationException("세션 저장 쓰기 전역 미연결");

    /// <summary>실제 입력 경로가 저장을 요청하는 같은 세션 쓰기 장치다.</summary>
    public SavedStateWriteHooks SavedStateWriteHooks => _savedStateWriteHooks ?? throw new InvalidOperationException("세션 저장 쓰기 장치 미연결");

    /// <summary>실제 입력 경로가 복원을 요청하는 같은 세션 읽기 전역이다.</summary>
    public SavedStateReadGlobals SavedStateReadGlobals => _savedStateReadGlobals ?? throw new InvalidOperationException("세션 저장 읽기 전역 미연결");

    /// <summary>실제 입력 경로가 복원을 요청하는 같은 세션 읽기 장치다.</summary>
    public SavedStateReadHooks SavedStateReadHooks => _savedStateReadHooks ?? throw new InvalidOperationException("세션 저장 읽기 장치 미연결");

    /// <summary>연결되지 않은 꼬리 전역·파일 장치를 R6 소비자에서 잇는다는 명시 실패다.</summary>
    private static uint MissingSavedStateField() => throw new InvalidOperationException("저장 상태 꼬리·파일 장치 미연결(R6)");

    /// <summary>같은 세션 전역으로 저장 쓰기·읽기 본체를 연결한다. 파일 장치는 호출자가 공급한다.</summary>
    /// <param name="writeHooks">실제 파일·Fort·SP·벽시각·진단 경계다. 파일 열기·쓰기·닫기·존재·읽기를 포함한다.</param>
    /// <param name="readHooks">실제 파일·Core·메타데이터 경계다. 파일 열기·읽기·닫기·존재·경로·초기화·복제를 포함한다.</param>
    /// <param name="writeGlobals">호출자가 미리 묶은 쓰기 전역이다. 비어 있으면 세션 실제 값으로 채운다.</param>
    /// <param name="readGlobals">호출자가 미리 묶은 읽기 전역이다. 비어 있으면 세션 실제 값으로 채운다.</param>
    /// <remarks>2026-11-26 1.061에서 SL-8용으로 추가했다. 근거: F2d RawSavedState의 쓰기·읽기 본체다. 꼬리 3전역의 실제 읽기 위치와 파일 장치는 R6 소비자에서 잇는다.</remarks>
    public void BuildSavedStateDevices(SavedStateWriteHooks writeHooks, SavedStateReadHooks readHooks, SavedStateWriteGlobals? writeGlobals = null, SavedStateReadGlobals? readGlobals = null)
    {
        ArgumentNullException.ThrowIfNull(writeHooks); ArgumentNullException.ThrowIfNull(readHooks);
        if (_savedStateWriteGlobals is not null || _savedStateWriteHooks is not null || _savedStateReadGlobals is not null || _savedStateReadHooks is not null)
            throw new InvalidOperationException("세션 저장 장치 중복 연결");
        _savedStateWriteGlobals = writeGlobals ?? new()
        {
            MissionPost = () => _fortState?.Mission ?? false,
            PlayerKnown = () => MissionPlayer.PlayerKnown != 0,
            Checking = () => AiChecking.Read() != 0,
            LocalOwner = () => MissionPlayer.BattleOwner,
            MySubscriber = () => Gameplay.Subscribers?.MySubscriber.Value ?? throw new InvalidOperationException("저장 상태의 실제 구독자 미연결"),
            Handle = ReadPlayerHandle,
            Players = () => PlayerRecords,
            SetRestoreOwner = value => SavedStateRestoreOwner.Value = value,
            SetRestoreSubscriber = value => SavedStateRestoreSubscriber.Value = value,
            PreviousHandle = SavedStatePreviousHandle,
            Tail = [MissingSavedStateField, MissingSavedStateField, MissingSavedStateField],
            LoginEnd = () => LocalState.LoginEnd.Value,
        };
        _savedStateWriteHooks = writeHooks;
        _savedStateReadGlobals = readGlobals ?? new()
        {
            Owner = () => SavedStateRestoreOwner,
            Subscriber = () => SavedStateRestoreSubscriber,
            PreviousHandle = () => SavedStatePreviousHandle,
            Players = () => PlayerRecords,
            Tail = [MissingSavedStateFieldBox, MissingSavedStateFieldBox, MissingSavedStateFieldBox],
        };
        _savedStateReadHooks = readHooks;
    }

    /// <summary>연결되지 않은 꼬리 상자를 R6 소비자에서 잇는다는 명시 실패다.</summary>
    private static UIntBox MissingSavedStateFieldBox() => throw new InvalidOperationException("저장 상태 꼬리 상자 미연결(R6)");

    /// <summary>현재 소유자 기록의 NUL 포함 이름 32바이트를 복사한다.</summary>
    /// <returns>이름 사본이다.</returns>
    private byte[] ReadPlayerHandle()
    {
        byte[] name = new byte[32];
        Array.Copy(PlayerRecords, checked((int)(MissionPlayer.BattleOwner * 180 + 12)), name, 0, name.Length);
        return name;
    }

    /// <summary>현재 PlayerKnown/owner를 다시 읽으며 미known은 0이다. 호출자의 LocalSp 갈고리에 연결한다.</summary>
    /// <returns>로컬 SP 단정도 값이다.</returns>
    /// <remarks>2026-11-26 1.061에서 SL-8용으로 추가했다. 근거: F2d RawSavedState의 LocalSp 호출 규약이다.</remarks>
    public float SavedStateLocalSp()
    {
        if (MissionPlayer.PlayerKnown == 0) return 0f;
        return BitConverter.UInt32BitsToSingle(Sp.Get(MissionPlayer.BattleOwner));
    }

    /// <summary>전투 중이면 기한 뒤 쓰기를 한 번 실행하고 10초를 예약한다. 입력 경로의 Poll 호출자다.</summary>
    /// <param name="battle">현재 전투 중이면 참이다.</param>
    public void PollSavedStateWrite(bool battle) =>
        RawSavedState.PollSavedStateWrite(battle, ref _savedStateDeadline, SavedStateWriteHooks.WallSeconds, () => RawSavedState.WriteSavedState(SavedStateWriteGlobals, SavedStateWriteHooks));

    /// <summary>실제 파일에서 저장 상태를 복원한다. 입력 경로의 Read 호출자다.</summary>
    /// <param name="scratch">명시 초기화한 392바이트다.</param>
    /// <returns>참가자 2명 이상일 때의 signed 저장 시각이며 그 밖은 0이다.</returns>
    public int ReadSavedState(byte[] scratch) => RawSavedState.ReadSavedState(scratch, SavedStateReadGlobals, SavedStateReadHooks);
}
