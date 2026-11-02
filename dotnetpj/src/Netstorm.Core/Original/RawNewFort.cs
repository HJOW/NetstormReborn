namespace Netstorm.Core.Original;

/// <summary>
/// 현재 Client가 노출한 가변 바이트 한 구간을 돌려주는 조회 장치다(NewFort의 현재 handle 버퍼, CoreData 10바이트가 쓴다).
/// 호출할 때마다 그 시점의 현재 물리 버퍼를 다시 빌려 준다. 사본을 만들지 않는다.
/// </summary>
/// <remarks>2026-11-02 TS6에서 추가했다. <see cref="Span{T}"/>를 돌려주므로 <see cref="Func{TResult}"/> 대신 전용 대리자를 쓴다.</remarks>
public delegate Span<byte> NewFortBytes();

/// <summary>
/// 004bd400(owner, ID, 현재 handle)의 Subscriber 등록 쓰기다. 등록 helper가 끝난 뒤 문자열 길이를 구하므로 부모는 길이를 미리 캡처하지 않고 현재 NUL 포함 버퍼만 넘긴다.
/// </summary>
/// <param name="owner">Subscriber를 쓸 Template 칸.</param>
/// <param name="id">방금 등록된 Subscriber ID(00540c74).</param>
/// <param name="handle">현재 NUL 포함 handle 버퍼.</param>
/// <remarks>2026-11-02 TS6에서 추가했다.</remarks>
public delegate void NewFortSubscriberWrite(uint owner, uint id, ReadOnlySpan<byte> handle);

/// <summary>004bd600의 로컬 20×6바이트 Territory 전체 저장이다.</summary>
/// <param name="owner">Territory를 쓸 Template 칸.</param>
/// <param name="records">20개 레코드 120바이트.</param>
/// <remarks>2026-11-02 TS6에서 추가했다.</remarks>
public delegate void NewFortTerritoryWrite(uint owner, ReadOnlySpan<byte> records);

/// <summary>
/// 새 Template manager 부모(004bfba0)의 선택된 경계다: 기존 header의 manager 진단, 파일 존재 재조회, 할당·생성·삭제, 임시 manager의 소멸·해제, 최종 canonical 섹션 초기화.
/// 미연결 경계는 빈 성공으로 대신하지 않고 그 호출 지점에서 명시 오류로 멈춘다(원본 <c>NewFortManagerHooks</c>).
/// </summary>
/// <remarks>2026-11-02 TS6에서 추가했다.</remarks>
public sealed class NewFortManagerHooks
{
    /// <summary>기존 header의 manager가 0이 아닐 때의 Template.cpp:0x172 진단이다. 복귀하면 그대로 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }

    /// <summary>0041ab10 파일 존재 재조회다. 0이 아닌 값은 모두 '삭제 임시 manager 경로'로 들어간다.</summary>
    public Func<string, int>? Exists { get; set; }

    /// <summary>operator new(0x114)의 실제 토큰 또는 0이다.</summary>
    public Func<uint, uint>? Allocate { get; set; }

    /// <summary>004bcc60 생성자다. 기존 Template manager 생성과 같은 직렬화 버전 전역을 쓴다. 생성된 토큰을 돌려준다.</summary>
    public Func<uint, string, uint>? Construct { get; set; }

    /// <summary>임시 manager의 가상 +0에서 읽는 실제 현재 파일 이름이다.</summary>
    public Func<uint, string>? FileName { get; set; }

    /// <summary>0041aef0 실제 파일 삭제 요청이다. 실제 삭제는 같은 EditorFiles 이름공간이 맡는다.</summary>
    public Action<string>? Erase { get; set; }

    /// <summary>임시 manager의 0044da60 소멸이다.</summary>
    public Action<uint>? Destroy { get; set; }

    /// <summary>임시 manager의 CRT free 해제다. 소멸 다음에 실행한다.</summary>
    public Action<uint>? Release { get; set; }

    /// <summary>0044d870 canonical 섹션 초기화다. 토큰 0도 그대로 전달하며 물리 실패를 성공으로 바꾸지 않는다.</summary>
    public Action<uint>? Initialize { get; set; }
}

/// <summary>
/// 새 Fort 자료 부모(004b3920)가 같은 Client·Player·난수 소유자에서 빌리는 현재 전역이다. 새 사본을 만들지 않고 읽기마다 현재 값을 다시 조회한다.
/// C++의 DWORD 참조(<c>uint32_t&amp;</c>)는 C#에서 읽기·쓰기 대리자 쌍으로 옮겼다(<c>FortCurrentNameGlobals</c>와 같은 방식).
/// </summary>
/// <remarks>2026-11-02 TS6에서 추가했다.</remarks>
public sealed class NewFortDataGlobals
{
    /// <summary>00540c74의 현재 Subscriber ID를 읽는다. 처음에 -1로 쓴 뒤 등록 부모가 같은 위치를 쓴다.</summary>
    public Func<uint>? Subscriber { get; set; }

    /// <summary>00540c74에 쓴다.</summary>
    public Action<uint>? SetSubscriber { get; set; }

    /// <summary>PTR00540ca8의 현재 NUL 포함 출력 버퍼다. 호출마다 그 시점의 현재 버퍼를 돌려준다.</summary>
    public NewFortBytes? Handle { get; set; }

    /// <summary>005c85a4의 현재 Editor 모드 조회다.</summary>
    public Func<uint>? Editor { get; set; }

    /// <summary>00595344의 현재 Player 확인 값 조회다.</summary>
    public Func<uint>? PlayerKnown { get; set; }

    /// <summary>00540c70의 현재 로컬 소유자 조회다.</summary>
    public Func<uint>? LocalOwner { get; set; }

    /// <summary>00540cac의 현재 Template 소유자 조회다.</summary>
    public Func<uint>? TemplateOwner { get; set; }

    /// <summary>00595348에 쓴다. SP 차감 뒤 1로 쓴다.</summary>
    public Action<uint>? SetCoreActive { get; set; }

    /// <summary>005c89b4 전체 DWORD를 읽는다. 난수 뒤 하위 WORD만 바꾸고 그 시점의 상위 WORD를 보존하기 위해 읽는다.</summary>
    public Func<uint>? CoreSeed { get; set; }

    /// <summary>005c89b4 전체 DWORD에 쓴다.</summary>
    public Action<uint>? SetCoreSeed { get; set; }
}

/// <summary>새 Fort 자료 부모가 요구하는 실제 공유 하위 장치다. 선택한 원문 위치에서만 요구한다(원본 <c>NewFortDataHooks</c>).</summary>
/// <remarks>2026-11-02 TS6에서 추가했다.</remarks>
public sealed class NewFortDataHooks
{
    /// <summary>004de9a0 번역이다. "Unnamed"의 원본 바이트 문자열을 돌려준다.</summary>
    public Func<string, string>? Translate { get; set; }

    /// <summary>004414d0 설정 기록이다. 원문 철자 그대로 "fort.mySubHandle"과 현재 handle 문자열을 받는다.</summary>
    public Action<string, string>? Configure { get; set; }

    /// <summary>00460730 현재 제목 갱신이다.</summary>
    public Action? Title { get; set; }

    /// <summary>기존 FinishFortSubscriberRegistration(004b38d0)이다.</summary>
    public Action? RegisterSubscriber { get; set; }

    /// <summary>0040ec50(owner)의 float32 SP를 읽는다. Player 확인 값이 0이 아닐 때만 호출한다.</summary>
    public Func<uint, float>? Sp { get; set; }

    /// <summary>0040eec0(-SP)의 현재 로컬 소유자 부수효과다. 선행 소유자를 대신 캡처하지 않는다.</summary>
    public Action<float>? AddSp { get; set; }

    /// <summary>0048f0d0의 현재 실제 CoreData 10바이트 이상이다.</summary>
    public NewFortBytes? CoreData { get; set; }

    /// <summary>004beb30 CoreData 쓰기다. Editor의 자연 건너뜀도 이 실제 부모가 결정한다.</summary>
    public Action<uint>? CoreWrite { get; set; }

    /// <summary>004559d0 난수 준비다. 같은 Client GameMode·CRT·GameRandom 준비를 한다.</summary>
    public Action? PrepareRandom { get; set; }

    /// <summary>004558c0의 현재 공유 난수다. 254 또는 65536을 그대로 받는다.</summary>
    public Func<uint, uint>? NextRandom { get; set; }

    /// <summary>004bd400(owner, ID, 현재 handle)의 Subscriber 쓰기다.</summary>
    public NewFortSubscriberWrite? Subscriber { get; set; }

    /// <summary>004bd600의 로컬 20×6바이트 Territory 전체 저장이다.</summary>
    public NewFortTerritoryWrite? Territory { get; set; }

    /// <summary>004beec0의 실제 Money 섹션 생성·저장이다. Deck 저장이 아니다.</summary>
    public Action<uint>? Money { get; set; }

    /// <summary>004bd550의 현재 State 포인터를 빌린다.</summary>
    public RawBytesLookup? State { get; set; }

    /// <summary>004bd360의 같은 현재 Template 알림이다.</summary>
    public Action<uint>? Notify { get; set; }
}

/// <summary>004b5ca0의 최초 파일 부재 분기를 따로 한정하는 경계다(원본 <c>NewFortMissingFileHooks</c>).</summary>
/// <remarks>2026-11-02 TS6에서 추가했다.</remarks>
public sealed class NewFortMissingFileHooks
{
    /// <summary>00460680 현재 경로다. 존재 검사 전과 새 manager 생성 전에 따로 두 번 호출한다.</summary>
    public Func<string>? Path { get; set; }

    /// <summary>0041ab10 현재 작업본 파일의 존재 출처다.</summary>
    public Func<string, int>? Exists { get; set; }

    /// <summary>각 원문 효과 직전의 현재 Template 소유자 조회다.</summary>
    public Func<uint>? Owner { get; set; }

    /// <summary>현재 header에 새 manager(이름, 인자 0)를 실제로 만든다.</summary>
    public Action<uint, string, uint>? Manager { get; set; }

    /// <summary>새 manager의 실제 State 포인터다.</summary>
    public RawBytesLookup? State { get; set; }

    /// <summary>State|1 이후의 알림이다.</summary>
    public Action<uint>? Notify { get; set; }

    /// <summary>마지막 004bd450 Subscriber 읽기다.</summary>
    public Action<uint>? Subscriber { get; set; }

    /// <summary>실제 004b3920 새 Fort 자료 부모다.</summary>
    public Action? Data { get; set; }

    /// <summary>실제 00460730 제목 갱신이다.</summary>
    public Action? Title { get; set; }

    /// <summary>Subscriber 읽기 뒤의 현재 handle 문자열이다.</summary>
    public Func<string>? Handle { get; set; }

    /// <summary>같은 fort.mySubHandle을 현재 내용으로 설정한다.</summary>
    public Action<string, string>? Configure { get; set; }
}

/// <summary>
/// 10.78의 새 Template manager·신규 자료·파일 부재 부모를 현재 실제 장치에 연결하는 원본 제어 순서다. cpppj <c>o/RawNewFort</c>를 옮겼다.
/// 부분 쓰기, 재조회, 외부 재진입으로 달라질 수 있는 전역은 매번 다시 읽고 원문에 없는 보정이나 사본을 만들지 않는다.
/// </summary>
/// <remarks>2026-11-02 TS6에서 추가했다. 원본 x86 fixture 네 개(manager 48행, data 1,248행, missing 26행, missing-bits 78행)로 대조한다.</remarks>
public static class RawNewFort
{
    /// <summary>Template 기록 한 개의 실제 폭이다.</summary>
    private const int HeaderBytes = 20;

    /// <summary>새 Template manager 한 개의 할당 크기(0x114)다.</summary>
    private const uint ManagerAllocation = 0x114;

    /// <summary>Territory 20개 레코드의 전체 폭이다(6바이트 × 20).</summary>
    private const int TerritoryBytes = 120;

    /// <summary>CoreData 쓰기 전에 0으로 지우는 구간이다(4/4/2바이트 쓰기의 합).</summary>
    private const int CoreClearBytes = 10;

    /// <summary>현재 State에서 kind를 쓰는 위치(+0x11)다.</summary>
    private const int StateKindOffset = 17;

    /// <summary>미선택 경계를 빈 성공 callback으로 요구하지 않고 그 지점에서 명시 오류로 멈춘다.</summary>
    /// <typeparam name="T">장치 대리자 형태.</typeparam>
    /// <param name="device">요구할 장치.</param>
    private static T Need<T>(T? device) where T : class
    {
        return device ?? throw new InvalidOperationException("NewFort의 선택된 실제 하위 미연결");
    }

    /// <summary>현재 handle 버퍼의 첫 NUL 앞까지를 문자열로 읽는다. 내용을 trim하거나 변환하지 않으며 NUL이 없으면 저장소 밖 읽기를 거부한다.</summary>
    /// <param name="bytes">현재 handle 버퍼.</param>
    private static string CString(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw RawStorageBytes.OutOfRange("NewFort 현재 handle의 NUL 물리 범위");
        }
        return RawStorageBytes.Latin1.GetString(bytes[..end]);
    }

    /// <summary>
    /// 004bfba0: 진단(기존 manager 비영)과 exists 재조회 뒤 같은 입력 이름으로 manager를 만든다. header의 정확한 20바이트 중 +4와 +16을 먼저 쓰고,
    /// 파일이 있으면 임시 manager를 만들어 그 파일 이름으로 삭제한 뒤 소멸·해제한다. 마지막에 새 manager를 header +0에 공개하고 초기화한다.
    /// </summary>
    /// <param name="header">Template 기록 20바이트.</param>
    /// <param name="fileName">새 manager의 파일 이름.</param>
    /// <param name="argument">header +16에 쓸 인자.</param>
    /// <param name="hooks">할당·생성·삭제·초기화 장치.</param>
    /// <exception cref="ArgumentOutOfRangeException">임시 manager가 NULL이어서 원문의 가상 이름 접근이 실패하는 경우.</exception>
    public static void CreateNewFortManager(Span<byte> header, string fileName, uint argument, NewFortManagerHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (header.Length != HeaderBytes)
        {
            throw new ArgumentException("NewFort의 실제20BYTE header 누락");
        }
        if (RawStorageBytes.Read(header, 0) != 0)
        {
            Need(hooks.Assertion)(0x172);
        }
        RawStorageBytes.Put(header, 4, 0);
        RawStorageBytes.Put(header, 16, argument);
        if (Need(hooks.Exists)(fileName) != 0)
        {
            uint stale = Need(hooks.Allocate)(ManagerAllocation);
            uint temporary = stale != 0 ? Need(hooks.Construct)(stale, fileName) : 0u;
            if (temporary == 0)
            {
                // 실제 004bfbff의 NULL 접근이며 삭제/해제를 성공으로 진행하지 않는다
                throw RawStorageBytes.OutOfRange("원문 새 manager 임시NULL의 가상 이름 접근");
            }
            string oldName = Need(hooks.FileName)(temporary);
            Need(hooks.Erase)(oldName);
            Need(hooks.Destroy)(temporary);
            Need(hooks.Release)(temporary);
        }
        uint allocation = Need(hooks.Allocate)(ManagerAllocation);
        uint current = allocation != 0 ? Need(hooks.Construct)(allocation, fileName) : 0u;
        // 공장 뒤 현재 토큰을 공개한 뒤 실제 초기화한다. 할당 0도 이 원문 호출로 이어진다
        RawStorageBytes.Put(header, 0, current);
        Need(hooks.Initialize)(current);
    }

    /// <summary>
    /// 004b3920: 현재 Client 전역을 읽어 초기 Unnamed handle, Editor가 아닐 때의 초기 120바이트 Territory 값, SP 차감, CoreData 10바이트 지움,
    /// 하위 WORD가 0이 아닌 seed, Subscriber·Territory·Money 저장, State kind 쓰기를 원문 순서로 수행한다.
    /// </summary>
    /// <param name="globals">Client·Player·난수 소유자의 현재 전역.</param>
    /// <param name="hooks">공유 하위 장치.</param>
    /// <exception cref="ArgumentOutOfRangeException">handle 버퍼·CoreData·State가 원문이 쓰는 물리 범위보다 짧다.</exception>
    public static void CreateNewFortData(NewFortDataGlobals globals, NewFortDataHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        Need(globals.SetSubscriber)(0xffffffffu);
        string unnamed = RawStorageBytes.CString(Need(hooks.Translate)("Unnamed"));
        Span<byte> handle = Need(globals.Handle)();
        if (handle.Length <= unnamed.Length)
        {
            throw RawStorageBytes.OutOfRange("NewFort Unnamed의 실제 handle 출력 폭");
        }
        // 원문의 앞으로 복사와 마지막 NUL을 기록한다
        for (int index = 0; index < unnamed.Length; index++)
        {
            handle[index] = (byte)unnamed[index];
        }
        handle[unnamed.Length] = 0;
        Need(hooks.Configure)("fort.mySubHandle", CString(Need(globals.Handle)()));
        Need(hooks.Title)();
        Need(hooks.RegisterSubscriber)();
        // 004b3975는 20개 레코드의 6바이트를 각각 0으로 쓴다
        byte[] records = new byte[TerritoryBytes];
        if (Need(globals.Editor)() == 0)
        {
            records[0] = (byte)((records[0] & 0xc7u) | 7u);
            Need(hooks.PrepareRandom)();
            records[3] = unchecked((byte)(Need(hooks.NextRandom)(254) + 1u));
            records[1] |= 1;
            records[2] = 0x11;
        }
        float money = Need(globals.PlayerKnown)() != 0 ? Need(hooks.Sp)(Need(globals.LocalOwner)()) : 0.0f;
        Need(hooks.AddSp)(-money);
        Need(globals.SetCoreActive)(1);
        Span<byte> core = Need(hooks.CoreData)();
        if (core.Length < CoreClearBytes)
        {
            throw RawStorageBytes.OutOfRange("NewFort 현재 CoreData의10BYTE 물리 범위");
        }
        // 4/4/2바이트로 0을 쓰는 0042b200의 본문이다
        core[..CoreClearBytes].Clear();
        Need(hooks.CoreWrite)(Need(globals.TemplateOwner)());
        Need(hooks.PrepareRandom)();
        // 매 반환 뒤 현재 상위 WORD를 보존하고 0도 실제로 기록한 뒤 하위 WORD가 0이면 다시 뽑는다
        do
        {
            uint next = Need(hooks.NextRandom)(65536);
            Need(globals.SetCoreSeed)((Need(globals.CoreSeed)() & 0xffff0000u) | (next & 0xffffu));
        }
        while ((Need(globals.CoreSeed)() & 0xffffu) == 0);
        // 004b3a48은 문자열 내용·길이 대신 현재 포인터만 캡처한다. 실제 writer의 등록 후 읽기를 보존한다
        uint subscriberOwner = Need(globals.TemplateOwner)();
        uint id = Need(globals.Subscriber)();
        Need(hooks.Subscriber)(subscriberOwner, id, Need(globals.Handle)());
        Need(hooks.Configure)("fort.mySubHandle", CString(Need(globals.Handle)()));
        Need(hooks.Title)();
        Need(hooks.Territory)(Need(globals.TemplateOwner)(), records);
        Need(hooks.Money)(Need(globals.TemplateOwner)());
        uint editor = Need(globals.Editor)();
        uint stateOwner = Need(globals.TemplateOwner)();
        byte kind = editor != 0 ? (byte)2 : (byte)0;
        Span<byte> state = Need(hooks.State)(stateOwner);
        if (state.Length <= StateKindOffset)
        {
            throw RawStorageBytes.OutOfRange("NewFort 현재 State+11의 물리 범위");
        }
        state[StateKindOffset] = kind;
        Need(hooks.Notify)(Need(globals.TemplateOwner)());
    }

    /// <summary>
    /// 004b5ca0의 최초 파일 부재 분기: 존재하는 파일이면 아무 신규 효과 없이 false를 돌려주고, 부재일 때만 새 manager 생성, State 첫 바이트 bit0, 알림,
    /// 신규 자료, alternate=1, Subscriber 읽기, 설정, 제목 갱신을 원문 순서로 수행한다.
    /// </summary>
    /// <param name="alternate">대체 경로 확정 표시(DWORD 참조). 신규 자료가 끝난 뒤 1로 쓴다.</param>
    /// <param name="hooks">경로·존재·소유자·manager·State·알림 장치.</param>
    /// <returns>파일 부재의 정상 경로를 수행했으면 true, 기존 파일이면 false.</returns>
    public static bool InstallMissingNewFort(ref uint alternate, NewFortMissingFileHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (Need(hooks.Exists)(Need(hooks.Path)()) != 0)
        {
            return false;
        }
        string name = Need(hooks.Path)();
        Need(hooks.Manager)(Need(hooks.Owner)(), name, 0);
        Span<byte> state = Need(hooks.State)(Need(hooks.Owner)());
        if (state.IsEmpty)
        {
            throw RawStorageBytes.OutOfRange("NewFort State 첫BYTE 물리 범위");
        }
        state[0] |= 1;
        Need(hooks.Notify)(Need(hooks.Owner)());
        Need(hooks.Data)();
        alternate = 1;
        Need(hooks.Subscriber)(Need(hooks.Owner)());
        Need(hooks.Configure)("fort.mySubHandle", Need(hooks.Handle)());
        Need(hooks.Title)();
        return true;
    }
}
